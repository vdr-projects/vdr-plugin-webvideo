#ifndef URLUTILS_TESTS_H
#define URLUTILS_TESTS_H

#include <glib.h>

void test_url_scheme();
void test_url_scheme_no_scheme();
void test_url_scheme_double_scheme();
void test_url_scheme_invalid_characters();
void test_url_root();
void test_url_root_full_path();
void test_url_root_terminated_by_query();
void test_url_path();
void test_url_path_ends_in_slash();
void test_url_path_query();
void test_url_path_fragment();
void test_url_path_no_path();
void test_url_path_no_server();
void test_url_path_no_scheme();
void test_url_path_dirname();
void test_url_path_dirname_no_file();
void test_url_path_dirname_query();
void test_url_path_dirname_no_server();
void test_url_path_and_query();
void test_url_path_and_query_no_query();
void test_url_path_and_query_double_query();
void test_url_path_and_query_fragment();
void test_url_path_and_query_no_path();
void test_url_rel2abs_file();
void test_url_rel2abs_root();
void test_url_rel2abs_query();
void test_url_rel2abs_double_query();
void test_url_rel2abs_append_query();
void test_url_rel2abs_fragment();
void test_url_rel2abs_append_fragment();
void test_url_rel2abs_scheme();

#endif // URLUTILS_TESTS_H
