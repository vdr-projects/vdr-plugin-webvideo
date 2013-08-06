#include <stdlib.h>
#include <string.h>
#include <libxml/tree.h>
#include "menubuilder.h"

struct MenuBuilder {
  xmlDocPtr doc;
  xmlNodePtr root;
  xmlNodePtr ul_node;
  xmlNodePtr title_node;
};

static void add_link_to_menu(gpointer data, gpointer instance);

MenuBuilder *menu_builder_create() {
  MenuBuilder *self = malloc(sizeof(MenuBuilder));
  if (!self)
    return NULL;
  self->doc = xmlNewDoc(BAD_CAST "1.0");
  self->root = xmlNewNode(NULL, BAD_CAST "wvmenu");
  xmlDocSetRootElement(self->doc, self->root);
  self->ul_node = xmlNewNode(NULL, BAD_CAST "ul");
  xmlAddChild(self->root, self->ul_node);
  self->title_node = NULL;
  return self;
}

void menu_builder_set_title(MenuBuilder *self, const char *title) {
  if (self->title_node) {
    xmlUnlinkNode(self->title_node);
    xmlFreeNode(self->title_node);
    self->title_node = NULL;
  }

  self->title_node = xmlNewNode(NULL, BAD_CAST "title");
  xmlNodeAddContent(self->title_node, BAD_CAST title);

  if (self->root->children) {
    xmlAddPrevSibling(self->root->children, self->title_node);
  } else {
    xmlAddChild(self->root, self->title_node);
  }
}

char *menu_builder_to_string(MenuBuilder *self) {
  xmlChar *buf;
  int buflen;
  char *menu;

  xmlDocDumpMemoryEnc(self->doc, &buf, &buflen, "UTF-8");
  menu = malloc(buflen+1);
  strcpy(menu, (const char *)buf);
  xmlFree(buf);
  g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Menu:\n%s", menu);
  return menu;
}

void menu_builder_append_link_list(MenuBuilder *self, GPtrArray *links) {
  g_ptr_array_foreach(links, add_link_to_menu, self);
}

void add_link_to_menu(gpointer data, gpointer instance) {
  MenuBuilder *menubuilder = (MenuBuilder *)instance;
  Link *link = (Link *)data;
  menu_builder_append_link(menubuilder, link);
}

void menu_builder_append_link(MenuBuilder *self, const Link *link) {
  const char *class;
  if (link_get_type(link) == LINK_ACTION_STREAM_LIBQUVI) {
    class = "stream";
  } else {
    class = "webvi";
  }
  menu_builder_append_link_plain(self, link_get_href(link),
                                 link_get_title(link), class);
}

void menu_builder_append_link_plain(MenuBuilder *self, const char *href,
                                    const char *title, const char *class)
{
  xmlNodePtr li_node = xmlNewNode(NULL, BAD_CAST "li");
  xmlNodePtr a_node = xmlNewNode(NULL, BAD_CAST "a");
  if (title)
    xmlNodeAddContent(a_node, BAD_CAST title);
  if (href)
    xmlNewProp(a_node, BAD_CAST "href", BAD_CAST href);
  if (class)
    xmlNewProp(a_node, BAD_CAST "class", BAD_CAST class);
  xmlAddChild(li_node, a_node);
  xmlAddChild(self->ul_node, li_node);
}

void menu_builder_delete(MenuBuilder *self) {
  if (self) {
    xmlFreeDoc(self->doc);
    free(self);
  }
}
