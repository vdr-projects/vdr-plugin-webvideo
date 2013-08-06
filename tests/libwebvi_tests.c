#include <glib.h>
#include "context_tests.h"
#include "linktemplates_tests.h"
#include "linkextractor_tests.h"
#include "menubuilder_tests.h"
#include "pipe_tests.h"
#include "urlutils_tests.h"

int main(int argc, char** argv)
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/context/create", test_context_create);
  g_test_add("/context/templatepath", ContextFixture, 0, context_fixture_setup,
             test_context_template_path, context_fixture_teardown);
  g_test_add("/context/request", ContextFixture, 0, context_fixture_setup,
             test_context_request_processing, context_fixture_teardown);

  g_test_add("/linktemplates/load", LinkTemplatesFixture, 0,
             link_templates_fixture_setup, test_link_templates_load,
             link_templates_fixture_teardown);
  g_test_add("/linktemplates/get", LinkTemplatesFixture, 0,
             link_templates_fixture_setup, test_link_templates_get,
             link_templates_fixture_teardown);

  g_test_add("/linkextractor/extract", LinkExtractorFixture, 0,
             link_extractor_fixture_setup, test_link_extractor_extract,
             link_extractor_fixture_teardown);
  g_test_add("/linkextractor/unrecognized", LinkExtractorFixture, 0,
             link_extractor_fixture_setup, test_link_extractor_unrecognized_link,
             link_extractor_fixture_teardown);
  g_test_add("/linkextractor/append", LinkExtractorFixture, 0,
             link_extractor_fixture_setup, test_link_extractor_append,
             link_extractor_fixture_teardown);
  g_test_add("/linkextractor/invalidHtml", LinkExtractorFixture, 0,
             link_extractor_fixture_setup, test_link_extractor_invalid_html,
             link_extractor_fixture_teardown);
  g_test_add("/linkextractor/relativeURL", LinkExtractorFixture, 0,
             link_extractor_fixture_setup, test_link_extractor_relative_urls,
             link_extractor_fixture_teardown);
  g_test_add("/linkextractor/html_title", LinkExtractorFixture, 0,
             link_extractor_fixture_setup, test_link_extractor_html_title,
             link_extractor_fixture_teardown);
  g_test_add("/linkextractor/html_title", LinkExtractorFixture, 0,
             link_extractor_fixture_setup,
             test_link_extractor_title_overrides_content,
             link_extractor_fixture_teardown);

  g_test_add_func("/menubuilder/mainmenu", test_mainmenu);
  g_test_add("/menubuilder/title", MenuBuilderFixture, 0,
             menu_builder_fixture_setup, test_menu_builder_title,
             menu_builder_fixture_teardown);
  g_test_add("/menubuilder/links", MenuBuilderFixture, 0,
             menu_builder_fixture_setup, test_menu_builder_append_links,
             menu_builder_fixture_teardown);
  g_test_add("/menubuilder/encoding", MenuBuilderFixture, 0,
             menu_builder_fixture_setup, test_menu_builder_link_title_encoding,
             menu_builder_fixture_teardown);

  g_test_add_func("/pipe/one_component", test_pipe_one_component);
  g_test_add_func("/pipe/two_components", test_pipe_two_components);
  g_test_add_func("/pipe/failing_component", test_pipe_failing_component);
  g_test_add_func("/pipe/append_after_finish",
                  test_pipe_not_appending_after_finished);
  g_test_add_func("/pipe/state_change_after_finish",
                  test_pipe_state_not_chaning_after_finished);
  g_test_add_func("/pipe/fdset", test_pipe_fdset);
  g_test_add_func("/pipe/delete_all", test_pipe_delete_all);

  g_test_add_func("/urlutils/scheme", test_url_scheme);
  g_test_add_func("/urlutils/scheme_no_scheme", test_url_scheme_no_scheme);
  g_test_add_func("/urlutils/scheme_double", test_url_scheme_double_scheme);
  g_test_add_func("/urlutils/scheme_invalid_characters",
                  test_url_scheme_invalid_characters);
  g_test_add_func("/urlutils/root", test_url_root);
  g_test_add_func("/urlutils/root_path", test_url_root_full_path);
  g_test_add_func("/urlutils/root_query", test_url_root_terminated_by_query);
  g_test_add_func("/urlutils/path", test_url_path);
  g_test_add_func("/urlutils/path_slash", test_url_path_ends_in_slash);
  g_test_add_func("/urlutils/path_query", test_url_path_query);
  g_test_add_func("/urlutils/path_fragment", test_url_path_fragment);
  g_test_add_func("/urlutils/path_no_path", test_url_path_no_path);
  g_test_add_func("/urlutils/path_no_server", test_url_path_no_server);
  g_test_add_func("/urlutils/path_no_scheme", test_url_path_no_scheme);
  g_test_add_func("/urlutils/dirname", test_url_path_dirname);
  g_test_add_func("/urlutils/dirname_no_file", test_url_path_dirname_no_file);
  g_test_add_func("/urlutils/dirname_query", test_url_path_dirname_query);
  g_test_add_func("/urlutils/dirname_no_server",
                  test_url_path_dirname_no_server);
  g_test_add_func("/urlutils/query", test_url_path_and_query);
  g_test_add_func("/urlutils/query_no_query",
                  test_url_path_and_query_no_query);
  g_test_add_func("/urlutils/query_double_query",
                  test_url_path_and_query_double_query);
  g_test_add_func("/urlutils/query_fragment", test_url_path_and_query_fragment);
  g_test_add_func("/urlutils/query_no_path", test_url_path_and_query_no_path);
  g_test_add_func("/urlutils/rel2abs", test_url_rel2abs_file);
  g_test_add_func("/urlutils/rel2abs_root", test_url_rel2abs_root);
  g_test_add_func("/urlutils/rel2abs_query", test_url_rel2abs_query);
  g_test_add_func("/urlutils/rel2abs_double_query",
                  test_url_rel2abs_double_query);
  g_test_add_func("/urlutils/rel2abs_append_query",
                  test_url_rel2abs_append_query);
  g_test_add_func("/urlutils/rel2abs_fragment", test_url_rel2abs_fragment);
  g_test_add_func("/urlutils/rel2abs_append_fragment",
                  test_url_rel2abs_append_fragment);
  g_test_add_func("/urlutils/rel2abs_scheme", test_url_rel2abs_scheme);

  return g_test_run();
}
