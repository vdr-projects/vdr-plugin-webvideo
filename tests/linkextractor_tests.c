#include <string.h>
#include "linkextractor_tests.h"

#define LINK_TEMPLATE_PATH TEST_DATA_DIR "/links"

#define BASEURL "http://example.com/index.html"

#define HTML1_HREF "http://example.com/test/link"
#define HTML1_TITLE "Test link"
#define HTML1 "<html><body>" \
  "<a href=\"" HTML1_HREF "\">" HTML1_TITLE "</a>" \
  "</body></html>"

#define HTML2_HREF "http://example.com/test/link"
#define HTML2_TITLE "Test link"
#define HTML2 "<html><body>" \
  "<a href=\"" HTML2_HREF "\">" HTML2_TITLE "</a>" \
  "<a href=\"http://example.com/index.html\">This is an unrecognized link</a>" \
  "</body></html>"

#define HTML3_HEADER "<html><body>"
#define HTML3_HREF "http://example.com/test/link"
#define HTML3_TITLE "Test link"
#define HTML3_BODY "<a href=\"" HTML3_HREF "\">" HTML3_TITLE "</a>"
#define HTML3_FOOTER "</body></html>"
#define HTML_INVALID "<a hrefxxx=0</a>"

#define HTML4_HREF "/test/link"
#define HTML4_HREF_ABSOLUTE "http://example.com" HTML4_HREF
#define HTML4_TITLE "Relative link"
#define HTML4 "<html><body>" \
  "<a href=\"" HTML4_HREF "\">" HTML4_TITLE "</a>" \
  "</body></html>"

#define HTML5_HREF "http://example.com/test/link"
#define HTML5_TITLE "Test link"
#define HTML5 "<html><body>" \
  "<a href=\"" HTML5_HREF "\"><span><b>   Test</b></span> <span>link</span></a>" \
  "</body></html>"

#define HTML6_HREF "http://example.com/test/link"
#define HTML6_TITLE "Test link"
#define HTML6 "<html><body>" \
  "<a href=\"" HTML6_HREF "\" title=\"" HTML6_TITLE "\">ignored</a>" \
  "</body></html>"

#define HTML_DUPLICATE_HREF1 "http://example.com/test/1"
#define HTML_DUPLICATE_TITLE1 "First link"
#define HTML_DUPLICATE_HREF2 "http://example.com/test/2"
#define HTML_DUPLICATE_TITLE2 "Second link"
#define HTML_DUPLICATE_LINKS "<html><body>" \
  "<a href=\"" HTML_DUPLICATE_HREF1 "\">d1</a>" \
  "<a href=\"" HTML_DUPLICATE_HREF1 "\">" HTML_DUPLICATE_TITLE1 "</a>" \
  "<a href=\"" HTML_DUPLICATE_HREF2 "\">" HTML_DUPLICATE_TITLE2 "</a>" \
  "<a href=\"" HTML_DUPLICATE_HREF2 "\">d2</a>" \
  "</body></html>"


void link_extractor_fixture_setup(LinkExtractorFixture *fixture,
                                  gconstpointer test_data) {
  fixture->templates = link_templates_create();
  g_assert(fixture->templates != NULL);
  link_templates_load(fixture->templates, LINK_TEMPLATE_PATH);

  fixture->extractor = link_extractor_create(fixture->templates, BASEURL);
  g_assert(fixture->extractor != NULL);
}

void link_extractor_fixture_teardown(LinkExtractorFixture *fixture,
                                     gconstpointer test_data) {
  link_extractor_delete(fixture->extractor);
  link_templates_delete(fixture->templates);
}

void test_link_extractor_extract(LinkExtractorFixture *fixture,
                                 G_GNUC_UNUSED gconstpointer test_data) {
  GPtrArray *links;
  link_extractor_append(fixture->extractor, HTML1, strlen(HTML1));
  links = link_extractor_get_links(fixture->extractor);
  g_assert(links);
  g_assert(links->len == 1);
  const struct Link *link = g_ptr_array_index(links, 0);
  const char *href = link_get_href(link);
  g_assert(href);
  g_assert(strcmp(href, HTML1_HREF) == 0);
  const char *title = link_get_title(link);
  g_assert(title);
  g_assert(strcmp(title, HTML1_TITLE) == 0);
  g_ptr_array_free(links, TRUE);
}

void test_link_extractor_unrecognized_link(LinkExtractorFixture *fixture,
                                           G_GNUC_UNUSED gconstpointer test_data) {
  GPtrArray *links;
  link_extractor_append(fixture->extractor, HTML2, strlen(HTML2));
  links = link_extractor_get_links(fixture->extractor);
  g_assert(links);
  g_assert(links->len == 1);
  const struct Link *link = g_ptr_array_index(links, 0);
  const char *href = link_get_href(link);
  g_assert(href);
  g_assert(strcmp(href, HTML2_HREF) == 0);
  const char *title = link_get_title(link);
  g_assert(title);
  g_assert(strcmp(title, HTML2_TITLE) == 0);
  g_ptr_array_free(links, TRUE);
}

