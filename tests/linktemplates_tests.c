#include "linktemplates_tests.h"

#define LINK_TEMPLATES_TEST_FILE TEST_DATA_DIR "/links"
#define TEST_REGULAR_LINK "http://example.com/test/testpage.html"
#define TEST_IGNORED_LINK "http://example.com/ignoredpage.html"
#define TEST_STREAM_LINK "http://example.com/video/videopage.html"

void link_templates_fixture_setup(LinkTemplatesFixture *fixture,
                                  G_GNUC_UNUSED gconstpointer test_data)
{
  fixture->templates = link_templates_create();
  g_assert(fixture->templates != NULL);
  link_templates_load(fixture->templates, LINK_TEMPLATES_TEST_FILE);
}

void link_templates_fixture_teardown(LinkTemplatesFixture *fixture,
                                     G_GNUC_UNUSED gconstpointer test_data)
{
  link_templates_delete(fixture->templates);
  fixture->templates = NULL;
}

void test_link_templates_load(LinkTemplatesFixture *fixture,
                              G_GNUC_UNUSED gconstpointer test_data)
{
  g_assert(link_templates_size(fixture->templates) >= 1);
}

void test_link_templates_get(LinkTemplatesFixture *fixture,
                             G_GNUC_UNUSED gconstpointer test_data)
{
  const struct LinkAction *action;
  action = link_templates_get_action(fixture->templates, TEST_REGULAR_LINK);
  g_assert(action);
  g_assert(link_action_get_type(action) == LINK_ACTION_PARSE);

  action = link_templates_get_action(fixture->templates, TEST_IGNORED_LINK);
  g_assert(!action);

  action = link_templates_get_action(fixture->templates, TEST_STREAM_LINK);
  g_assert(action);
  g_assert(link_action_get_type(action) == LINK_ACTION_EXTERNAL_COMMAND);
  const char *command = link_action_get_command(action);
  g_assert(command);
  g_assert(strcmp(command, "test_command") == 0);
}
