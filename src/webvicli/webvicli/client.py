#!/usr/bin/env python

# client.py - webvi command line client
#
# Copyright (c) 2009-2013 Antti Ajanki <antti.ajanki@iki.fi>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

import sys
import cmd
import mimetypes
import select
import os.path
import subprocess
import time
import re
import datetime
import urllib
import shlex
import shutil
import tempfile
import libxml2
from pywebvi import WebviContext, WebviRequest, WebviState
from optparse import OptionParser
from ConfigParser import RawConfigParser
from urlparse import urlparse
from StringIO import StringIO
from . import menu

VERSION = '0.5.0'

# Default options
DEFAULT_PLAYERS = ['mplayer -cache-min 10 "%s"',
                   'vlc --play-and-exit --file-caching 5000 "%s"',
                   'totem "%s"',
                   'xine "%s"']

# These mimetypes are common but often missing
mimetypes.init()
mimetypes.add_type('video/flv', '.flv')
mimetypes.add_type('video/x-flv', '.flv')
mimetypes.add_type('video/webm', '.webm')

def safe_filename(name, vfat):
    """Sanitize a filename. If vfat is False, replace '/' with '_', if
    vfat is True, replace also other characters that are illegal on
    VFAT. Remove dots from the beginning of the filename."""
    if vfat:
        excludechars = r'[\\"*/:<>?|]'
    else:
        excludechars = r'[/]'

    res = re.sub(excludechars, '_', name)
    res = res.lstrip('.')
    res = res.encode(sys.getfilesystemencoding(), 'ignore')

    return res

def get_content_unicode(node):
    """node.getContent() returns an UTF-8 encoded sequence of bytes (a
    string). Convert it to a unicode object."""
    return unicode(node.getContent(), 'UTF-8', 'replace')

def guess_video_extension(mimetype, url):
    """Return extension for a video at url with a given mimetype.

    This assumes that the target is a video stream and therefore ignores
    mimetype if it is text/plain, which some incorrectly configured servers
    return as the mimetype.
    """
    ext = mimetypes.guess_extension(mimetype)
    if (ext is None) or (mimetype == 'text/plain'):
        lastcomponent = re.split(r'[?#]', url, 1)[0].split('/')[-1]
        i = lastcomponent.rfind('.')
        if i == -1:
            ext = ''
        else:
            ext = lastcomponent[i:]
    return ext

def dl_progress(count, blockSize, totalSize):
    if totalSize == -1:
        return
    percent = int(count*blockSize*100/totalSize)
    sys.stdout.write("\r%d %%" % percent)
    sys.stdout.flush()

def next_available_file_name(basename, ext):
    fullname = basename + ext
    if not os.path.exists(fullname):
        return fullname
    i = 1
    while os.path.exists('%s-%d%s' % (basename, i, ext)):
        i += 1
    return '%s-%d%s' % (basename, i, ext)

class StringIOCallback(StringIO):
    def write_and_return_length(self, buf):
        self.write(buf)
        return len(buf)


