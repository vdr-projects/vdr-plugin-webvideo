/*
 * download.c: Web video plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <vdr/tools.h>
#include "download.h"
#include "common.h"

static void diff_timeval(struct timeval *a, struct timeval *b,
                         struct timeval *result) {
  long usec_diff = a->tv_usec - b->tv_usec;
  result->tv_sec = a->tv_sec - b->tv_sec;

  while (usec_diff < 0) {
    usec_diff += 1000000;
    result->tv_sec -= 1;
  }

  result->tv_usec = usec_diff;
}

static void merge_fdsets(fd_set *outputfds, fd_set *inputfds, int maxfd) {
  for (int fd=0; fd<maxfd; fd++) {
    if (FD_ISSET(fd, inputfds)) {
      FD_SET(fd, outputfds);
    }
  }
}

// --- cWebviThread --------------------------------------------------------

cWebviThread::cWebviThread()
{
  int pipefd[2];

  if (pipe(pipefd) == -1)
    LOG_ERROR_STR("new request pipe");
  newreqread = pipefd[0];
  newreqwrite = pipefd[1];
  //fcntl(newreqread, F_SETFL, O_NONBLOCK);
  //fcntl(newreqwrite, F_SETFL, O_NONBLOCK);
  timerActive = false;

  webvi = webvi_initialize_context();
  if (webvi != 0) {
    webvi_set_config(webvi, WEBVI_CONFIG_TIMEOUT_DATA, this);
    webvi_set_config(webvi, WEBVI_CONFIG_TIMEOUT_CALLBACK, UpdateTimeout);
  }

  // FIXME: downloadManager timeout callbacks
}

cWebviThread::~cWebviThread() {
  int numactive = activeRequestList.Size();
  for (int i=0; i<activeRequestList.Size(); i++)
    delete activeRequestList[i];
  activeRequestList.Clear();

  for (int i=0; i<finishedRequestList.Size(); i++) {
    delete finishedRequestList[i];
  }
  finishedRequestList.Clear();

  webvi_cleanup_context(webvi);

  if (numactive > 0) {
    esyslog("%d requests failed to complete", numactive);
  }
}

void cWebviThread::UpdateTimeout(long timeout, void *instance) {
  cWebviThread *self = (cWebviThread *)instance;
  if (!self)
    return;

  if (timeout < 0) {
    self->timerActive = false;
  } else {
    struct timeval now;
    long alrm;
    gettimeofday(&now, NULL);
    alrm = timeout + now.tv_usec/1000;
    self->timer.tv_sec = now.tv_sec + alrm/1000;
    self->timer.tv_usec = (alrm % 1000) * 1000;
    self->timerActive = true;
  }
}

cWebviThread &cWebviThread::Instance() {
  static cWebviThread instance;
  
  return instance;
}

void cWebviThread::SetTemplatePath(const char *path) {
  if (webvi != 0 && path)
    webvi_set_config(webvi, WEBVI_CONFIG_TEMPLATE_PATH, path);
}

void cWebviThread::MoveToFinishedList(cMenuRequest *req) {
  // Move the request from the activeList to finishedList.
  requestMutex.Lock();
  for (int i=0; i<activeRequestList.Size(); i++) {
    if (activeRequestList[i] == req) {
      activeRequestList.Remove(i);
      break;
    }
  }
  finishedRequestList.Append(req);

  requestMutex.Unlock();
}

void cWebviThread::ActivateNewRequest() {
  // Move requests from newRequestList to activeRequestList and start
  // them.
  requestMutex.Lock();
  for (int i=0; i<newRequestList.Size(); i++) {
    cMenuRequest *req = newRequestList[i];
    if (req->IsAborted()) {
      // The request has been aborted even before we got a chance to
      // start it.
      MoveToFinishedList(req);
    } else {
      if (!req->Start(webvi)) {
        error("Request failed to start");
        req->RequestDone(-1, "Request failed to start");
        MoveToFinishedList(req);
      } else {
        activeRequestList.Append(req);
      }
    }
  }

  newRequestList.Clear();
  requestMutex.Unlock();
}

void cWebviThread::StopFinishedRequests() {
  // Check if some requests have finished, and move them to
  // finishedRequestList.
  int msg_remaining;
  WebviMsg *donemsg;
  cMenuRequest *req;

  downloadManager.CheckForFinished();
  // FIXME: MoveToFinishedList


  do {
    donemsg = webvi_get_message(webvi, &msg_remaining);

    if (donemsg && donemsg->msg == WEBVIMSG_DONE) {
      requestMutex.Lock();
      req = activeRequestList.FindByHandle(donemsg->handle);
      if (req) {
        req->RequestDone(donemsg->status_code, donemsg->data);
        if (req->IsFinished())
          MoveToFinishedList(req);
      }
      requestMutex.Unlock();
    }
  } while (msg_remaining > 0);
}

void cWebviThread::Stop() {
  // The thread may be sleeping, wake it up first.
  TEMP_FAILURE_RETRY(write(newreqwrite, "S", 1));
  Cancel(5);
}

void cWebviThread::Action(void) {
  fd_set readfds, writefds, excfds;
  fd_set webvi_readfds, webvi_writefds, webvi_excfds;
  fd_set curl_readfds, curl_writefds, curl_excfds;
  int maxfd, s;
  struct timeval timeout, now;
  bool check_for_finished_requests = false;
  bool has_request_files = false;
  long running_webvi_handles;
  long running_curl_handles = 0;

  if (webvi == 0) {
    error("Failed to get libwebvi context");
    return;
  }

  while (Running()) {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&excfds);
    FD_ZERO(&webvi_readfds);
    FD_ZERO(&webvi_writefds);
    FD_ZERO(&webvi_excfds);
    FD_ZERO(&curl_readfds);
    FD_ZERO(&curl_writefds);
    FD_ZERO(&curl_excfds);
    maxfd = -1;

    int tmpmaxfd;
    webvi_fdset(webvi, &webvi_readfds, &webvi_writefds, &webvi_excfds, &tmpmaxfd);
    if (tmpmaxfd != -1) {
      merge_fdsets(&readfds, &webvi_readfds, tmpmaxfd);
      merge_fdsets(&writefds, &webvi_writefds, tmpmaxfd);
      merge_fdsets(&excfds, &webvi_excfds, tmpmaxfd);
      if (tmpmaxfd > maxfd)
        maxfd = tmpmaxfd;
    }

    downloadManager.FDSet(&curl_readfds, &curl_writefds, &curl_excfds, &tmpmaxfd);
    if (tmpmaxfd != -1) {
      merge_fdsets(&readfds, &curl_readfds, tmpmaxfd);
      merge_fdsets(&writefds, &curl_writefds, tmpmaxfd);
      merge_fdsets(&excfds, &curl_excfds, tmpmaxfd);
      if (tmpmaxfd > maxfd)
        maxfd = tmpmaxfd;
    }
    
    FD_SET(newreqread, &readfds);
    if (newreqread > maxfd)
      maxfd = newreqread;

    has_request_files = false;
    requestMutex.Lock();
    for (int i=0; i<activeRequestList.Size(); i++) {
      int fd = activeRequestList[i]->ReadFile();
      if (fd != -1) {
        FD_SET(fd, &readfds);
        if (fd > maxfd)
          maxfd = fd;
        has_request_files = true;
      }
    }
    requestMutex.Unlock();

    if (!timerActive) {
      timeout.tv_sec = 60;
      timeout.tv_usec = 0;
    } else {
      gettimeofday(&now, NULL);
      diff_timeval(&timer, &now, &timeout);
      if (timeout.tv_sec < 0 || timeout.tv_usec < 0) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
      }
    }

    s = TEMP_FAILURE_RETRY(select(maxfd+1, &readfds, &writefds, NULL,
                                  &timeout));
    if (s == -1) {
      // select error
      LOG_ERROR_STR("select() error in webvideo downloader thread:");
      Cancel(-1);

    } else if (s == 0) {
      // timeout
      timerActive = false;
      webvi_perform(webvi, WEBVI_SELECT_TIMEOUT, WEBVI_SELECT_CHECK, &running_webvi_handles);
      check_for_finished_requests = true;

      // FIXME: curl timeout

    } else {
      int num_processed_fds = 0;
      for (int fd=0; (fd<=maxfd) && (num_processed_fds<=s); fd++) {
        if (FD_ISSET(fd, &readfds)) {
          if (FD_ISSET(fd, &webvi_readfds)) {
            webvi_perform(webvi, fd, WEBVI_SELECT_READ, &running_webvi_handles);
            num_processed_fds += 1;
          } else if (FD_ISSET(fd, &curl_readfds)) {
            downloadManager.HandleSocket(fd, WEBVI_SELECT_READ, &running_curl_handles);
            num_processed_fds += 1;
          } else if (fd == newreqread) {
            char tmpbuf[8];
            int n = read(fd, tmpbuf, 8);
            if (n > 0 && memchr(tmpbuf, 'S', n))
              Cancel(-1);
            ActivateNewRequest();
            num_processed_fds += 1;
          } else if (has_request_files) {
            requestMutex.Lock();
            cMenuRequest *readRequest = NULL;
            for (int i=0; i<activeRequestList.Size(); i++) {
              if (fd == activeRequestList[i]->ReadFile()) {
                readRequest = activeRequestList[i];
                break;
              }
            }
            requestMutex.Unlock();

            // call Read() after releasing the mutex
            if (readRequest) {
              num_processed_fds += 1;
              readRequest->Read();
              if (readRequest->IsFinished())
                MoveToFinishedList(readRequest);
            }
          }
        }
        if (FD_ISSET(fd, &writefds)) {
          if (FD_ISSET(fd, &webvi_writefds)) {
            webvi_perform(webvi, fd, WEBVI_SELECT_WRITE, &running_webvi_handles);
            num_processed_fds += 1;
          } else if (FD_ISSET(fd, &curl_writefds)) {
            downloadManager.HandleSocket(fd, WEBVI_SELECT_WRITE, &running_curl_handles);
            num_processed_fds += 1;
          }
        }
        if (FD_ISSET(fd, &excfds)) {
          if (FD_ISSET(fd, &webvi_excfds)) {
            webvi_perform(webvi, fd, WEBVI_SELECT_EXCEPTION, &running_webvi_handles);
            num_processed_fds += 1;
          } else if (FD_ISSET(fd, &curl_excfds)) {
            downloadManager.HandleSocket(fd, WEBVI_SELECT_EXCEPTION, &running_curl_handles);
            num_processed_fds += 1;
          }
        }
      }

      check_for_finished_requests = (num_processed_fds > 0);
    }

    if (check_for_finished_requests) {
      StopFinishedRequests();
    }
  }
}

void cWebviThread::AddRequest(cMenuRequest *req) {
  requestMutex.Lock();
  req->SetDownloader(&downloadManager);
  newRequestList.Append(req);
  requestMutex.Unlock();

  int s = TEMP_FAILURE_RETRY(write(newreqwrite, "*", 1));
  if (s == -1)
    LOG_ERROR_STR("Failed to signal new webvideo request");
}

cMenuRequest *cWebviThread::GetFinishedRequest() {
  cMenuRequest *res = NULL;
  requestMutex.Lock();
  if (finishedRequestList.Size() > 0) {
    res = finishedRequestList[finishedRequestList.Size()-1];
    finishedRequestList.Remove(finishedRequestList.Size()-1);
  }
  requestMutex.Unlock();

  return res;
}

int cWebviThread::GetUnfinishedCount() {
  if (!Running())
    return 0;
  else
    return activeRequestList.Size();
}
