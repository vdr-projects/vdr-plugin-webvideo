#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "pipecomponent.h"
#include "pipe_tests.h"

#define TEST_STRING "ABCDEF"
#define TEST_FD1 1
#define TEST_FD2 2

#define MENU_VALID "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
  "<wvmenu><title>Test menu</title></wvmenu>"
#define INVALID_XML ">> this & is not XML >>"
#define MENU_INVALID_ROOT "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
  "<invalid></invalid>"

typedef struct CountingPipe {
  PipeComponent p;
  size_t bytes;
  gboolean finished;
  gboolean failed;
} CountingPipe;

static gboolean delete1_called;
static gboolean delete2_called;

static CountingPipe *testpipe_create(
    gboolean (*process)(PipeComponent *, char *, size_t),
    void (*finished)(PipeComponent *, RequestState state),
    void (*delete)(PipeComponent *));
static CountingPipe *testpipe_create_fdset(
    gboolean (*process)(PipeComponent *, char *, size_t),
    void (*finished)(PipeComponent *, RequestState state),
    void (*delete)(PipeComponent *),
    void (*fdset)(PipeComponent *, fd_set *, fd_set *, fd_set *, int *),
    gboolean (*handle_socket)(PipeComponent *, int, int));
static gboolean testpipe_forward(PipeComponent *component, char *buf, size_t len);
static gboolean testpipe_fail(PipeComponent *component, char *buf, size_t len);
static void testpipe_fdset1(PipeComponent *self, fd_set *readfd,
                            fd_set *writefd, fd_set *excfd, int *max_fd);
static void testpipe_fdset2(PipeComponent *self, fd_set *readfd,
                            fd_set *writefd, fd_set *excfd, int *max_fd);
static void testpipe_finished(PipeComponent *component, RequestState state);
static void testpipe_delete(PipeComponent *component);
static void testpipe_delete1(PipeComponent *component);
static void testpipe_delete2(PipeComponent *component);

void test_pipe_one_component() {
  CountingPipe *pipe = 
    testpipe_create(testpipe_forward, testpipe_finished, testpipe_delete);

  pipe_component_append(&pipe->p, TEST_STRING, strlen(TEST_STRING));
  g_assert(!pipe->finished);
  g_assert(!pipe->failed);
  g_assert(pipe_component_get_state(&pipe->p) == WEBVISTATE_NOT_FINISHED);
  g_assert(pipe->bytes == strlen(TEST_STRING));

  pipe_component_finished(&pipe->p, WEBVISTATE_FINISHED_OK);
  g_assert(pipe->finished);
  g_assert(!pipe->failed);
  g_assert(pipe_component_get_state(&pipe->p) == WEBVISTATE_FINISHED_OK);
  g_assert(pipe->bytes == strlen(TEST_STRING));

  pipe_delete_full(&pipe->p);
}

void test_pipe_not_appending_after_finished() {
  CountingPipe *pipe = 
    testpipe_create(testpipe_forward, testpipe_finished, testpipe_delete);

  pipe_component_append(&pipe->p, TEST_STRING, strlen(TEST_STRING));
  g_assert(pipe->bytes == strlen(TEST_STRING));

  pipe_component_finished(&pipe->p, WEBVISTATE_FINISHED_OK);

  pipe_component_append(&pipe->p, TEST_STRING, strlen(TEST_STRING));
  g_assert(pipe->bytes == strlen(TEST_STRING));
  
  pipe_delete_full(&pipe->p);
}

void test_pipe_state_not_chaning_after_finished() {
  CountingPipe *pipe = 
    testpipe_create(testpipe_forward, testpipe_finished, testpipe_delete);

  pipe_component_append(&pipe->p, TEST_STRING, strlen(TEST_STRING));
  g_assert(pipe_component_get_state(&pipe->p) == WEBVISTATE_NOT_FINISHED);

  pipe_component_finished(&pipe->p, WEBVISTATE_INTERNAL_ERROR);
  g_assert(pipe_component_get_state(&pipe->p) == WEBVISTATE_INTERNAL_ERROR);

  pipe_component_finished(&pipe->p, WEBVISTATE_FINISHED_OK);
  g_assert(pipe_component_get_state(&pipe->p) == WEBVISTATE_INTERNAL_ERROR);

  pipe_delete_full(&pipe->p);
}