class ProgressMeter:
    def __init__(self, stream):
        self.last_update = None
        self.samples = []
        self.total_bytes = 0
        self.stream = stream
        self.progress_len = 0
        self.starttime = time.time()

    def pretty_bytes(self, bytes):
        """Pretty print bytes as kB or MB."""
        if bytes < 1100:
            return '%d B' % bytes
        elif bytes < 1024*1024:
            return '%.1f kB' % (float(bytes)/1024)
        elif bytes < 1024*1024*1024:
            return '%.1f MB' % (float(bytes)/1024/1024)
        else:
            return '%.1f GB' % (float(bytes)/1024/1024/1024)

    def pretty_time(self, seconds):
        """Pretty print seconds as hour and minutes."""
        seconds = int(round(seconds))
        if seconds < 60:
            return '%d s' % seconds
        elif seconds < 60*60:
            secs = seconds % 60
            mins = seconds/60
            return '%d min %d s' % (mins, secs)
        else:
            hours = seconds / (60*60)
            mins = (seconds-60*60*hours) / 60
            return '%d hours %d min' % (hours, mins)
        
    def update(self, bytes):
        """Update progress bar.

        Updates the estimates of download rate and remaining time.
        Prints progress bar, if at least one second has passed since
        the previous update.
        """
        now = time.time()

        if self.total_bytes > 0:
            percentage = float(bytes)/self.total_bytes * 100.0
        else:
            percentage = 0

        if self.total_bytes > 0 and bytes >= self.total_bytes:
            self.stream.write('\r')
            self.stream.write(' '*self.progress_len)
            self.stream.write('\r')
            self.stream.write('%3.f %% of %s downloaded in %s (%.1f kB/s)\n' % 
                              (percentage, self.pretty_bytes(self.total_bytes), 
                               self.pretty_time(now-self.starttime),
                               float(bytes)/(now-self.starttime)/1024.0))
            self.stream.flush()
            return

        force_refresh = False
        if self.last_update is None:
            # This is a new progress meter
            self.last_update = now
            force_refresh = True

        if (not force_refresh) and (now <= self.last_update + 1):
            # do not update too often
            return

        self.last_update = now

        # Estimate bytes per second rate from the last 10 samples
        self.samples.append((bytes, now))
        if len(self.samples) > 10:
            self.samples.pop(0)

        bytes_old, time_old = self.samples[0]
        if now > time_old:
            rate = float(bytes-bytes_old)/(now-time_old)
        else:
            rate = 0

        if self.total_bytes > 0:
            remaining = self.total_bytes - bytes
        
            if rate > 0:
                time_left = self.pretty_time(remaining/rate)
            else:
                time_left = '???'
        
            progress = '%3.f %% of %s (%.1f kB/s) %s remaining' % \
                       (percentage, self.pretty_bytes(self.total_bytes),
                        rate/1024.0, time_left)
        else:
            progress = '%s downloaded (%.1f kB/s)' % \
                       (self.pretty_bytes(bytes), rate/1024.0)

        new_progress_len = len(progress)
        if new_progress_len < self.progress_len:
            progress += ' '*(self.progress_len - new_progress_len)
        self.progress_len = new_progress_len

        self.stream.write('\r')
        self.stream.write(progress)
        self.stream.flush()


