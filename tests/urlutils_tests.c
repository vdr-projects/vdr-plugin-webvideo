#include "urlutils_tests.h"
#include "urlutils.h"

void test_url_scheme() {
  gchar *prefix = url_scheme("http://example.com/");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http") == 0);
  g_free(prefix);
}

void test_url_scheme_no_scheme() {
  gchar *prefix = url_scheme("example.com/");
  g_assert(prefix);
  g_assert(strcmp(prefix, "") == 0);
  g_free(prefix);
}

void test_url_scheme_double_scheme() {
  gchar *prefix = url_scheme("ftp://http://example.com/");
  g_assert(prefix);
  g_assert(strcmp(prefix, "ftp") == 0);
  g_free(prefix);
}

void test_url_scheme_invalid_characters() {
  gchar *prefix = url_scheme("invalid/http://example.com/");
  g_assert(prefix);
  g_assert(strcmp(prefix, "") == 0);
  g_free(prefix);
}

void test_url_scheme_mailto() {
  gchar *prefix = url_scheme("mailto:john.doe@example.com");
  g_assert(prefix);
  g_assert(strcmp(prefix, "mailto") == 0);
  g_free(prefix);
}

void test_url_root() {
  gchar *prefix = url_root("http://example.com/");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/") == 0);
  g_free(prefix);
}

void test_url_root_full_path() {
  gchar *prefix = url_root("http://example.com/path/to/file.html");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/") == 0);
  g_free(prefix);
}

void test_url_root_terminated_by_query() {
  gchar *prefix = url_root("http://example.com?query=path");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/") == 0);
  g_free(prefix);
}

void test_url_path() {
  gchar *prefix = url_path_including_file("http://example.com/path/to/file.html");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/to/file.html") == 0);
  g_free(prefix);
}

void test_url_path_ends_in_slash() {
  gchar *prefix = url_path_including_file("http://example.com/path/");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/") == 0);
  g_free(prefix);
}

void test_url_path_query() {
  gchar *prefix = url_path_including_file("http://example.com/path/to/file.html?foo=bar");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/to/file.html") == 0);
  g_free(prefix);
}

void test_url_path_fragment() {
  gchar *prefix = url_path_including_file("http://example.com/path/to/file.html#frag");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/to/file.html") == 0);
  g_free(prefix);
}

void test_url_path_no_path() {
  gchar *prefix = url_path_including_file("http://example.com");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/") == 0);
  g_free(prefix);
}

void test_url_path_no_server() {
  gchar *prefix = url_path_including_file("http:///path/file.html");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http:///path/file.html") == 0);
  g_free(prefix);
}

void test_url_path_no_scheme() {
  gchar *prefix = url_path_including_file("example.com/path/file.html");
  g_assert(prefix);
  g_assert(strcmp(prefix, "example.com/path/file.html") == 0);
  g_free(prefix);
}

void test_url_path_dirname() {
  gchar *prefix = url_path_dirname("http://example.com/path/to/file.html");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/to/") == 0);
  g_free(prefix);
}

void test_url_path_dirname_no_file() {
  gchar *prefix = url_path_dirname("http://example.com/path/to/");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/to/") == 0);
  g_free(prefix);
}

void test_url_path_dirname_query() {
  gchar *prefix = url_path_dirname("http://example.com/path/to/file.html?foo=bar");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/to/") == 0);
  g_free(prefix);
}

void test_url_path_dirname_no_server() {
  gchar *prefix = url_path_dirname("/path/to/file.html");
  g_assert(prefix);
  g_assert(strcmp(prefix, "/path/to/") == 0);
  g_free(prefix);
}

void test_url_path_and_query() {
  gchar *prefix = url_path_and_query("http://example.com/path/to/file.html?foo=1&bar=2");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/to/file.html?foo=1&bar=2") == 0);
  g_free(prefix);
}

void test_url_path_and_query_no_query() {
  gchar *prefix = url_path_and_query("http://example.com/path/to/file.html");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/to/file.html") == 0);
  g_free(prefix);
}

void test_url_path_and_query_double_query() {
  gchar *prefix = url_path_and_query("http://example.com/path/to/file.html?foo=1?bar=2");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/to/file.html?foo=1?bar=2") == 0);
  g_free(prefix);
}

void test_url_path_and_query_fragment() {
  gchar *prefix = url_path_and_query("http://example.com/path/to/file.html?foo=1&bar=2#frag");
  g_assert(prefix);
  g_assert(strcmp(prefix, "http://example.com/path/to/file.html?foo=1&bar=2") == 0);
  g_free(prefix);
}

void test_url_path_and_query_no_path() {
  gchar *prefix = url_path_and_query("?foo=1&bar=2");
  g_assert(prefix);
  g_assert(strcmp(prefix, "?foo=1&bar=2") == 0);
  g_free(prefix);
}

void test_url_rel2abs_file() {
  gchar *abs = relative_url_to_absolute("http://example.com/index.html",
                                        "file.html");
  g_assert(abs);
  g_assert(strcmp(abs, "http://example.com/file.html") == 0);
  g_free(abs);
}

void test_url_rel2abs_root() {
  gchar *abs = relative_url_to_absolute("http://example.com/path/index.html",
                                        "/another/path/file.html");
  g_assert(abs);
  g_assert(strcmp(abs, "http://example.com/another/path/file.html") == 0);
  g_free(abs);
}

void test_url_rel2abs_query() {
  gchar *abs = relative_url_to_absolute("http://example.com/index.html?foo=1",
                                        "?bar=2");
  g_assert(abs);
  g_assert(strcmp(abs, "http://example.com/index.html?bar=2") == 0);
  g_free(abs);
}

void test_url_rel2abs_double_query() {
  gchar *abs = relative_url_to_absolute("http://example.com/index.html?foo=1?bar=2",
                                        "?baz=3");
  g_assert(abs);
  g_assert(strcmp(abs, "http://example.com/index.html?baz=3") == 0);
  g_free(abs);
}

void test_url_rel2abs_append_query() {
  gchar *abs = relative_url_to_absolute("http://example.com/index.html",
                                        "?bar=2");
  g_assert(abs);
  g_assert(strcmp(abs, "http://example.com/index.html?bar=2") == 0);
  g_free(abs);
}

void test_url_rel2abs_fragment() {
  gchar *abs = relative_url_to_absolute("http://example.com/index.html#frag",
                                        "#bar");
  g_assert(abs);
  g_assert(strcmp(abs, "http://example.com/index.html#bar") == 0);
  g_free(abs);
}

void test_url_rel2abs_append_fragment() {
  gchar *abs = relative_url_to_absolute("http://example.com/index.html",
                                        "#bar");
  g_assert(abs);
  g_assert(strcmp(abs, "http://example.com/index.html#bar") == 0);
  g_free(abs);
}

void test_url_rel2abs_scheme() {
  gchar *abs = relative_url_to_absolute("http://example.com/index.html",
                                        "//server.org/path2/file2");
  g_assert(abs);
  g_assert(strcmp(abs, "http://server.org/path2/file2") == 0);
  g_free(abs);
}

void test_url_rel2abs_mailto_scheme() {
  gchar *abs = relative_url_to_absolute("http://example.com/index.html",
                                        "mailto:john.doe@example.com");
  g_assert(abs);
  g_assert(strcmp(abs, "mailto:john.doe@example.com") == 0);
  g_free(abs);
}
