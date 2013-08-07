#include <string.h>
#include "urlutils.h"

static const gchar *skip_scheme(const char *url);
static gboolean is_scheme_character(gchar c);
static gboolean has_scheme(const gchar *url);

gchar *relative_url_to_absolute(const gchar *baseurl, const gchar *href) {
  gchar *absolute;
  gchar *prefix;
  const gchar *postfix = href;
  if (!href || !baseurl)
    return NULL;

  if ((href[0] == '/') && (href[1] == '/')) {
    gchar *scheme = url_scheme(baseurl);
    prefix = g_strconcat(scheme, ":", NULL);
    g_free(scheme);
  } else if (href[0] == '/') {
    prefix = url_root(baseurl);
    if (g_str_has_suffix(prefix, "/")) {
      postfix = href+1;
    }
  } else if (href[0] == '?') {
    prefix = url_path_including_file(baseurl);
  } else if (href[0] =='#') {
    prefix = url_path_and_query(baseurl);
  } else if (!has_scheme(href)) {
    prefix = url_path_dirname(baseurl);
  } else {
    // href is absolute
    prefix = NULL;
  }

  if (prefix) {
    absolute = g_strconcat(prefix, postfix, NULL);
    g_free(prefix);
  } else {
    absolute = g_strdup(href);
  }

  return absolute;
}

gchar *url_scheme(const gchar *url) {
  if (!url)
    return NULL;

  const gchar *scheme_end = skip_scheme(url);
  if (scheme_end == url) {
    /* no scheme */
    return g_strdup("");
  } else {
    /* scheme found */
    /* Do not include the scheme postfix, which is either :// or : */
    int scheme_postfix_len;
    if ((scheme_end >= url+3) && (strncmp(scheme_end-3, "://", 3) == 0)) {
      scheme_postfix_len = 3;
    } else {
      g_assert((scheme_end >= url+1) && (scheme_end[-1] == ':'));
      scheme_postfix_len = 1;
    }
    return g_strndup(url, scheme_end - scheme_postfix_len - url);
  }
}

gchar *url_root(const gchar *url) {
  if (!url)
    return NULL;

  const gchar *authority = skip_scheme(url);
  size_t authority_len = strcspn(authority, "/?#");
  const gchar *authority_end = authority + authority_len;
  gchar *root_without_slash = g_strndup(url, authority_end - url);
  gchar *root = g_strconcat(root_without_slash, "/", NULL);
  g_free(root_without_slash);
  return root;
}

gchar *url_path_including_file(const gchar *url) {
  if (!url)
    return NULL;

  const gchar *scheme_end = skip_scheme(url);
  size_t path_len = strcspn(scheme_end, "?#");
  const gchar *end = scheme_end + path_len;
  gchar *path = g_strndup(url, end - url);
  if (memchr(scheme_end, '/', path_len) == NULL) {
    gchar *path2 = g_strconcat(path, "/", NULL);
    g_free(path);
    path = path2;
  }

  return path;
}

gchar *url_path_dirname(const gchar *url) {
  if (!url)
    return NULL;

  const gchar *scheme_end = skip_scheme(url);
  size_t path_len = strcspn(scheme_end, "?#");
  const gchar *p = scheme_end + path_len;
  while ((p >= url) && (*p != '/')) {
    p--;
  }

  if (*p == '/') {
    return g_strndup(url, (p+1) - url);
  } else {
    return g_strdup("/");
  }
}

gchar *url_path_and_query(const gchar *url) {
  if (!url)
    return NULL;

  const gchar *scheme_end = skip_scheme(url);
  size_t path_len = strcspn(scheme_end, "#");
  const gchar *end = scheme_end + path_len;
  return g_strndup(url, end - url);
}

gboolean has_scheme(const gchar *url) {
  return skip_scheme(url) != url;
}

const gchar *skip_scheme(const char *url) {
  const gchar *c = url;
  while (is_scheme_character(*c)) {
    c++;
  }

  if (strncmp(c, "://", 3) == 0) {
    // scheme found
    return c + 3;
  } else if (c[0] == ':') {
    // scheme without // such as mailto:
    return c + 1;
  } else {
    // schemeless url
    return url;
  }
}

gboolean is_scheme_character(gchar c) {
  return g_ascii_isalnum(c) || (c == '+') || (c == '-') || (c == '.');
}