class WVClient:
    def __init__(self, streamplayers, downloadlimits, streamlimits, vfatfilenames):
        self.streamplayers = streamplayers
        self.history = []
        self.history_pointer = 0
        self.quality_limits = {'download': downloadlimits,
                                'stream': streamlimits}
        self.vfatfilenames = vfatfilenames
        self.alarm = None
        self.webvi = WebviContext()
        self.webvi.set_timeout_callback(self.update_timeout)

    def set_debug(self, enabled):
        self.webvi.set_debug(enabled)

    def set_template_path(self, path):
        self.webvi.set_template_path(path)
        
    def update_timeout(self, timeout_ms):
        if timeout_ms < 0:
            self.alarm = None
        else:
            now = datetime.datetime.now()
            self.alarm = now + datetime.timedelta(milliseconds=timeout_ms)

    def parse_page(self, page):
        if page is None:
            return None
        try:
            doc = libxml2.parseDoc(page)
        except libxml2.parserError:
            return None

        root = doc.getRootElement()
        if root.name != 'wvmenu':
            return None
        queryitems = []
        menupage = menu.Menu()
        node = root.children
        while node:
            if node.name == 'title':
                menupage.title = get_content_unicode(node)
            elif node.name == 'ul':
                li_node = node.children
                while li_node:
                    if li_node.name == 'li':
                        menuitem = self.parse_link(li_node)
                        menupage.add(menuitem)
                    li_node = li_node.next
                
            # elif node.name == 'link':
            #     menuitem = self.parse_link(node)
            #     menupage.add(menuitem)
            # elif node.name == 'textfield':
            #     menuitem = self.parse_textfield(node)
            #     menupage.add(menuitem)
            #     queryitems.append(menuitem)
            # elif node.name == 'itemlist':
            #     menuitem = self.parse_itemlist(node)
            #     menupage.add(menuitem)
            #     queryitems.append(menuitem)
            # elif node.name == 'textarea':
            #     menuitem = self.parse_textarea(node)
            #     menupage.add(menuitem)
            # elif node.name == 'button':
            #     menuitem = self.parse_button(node, queryitems)
            #     menupage.add(menuitem)
            node = node.next
        doc.freeDoc()
        return menupage

    def parse_link(self, node):
        label = ''
        ref = None
        is_stream = False
        child = node.children
        while child:
            if child.name == 'a':
                label = get_content_unicode(child)
                ref = child.prop('href')
                is_stream = child.prop('class') != 'webvi'
            child = child.next
        return menu.MenuItemLink(label, ref, is_stream)

    def parse_textfield(self, node):
        label = ''
        name = node.prop('name')
        child = node.children
        while child:
            if child.name == 'label':
                label = get_content_unicode(child)
            child = child.next
        return menu.MenuItemTextField(label, name)

    def parse_textarea(self, node):
        label = ''
        child = node.children
        while child:
            if child.name == 'label':
                label = get_content_unicode(child)
            child = child.next
        return menu.MenuItemTextArea(label)

    def parse_itemlist(self, node):
        label = ''
        name = node.prop('name')
        items = []
        values = []
        child = node.children
        while child:
            if child.name == 'label':
                label = get_content_unicode(child)
            elif child.name == 'item':
                items.append(get_content_unicode(child))
                values.append(child.prop('value'))
            child = child.next
        return menu.MenuItemList(label, name, items, values, sys.stdout)

    def parse_button(self, node, queryitems):
        label = ''
        submission = None
        encoding = 'utf-8'
        child = node.children
        while child:
            if child.name == 'label':
                label = get_content_unicode(child)
            elif child.name == 'submission':
                submission = get_content_unicode(child)
                enc = child.hasProp('encoding')
                if enc is not None:
                    encoding = get_content_unicode(enc)
            child = child.next
        return menu.MenuItemSubmitButton(label, submission, queryitems, encoding)

    def execute_webvi(self, request):
        """Call self.webvi.process_some until request is finished."""
        while True:
            if self.alarm is None:
                timeout = 10
            else:
                delta = self.alarm - datetime.datetime.now()
                if delta < datetime.timedelta(0):
                    timeout = 10
                    self.alarm = None
                else:
                    timeout = delta.microseconds/1000000.0 + delta.seconds

            self.webvi.process_some(timeout)
            finished = self.webvi.get_finished_request()
            if finished is not None and finished[0] == request:
                return (finished[1], finished[2])

    def getmenu(self, ref):
        dlbuffer = StringIOCallback()
        request = WebviRequest(self.webvi, ref)
        request.set_read_callback(dlbuffer.write_and_return_length)
        request.start()
        status, err = self.execute_webvi(request)
        del request

        if status != WebviState.FINISHED_OK:
            print 'Download failed:', err
            return (status, err, None)

        return (status, err, self.parse_page(dlbuffer.getvalue()))

    def get_quality_params(self, videosite, streamtype):
        params = []
        lim = self.quality_limits[streamtype].get(videosite, {})
        if lim.has_key('min'):
            params.append('minquality=' + lim['min'])
        if lim.has_key('max'):
            params.append('maxquality=' + lim['max'])

        return '&'.join(params)
        
    def download(self, stream):
        streamurl, streamtitle = self.get_stream_url_and_title(stream)
        if streamurl is None:
            return True
        
        self.download_stream(streamurl, streamtitle)
        return True

    def get_stream_url_and_title(self, stream):
        dlbuffer = StringIOCallback()
        request = WebviRequest(self.webvi, stream)
        request.set_read_callback(dlbuffer.write_and_return_length)
        request.start()
        status, err = self.execute_webvi(request)
        del request

        if status != WebviState.FINISHED_OK:
            print 'Download failed:', err
            return (None, None)

        menu = self.parse_page(dlbuffer.getvalue())
        if menu is None or len(menu) == 0:
            print 'Failed to parse menu'
            return (None, None)

        return (menu[0].activate(), menu[0].label)

    def download_stream(self, url, title):
        try:
            (tmpfilename, headers) = \
              urllib.urlretrieve(url, reporthook=dl_progress)
            print 
        except urllib.ContentTooShortError, exc:
            print 'Got too few bytes, connection may have been interrupted'
            headers = {}
            tmpfile, tmpfilename = tempfile.mkstemp()
            tmpfile.write(exc.content)
            tmpfile.close()

        # rename the tempfile to final name
        contenttype = headers.get('Content-Type', 'video')
        ext = guess_video_extension(contenttype, url)
        safename = safe_filename(title, self.vfatfilenames)
        destfilename = next_available_file_name(safename, ext)
        shutil.move(tmpfilename, destfilename)
        print 'Saved to %s' % destfilename

    def play_stream(self, ref):
        streamurl = self.get_stream_url(ref)
        if streamurl == '':
            print 'Did not find URL'
            return False

        if streamurl.startswith('wvt://'):
            print 'Streaming not supported, try downloading'
            return False

        # Found url, now find a working media player
        for player in self.streamplayers:
            if '%s' not in player:
                player = player + ' %s'

            playcmd = shlex.split(player)

            # Hack for playing from fifo in VLC
            if 'vlc' in playcmd[0] and streamurl.startswith('file://'):
                realurl = 'stream://' + streamurl[len('file://'):]
            else:
                realurl = streamurl

            try:
                playcmd[playcmd.index('%s')] = realurl
            except ValueError:
                print 'Can\'t substitute URL in', player
                continue

            try:
                print 'Trying player: ' + ' '.join(playcmd)
                retcode = subprocess.call(playcmd)
                if retcode > 0:
                    print 'Player failed with returncode', retcode
                # else:
                #     # After the player has finished, the library
                #     # generates a read event on a control socket. When
                #     # the client calls perform on the socket the
                #     # library removes temporary files.
                #     readfds, writefds = webvi.api.fdset()[1:3]
                #     readyread, readywrite, readyexc = \
                #         select.select(readfds, writefds, [], 0.1)
                #     for fd in readyread:
                #         webvi.api.perform(fd, WebviSelectBitmask.READ)
                #     for fd in readywrite:
                #         webvi.api.perform(fd, WebviSelectBitmask.WRITE)

                    return True
            except OSError, err:
                print 'Execution failed:', err

        return False

    def get_stream_url(self, ref):
        m = re.match(r'wvt:///([^/]+)/', ref)
        if m is not None:
            ref += '&' + self.get_quality_params(m.group(1), 'stream')

        request = WebviRequest(self.webvi, ref)

        dlbuffer = StringIOCallback()
        request.set_read_callback(dlbuffer.write_and_return_length)
        request.start()
        status, err = self.execute_webvi(request)
        del request
        
        if status != WebviState.FINISHED_OK:
            print 'Download failed:', err
            return ''

        return dlbuffer.getvalue()

    def get_current_menu(self):
        if (self.history_pointer >= 0) and \
               (self.history_pointer < len(self.history)):
            return self.history[self.history_pointer]
        else:
            return None

    def history_add(self, menupage):
        if menupage is not None:
            self.history = self.history[:(self.history_pointer+1)]
            self.history.append(menupage)
            self.history_pointer = len(self.history)-1

    def history_back(self):
        if self.history_pointer > 0:
            self.history_pointer -= 1
        return self.get_current_menu()

    def history_forward(self):
        if self.history_pointer < len(self.history)-1:
            self.history_pointer += 1
        return self.get_current_menu()


