#include <string.h>
#ifdef HAVE_TIDY_ULONG_VERSION
#define __USE_MISC
#include <sys/types.h>
#undef __USE_MISC
#endif
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include "linkextractor.h"
#include "urlutils.h"

#define MENU_HEADER "<?xml version=\"1.0\"?><wvmenu>"
#define MENU_FOOTER "</wvmenu>"

struct LinkExtractor {
  const LinkTemplates *link_templates;
  TidyBuffer html_buffer;
  gchar *baseurl;
};

static GPtrArray *extract_links(const LinkExtractor *self, TidyDoc tdoc);
static void free_link(gpointer p);
static void get_links_recursively(TidyDoc tdoc,
                                  TidyNode node,
                                  const LinkTemplates *link_templates,
                                  const gchar *baseurl,
                                  GPtrArray *links_found);
static gchar *parse_link_title(TidyDoc tdoc, TidyNode node);
static void get_text_content(TidyDoc tdoc, TidyNode node, TidyBuffer* buf);
static void remove_duplicate_urls(GPtrArray *links);
static void insert_links_with_longest_titles(gpointer data, gpointer userdata);

LinkExtractor *link_extractor_create(const LinkTemplates *link_templates, const gchar *baseurl) {
  LinkExtractor *extractor;
  extractor = malloc(sizeof(LinkExtractor));
  memset(extractor, 0, sizeof(LinkExtractor));
  extractor->link_templates = link_templates;
  tidyBufInit(&extractor->html_buffer);
  extractor->baseurl = baseurl ? g_strdup(baseurl) : g_strdup("");
  return extractor;
}

void link_extractor_delete(LinkExtractor *self) {
  if (self) {
    tidyBufFree(&self->html_buffer);
    g_free(self->baseurl);
    free(self);
  }
}

void link_extractor_append(LinkExtractor *self, const char *buf, size_t len) {
  tidyBufAppend(&self->html_buffer, (void *)buf, len);
}

GPtrArray *link_extractor_get_links(LinkExtractor *self) {
  GPtrArray *links = NULL;
  TidyDoc tdoc;
  int err;
  TidyBuffer errbuf; // swallow errors here instead of printing to stderr

  tdoc = tidyCreate();
  tidyOptSetBool(tdoc, TidyForceOutput, yes);
  tidyOptSetInt(tdoc, TidyWrapLen, 4096);
  tidyBufInit(&errbuf);
  tidySetErrorBuffer(tdoc, &errbuf);

  err = tidyParseBuffer(tdoc, &self->html_buffer);
  if (err >= 0) {
    err = tidyCleanAndRepair(tdoc);
    if ( err >= 0 ) {
      links = extract_links(self, tdoc);
      remove_duplicate_urls(links);
    }
  }

  tidyBufFree(&errbuf);
  tidyRelease(tdoc);

  return links;
}

GPtrArray *extract_links(const LinkExtractor *self, TidyDoc tdoc) {
  GPtrArray *links = g_ptr_array_new_full(0, free_link);
  TidyNode root = tidyGetBody(tdoc);
  get_links_recursively(tdoc, root, self->link_templates, self->baseurl, links);
  return links;
}

void get_links_recursively(TidyDoc tdoc, TidyNode node,
                           const LinkTemplates *link_templates,
                           const gchar *baseurl,
                           GPtrArray *links_found) {
  TidyNode child;
  for (child = tidyGetChild(node); child; child = tidyGetNext(child)) {
    if (tidyNodeIsA(child)) {
      TidyAttr href_attr = tidyAttrGetById(child, TidyAttr_HREF);
      ctmbstr href = tidyAttrValue(href_attr);
      if (href && *href != '\0' && href[strlen(href)-1] != '#') {
        gchar *absolute_href = relative_url_to_absolute(baseurl, href);
        const LinkAction *action = \
          link_templates_get_action(link_templates, absolute_href);
        if (action) {
          LinkActionType type = link_action_get_type(action);
          gchar *title = parse_link_title(tdoc, child);
          Link *link = link_create(absolute_href, title, type);
          g_ptr_array_add(links_found, link);
          g_free(title);
        }
        g_free(absolute_href);
      }
    } else {
      TidyNodeType node_type = tidyNodeGetType(node);
      if (node_type == TidyNode_Root || node_type == TidyNode_Start) {
        get_links_recursively(tdoc, child, link_templates, baseurl, links_found);
      }
    }
  }
}

gchar *parse_link_title(TidyDoc tdoc, TidyNode node) {
  gchar *title;
  TidyAttr title_attr = tidyAttrGetById(node, TidyAttr_TITLE);
  if (title_attr) {
    ctmbstr tidy_title = tidyAttrValue(title_attr);
    title = g_strdup(tidy_title);
  } else {
    TidyBuffer titlebuf;
    tidyBufInit(&titlebuf);
    get_text_content(tdoc, node, &titlebuf);
    tidyBufPutByte(&titlebuf, '\0');
    title = g_strdup((const gchar*)titlebuf.bp);
    tidyBufFree(&titlebuf);
  }

  g_strstrip(title);
  return title;
}

void get_text_content(TidyDoc tdoc, TidyNode node, TidyBuffer* buf) {
  if (tidyNodeGetType(node) == TidyNode_Text) {
    TidyBuffer content;
    tidyBufInit(&content);
    tidyNodeGetValue(tdoc, node, &content);
    tidyBufAppend(buf, content.bp, content.size);
  } else {
    TidyNode child;
    for (child = tidyGetChild(node); child; child = tidyGetNext(child)) {
      get_text_content(tdoc, child, buf);
    }
  }
}

void remove_duplicate_urls(GPtrArray *links) {
  /* Remove links with duplicated URLs. Keep the link with the longest
   * title on the assumption that it is more informative. Keep the
   * sort order. */

  if (links->len <= 1)
    return;

  /* seen_urls maps an href (char *) to Link * which has the longest
   * title. Both keys and values are borrowed references!
   */
  GHashTable *seen_urls = g_hash_table_new(g_str_hash, g_str_equal);
  g_ptr_array_foreach(links, insert_links_with_longest_titles, seen_urls);

  /* Delete links which are not in the hash table */
  int i = 0;
  while (i < links->len) {
    Link *link = g_ptr_array_index(links, i);
    const char *href = link_get_href(link);
    Link *link_with_longest_title =
      (Link *)g_hash_table_lookup(seen_urls, href);

    if (link == link_with_longest_title) {
      i++;
    } else {
      g_ptr_array_remove_index(links, i);
    }
  }

  g_hash_table_unref(seen_urls);
}

void insert_links_with_longest_titles(gpointer data, gpointer userdata) {
  Link *link = (Link *)data;
  GHashTable *hashtable = (GHashTable *)userdata;
  int title_len = strlen(link_get_title(link));
  const char *href = link_get_href(link);
  Link *prev_link = (Link *)g_hash_table_lookup(hashtable, href);
  int prev_link_title_len = 0;
  if (prev_link)
    prev_link_title_len = strlen(link_get_title(prev_link));
  if (!prev_link || (prev_link_title_len < title_len)) {
    g_hash_table_replace(hashtable, (gpointer)href, (gpointer)link);
  }
}

void free_link(gpointer p) {
  link_delete((Link *)p);
}
