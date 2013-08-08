/*
 * libwebvi.h: C bindings for webvi Python module
 *
 * Copyright (c) 2010-2013 Antti Ajanki <antti.ajanki@iki.fi>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LIBWEBVI_H
#define __LIBWEBVI_H

#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>

#if webvi_EXPORTS /* defined when building as shared library */
  #if defined _WIN32 || defined __CYGWIN__
    #define LIBWEBVI_DLL_EXPORT __declspec(dllexport)
  #else
    #if __GNUC__ >= 4
      #define LIBWEBVI_DLL_EXPORT __attribute__((__visibility__("default")))
    #endif
  #endif
#endif

#ifndef LIBWEBVI_DLL_EXPORT
#define LIBWEBVI_DLL_EXPORT
#endif

typedef int WebviHandle;
typedef long WebviCtx;

typedef ssize_t (*webvi_callback)(const char *, size_t, void *);
typedef void (*webvi_timeout_callback)(long, void *);

#define WEBVI_INVALID_HANDLE -1

typedef enum {
  WEBVIMSG_DONE
} WebviMsgType;

typedef enum {
  WEBVISTATE_NOT_FINISHED = 0,
  WEBVISTATE_FINISHED_OK = 1,
  WEBVISTATE_MEMORY_ALLOCATION_ERROR = 2,
  WEBVISTATE_NOT_FOUND = 3,
  WEBVISTATE_NETWORK_READ_ERROR = 4,
  WEBVISTATE_IO_ERROR = 5,
  WEBVISTATE_TIMEDOUT = 6,
  WEBVISTATE_SUBPROCESS_FAILED = 7,
  WEBVISTATE_INTERNAL_ERROR = 999,
} RequestState;

typedef enum {
  WEBVIREQ_MENU,
  WEBVIREQ_FILE,
  WEBVIREQ_STREAMURL
} WebviRequestType;

typedef enum {
  WEBVIERR_UNKNOWN_ERROR = -1,
  WEBVIERR_OK = 0,
  WEBVIERR_INVALID_HANDLE,
  WEBVIERR_INVALID_PARAMETER
} WebviResult;

typedef enum {
  WEBVIOPT_WRITEFUNC,
  WEBVIOPT_READFUNC,
  WEBVIOPT_WRITEDATA,
  WEBVIOPT_READDATA,
} WebviOption;

typedef enum {
  WEBVIINFO_URL,
  WEBVIINFO_CONTENT_LENGTH,
  WEBVIINFO_CONTENT_TYPE,
  WEBVIINFO_STREAM_TITLE
} WebviInfo;

#define WEBVI_SELECT_TIMEOUT -1

typedef enum {
  WEBVI_SELECT_CHECK = 0,
  WEBVI_SELECT_READ = 1,
  WEBVI_SELECT_WRITE = 2,
  WEBVI_SELECT_EXCEPTION = 4
} WebviSelectBitmask;

typedef enum {
  WEBVI_CONFIG_TEMPLATE_PATH,
  WEBVI_CONFIG_DEBUG,
  WEBVI_CONFIG_TIMEOUT_CALLBACK,
  WEBVI_CONFIG_TIMEOUT_DATA,
  WEBVI_CONFIG_MENU_SCRIPT_PATH
} WebviConfig;

typedef struct {
  WebviMsgType msg;
  WebviHandle handle;
  RequestState status_code;
  const char *data;
} WebviMsg;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize the library. Must be called before any other functions
 * (the only exception is webvi_version() which can be called before
 * the library is initialized).
 *
 * Returns 0, if initialization succeeds.
 */
LIBWEBVI_DLL_EXPORT int webvi_global_init(void);

/*
 * Frees all resources currently used by libwebvi and terminates all
 * active connections. Do not call any libwebvi function after this.
 */
LIBWEBVI_DLL_EXPORT void webvi_cleanup();

/*
 * Create a new context. A valid context is required for calling other
 * functions in the library. The created contextes are independent of
 * each other. The context must be destroyed by a call to
 * webvi_cleanup_context when no longer needed.
 *
 * Return value 0 indicates an error.
 */
LIBWEBVI_DLL_EXPORT WebviCtx webvi_initialize_context(void);

/*
 * Free resources allocated by context ctx. The context can not be
 * used anymore after a call to this function.
 */
LIBWEBVI_DLL_EXPORT void webvi_cleanup_context(WebviCtx ctx);

