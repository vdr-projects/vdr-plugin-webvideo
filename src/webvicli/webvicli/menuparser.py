import sys
import libxml2
from menu import Menu, MenuItemLink, MenuItemTextField, MenuItemList, MenuItemSubmitButton

def get_content_unicode(node):
    """node.getContent() returns an UTF-8 encoded sequence of bytes (a
    string). Convert it to a unicode object."""
    return unicode(node.getContent(), 'UTF-8', 'replace')

class InputElements:
    def __init__(self):
        self.elements = []

    def find_by_name(self, name):
        for elem in self.elements:
            if elem.name == name:
                return elem
        return None
        
    def append(self, inputtype, name, mainlabel, value, label, formurl):
        item = self.find_by_name(name)
        if item:
            if (inputtype == 'radio') and hasattr(item, 'add_value'):
                item.add_value(value, label)
            else:
                print 'Unexpected duplicate %s element with name %s' % \
                  (inputtype, name)
                return
        else:
            # was not already in the list, create a new menu item
            if inputtype == 'text':
                item = MenuItemTextField(mainlabel, name)
            elif inputtype == 'radio':
                item = MenuItemList(mainlabel, name, [label], [value], sys.stdout)
            elif inputtype == 'submit':
                item = MenuItemSubmitButton(value, formurl, None, 'UTF-8')
            
            self.elements.append(item)

    def to_menuitems(self):
        submitbuttons = []
        selectors = []
        for elem in self.elements:
            if isinstance(elem, MenuItemSubmitButton):
                submitbuttons.append(elem)
            else:
                selectors.append(elem)

        for button in submitbuttons:
            button.subitems = selectors

        return self.elements


class MenuParser:
    def parse_page(self, pagexml):
        if pagexml is None:
            return None
        try:
            doc = libxml2.parseDoc(pagexml)
        except libxml2.parserError:
            return None

        root = doc.getRootElement()
        if root.name != 'wvmenu':
            return None
        
        queryitems = []
        menupage = Menu()
        node = root.children
        while node:
            if node.name == 'title':
                menupage.title = get_content_unicode(node)
            elif node.name == 'ul':
                li_node = node.children
                while li_node:
                    if li_node.name == 'li':
                        menuitem = self.parse_link(li_node)
                        menupage.append(menuitem)
                    li_node = li_node.next
            elif node.name == 'form':
                menupage.extend(self.parse_form(node))
            node = node.next
        doc.freeDoc()
        return menupage

    def parse_link(self, node):
        label = ''
        ref = None
        is_stream = False
        child = node.children
        while child:
            if (child.name == 'a') and (child.prop('href') != ''):
                label = get_content_unicode(child)
                ref = child.prop('href')
                is_stream = child.prop('class') != 'webvi'
                break
            child = child.next
        return MenuItemLink(label, ref, is_stream)

    def parse_form(self, node):
        formurl = node.prop('action')
        if formurl == '':
            return []
        inputs = InputElements()
        child = node.children
        while child:
            if child.name == 'ul':
                li_node = child.children
                while li_node:
                    if li_node.name == 'li':
                        self.parse_form_item(li_node, inputs, formurl)
                    li_node = li_node.next
            child = child.next
        return inputs.to_menuitems()

    def parse_form_item(self, node, inputs, formurl):
        inputnode = None
        mainlabel = self.get_item_label(node)
        child = node.children
        while child:
            if child.name == 'input':
                self.parse_input(inputs, child, mainlabel, formurl=formurl)
            elif child.name == 'label':
                inputnode = self.get_input_node(child)
                if inputnode:
                    input_label = self.get_item_label(child)
                    self.parse_input(inputs, inputnode, mainlabel, input_label)
            child = child.next

    def parse_input(self, inputs, node, mainlabel, inputlabel=None, formurl=None):
        inputtype = node.prop('type')
        name = node.prop('name')
        value = node.prop('value')
        inputs.append(inputtype, name, mainlabel, value, inputlabel, formurl)

    def get_item_label(self, node):
        child = node.children
        label = ''
        while child:
            if child.type == 'text':
                label += get_content_unicode(child)
            child = child.next
        return label.strip()
        
    def get_input_node(self, node):
        child = node.children
        while child:
            if child.name == 'input':
                return child
            child = child.next
        return None
