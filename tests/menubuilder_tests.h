#ifndef MENU_BUILDER_TESTS_H
#define MENU_BUILDER_TESTS_H

#include <glib.h>
#include "menubuilder.h"

typedef struct {
  struct MenuBuilder *menu_builder;
} MenuBuilderFixture;


void menu_builder_fixture_setup(MenuBuilderFixture *fixture,
                                gconstpointer test_data);
void menu_builder_fixture_teardown(MenuBuilderFixture *fixture,
                                   gconstpointer test_data);

void test_menu_builder_title(MenuBuilderFixture *fixture,
                             gconstpointer test_data);
void test_menu_builder_append_links(MenuBuilderFixture *fixture,
                                    gconstpointer test_data);
void test_menu_builder_link_title_encoding(MenuBuilderFixture *fixture,
                                           gconstpointer test_data);

void test_mainmenu();

#endif // MENU_BUILDER_TESTS_H