class WVShell(cmd.Cmd):
    def __init__(self, client, completekey='tab', stdin=None, stdout=None):
        cmd.Cmd.__init__(self, completekey, stdin, stdout)
        self.prompt = '> '
        self.client = client

    def preloop(self):
        self.stdout.write('webvicli %s starting\n' % VERSION)
        self.do_menu(None)

    def precmd(self, arg):
        try:
            int(arg)
            menuitem = self._get_numbered_item(int(arg))
            if getattr(menuitem, 'is_stream', False):
                return 'download ' + arg
            else:
                return 'select ' + arg
        except ValueError:
            return arg

    def onecmd(self, c):
        try:
            return cmd.Cmd.onecmd(self, c)
        except Exception:
            import traceback
            print 'Exception occurred while handling command "' + c + '"'
            print traceback.format_exc()
            return False

    def emptyline(self):
        pass

    def display_menu(self, menupage):
        if menupage is not None:
            enc = self.stdout.encoding or 'UTF-8'
            self.stdout.write(unicode(menupage).encode(enc, 'replace'))
            self.stdout.flush()
    
    def _get_numbered_item(self, arg):
        menupage = self.client.get_current_menu()
        try:
            v = int(arg)-1
            if (menupage is None) or (v < 0) or (v >= len(menupage)):
                raise ValueError
        except ValueError:
            self.stdout.write('Invalid selection: %s\n' % arg)
            self.stdout.flush()
            return None
        return menupage[v]

    def do_select(self, arg):
        """select x
Select the link whose index is x.
        """
        menuitem = self._get_numbered_item(arg)
        if menuitem is None:
            return False
        ref = menuitem.activate()
        if ref is not None:
            status, statusmsg, menupage = self.client.getmenu(ref)
            if menupage is not None:
                self.client.history_add(menupage)
            else:
                self.stdout.write('Error: %d %s\n' % (status, statusmsg))
                self.stdout.flush()
        else:
            menupage = self.client.get_current_menu()
        self.display_menu(menupage)
        return False

    def do_download(self, arg):
        """download x
Download a stream to a file. x can be an integer referring to a
downloadable item (item without brackets) in the current menu or an
URL of a video page.
        """
        stream = None
        try:
            menuitem = self._get_numbered_item(int(arg))
            if menuitem is not None:
                stream = menuitem.activate()
        except (ValueError, AttributeError):
            pass

        if stream is None and arg.find('://') != -1:
            stream = arg

        if stream is not None:
            self.client.download(stream)
        else:
            self.stdout.write('Not a stream\n')
            self.stdout.flush()
        return False

    def do_stream(self, arg):
        """stream x
Play a stream. x can be an integer referring to a downloadable item
(item without brackets) in the current menu or an URL of a video page.
        """
        stream = None
        try:
            menuitem = self._get_numbered_item(int(arg))
            if menuitem is not None:
                stream = menuitem.activate()
        except (ValueError, AttributeError):
            pass

        if stream is None and arg.find('://') != -1:
            stream = arg

        if stream is not None:
            self.client.play_stream(stream)
        else:
            self.stdout.write('Not a stream\n')
            self.stdout.flush()
        return False

    def do_display(self, arg):
        """Redisplay the current menu."""
        if not arg:
            self.display_menu(self.client.get_current_menu())
        else:
            self.stdout.write('Unknown parameter %s\n' % arg)
            self.stdout.flush()
        return False

    def do_menu(self, arg):
        """Get back to the main menu."""
        status, statusmsg, menupage = self.client.getmenu('wvt://mainmenu')
        if menupage is not None:
            self.client.history_add(menupage)
            self.display_menu(menupage)
        else:
            self.stdout.write('Error: %d %s\n' % (status, statusmsg))
            self.stdout.flush()
            return True
        return False

    def do_back(self, arg):
        """Go to the previous menu in the history."""
        menupage = self.client.history_back()
        self.display_menu(menupage)
        return False

    def do_forward(self, arg):
        """Go to the next menu in the history."""
        menupage = self.client.history_forward()
        self.display_menu(menupage)
        return False

    def do_quit(self, arg):
        """Quit the program."""
        return True

    def do_EOF(self, arg):
        """Quit the program."""
        return True


