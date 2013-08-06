#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "menubuilder_tests.h"
#include "link.h"
#include "mainmenu.h"

#define TEST_TITLE "Test menu"
#define LINK1_HREF "http://example.com/test/link"
#define LINK1_TITLE "Test link"
#define LINK2_HREF "http://example.com/video/00001"
#define LINK2_TITLE "Test stream"
#define LINK3_HREF "http://example.com/test/link3"
#define LINK3_TITLE "(large > small) & {50% of huge?}"
#define LINK3_TITLE_ENCODED "(large &gt; small) &amp; {50% of huge?}"

#define TITLE_TAG "<h1>"
#define LINK_TAG "<a "

static bool contains_substring(const char *buffer, const char *substr);
static int count_nonoverlapping_substrings(const char *buffer, const char *substr);
static void free_link(gpointer data);

void menu_builder_fixture_setup(MenuBuilderFixture *fixture,
                                G_GNUC_UNUSED gconstpointer test_data) {
  fixture->menu_builder = menu_builder_create();
  g_assert(fixture->menu_builder != NULL);
}

void menu_builder_fixture_teardown(MenuBuilderFixture *fixture,
                                   G_GNUC_UNUSED gconstpointer test_data) {
  menu_builder_delete(fixture->menu_builder);
  fixture->menu_builder = NULL;
}

void test_menu_builder_title(MenuBuilderFixture *fixture,
                             G_GNUC_UNUSED gconstpointer test_data) {
  menu_builder_set_title(fixture->menu_builder, TEST_TITLE);
  char *menu = menu_builder_to_string(fixture->menu_builder);
  g_assert(menu != NULL);
  g_assert(contains_substring(menu, TEST_TITLE));
  int numlinks = count_nonoverlapping_substrings(menu, LINK_TAG);
  g_assert(numlinks == 0);
  free(menu);
}

void test_menu_builder_append_links(MenuBuilderFixture *fixture,
                                    G_GNUC_UNUSED gconstpointer test_data) {
  GPtrArray *links = g_ptr_array_new_with_free_func(free_link);
  g_assert(links != NULL);
  g_ptr_array_add(links, link_create(LINK1_HREF, LINK1_TITLE, LINK_ACTION_PARSE));
  g_ptr_array_add(links, link_create(LINK2_HREF, LINK2_TITLE, LINK_ACTION_EXTERNAL_COMMAND));

  menu_builder_append_link_list(fixture->menu_builder, links);
  char *menu = menu_builder_to_string(fixture->menu_builder);
  g_assert(menu != NULL);
  int numlinks = count_nonoverlapping_substrings(menu, LINK_TAG);
  g_assert(numlinks == 2);
  g_assert(contains_substring(menu, LINK1_HREF));
  g_assert(contains_substring(menu, LINK1_TITLE));
  g_assert(contains_substring(menu, LINK2_HREF));
  g_assert(contains_substring(menu, LINK2_TITLE));

  free(menu);
  g_ptr_array_free(links, TRUE);
}

void test_menu_builder_link_title_encoding(MenuBuilderFixture *fixture,
                                           G_GNUC_UNUSED gconstpointer test_data) {
  GPtrArray *links = g_ptr_array_new_with_free_func(free_link);
  g_ptr_array_add(links, link_create(LINK3_HREF, LINK3_TITLE, LINK_ACTION_PARSE));

  menu_builder_append_link_list(fixture->menu_builder, links);
  char *menu = menu_builder_to_string(fixture->menu_builder);
  g_assert(menu != NULL);
  g_assert(contains_substring(menu, LINK3_TITLE_ENCODED));

  free(menu);
  g_ptr_array_free(links, TRUE);
}

void test_mainmenu() {
  char *menu = build_mainmenu(TEST_DATA_DIR "/websites");
  g_assert(menu);

  xmlDoc *doc = xmlReadMemory(menu, strlen(menu), "mainmenu.xml", NULL, 0);
  g_assert(doc);
  xmlNode *root = xmlDocGetRootElement(doc);
  g_assert(root);
  g_assert(xmlStrEqual(root->name, BAD_CAST "wvmenu"));

  unsigned int title_count = 0;
  unsigned int link_count = 0;
  unsigned int ul_count = 0;
  xmlNode *node;
  for (node = root->children; node; node = node->next) {
    if (xmlStrEqual(node->name, BAD_CAST "title")) {
      title_count++;

    } else if (xmlStrEqual(node->name, BAD_CAST "ul")) {
      ul_count++;
      
      xmlNode *li_node;
      for (li_node = node->children; li_node; li_node = li_node->next) {
        g_assert(xmlStrEqual(li_node->name, BAD_CAST "li"));
        
        xmlNode *a_node;
        for (a_node = li_node->children; a_node; a_node = a_node->next) {
          g_assert(xmlStrEqual(a_node->name, BAD_CAST "a"));
          g_assert(xmlHasProp(a_node, BAD_CAST "href"));
          link_count++;
        }
      }

    } else if (node->type != XML_TEXT_NODE) {
      g_assert_not_reached();
    }
  }

  g_assert(title_count == 1);
  g_assert(ul_count == 1);
  g_assert(link_count >= 1);

  xmlFreeDoc(doc);
  free(menu);
}

int count_nonoverlapping_substrings(const char *buffer, const char *substr) {
  if (!buffer || !substr)
    return 0;

  int count = 0;
  int substrlen = strlen(substr);
  const char *p = buffer;

  while ((p = strstr(p, substr))) {
    p += substrlen;
    count++;
  }

  return count;
}

bool contains_substring(const char *buffer, const char *substr) {
  if (!buffer || !substr)
    return false;

  return strstr(buffer, substr) != NULL;
}

void free_link(gpointer data) {
  link_delete((struct Link *)data);
}