void test_pipe_two_components() {
  CountingPipe *first = 
    testpipe_create(testpipe_forward, testpipe_finished, testpipe_delete);
  CountingPipe *second = 
    testpipe_create(testpipe_forward, testpipe_finished, testpipe_delete);
  pipe_component_set_next(&first->p, &second->p);

  pipe_component_append(&first->p, TEST_STRING, strlen(TEST_STRING));
  pipe_component_finished(&first->p, WEBVISTATE_FINISHED_OK);

  g_assert(first->finished);
  g_assert(!first->failed);
  g_assert(first->bytes == strlen(TEST_STRING));
  g_assert(second->finished);
  g_assert(!second->failed);
  g_assert(second->bytes == strlen(TEST_STRING));

  pipe_delete_full(&first->p);
}

void test_pipe_failing_component() {
  CountingPipe *first = 
    testpipe_create(testpipe_fail, testpipe_finished, testpipe_delete);
  CountingPipe *second = 
    testpipe_create(testpipe_forward, testpipe_finished, testpipe_delete);
  pipe_component_set_next(&first->p, &second->p);

  pipe_component_append(&first->p, TEST_STRING, strlen(TEST_STRING));
  pipe_component_finished(&first->p, WEBVISTATE_FINISHED_OK);

  g_assert(first->failed);
  g_assert(second->finished);
  g_assert(second->failed);
  g_assert(second->bytes == 0);

  pipe_delete_full(&first->p);
}

void test_pipe_delete_all() {
  CountingPipe *first = 
    testpipe_create(testpipe_forward, testpipe_finished, testpipe_delete1);
  CountingPipe *second = 
    testpipe_create(testpipe_forward, testpipe_finished, testpipe_delete2);
  pipe_component_set_next(&first->p, &second->p);

  delete1_called = FALSE;
  delete2_called = FALSE;

  pipe_delete_full(&first->p);

  g_assert(delete1_called);
  g_assert(delete2_called);
}

void test_pipe_fdset() {
  fd_set readfds;
  fd_set writefds;
  fd_set excfds;
  int maxfd = 0;
  CountingPipe *first = 
    testpipe_create_fdset(testpipe_forward, testpipe_finished,
                          testpipe_delete, testpipe_fdset1, NULL);
  CountingPipe *second = 
    testpipe_create_fdset(testpipe_forward, testpipe_finished,
                          testpipe_delete, testpipe_fdset2, NULL);
  pipe_component_set_next(&first->p, &second->p);

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_ZERO(&excfds);
  pipe_fdset(&first->p, &readfds, &writefds, &excfds, &maxfd);
  g_assert(FD_ISSET(TEST_FD1, &readfds));
  g_assert(FD_ISSET(TEST_FD2, &readfds));

  pipe_delete_full(&first->p);
}

CountingPipe *testpipe_create(
    gboolean (*process)(PipeComponent *, char *, size_t),
    void (*finished)(PipeComponent *, RequestState state),
    void (*delete)(PipeComponent *))
{
  CountingPipe *pipe = malloc(sizeof(CountingPipe));
  g_assert(pipe);
  memset(pipe, 0, sizeof(CountingPipe));
  pipe_component_initialize(&pipe->p, process, finished, delete);
  return pipe;
}

