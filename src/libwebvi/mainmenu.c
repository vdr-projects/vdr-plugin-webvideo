#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "mainmenu.h"
#include "menubuilder.h"

static GPtrArray *load_websites(const char *path);
static gint title_cmp(gconstpointer a, gconstpointer b);
static gchar *get_site_title(xmlDocPtr doc);
static gchar *get_site_href(xmlDocPtr doc, const gchar *filename);

char *build_mainmenu(const char *path) {
  g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "building main menu %s", path);

  MenuBuilder *menu_builder = menu_builder_create();
  menu_builder_set_title(menu_builder, "Select video source");

  GPtrArray *websites = load_websites(path);
  menu_builder_append_link_list(menu_builder, websites);
  char *menu = menu_builder_to_string(menu_builder);

  g_ptr_array_free(websites, TRUE);
  menu_builder_delete(menu_builder);
  return menu;
}

/*
 * Load known websites from the given directory.
 *
 * Traverses each subdirectory looking for file called menu.xml. If
 * found, reads the file to find site title. Returns an array of
 * titles and wvt references. The caller must call g_ptr_array_free()
 * on the returned array (the content of the array will be freed
 * automatically).
 */
GPtrArray *load_websites(const char *path) {
  GPtrArray *websites = g_ptr_array_new_with_free_func(g_free_link);

  GDir *dir = g_dir_open(path, 0, NULL);
  if (!dir)
    return websites;

  const gchar *dirname;
  while ((dirname = g_dir_read_name(dir))) {
    gchar *menudir = g_strconcat(path, "/", dirname, NULL);
    if (g_file_test(menudir, G_FILE_TEST_IS_DIR)) {
      gchar *menufile = g_strconcat(menudir, "/menu.xml", NULL);

      g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
            "processing website menu %s", menufile);

      gchar *sitemenu = NULL;
      gsize sitemenu_len;
      if (g_file_get_contents(menufile, &sitemenu, &sitemenu_len, NULL)) {
        xmlDocPtr doc = xmlReadMemory(sitemenu, sitemenu_len, menufile, NULL,
                                      XML_PARSE_NOWARNING | XML_PARSE_NONET);
        gchar *href = get_site_href(doc, menufile);
        gchar *title = get_site_title(doc);
        if (!title) {
          title = g_strdup(dirname);
        }

        Link *menuitem = link_create(href, title, LINK_ACTION_PARSE);
        g_ptr_array_add(websites, menuitem);
        g_free(href);
        g_free(title);
        g_free(sitemenu);
        xmlFreeDoc(doc);
      }

      g_free(menufile);
    }

    g_free(menudir);
  }
  
  g_dir_close(dir);

  g_ptr_array_sort(websites, title_cmp);

  return websites;
}

gint title_cmp(gconstpointer a, gconstpointer b) {
  // a and b are pointers to Link pointers!
  Link *link1 = *(Link **)a;
  Link *link2 = *(Link **)b;
  
  return g_ascii_strcasecmp(link_get_title(link1),
                            link_get_title(link2));
}

/* Get site's title from menu.xml document. */
gchar *get_site_title(xmlDocPtr doc) {
  gchar *title = NULL;
  if (!doc)
    return NULL;

  xmlNode *root = xmlDocGetRootElement(doc);
  xmlNode *node = root->children;
  while (node) {
    if (node->type == XML_ELEMENT_NODE &&
        xmlStrEqual(node->name, BAD_CAST "title")) {
      xmlChar *xmltitle = xmlNodeGetContent(node);
      if (xmltitle) {
        title = g_strdup((gchar *)xmltitle);
        xmlFree(xmltitle);
        break;
      }
    }

    node = node->next;
  }

  return title;
}

/* Get <redirection> from menu.xml or build href from menu file name. */
gchar *get_site_href(xmlDocPtr doc, const gchar *filename) {
  gchar *href = NULL;

  if (doc) {
    xmlNode *root = xmlDocGetRootElement(doc);
    xmlNode *node = root->children;
    while (node) {
      if (xmlStrEqual(node->name, BAD_CAST "redirect")) {
        xmlChar *xmlhref = xmlNodeGetContent(node);
        if (xmlhref) {
          href = g_strdup((char *)xmlhref);
          xmlFree(xmlhref);
          break;
        }
      }
      node = node->next;
    }
  }

  if (!href && filename) {
    href = g_strconcat("wvt://", filename, NULL);
  }

  g_assert(href);
  if (!href) {
    g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
          "Could not get href for menu %s", filename);
    href = g_strdup("");
  }

  return href;
}