/*
 * Return the version of libwebvi as a string. The returned value
 * points to a static buffer, and the caller should modify or not free() it.
 */
LIBWEBVI_DLL_EXPORT const char* webvi_version(void);

/*
 * Return a string describing an error code. The returned value points
 * to a read-only buffer, and the caller should not modify or free() it.
 */
LIBWEBVI_DLL_EXPORT const char* webvi_strerror(WebviResult err);

/*
 * Set a new value for a global configuration option conf.
 *
 * Possible values and their meanings:
 *
 * WEBVI_CONFIG_TEMPLATE_PATH
 *   Set the base directory for the XSLT templates (char *)
 *
 * WEBVI_CONFIG_MENU_SCRIPT_PATH
 *   Specify the directory where to look for the menu scripts (char *)
 *
 * WEBVI_CONFIG_DEBUG
 *   If value is not "0", print debug output to stdin (char *)
 *
 * WEBVI_CONFIG_TIMEOUT_CALLBACK
 *   Set timeout callback function (webvi_timeout_callback)
 *
 * WEBVI_CONFIG_TIMEOUT_DATA
 *   Set user data which will passed as second argument of the timeout
 *   callback (void *)
 *
 * The strings (char * arguments) are copied to the library (the user
 * can free their original copy).
 */
LIBWEBVI_DLL_EXPORT WebviResult webvi_set_config(WebviCtx ctx, WebviConfig conf, ...);

/*
 * Creates a new download request.
 *
 * href is a URI of the resource that should be downloaded. Typically,
 * the reference has been acquired from a previously downloaded menu.
 * A special constant "wvt://mainmenu" can be used to download the
 * mainmenu.
 *
 * The return value is a handle to the newly created request. Value
 * WEBVI_INVALID_HANDLE indicates an error.
 *
 * The request is initialized but the actual network transfer is not
 * started. You can set up additional configuration options on the
 * handle using webvi_set_opt() before starting the handle with
 * webvi_start_handle().
 */
LIBWEBVI_DLL_EXPORT WebviHandle webvi_new_request(WebviCtx ctx, const char *href);

/*
 * Starts the transfer on request h. The transfer one or more sockets
 * whose file descriptors are returned by webvi_fdset(). The actual
 * transfer is done during webvi_perform() calls.
 */
LIBWEBVI_DLL_EXPORT WebviResult webvi_start_request(WebviCtx ctx, WebviHandle h);

/*
 * Requests that the transfer on request h shoud be aborted. After the
 * library has actually finished aborting the transfer, the handle h
 * is returned by webvi_get_message() with non-zero status code.
 */
LIBWEBVI_DLL_EXPORT WebviResult webvi_stop_request(WebviCtx ctx, WebviHandle h);

/*
 * Frees resources associated with request h. The handle can not be
 * used after this call. If the handle is still in the middle of a
 * transfer, the transfer is forcefully aborted.
 */
LIBWEBVI_DLL_EXPORT WebviResult webvi_delete_request(WebviCtx ctx, WebviHandle h);

/*
 * Sets configuration options that changes behaviour of the handle.
 * opt is one of the values of WebviOption enum as indicated below.
 * The fourth parameter sets the value of the specified option. Its
 * type depends on opt as discussed below.
 *
 * Possible values for opt:
 *
 * WEBVIOPT_WRITEFUNC
 *
 * Set the callback function that shall be called when data is read
 * from the network. The fourth parameter is a pointer to the callback
 * function
 *
 * ssize_t (*webvi_callback)(const char *, size_t, void *).
 *
 * When the function is called, the first parameter is a pointer to
 * the incoming data, the second parameters is the size of the
 * incoming data block in bytes, and the third parameter is a pointer
 * to user's data structure can be set by WEBVIOPT_WRITEDATA option.
 *
 * The callback function should return the number of bytes is
 * processed. If this differs from the size of the incoming data
 * block, it indicates that an error occurred and the transfer will be
 * aborted.
 *
 * If write callback has not been set (or if it is set to NULL) the
 * incoming data is printed to stdout.
 *
 * WEBVIOPT_WRITEDATA
 *
 * Sets the value that will be passed to the write callback. The
 * fourth parameter is of type void *.
 *
 * WEBVIOPT_READFUNC
 *
 * Set the callback function that shall be called when data is to be
 * send to network. The fourth parameter is a pointer to the callback
 * function
 *
 * ssize_t (*webvi_callback)(const char *, size_t, void *)
 *
 * The first parameter is a pointer to a buffer where the data that is
 * to be sent should be written. The second parameter is the maximum
 * size of the buffer. The thirs parameter is a pointer to user data
 * set with WEBVIOPT_READDATA.
 *
 * The return value should be the number of bytes actually written to
 * the buffer. If the return value is -1, the transfer is aborted.
 *
 * WEBVIOPT_READDATA
 *
 * Sets the value that will be passed to the read callback. The
 * fourth parameter is of type void *.
 *
 */