CountingPipe *testpipe_create_fdset(
    gboolean (*process)(PipeComponent *, char *, size_t),
    void (*finished)(PipeComponent *, RequestState state),
    void (*delete)(PipeComponent *),
    void (*fdset)(PipeComponent *, fd_set *, fd_set *, fd_set *, int *),
    gboolean (*handle_socket)(PipeComponent *, int, int))
{
  CountingPipe *pipe = malloc(sizeof(CountingPipe));
  g_assert(pipe);
  memset(pipe, 0, sizeof(CountingPipe));
  pipe_component_initialize_fdset(&pipe->p, process, finished, delete,
                                  fdset, handle_socket);
  return pipe;
}

gboolean testpipe_forward(PipeComponent *component, char *buf, size_t len) {
  CountingPipe *self = (CountingPipe *)component;
  self->bytes += len;
  return TRUE;
}

gboolean testpipe_fail(PipeComponent *component, char *buf, size_t len) {
  CountingPipe *self = (CountingPipe *)component;
  pipe_component_finished(component, WEBVISTATE_INTERNAL_ERROR);
  return TRUE;
}

void testpipe_fdset1(PipeComponent *self, fd_set *readfd,
                     fd_set *writefd, fd_set *excfd, int *max_fd)
{
  FD_SET(TEST_FD1, readfd);
  if (TEST_FD1 > *max_fd)
    *max_fd = TEST_FD1;
}

void testpipe_fdset2(PipeComponent *self, fd_set *readfd,
                     fd_set *writefd, fd_set *excfd, int *max_fd)
{
  FD_SET(TEST_FD2, readfd);
  if (TEST_FD2 > *max_fd)
    *max_fd = TEST_FD2;
}

void testpipe_finished(PipeComponent *component, RequestState state) {
  CountingPipe *self = (CountingPipe *)component;
  self->finished = TRUE;
  if (state != WEBVISTATE_FINISHED_OK)
    self->failed = TRUE;
}

void testpipe_delete(PipeComponent *component) {
  CountingPipe *self = (CountingPipe *)component;
  free(self);
}

void testpipe_delete1(PipeComponent *component) {
  CountingPipe *self = (CountingPipe *)component;
  delete1_called = TRUE;
  free(self);
}

void testpipe_delete2(PipeComponent *component) {
  CountingPipe *self = (CountingPipe *)component;
  delete2_called = TRUE;
  free(self);
}

void test_pipe_menu_validator_valid_menu() {
  PipeMenuValidator *pipe = pipe_menu_validator_create();
  g_assert(pipe);

  pipe_component_append((PipeComponent *)pipe, MENU_VALID,
                        strlen(MENU_VALID));
  pipe_component_finished((PipeComponent *)pipe, WEBVISTATE_FINISHED_OK);
  RequestState state = pipe_component_get_state((PipeComponent *)pipe);
  g_assert(state == WEBVISTATE_FINISHED_OK);

  pipe_delete_full((PipeComponent *)pipe);
}

void test_pipe_menu_validator_invalid_xml() {
  PipeMenuValidator *pipe = pipe_menu_validator_create();
  g_assert(pipe);

  pipe_component_append((PipeComponent *)pipe, INVALID_XML,
                        strlen(INVALID_XML));
  pipe_component_finished((PipeComponent *)pipe, WEBVISTATE_FINISHED_OK);
  RequestState state = pipe_component_get_state((PipeComponent *)pipe);
  g_assert((state != WEBVISTATE_NOT_FINISHED) && 
           (state != WEBVISTATE_FINISHED_OK));

  pipe_delete_full((PipeComponent *)pipe);
}

void test_pipe_menu_validator_invalid_root() {
  PipeMenuValidator *pipe = pipe_menu_validator_create();
  g_assert(pipe);

  pipe_component_append((PipeComponent *)pipe, MENU_INVALID_ROOT,
                        strlen(MENU_INVALID_ROOT));
  pipe_component_finished((PipeComponent *)pipe, WEBVISTATE_FINISHED_OK);
  RequestState state = pipe_component_get_state((PipeComponent *)pipe);
  g_assert((state != WEBVISTATE_NOT_FINISHED) && 
           (state != WEBVISTATE_FINISHED_OK));

  pipe_delete_full((PipeComponent *)pipe);
}
