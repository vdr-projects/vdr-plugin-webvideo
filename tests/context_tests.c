#include "context_tests.h"
#include "request.h"

void context_fixture_setup(ContextFixture *fixture,
                           gconstpointer test_data)
{
  fixture->handle = webvi_context_initialize();
  g_assert(fixture->handle != 0);
  fixture->context = get_context_by_handle(fixture->handle);
  g_assert(fixture->context != NULL);
}

void context_fixture_teardown(ContextFixture *fixture,
                              gconstpointer test_data)
{
  g_assert(fixture->handle != 0);
  g_assert(fixture->context != NULL);
  webvi_context_cleanup(fixture->handle);
  fixture->handle = 0;
  fixture->context = NULL;
}

void test_context_create(void) {
  WebviCtx ctx = webvi_initialize_context();
  g_assert(ctx != 0);
  WebviCtx ctx2 = webvi_initialize_context();
  g_assert(ctx != ctx2);

  webvi_context_cleanup(ctx2);
  webvi_context_cleanup(ctx);
}

void test_context_template_path(ContextFixture *fixture,
                                gconstpointer test_data) {
  const char *tpath = "testpath";
  webvi_context_set_template_path(fixture->context, tpath);
  const char *output_path = webvi_context_get_template_path(fixture->context);
  g_assert_cmpstr(output_path, ==, tpath);
}

void test_context_request_processing(ContextFixture *fixture,
                                     G_GNUC_UNUSED gconstpointer test_data) {
  WebviRequest *mock_request = request_create("testuri", fixture->context);

  WebviHandle h = webvi_context_add_request(fixture->context, mock_request);
  g_assert(h != WEBVI_INVALID_HANDLE);
  WebviRequest *req = webvi_context_get_request(fixture->context, h);
  g_assert(req == mock_request);

  webvi_context_remove_request(fixture->context, h);

  req = webvi_context_get_request(fixture->context, h);
  g_assert(req == NULL);
}
