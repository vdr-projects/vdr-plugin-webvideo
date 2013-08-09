# menu.py - menu elements for webvicli
#
# Copyright (c) 2009-2012 Antti Ajanki <antti.ajanki@iki.fi>
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
import textwrap
import urllib

LINEWIDTH = 72

def URI_template_substitute(template, substitutions):
    res = template
    for key, value in substitutions.iteritems():
        keytemplate = '{' + str(key) + '}'
        res = res.replace(keytemplate, str(value).replace(' ', '+'))
    return res

class Menu:
    def __init__(self):
        self.title = None
        self.items = []

    def __str__(self):
        s = u''
        if self.title:
            s = self.title + '\n' + '='*len(self.title) + '\n'
        for i, item in enumerate(self.items):
            if isinstance(item, MenuItemTextArea):
                num = '  '
            else:
                num = '%d.' % (i+1)

            s += u'%s %s\n' % (num, unicode(item).replace('\n', '\n   '))
        return s

    def __getitem__(self, i):
        return self.items[i]

    def __len__(self):
        return len(self.items)

    def append(self, menuitem):
        self.items.append(menuitem)

    def extend(self, menuitems):
        for item in menuitems:
            self.append(item)


class MenuItemLink:
    def __init__(self, label, ref, is_stream):
        self.label = label
        if type(ref) == unicode:
            self.ref = ref.encode('utf-8')
        else:
            self.ref = ref
        self.is_stream = is_stream

    def __str__(self):
        res = self.label
        if not self.is_stream:
            res = '[' + res + ']'
        return res

    def activate(self):
        return self.ref


class MenuItemTextField:
    def __init__(self, label, name):
        self.label = label
        self.name = name
        self.value = u''

    def __str__(self):
        return u'%s: %s' % (self.label, self.value)

    def get_query(self):
        return {self.name: self.value}

    def activate(self):
        self.value = unicode(raw_input('%s> ' % self.label), sys.stdin.encoding or 'utf-8')
        return None


class MenuItemTextArea:
    def __init__(self, label):
        self.label = label

    def __str__(self):
        return textwrap.fill(self.label, width=LINEWIDTH)

    def activate(self):
        return None


class MenuItemList:
    def __init__(self, label, name, items, values, stdout):
        self.label = label
        self.name = name
        assert len(items) == len(values)
        self.items = items
        self.values = values
        self.current = 0
        self.stdout = stdout

    def __str__(self):
        itemstrings = []
        for i, itemname in enumerate(self.items):
            if i == self.current:
                itemstrings.append('<' + itemname + '>')
            else:
                itemstrings.append(itemname)

        lab = self.label + ': '
        return textwrap.fill(u', '.join(itemstrings), width=LINEWIDTH,
                             initial_indent=lab,
                             subsequent_indent=' '*len(lab))

    def add_value(self, value, label):
        self.items.append(label)
        self.values.append(value)

    def get_query(self):
        if (self.current >= 0) and (self.current < len(self.items)):
            return {self.name: self.values[self.current]}
        else:
            return {}

    def activate(self):
        itemstrings = []
        for i, itemname in enumerate(self.items):
            itemstrings.append('%d. %s' % (i+1, itemname))

        self.stdout.write(u'\n'.join(itemstrings).encode(self.stdout.encoding, 'replace'))
        self.stdout.write('\n')

        tmp = raw_input('Select item (1-%d)> ' % len(self.items))
        try:
            i = int(tmp)
            if (i < 1) or (i > len(self.items)):
                raise ValueError
            self.current = i-1
        except ValueError:
            self.stdout.write('Must be an integer in the range 1 - %d\n' % len(self.items))
        return None


class MenuItemSubmitButton:
    def __init__(self, label, uritemplate, subitems, encoding):
        self.label = label
        if type(uritemplate) == unicode:
            self.uritemplate = uritemplate.encode('utf-8')
        else:
            self.uritemplate = uritemplate
        self.subitems = subitems
        self.encoding = encoding

    def __str__(self):
        return '[' + self.label + ']'

    def activate(self):
        substitutions = {}
        for item in self.subitems:
            for key, value in item.get_query().iteritems():
                if type(value) == unicode:
                    value = value.encode('UTF-8')
                substitutions[str(key)] = str(value)
        
        return URI_template_substitute(self.uritemplate, substitutions)