def load_config(options):
    """Load options from config files."""
    cfgprs = RawConfigParser()
    cfgprs.read(['/etc/webvi.conf', os.path.expanduser('~/.webvi')])
    for sec in cfgprs.sections():
        if sec == 'webvi':
            for opt, val in cfgprs.items('webvi'):
                if opt in ['vfat', 'verbose']:
                    try:
                        options[opt] = cfgprs.getboolean(sec, opt)
                    except ValueError:
                        print 'Invalid config: %s = %s' % (opt, val)

                    # convert verbose to integer
                    if opt == 'verbose':
                        if options['verbose']:
                            options['verbose'] = 1
                        else:
                            options['verbose'] = 0

                else:
                    options[opt] = val

        else:
            sitename = urlparse(sec).netloc
            if sitename == '':
                sitename = sec

            if not options.has_key('download-limits'):
                options['download-limits'] = {}
            if not options.has_key('stream-limits'):
                options['stream-limits'] = {}
            options['download-limits'][sitename] = {}
            options['stream-limits'][sitename] = {}

            for opt, val in cfgprs.items(sec):
                if opt == 'download-min-quality':
                    options['download-limits'][sitename]['min'] = val
                elif opt == 'download-max-quality':
                    options['download-limits'][sitename]['max'] = val
                elif opt == 'stream-min-quality':
                    options['stream-limits'][sitename]['min'] = val
                elif opt == 'stream-max-quality':
                    options['stream-limits'][sitename]['max'] = val

    return options

