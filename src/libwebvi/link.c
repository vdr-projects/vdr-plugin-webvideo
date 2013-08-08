#include <stdlib.h>
#include <glib.h>
#include "link.h"

struct Link {
  gchar *href;
  gchar *title;
  LinkActionType type;
};

struct LinkAction {
  LinkActionType type;
  gchar *command;
};

struct Link *link_create(const char *href, const char *title, LinkActionType type) {
  struct Link *self = malloc(sizeof(struct Link));
  self->href = g_strdup(href ? href : "");
  self->title = g_strdup(title ? title : "");
  self->type = type;
  return self;
}

const char *link_get_href(const struct Link *self) {
  return self->href;
}

const char *link_get_title(const struct Link *self) {
  return self->title;
}

LinkActionType link_get_type(const struct Link *self) {
  return self->type;
}

void link_delete(struct Link *self) {
  g_free(self->href);
  g_free(self->title);
  free(self);
}

void g_free_link(gpointer data) {
  link_delete((struct Link *)data);
}

struct LinkAction *link_action_create(LinkActionType type, const char *command) {
  struct LinkAction *self = malloc(sizeof(struct LinkAction));
  self->type = type;
  self->command = g_strdup(command ? command : "");
  return self;
}

LinkActionType link_action_get_type(const struct LinkAction *self) {
  return self->type;
}

const char *link_action_get_command(const struct LinkAction *self) {
  return self->command;
}

void link_action_delete(struct LinkAction *self) {
  if (self) {
    g_free(self->command);
    free(self);
  }
}

struct ActionTypeMessage {
  LinkActionType type;
  const char *message;
};

const char *link_action_type_to_string(LinkActionType atype) {
  static struct ActionTypeMessage messages[] = 
    {{LINK_ACTION_PARSE, "regular link"},
     {LINK_ACTION_STREAM, "stream"},
     {LINK_ACTION_EXTERNAL_COMMAND, "external command"}};

  for (int i=0;  i<(sizeof(messages)/sizeof(messages[0])); i++) {
    if (atype == messages[i].type) {
      return messages[i].message;
    }
  }

  return "???";
}
