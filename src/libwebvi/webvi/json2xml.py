# json2xml.py - Convert JSON into XML
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

"""Convert a JSON-serialized object into XML.

JSON strings and numbers are converted into nodes.

JSON array [...] is converted into <list><li>...</li></list>

JSON object {"key": "bar"} is converted into <dict><key>bar</key><dict>
"""

import sys
import libxml2

try:
    import json
except ImportError:
    try:
        import simplejson as json
    except ImportError:
        print 'Error: install simplejson'
        raise

def _serialize_to_xml(obj, xmlnode):
    """Create XML representation of a Python object (list, tuple,
    dist, or basic number and string types)."""
    if type(obj) in (list, tuple):
        listnode = libxml2.newNode('list')
        for li in obj:
            itemnode = libxml2.newNode('li')
            _serialize_to_xml(li, itemnode)
            listnode.addChild(itemnode)
        xmlnode.addChild(listnode)

    elif type(obj) == dict:
        dictnode = libxml2.newNode('dict')
        for key, val in obj.iteritems():
            itemnode = libxml2.newNode(key.encode('utf-8'))
            _serialize_to_xml(val, itemnode)
            dictnode.addChild(itemnode)
        xmlnode.addChild(dictnode)

    elif type(obj) in (str, unicode, int, long, float, complex, bool):
        content = libxml2.newText(unicode(obj).encode('utf-8'))
        xmlnode.addChild(content)

    elif type(obj) == type(None):
        pass

    else:
        raise TypeError('Unsupported type %s while serializing to xml'
                          % type(obj))

def json2xml(jsonstr, encoding=None):
    """Convert JSON string jsonstr to XML tree."""
    try:
        parsed = json.loads(jsonstr, encoding)
    except ValueError:
        return None

    xmldoc = libxml2.newDoc("1.0")
    root = libxml2.newNode("jsondocument")
    xmldoc.setRootElement(root)

    _serialize_to_xml(parsed, root)

    return xmldoc

def test():
    xml = json2xml(open(sys.argv[1]).read())

    if xml is None:
        return

    print xml.serialize('utf-8')

    xml.freeDoc()

if __name__ == '__main__':
    test()
