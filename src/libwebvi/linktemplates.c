#include <string.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include "linktemplates.h"
#include "link.h"

#define STREAM_LIBQUVI_SELECTOR "stream:libquvi"
#define STREAM_LIBQUVI_SELECTOR_LEN strlen(STREAM_LIBQUVI_SELECTOR)
#define STREAM_SELECTOR "stream:"
#define STREAM_SELECTOR_LEN strlen(STREAM_SELECTOR)
#define EXT_CMD_SELECTOR "bin:"
#define EXT_CMD_SELECTOR_LEN strlen(EXT_CMD_SELECTOR)

struct LinkTemplates {
  GPtrArray *matcher;
};

struct LinkMatcher {
  GRegex *pattern;
  LinkAction *action;
};

static void free_link_matcher(gpointer link);
static struct LinkMatcher *parse_line(const char *line);
static LinkAction *parse_action(const gchar *actionstr);

LinkTemplates *link_templates_create() {
  LinkTemplates *config = malloc(sizeof(LinkTemplates));
  if (!config)
    return NULL;
  memset(config, 0, sizeof(LinkTemplates));
  config->matcher = g_ptr_array_new_with_free_func(free_link_matcher);
  return config;
}

void link_templates_delete(LinkTemplates *conf) {
  g_ptr_array_free(conf->matcher, TRUE);
  free(conf);
}

void link_templates_load(LinkTemplates *conf, const char *filename) {
  g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
        "Loading matchers from %s", filename);
  FILE *file = fopen(filename, "r");
  if (!file) {
    g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
          "Failed to read file %s", filename);
    return;
  }

  char line[1024];
  while (fgets(line, sizeof line, file)) {
    struct LinkMatcher *link = parse_line(line);
    if (link) {
      g_ptr_array_add(conf->matcher, link);
    }
  }

  fclose(file);
}

struct LinkMatcher *parse_line(const char *line) {
  if (!line)
    return NULL;

  const char *p = line;
  while (*p == ' ')
    p++;

  if (*p == '\0' || *p == '#' || *p == '\n' || *p == '\r')
    return NULL;

  const char *end = line + strlen(line);
  while ((end-1 >= p) && 
         (end[-1] == ' ' || end[-1] == '\r' || end[-1] == '\n' || end[-1] == '\t'))
    end--;

  if (end <= p)
    return NULL;

  const char *tab = memchr(p, '\t', end-p);
  gchar *pattern;
  LinkAction *action;
  if (tab && tab < end) {
    pattern = g_strndup(p, tab-p);
    const char *cmdstart = tab+1;
    gchar *action_field = g_strndup(cmdstart, end-cmdstart);
    action = parse_action(action_field);
    g_free(action_field);
  } else {
    pattern = g_strndup(p, end-p);
    action = link_action_create(LINK_ACTION_PARSE, NULL);
  }

  if (!action) {
    g_free(pattern);
    return NULL;
  }

  g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
        "Compiling pattern %s %s %s", pattern, 
        link_action_type_to_string(link_action_get_type(action)),
        link_action_get_command(action));

  struct LinkMatcher *matcher = malloc(sizeof(struct LinkMatcher));
  GError *err = NULL;
  matcher->pattern = g_regex_new(pattern, G_REGEX_OPTIMIZE, 0, &err);
  matcher->action = action;

  if (err) {
    g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
          "Error compiling pattern %s: %s", pattern, err->message);
    g_error_free(err);
    free_link_matcher(matcher);
    matcher = NULL;
  }

  g_free(pattern);
  return matcher;
}

LinkAction *parse_action(const gchar *actionstr) {
  if (!actionstr)
    return NULL;

  if (*actionstr == '\0') {
    return link_action_create(LINK_ACTION_PARSE, NULL);
  } else if (strcmp(actionstr, STREAM_LIBQUVI_SELECTOR) == 0) {
    return link_action_create(LINK_ACTION_STREAM_LIBQUVI, NULL);
  } else if (strncmp(actionstr, EXT_CMD_SELECTOR, EXT_CMD_SELECTOR_LEN) == 0) {
    const gchar *command = actionstr + EXT_CMD_SELECTOR_LEN;
    return link_action_create(LINK_ACTION_EXTERNAL_COMMAND, command);
  } else if (strncmp(actionstr, STREAM_SELECTOR, STREAM_SELECTOR_LEN) == 0) {
    g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
          "Unknown streamer %s in link template file", actionstr);
    return NULL;
  } else {
    g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
          "Invalid action %s in link template file", actionstr);
    return NULL;
  }
}

const LinkAction *link_templates_get_action(const LinkTemplates *conf,
                                                   const char *url)
{
  for (int i=0; i<link_templates_size(conf); i++) {
    struct LinkMatcher *matcher = g_ptr_array_index(conf->matcher, i);
    if (g_regex_match(matcher->pattern, url, 0, NULL))
    {
      return matcher->action;
    }
  }

  return NULL;
}

int link_templates_size(const LinkTemplates *conf) {
  return (int)conf->matcher->len;
}

void free_link_matcher(gpointer ptr) {
  if (ptr) {
    struct LinkMatcher *matcher = (struct LinkMatcher *)ptr;
    if (matcher->pattern)
      g_regex_unref(matcher->pattern);
    if (matcher->action)
      link_action_delete(matcher->action);
    free(matcher);
  }
}
