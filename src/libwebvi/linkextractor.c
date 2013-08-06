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
static void getTextContent(TidyDoc tdoc, TidyNode node, TidyBuffer* buf);

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
          TidyBuffer titlebuf;
          tidyBufInit(&titlebuf);
          getTextContent(tdoc, child, &titlebuf);
          tidyBufPutByte(&titlebuf, '\0');
          gchar *title = g_strdup((const gchar*)titlebuf.bp);
          g_strstrip(title);
          LinkActionType type = link_action_get_type(action);
          Link *link = link_create(absolute_href, title, type);
          g_ptr_array_add(links_found, link);
          g_free(title);
          tidyBufFree(&titlebuf);
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

void getTextContent(TidyDoc tdoc, TidyNode node, TidyBuffer* buf) {
  if (tidyNodeGetType(node) == TidyNode_Text) {
    TidyBuffer content;
    tidyBufInit(&content);
    tidyNodeGetValue(tdoc, node, &content);
    tidyBufAppend(buf, content.bp, content.size);
  } else {
    TidyNode child;
    for (child = tidyGetChild(node); child; child = tidyGetNext(child)) {
      getTextContent(tdoc, child, buf);
    }
  }
}

void free_link(gpointer p) {
  link_delete((Link *)p);
}