def parse_command_line(cmdlineargs, options):
    parser = OptionParser()
    parser.add_option('-t', '--templatepath', type='string',
                      dest='templatepath',
                      help='read video site templates from DIR', metavar='DIR',
                      default=None)
    parser.add_option('-v', '--verbose', action='store_const', const=1,
                      dest='verbose', help='debug output', default=0)
    parser.add_option('--vfat', action='store_true',
                      dest='vfat', default=False,
                      help='generate Windows compatible filenames')
    parser.add_option('-u', '--url', type='string',
                      dest='url',
                      help='Download video from URL and exit',
                      metavar='URL', default=None)
    cmdlineopt = parser.parse_args(cmdlineargs)[0]

    if cmdlineopt.templatepath is not None:
        options['templatepath'] = cmdlineopt.templatepath
    if cmdlineopt.verbose > 0:
        options['verbose'] = cmdlineopt.verbose
    if cmdlineopt.vfat:
        options['vfat'] = cmdlineopt.vfat
    if cmdlineopt.url:
        options['url'] = cmdlineopt.url

    return options

def player_list(options):
    """Return a sorted list of player commands extracted from options
    dictionary."""
    # Load streamplayer items from the config file and sort them
    # according to quality.
    players = []
    for opt, val in options.iteritems():
        m = re.match(r'streamplayer([1-9])$', opt)
        if m is not None:
            players.append((int(m.group(1)), val))

    players.sort()
    ret = []
    for quality, playcmd in players:
        ret.append(playcmd)

    # If the config file did not define any players use the default
    # players
    if not ret:
        ret = list(DEFAULT_PLAYERS)

    return ret

def main(argv):
    options = load_config({})
    options = parse_command_line(argv, options)

    client = WVClient(player_list(options),
                      options.get('download-limits', {}),
                      options.get('stream-limits', {}),
                      options.get('vfat', False))

    if options.has_key('verbose'):
        client.set_debug(options['verbose'])
    if options.has_key('templatepath'):
        client.set_template_path(options['templatepath'])

    if options.has_key('url'):
        stream = options['url']
        if not client.download(stream):
            # FIXME: more helpful error message if URL is not a
            # supported site
            sys.exit(1)
            
        sys.exit(0)

    shell = WVShell(client)
    shell.cmdloop()

if __name__ == '__main__':
    main([])