void test_link_extractor_append(LinkExtractorFixture *fixture,
                                G_GNUC_UNUSED gconstpointer test_data) {
  GPtrArray *links;
  link_extractor_append(fixture->extractor, HTML3_HEADER, strlen(HTML3_HEADER));
  link_extractor_append(fixture->extractor, HTML3_BODY, strlen(HTML3_BODY));
  link_extractor_append(fixture->extractor, HTML3_FOOTER, strlen(HTML3_FOOTER));
  links = link_extractor_get_links(fixture->extractor);
  g_assert(links);
  g_assert(links->len == 1);
  const struct Link *link = g_ptr_array_index(links, 0);
  const char *href = link_get_href(link);
  g_assert(href);
  g_assert(strcmp(href, HTML3_HREF) == 0);
  const char *title = link_get_title(link);
  g_assert(title);
  g_assert(strcmp(title, HTML3_TITLE) == 0);
  g_ptr_array_free(links, TRUE);
}

void test_link_extractor_invalid_html(LinkExtractorFixture *fixture,
                                      G_GNUC_UNUSED gconstpointer test_data) {
  GPtrArray *links;
  link_extractor_append(fixture->extractor, HTML_INVALID, strlen(HTML_INVALID));
  links = link_extractor_get_links(fixture->extractor);
  g_assert(links);
  g_assert(links->len == 0);
  g_ptr_array_free(links, TRUE);
}

void test_link_extractor_relative_urls(LinkExtractorFixture *fixture,
                                       G_GNUC_UNUSED gconstpointer test_data) {
  GPtrArray *links;
  link_extractor_append(fixture->extractor, HTML4, strlen(HTML4));
  links = link_extractor_get_links(fixture->extractor);
  g_assert(links);
  g_assert(links->len == 1);
  const struct Link *link = g_ptr_array_index(links, 0);
  const char *href = link_get_href(link);
  g_assert(href);
  g_assert(strcmp(href, HTML4_HREF_ABSOLUTE) == 0);
  const char *title = link_get_title(link);
  g_assert(title);
  g_assert(strcmp(title, HTML4_TITLE) == 0);
  g_ptr_array_free(links, TRUE);
}

void test_link_extractor_html_title(LinkExtractorFixture *fixture,
                                    G_GNUC_UNUSED gconstpointer test_data) {
  GPtrArray *links;
  link_extractor_append(fixture->extractor, HTML5, strlen(HTML5));
  links = link_extractor_get_links(fixture->extractor);
  g_assert(links);
  g_assert(links->len == 1);
  const struct Link *link = g_ptr_array_index(links, 0);
  const char *href = link_get_href(link);
  g_assert(href);
  g_assert(strcmp(href, HTML5_HREF) == 0);
  const char *title = link_get_title(link);
  g_assert(title);
  g_assert(strcmp(title, HTML5_TITLE) == 0);
  g_ptr_array_free(links, TRUE);
}

void test_link_extractor_title_overrides_content(
  LinkExtractorFixture *fixture, G_GNUC_UNUSED gconstpointer test_data)
{
  GPtrArray *links;
  link_extractor_append(fixture->extractor, HTML6, strlen(HTML6));
  links = link_extractor_get_links(fixture->extractor);
  g_assert(links);
  g_assert(links->len == 1);
  const struct Link *link = g_ptr_array_index(links, 0);
  const char *href = link_get_href(link);
  g_assert(href);
  g_assert(strcmp(href, HTML6_HREF) == 0);
  const char *title = link_get_title(link);
  g_assert(title);
  g_assert(strcmp(title, HTML6_TITLE) == 0);
  g_ptr_array_free(links, TRUE);
}

void test_link_extractor_remove_duplicates(LinkExtractorFixture *fixture,
                                           gconstpointer test_data)
{
  link_extractor_append(fixture->extractor, HTML_DUPLICATE_LINKS,
                        strlen(HTML_DUPLICATE_LINKS));
  GPtrArray *links = link_extractor_get_links(fixture->extractor);
  g_assert(links);
  g_assert(links->len == 2);

  const struct Link *link;
  const char *href;
  const char *title;
  link = g_ptr_array_index(links, 0);
  href = link_get_href(link);
  g_assert(href);
  g_assert(strcmp(href, HTML_DUPLICATE_HREF1) == 0);
  title = link_get_title(link);
  g_assert(title);
  g_assert(strcmp(title, HTML_DUPLICATE_TITLE1) == 0);

  link = g_ptr_array_index(links, 1);
  href = link_get_href(link);
  g_assert(href);
  g_assert(strcmp(href, HTML_DUPLICATE_HREF2) == 0);
  title = link_get_title(link);
  g_assert(title);
  g_assert(strcmp(title, HTML_DUPLICATE_TITLE2) == 0);

  g_ptr_array_free(links, TRUE);
}
