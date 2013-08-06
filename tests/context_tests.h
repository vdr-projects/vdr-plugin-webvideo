#ifndef CONTEXT_TESTS_H
#define CONTEXT_TESTS_H

#include <glib.h>
#include "webvicontext.h"

typedef struct {
  WebviCtx handle;
  struct WebviContext *context;
} ContextFixture;

void context_fixture_setup(ContextFixture *fixture,
                           gconstpointer test_data);
void context_fixture_teardown(ContextFixture *fixture,
                              gconstpointer test_data);

void test_context_create(void);
void test_context_template_path(ContextFixture *fixture,
                                gconstpointer test_data);
void test_context_request_processing(ContextFixture *fixture,
                                     gconstpointer test_data);

#endif // CONTEXT_TESTS_H
