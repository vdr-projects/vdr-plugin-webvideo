#ifndef LINK_TEMPLATES_TESTS_H
#define LINK_TEMPLATES_TESTS_H

#include <unistd.h>
#include <glib.h>
#include "linktemplates.h"

typedef struct {
  struct LinkTemplates *templates;
} LinkTemplatesFixture;


void link_templates_fixture_setup(LinkTemplatesFixture *fixture,
                               gconstpointer test_data);
void link_templates_fixture_teardown(LinkTemplatesFixture *fixture,
                                  gconstpointer test_data);

void test_link_templates_load(LinkTemplatesFixture *fixture,
                           gconstpointer test_data);
void test_link_templates_get(LinkTemplatesFixture *fixture,
                          gconstpointer test_data);

#endif // LINK_TEMPLATES_TESTS_H
