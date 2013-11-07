/*
 * filedownloader.c: Web video plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <exception>

// --- cCurlDownloadTask ---------------------------------------------------

cCurlDownloadTask::cCurlDownloadTask(cCurlMultiManager *_manager, const cString& url)
  : manager(_manager), curl(NULL), writeCallback(NULL), writeData(NULL),
    finishCallback(NULL), finishData(NULL)
{
  if (!manager)
    throw new invalid_argument("manager can not be null");

  curl = curl_easy_init();
  if (!curl)
    throw new runtime_error("failed to initialize curl");

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReadWrapper);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
  curl_multi_add_handle(manager->CurlMultiHandle(), curl);
}

cCurlDownloadTask::~cCurlDownloadTask() {
  manager->RemoveTask(this);
  curl_multi_remove_handle(manager->CurlMultiHandle(), curl);
  curl_easy_cleanup(curl);
}

size_t cCurlDownloadTask::ReadWrapper(char *ptr, size_t size, size_t nmemb, void *userdata) {
  if (self->writeCallback) {
    cCurlDownloadTask *self = (cCurlDownloadTask *)userdata;
    return self->ExecuteReadCallback(ptr, size*nmemb);
  } else {
    // ignore data
    return size*nmemb;
  }
}

size_t cCurlDownloadTask::ExecuteReadCallback(void *buf, size_t count) {
  return (size_t)writeCallback(buf, count, writeData);
}

void cCurlDownloadTask::SetReadCallback(ssize_t (*cb)(void *buf, size_t count, void *data), void *data) {
  writeCallback = cb;
  writeData = data;
}

void cCurlDownloadTask::SetFinishedCallback(void (*cb)(void *data), void *data) {
  finishCallback = cb;
  finishData = data;
}

void cCurlDownloadTask::MarkFinished(CURLcode result) {

  // FIXME: CURLcode

  if (finishCallback) {
    finishCallback(finishData);
  }

  curl_multi_remove_handle(manager->CurlMultiHandle(), curl);
}

// --- cCurlMultiManager ---------------------------------------------------

cCurlMultiManager::cCurlMultiManager() {
  curlmulti = curl_multi_init();
  if (!curlmulti)
    throw new runtime_error("curl initialization failed");
}

cCurlMultiManager::~cCurlMultiManager() {
  std::list<cCurlDownloadTask *>::iterator it;
  for (it=activeTasks.begin(); it!=activeTasks.end(); ++it) {
    delete (*it);
  }

  curl_multi_cleanup(curlmulti);
}

iFileDownloadTask *cCurlMultiManager::CreateDownloadTask(const cString& url) {
  cCurlDownloadTask *task = new cCurlDownloadTask(url);
  activeTasks.push_back(task);
  return task;
}

void cCurlMultiManager::FDSet(fd_set *readfds, fd_set *writefds,
                              fd_set *excfds, int *maxfd)
{
  curl_multi_fdset(curlmulti, readfds, writefds, excfds, maxfd);
}

void cCurlMultiManager::HandleSocket(int fd, int ev_bitmask,
                                     long *running_handles)
{
  int curl_bitmask = 0;
  if ((ev_bitmask & WEBVI_SELECT_READ) != 0)
    curl_bitmask |= CURL_CSELECT_IN;
  if ((ev_bitmask & WEBVI_SELECT_WRITE) != 0)
    curl_bitmask |= CURL_CSELECT_OUT;
  if ((ev_bitmask & WEBVI_SELECT_EXCEPTION) != 0)
    curl_bitmask |= CURL_CSELECT_ERR;

  curl_multi_socket_action(curlmulti, fd, curl_bitmask, running_handles);
}

void cCurlMultiManager::CheckForFinished() {
  int remaining = 0;
  CURLMsg *msg;

  while ((msg = curl_multi_info_read(curlmulti, &remaining))) {
    if (msg->msg == CURLMSG_DONE) {
      cCurlDownloadTask *task = FindByHandle(msg->easy_handle);
      if (task) {
        task->MarkFinished(msg->data.result);
      }
    }
  }
}

cCurlDownloadTask *cCurlMultiManager::FindByHandle(CURL *handle) {
  std::list<cCurlDownloadTask *>::iterator it;
  for (it=activeTasks.begin(); it!=activeTasks.end(); ++it) {
    if ((*it)->Handle() == handle)
      return *it;
  }

  return NULL;
}

struct EqualsComparator {
  cCurlDownloadTask *mark;
  bool operator() (cCurlDownloadTask *value) { return (value == mark); }
};

void cCurlMultiManager::RemoveTask(cCurlDownloadTask *task) {
  EqualsComparator cmp;
  cmp.mark = task;
  activeTasks.remove_if(cmp());
}