LIBWEBVI_DLL_EXPORT WebviResult webvi_set_opt(WebviCtx ctx, WebviHandle h, WebviOption opt, ...);

/*
 * Get information specific to a WebviHandle. The value will be
 * written to the memory location pointed by the third argument. The
 * type of the pointer depends in the second parameter as discused
 * below. 
 *
 * Available information:
 * 
 * WEBVIINFO_URL
 *
 * Receive URL. The third parameter must be a pointer to char *. The
 * caller must free() the memory.
 *
 * WEBVIINFO_CONTENT_LENGTH
 *
 * Receive the value of Content-length field, or -1 if the size is
 * unknown. The third parameter must be a pointer to long.
 *
 * WEBVIINFO_CONTENT_TYPE
 *
 * Receive the Content-type string. The returned value is NULL, if the
 * Content-type is unknown. The third parameter must be a pointer to
 * char *. The caller must free() the memory.
 * 
 * WEBVIINFO_STREAM_TITLE
 *
 * Receive stream title. The returned value is NULL, if title is
 * unknown. The third parameter must be a pointer to char *. The
 * caller must free() the memory.
 *
 */
LIBWEBVI_DLL_EXPORT WebviResult webvi_get_info(WebviCtx ctx, WebviHandle h, WebviInfo info, ...);

/*
 * Get active file descriptors in use by the library. The file
 * descriptors that should be waited for reading, writing or
 * exceptions are returned in read_fd_set, write_fd_set and
 * exc_fd_set, respectively. The fd_sets are not cleared, but the new
 * file descriptors are added to them. max_fd will contain the highest
 * numbered file descriptor that was returned in one of the fd_sets.
 *
 * One should wait for action in one of the file descriptors returned
 * by this function using select(), poll() or similar system call,
 * and, after seeing action on a file descriptor, call webvi_perform
 * on that descriptor.
 */
LIBWEBVI_DLL_EXPORT WebviResult webvi_fdset(WebviCtx ctx, fd_set *readfd, fd_set *writefd, fd_set *excfd, int *max_fd);

/*
 * Perform input or output action on a file descriptor.
 * 
 * activefd is a file descriptor that was returned by an earlier call
 * to webvi_fdset and has been signalled to be ready by select() or
 * similar function. ev_bitmask should be OR'ed combination of
 * WEBVI_SELECT_READ, WEBVI_SELECT_WRITE, WEBVI_SELECT_EXCEPTION to
 * indicate that activefd has been signalled to be ready for reading,
 * writing or being in exception state, respectively. ev_bitmask can
 * also set to WEBVI_SELECT_CHECK which means that the state is
 * checked internally. On return, running_handles will contain the
 * number of still active file descriptors.
 *
 * If a timeout occurs before any file descriptor becomes ready, this
 * function should be called with sockfd set to WEBVI_SELECT_TIMEOUT
 * and ev_bitmask set to WEBVI_SELECT_CHECK.
 */
LIBWEBVI_DLL_EXPORT WebviResult webvi_perform(WebviCtx ctx, int sockfd, int ev_bitmask, long *running_handles);


LIBWEBVI_DLL_EXPORT int webvi_process_some(WebviCtx ctx, int timeout_seconds);


/*
 * Return the next message from the message queue. Currently the only
 * message, WEBVIMSG_DONE, indicates that a transfer on a handle has
 * finished. The number of messages remaining in the queue after this
 * call is written to remaining_messages. The pointers in the returned
 * WebviMsg point to handle's internal buffers and is valid until the
 * next call to webvi_get_message(). The caller should not free the
 * returned WebviMsg. The return value is NULL if there is no messages
 * in the queue.
 */
LIBWEBVI_DLL_EXPORT WebviMsg *webvi_get_message(WebviCtx ctx, int *remaining_messages);

#ifdef __cplusplus
}
#endif


#endif
