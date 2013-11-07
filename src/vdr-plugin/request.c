/*
 * request.c: Web video plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <vdr/tools.h>
#include <vdr/i18n.h>
#include "request.h"
#include "common.h"
#include "mimetypes.h"
#include "config.h"
#include "timer.h"

// --- cDownloadProgress ---------------------------------------------------

cDownloadProgress::cDownloadProgress() {
  strcpy(name, "???");
  downloaded = -1;
  total = -1;
  statusCode = -1;
  req = NULL;
}

void cDownloadProgress::AssociateWith(cFileDownloadRequest *request) {
  req = request;
}

void cDownloadProgress::SetContentLength(long bytes) {
  total = bytes;
}

void cDownloadProgress::SetTitle(const char *title) {
  cMutexLock lock(&mutex);

  strncpy(name, title, NAME_LEN-1);
  name[NAME_LEN-1] = '\0';
}

void cDownloadProgress::Progress(long downloadedbytes) {
  // Atomic operation, no mutex needed
  downloaded = downloadedbytes;
}

void cDownloadProgress::MarkDone(int errorcode, cString pharse) {
  cMutexLock lock(&mutex);

  statusCode = errorcode;
  statusPharse = pharse;
}

bool cDownloadProgress::IsFinished() {
  return statusCode != -1;
}

cString cDownloadProgress::GetTitle() {
  cMutexLock lock(&mutex);

  if (req && req->IsAborted())
    return cString::sprintf("[%s] %s", tr("Aborted"), name);
  else
    return cString(name);
}

cString cDownloadProgress::GetPercentage() {
  cMutexLock lock(&mutex);

  if ((const char*)statusPharse != NULL && statusCode != 0)
    // TRANSLATORS: at most 5 characters
    return cString(tr("Error"));
  else if ((downloaded < 0) || (total < 0))
    return cString("???");
  else
    return cString::sprintf("%3d%%", (int) (100*(float)downloaded/total + 0.5));
}

cString cDownloadProgress::GetStatusPharse() {
  cMutexLock lock(&mutex);

  return statusPharse;
}

bool cDownloadProgress::Error() {
  return (const char *)statusPharse != NULL;
}

// --- cProgressVector -----------------------------------------------------

cDownloadProgress *cProgressVector::NewDownload() {
  cDownloadProgress *progress = new cDownloadProgress();
  Append(progress);
  return progress;
}

// --- cRequest ------------------------------------------------------------

cRequest::cRequest(int _ID, eRequestType _type, const char *_href)
: reqID(_ID), type(_type), href(strdup(_href)), aborted(false), finished(false)
{
}

cRequest::~cRequest() {
  free(href);
}

void cRequest::Abort() {
  if (aborted || finished)
    return;

  aborted = true;
};

void cRequest::RequestDone(int errorcode, cString pharse) {
  debug("RequestDone %d %s", errorcode, (const char *)pharse);
  finished = true;
}

// --- cMenuRequest --------------------------------------------------------

cMenuRequest::cMenuRequest(int ID, eRequestType type, const char *wvtreference)
: cRequest(ID, type, wvtreference), status(0), webvi(-1), handle(-1), timer(NULL)
{
}

cMenuRequest::~cMenuRequest() {
  if (handle != -1) {
    if (!IsFinished())
      Abort();
    webvi_delete_request(webvi, handle);
  }

  // do not delete timer
}

ssize_t cMenuRequest::WriteCallback(const char *ptr, size_t len, void *request) {
  cMenuRequest *instance = (cMenuRequest *)request;
  if (instance)
    return instance->WriteData(ptr, len);
  else
    return len;
}

ssize_t cMenuRequest::WriteData(const char *ptr, size_t len) {
  return inBuffer.Put(ptr, len);
}

char *cMenuRequest::ExtractSiteName(const char *ref) {
  if (strncmp(ref, "wvt:///", 7) != 0)
    return NULL;

  const char *first = ref+7;
  const char *last = strchr(first, '/');
  if (!last)
    last = first+strlen(first);

  return strndup(first, last-first);
}

void cMenuRequest::AppendQualityParamsToRef() {
  if (!href)
    return;

  char *site = ExtractSiteName(href);
  if (site) {
    const char *min = webvideoConfig->GetMinQuality(site, GetType());
    const char *max = webvideoConfig->GetMaxQuality(site, GetType());
    free(site);

    if (min && !max) {
      cString newref = cString::sprintf("%s&minquality=%s", href, min);
      free(href);
      href = strdup((const char *)newref);

    } else if (!min && max) {
      cString newref = cString::sprintf("%s&maxquality=%s", href, max);
      free(href);
      href = strdup((const char *)newref);

    } else if (min && max) {
      cString newref = cString::sprintf("%s&minquality=%s&maxquality=%s", href, min, max);
      free(href);
      href = strdup((const char *)newref);
    }
  }
}

WebviHandle cMenuRequest::PrepareHandle() {
  if (handle == -1) {
    handle = webvi_new_request(webvi, href);

    if (handle != -1) {
      webvi_set_opt(webvi, handle, WEBVIOPT_WRITEFUNC, WriteCallback);
      webvi_set_opt(webvi, handle, WEBVIOPT_WRITEDATA, this);
    }
  }

  return handle;
}

bool cMenuRequest::Start(WebviCtx webvictx) {
  debug("starting request %d", reqID);

  webvi = webvictx;
  if ((PrepareHandle() != -1) &&
      (webvi_start_request(webvi, handle) == WEBVIERR_OK)) {
    finished = false;
    return true;
  } else 
    return false;
}

void cMenuRequest::RequestDone(int errorcode, cString pharse) {
  cRequest::RequestDone(errorcode, pharse);
  status = errorcode;
  statusPharse = pharse;
}

void cMenuRequest::Abort() {
  if (IsAborted() || IsFinished() || handle == -1)
    return;

  aborted = true;
  webvi_stop_request(webvi, handle);
};

bool cMenuRequest::Success() const {
  return status == 0;
}

cString cMenuRequest::GetStatusPharse() const {
  return statusPharse;
}

cString cMenuRequest::GetResponse() {
  size_t len = inBuffer.Length();
  const char *src = inBuffer.Get();
  char *buf = (char *)malloc((len+1)*sizeof(char));
  strncpy(buf, src, len);
  buf[len] = '\0';
  return cString(buf, true);
}

// --- cFileDownloadRequest ------------------------------------------------

cFileDownloadRequest::cFileDownloadRequest(int ID, const char *streamref, 
                                           cDownloadProgress *progress)
:  cMenuRequest(ID, REQT_FILE, streamref), title(NULL), bytesDownloaded(0),
   contentLength(-1), streamSocket(1), destfile(NULL), destfilename(NULL),
   progressUpdater(progress), state(STATE_GET_STREAM_URL),
   downloadManager(NULL), streamDownloader(NULL)
{
  if (progressUpdater)
    progressUpdater->AssociateWith(this);

  //AppendQualityParamsToRef();
}

cFileDownloadRequest::~cFileDownloadRequest() {
  if (destfile) {
    destfile->Close();
    delete destfile;
  }
  if (destfilename) {
    free(destfilename);
  }
  if (streamDownloader) {
    delete streamDownloader;
    streamDownloader = NULL;
  }
  if (title) {
    free(title);
  }
  // do not delete progressUpdater
}

void cFileDownloadRequest::SetDownloader(iAsyncFileDownloaderManager *dlmanager) {
  downloadManager = dlmanager;
}

WebviHandle cFileDownloadRequest::PrepareHandle() {
  if (handle == -1) {
    handle = webvi_new_request(webvi, href);

    if (handle != -1) {
      webvi_set_opt(webvi, handle, WEBVIOPT_WRITEFUNC, WriteCallback);
      webvi_set_opt(webvi, handle, WEBVIOPT_WRITEDATA, this);
    }
  }

  return handle;
}

bool cFileDownloadRequest::OpenDestFile() {
  char *contentType;
  char *url;
  char *ext;
  cString filename;
  int fd, i;

  if (handle == -1) {
    error("handle == -1 while trying to open destination file");
    return false;
  }

  if (destfile)
    delete destfile;

  destfile = new cUnbufferedFile;

  webvi_get_info(webvi, handle, WEBVIINFO_URL, &url);
  webvi_get_info(webvi, handle, WEBVIINFO_STREAM_TITLE, &title);
  webvi_get_info(webvi, handle, WEBVIINFO_CONTENT_TYPE, &contentType);
  webvi_get_info(webvi, handle, WEBVIINFO_CONTENT_LENGTH, &contentLength);

  if (!contentType || !url) {
    if(contentType)
      free(contentType);
    if (url)
      free(url);

    error("no content type or url, can't infer extension");
    return false;
  }

  ext = GetExtension(contentType, url);

  free(url);
  free(contentType);

  const char *destdir = webvideoConfig->GetDownloadPath();
  char *basename = strdup(title ? title : "???");
  basename = safeFilename(basename, webvideoConfig->GetUseVFATNames());

  i = 1;
  filename = cString::sprintf("%s/%s%s", destdir, basename, ext);
  while (true) {
    debug("trying to open %s", (const char *)filename);

    fd = destfile->Open(filename, O_WRONLY | O_CREAT | O_EXCL, DEFFILEMODE);

    if (fd == -1 && errno == EEXIST)
      filename = cString::sprintf("%s/%s-%d%s", destdir, basename, i++, ext);
    else
      break;
  };

  free(basename);
  free(ext);

  if (fd < 0) {
    error("Failed to open file %s: %m", (const char *)filename);
    delete destfile;
    destfile = NULL;
    return false;
  }

  if (destfilename)
    free(destfilename);
  destfilename = strdup(filename);
  info("Saving to %s", destfilename);

  if (progressUpdater) {
    progressUpdater->SetTitle(title);
    progressUpdater->SetContentLength(contentLength);
  }

  return true;
}

char *cFileDownloadRequest::GetExtension(const char *contentType, const char *url) {
  // Get extension from Content-Type
  char *ext = NULL;
  char *ext2 = MimeTypes->ExtensionFromMimeType(contentType);

  // Workaround for buggy servers: If the server claims that the mime
  // type is text/plain, ignore the server and fall back to extracting
  // the extension from the URL. This function should be called only
  // for video, audio or ASX files and therefore text/plain is clearly
  // incorrect.
  if (ext2 && contentType && !strcasecmp(contentType, "text/plain")) {
    debug("Ignoring content type text/plain, getting extension from url.");
    free(ext2);
    ext2 = NULL;
  }

  if (ext2) {
    // Append dot in the start of the extension
    ext = (char *)malloc(strlen(ext2)+2);
    ext[0] = '.';
    ext[1] = '\0';
    strcat(ext, ext2);
    free(ext2);
    return ext;
  }

  // Get extension from URL
  ext = extensionFromUrl(url);
  if (ext)
    return ext;

  // No extension!
  return strdup("");
}

void cFileDownloadRequest::RequestDone(int errorcode, cString pharse) {
  if (errorcode == REQERR_OK) {
    if (state == STATE_GET_STREAM_URL) {
      parseStreamMetadataFromXml(inBuffer.Get(), inBuffer.Length(), streamUrl, streamTitle);

      StartStreamDownload();

    } else if (state == STATE_STREAM_DOWNLOAD) {
      if (destfile)
        destfile->Close();

      StartPostProcessing();

    } else if (state == STATE_POSTPROCESS) {
      postProcessPipe.Close();
      state = STATE_FINISHED;
    }
  } else {
    state = STATE_FINISHED;
  }

  if (state == STATE_FINISHED) {
    cMenuRequest::RequestDone(errorcode, pharse);
    if (progressUpdater)
      progressUpdater->MarkDone(errorcode, pharse);
  }
}

void cFileDownloadRequest::Abort() {
  if (state == STATE_STREAM_DOWNLOAD) {
    if (streamDownloader) {
      delete streamDownloader;
      streamDownloader = NULL;
    }
  } else if (state == STATE_POSTPROCESS) {
    postProcessPipe.Close();
  }

  cMenuRequest::Abort();
}

void cFileDownloadRequest::StartStreamDownload() {
  state = STATE_STREAM_DOWNLOAD;

  if (IsRTMPStream(streamUrl)) {
    RequestDone(REQERR_INTERNAL, "FIXME: downloading RTMP stream");
  }

  assert(!streamDownloader);
  if (downloadManager) {
    streamDownloader = downloadManager->CreateDownloadTask(streamUrl);
    streamDownloader->SetReadCallback(StreamReadWrapper, this);
    streamDownloader->SetFinishedCallback(StreamFinishedWrapper, this);
  } else {
    RequestDone(REQERR_INTERNAL, "No downloadManager");
  }
}

bool cFileDownloadRequest::IsRTMPStream(const char *url) {
  return (strncmp(url, "rtmp://", 7) == 0) ||
    (strncmp(url, "rtmpe://", 8) == 0) ||
    (strncmp(url, "rtmpt://", 8) == 0) ||
    (strncmp(url, "rtmps://", 8) == 0) ||
    (strncmp(url, "rtmpte://", 9) == 0) ||
    (strncmp(url, "rtmpts://", 9) == 0);
}

ssize_t cFileDownloadRequest::StreamReadWrapper(void *buf, size_t len, void *data) {
  cFileDownloadRequest *self = (cFileDownloadRequest *)data;
  return self->WriteToDestFile(buf, len);
}

ssize_t cFileDownloadRequest::WriteToDestFile(void *buf, size_t len) {
  if (!destfile) {
    if (!OpenDestFile())
      return -1;
  }

  bytesDownloaded += len;
  if (progressUpdater)
    progressUpdater->Progress(bytesDownloaded);

  return destfile->Write(buf, len);
}

void cFileDownloadRequest::StreamFinishedWrapper(void *data) {
  cFileDownloadRequest *self = (cFileDownloadRequest *)data;
  self->RequestDone(REQERR_OK, "");
}

void cFileDownloadRequest::StartPostProcessing() {
  state = STATE_POSTPROCESS;

  const char *script = webvideoConfig->GetPostProcessCmd();
  if (!script || !destfilename) {
    state = STATE_FINISHED;
    return;
  }

  info("post-processing %s", destfilename);

  cString cmd = cString::sprintf("%s %s", 
                                 (const char *)shellEscape(script),
                                 (const char *)shellEscape(destfilename));
  debug("executing %s", (const char *)cmd);

  if (!postProcessPipe.Open(cmd, "r")) {
    state = STATE_FINISHED;
    return;
  }

  int flags = fcntl(fileno(postProcessPipe), F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(fileno(postProcessPipe), F_SETFL, flags);
}

int cFileDownloadRequest::ReadFile() {
  if (state == STATE_STREAM_DOWNLOAD) {
    return streamSocket;
  } else if (state == STATE_POSTPROCESS) {
    FILE *f = postProcessPipe;

    if (f)
      return fileno(f);
    else
      return -1;
  }

  return -1;
}

bool cFileDownloadRequest::Read() {
  const size_t BUF_LEN = 512;
  char buf[BUF_LEN];

  if (!(FILE *)postProcessPipe)
    return false;
  
  while (true) {
    ssize_t nbytes = read(fileno(postProcessPipe), buf, BUF_LEN);

    if (nbytes < 0) {
      if (errno != EAGAIN && errno != EINTR) {
        LOG_ERROR_STR("post process pipe");
        return false;
      }
    } else if (nbytes == 0) {
      info("post-processing of %s finished", destfilename);

      if (IsAborted())
        RequestDone(REQERR_ABORT, "Aborted");
      else
        RequestDone(REQERR_OK, "");

      return true;
    } else {
      debug("pp: %.*s", nbytes, buf);

      if (nbytes < (ssize_t)BUF_LEN)
        return true;
    }
  }

  return true;
}

// --- cStreamUrlRequest ---------------------------------------------------

cStreamUrlRequest::cStreamUrlRequest(int ID, const char *ref)
: cMenuRequest(ID, REQT_STREAM, ref) {
  //AppendQualityParamsToRef();
}

WebviHandle cStreamUrlRequest::PrepareHandle() {
  if (handle == -1) {
    handle = webvi_new_request(webvi, href);

    if (handle != -1) {
      webvi_set_opt(webvi, handle, WEBVIOPT_WRITEFUNC, WriteCallback);
      webvi_set_opt(webvi, handle, WEBVIOPT_WRITEDATA, this);
    }
  }

  return handle;
}

void cStreamUrlRequest::RequestDone(int errorcode, cString pharse) {
  if (errorcode == 0) {
    parseStreamMetadataFromXml(inBuffer.Get(), inBuffer.Length(), streamUrl, streamTitle);
  }
  cMenuRequest::RequestDone(errorcode, pharse);
}

void parseStreamMetadataFromXml(const char *xml, size_t length, cString& outUrl, cString& outTitle) {
  outUrl = "";
  outTitle = "";

  xmlDocPtr doc = xmlReadMemory(xml, length, "menu.xml", NULL, 0);
  if (doc == NULL) {
    return;
  }

  xmlNodePtr root = xmlDocGetRootElement(doc);
  if (root && xmlStrEqual(root->name, BAD_CAST "wvmenu")) {
    xmlNodePtr node = root->children;
    while (node) {
      if (xmlStrEqual(node->name, BAD_CAST "ul")) {
        xmlNodePtr linode = node->children;
        while (linode) {
          xmlNodePtr anode = linode->children;
          while (anode) {
            if (xmlStrEqual(anode->name, BAD_CAST "a")) {
              xmlChar *xmlTitle = xmlNodeGetContent(anode);
              if (xmlTitle) {
                outTitle = cString((const char*)xmlTitle);
                xmlFree(xmlTitle);
              }

              xmlChar *xmlHref = xmlGetNoNsProp(anode, BAD_CAST "href");
              if (xmlHref) {
                outUrl = cString((const char *)xmlHref);
                xmlFree(xmlHref);
              }

              xmlFreeDoc(doc);
              return;
            }

            anode = anode->next;
          }

          linode = linode->next;
        }
      }

      node = node->next;
    }
  }

  xmlFreeDoc(doc);
}

cString cStreamUrlRequest::getStreamUrl() {
  return streamUrl;
}

cString cStreamUrlRequest::getStreamTitle() {
  return streamTitle;
}

// --- cTimerRequest -------------------------------------------------------

cTimerRequest::cTimerRequest(int ID, const char *ref)
: cMenuRequest(ID, REQT_TIMER, ref)
{
}

// --- cRequestVector ------------------------------------------------------

cMenuRequest *cRequestVector::FindByHandle(WebviHandle handle) {
  for (int i=0; i<Size(); i++)
    if (At(i)->GetHandle() == handle)
      return At(i);

  return NULL;
}
