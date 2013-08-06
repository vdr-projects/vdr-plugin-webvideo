#ifndef LINK_EXTRACTOR_TESTS_H
#define LINK_EXTRACTOR_TESTS_H

#include "linkextractor.h"
#include "linktemplates_tests.h"

typedef struct {
  struct LinkTemplates *templates;
  struct LinkExtractor *extractor;
} LinkExtractorFixture;

void link_extractor_fixture_setup(LinkExtractorFixture *fixture,
                                  gconstpointer test_data);
void link_extractor_fixture_teardown(LinkExtractorFixture *fixture,
                                     gconstpointer test_data);

void test_link_extractor_extract(LinkExtractorFixture *fixture,
                                 gconstpointer test_data);
void test_link_extractor_unrecognized_link(LinkExtractorFixture *fixture,
                                           gconstpointer test_data);
void test_link_extractor_append(LinkExtractorFixture *fixture,
                                gconstpointer test_data);
void test_link_extractor_invalid_html(LinkExtractorFixture *fixture,
                                      gconstpointer test_data);
void test_link_extractor_relative_urls(LinkExtractorFixture *fixture,
                                       gconstpointer test_data);
void test_link_extractor_html_title(LinkExtractorFixture *fixture,
                                    G_GNUC_UNUSED gconstpointer test_data);
 
void test_link_extractor_xml(LinkExtractorFixture *fixture,
                             gconstpointer test_data);


#endif // LINK_EXTRACTOR_TESTS_H
