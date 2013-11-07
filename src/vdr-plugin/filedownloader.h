/*
 * filedownloader.h: Web video plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <sys/select.h>
#include <curl/curl.h>
#include <list>

#ifndef __WEBVIDEO_FILEDOWNLOADER_H
#define __WEBVIDEO_FILEDOWNLOADER_H

// --- iFileDownloadTask ---------------------------------------------------

class iFileDownloadTask {
public:
  virtual ~iFileDownloadTask() {};
  virtual void SetReadCallback(ssize_t (*cb)(void *buf, size_t count, void *data), void *data) = 0;
  virtual void SetFinishedCallback(void (*cb)(void *data), void *data) = 0;
};

// --- iAsyncFileDownloaderManager -----------------------------------------

class iAsyncFileDownloaderManager {
public:
  virtual ~iAsyncFileDownloaderManager() {};
  virtual iFileDownloadTask *CreateDownloadTask(const cString& url) = 0;
  virtual void FDSet(fd_set *readfds, fd_set *writefds, fd_set *excfds, int *maxfd) = 0;
  virtual void HandleSocket(int fd, int ev_bitmask, long *running_handles) = 0;
  virtual void CheckForFinished() = 0;
};

// --- cCurlDownloadTask ---------------------------------------------------

class cCurlMultiManager;

class cCurlDownloadTask : public iFileDownloadTask {
private:
  cCurlMultiManager *manager;
  CURL *curl;
  ssize_t (*writeCallback)(void *buf, size_t count, void *data);
  void *writeData;
  void (*finishCallback)(void *data);
  void *finishData;

  static size_t ReadWrapper(char *ptr, size_t size, size_t nmemb, void *userdata);
  size_t ExecuteReadCallback(void *buf, size_t count);

public:
  cCurlDownloadTask(cCurlMultiManager *manager, const cString& url);
  virtual ~cCurlDownloadTask();

  CURL *Handle() { return curl; }
  void MarkFinished(CURLcode result);
  virtual void SetReadCallback(ssize_t (*cb)(void *buf, size_t count, void *data), void *data) = 0;
  virtual void SetFinishedCallback(void (*cb)(void *data), void *data) = 0;
};

// --- cCurlMultiManager ---------------------------------------------------

class cCurlMultiManager : public iAsyncFileDownloaderManager {
private:
  CURLM *curlmulti;
  std::list<cCurlDownloadTask *> activeTasks;

  cCurlDownloadTask *FindByHandle(CURL *handle);

public:
  cCurlMultiManager();
  virtual ~cCurlMultiManager();

  CURLM *CurlMultiHandle() { return curlmulti; }

  virtual iFileDownloadTask *CreateDownloadTask(const cString& url);
  virtual void FDSet(fd_set *readfds, fd_set *writefds, fd_set *excfds, int *maxfd);
  virtual void HandleSocket(int fd, int ev_bitmask, long *running_handles);
  virtual void CheckForFinished();

  void RemoveTask(cCurlDownloadTask *task);
};

#endif
