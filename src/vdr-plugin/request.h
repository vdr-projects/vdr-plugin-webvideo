/*
 * request.h: Web video plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __WEBVIDEO_REQUEST_H
#define __WEBVIDEO_REQUEST_H

#include <vdr/tools.h>
#include <vdr/thread.h>
#include <libwebvi.h>
#include "buffer.h"
#include "filedownloader.h"

enum eRequestType { REQT_NONE, REQT_MENU, REQT_FILE, REQT_STREAM, REQT_TIMER };

#define REQERR_OK 0
#define REQERR_INTERNAL -1
#define REQERR_ABORT -2

class cFileDownloadRequest;
class cWebviTimer;

void parseStreamMetadataFromXml(const char *xml, size_t length, cString& outUrl, cString& outTitle);

// --- cDownloadProgress ---------------------------------------------------

class cDownloadProgress {
private:
  const static int NAME_LEN = 128;

  char name[NAME_LEN];
  long downloaded;
  long total;
  int statusCode;
  cString statusPharse;
  cFileDownloadRequest *req;
  cMutex mutex;
public:
  cDownloadProgress();

  void AssociateWith(cFileDownloadRequest *request);
  void SetContentLength(long bytes);
  void SetTitle(const char *title);
  void Progress(long downloadedbytes);
  void MarkDone(int errorcode, cString pharse);
  bool IsFinished();

  cString GetTitle();
  cString GetPercentage();
  cString GetStatusPharse();
  bool Error();
  cFileDownloadRequest *GetRequest() { return req; }
};

// --- cProgressVector -----------------------------------------------------

class cProgressVector : public cVector<cDownloadProgress *> {
public:
  cDownloadProgress *NewDownload();
};

// --- cRequest ------------------------------------------------------------

class cRequest {
private:
  int reqID;
  eRequestType type;

protected:
  char *href;
  bool aborted;
  bool finished;

public:
  cRequest(int ID, eRequestType type, const char *href);
  virtual ~cRequest();

  int GetID() const { return reqID; }
  virtual eRequestType GetType() const { return type; }
  const char *GetReference() const { return href; }

  virtual void RequestDone(int errorcode, cString pharse);
  bool IsFinished() const { return finished; }
  virtual void Abort();
  bool IsAborted() const { return aborted; }

  virtual int ReadFile() { return -1; }
  virtual bool Read() { return true; }
};

// --- cMenuRequest --------------------------------------------------------

class cMenuRequest : public cRequest {
private:
  int status;
  cString statusPharse;

protected:
  WebviCtx webvi;
  WebviHandle handle;
  cMemoryBuffer inBuffer;
  cWebviTimer *timer;

  virtual ssize_t WriteData(const char *ptr, size_t len);
  virtual WebviHandle PrepareHandle();
  static ssize_t WriteCallback(const char *ptr, size_t len, void *request);

  char *ExtractSiteName(const char *ref);
  void AppendQualityParamsToRef();

public:
  cMenuRequest(int ID, eRequestType type, const char *wvtreference);
  virtual ~cMenuRequest();

  WebviHandle GetHandle() const { return handle; }

  bool Start(WebviCtx webvictx);
  virtual void Abort();
  virtual void RequestDone(int errorcode, cString pharse);

  // Return true if the lastest status code indicates success.
  bool Success() const;
  // Return the status code
  int GetStatusCode() const { return status; }
  // Return the response pharse
  cString GetStatusPharse() const;
  virtual void SetDownloader(iAsyncFileDownloaderManager *dlmanager) {};

  // Return the content of the response message
  virtual cString GetResponse();

  void SetTimer(cWebviTimer *t) { timer = t; }
  cWebviTimer *GetTimer() { return timer; }
};

// --- cFileDownloadRequest ------------------------------------------------

class cFileDownloadRequest : public cMenuRequest {
private:
  enum eDownloadState { STATE_GET_STREAM_URL, STATE_STREAM_DOWNLOAD,
                        STATE_POSTPROCESS, STATE_FINISHED };

  char *title;
  long bytesDownloaded;
  long contentLength;
  int streamSocket;
  cUnbufferedFile *destfile;
  char *destfilename;
  cDownloadProgress *progressUpdater;
  cPipe postProcessPipe;
  eDownloadState state;
  cString streamUrl;
  cString streamTitle;
  iAsyncFileDownloaderManager *downloadManager;
  iFileDownloadTask *streamDownloader;

  static ssize_t StreamReadWrapper(void *buf, size_t count, void *data);
  static void StreamFinishedWrapper(void *data);

protected:
  virtual WebviHandle PrepareHandle();
  bool OpenDestFile();
  char *GetExtension(const char *contentType, const char *url);
  void StartStreamDownload();
  void StartPostProcessing();
  ssize_t WriteToDestFile(void *buf, size_t len);

  static bool IsRTMPStream(const char *url);

public:
  cFileDownloadRequest(int ID, const char *streamref, 
                       cDownloadProgress *progress);
  virtual ~cFileDownloadRequest();

  void SetDownloader(iAsyncFileDownloaderManager *dlmanager);

  void RequestDone(int errorcode, cString pharse);
  void Abort();

  virtual int ReadFile();
  bool Read();
};

// --- cStreamUrlRequest ---------------------------------------------------

class cStreamUrlRequest : public cMenuRequest {
private:
  cString streamUrl;
  cString streamTitle;

protected:
  virtual WebviHandle PrepareHandle();

public:
  cStreamUrlRequest(int ID, const char *ref);

  virtual void RequestDone(int errorcode, cString pharse);

  cString getStreamUrl();
  cString getStreamTitle();
};

// --- cTimerRequest -------------------------------------------------------

class cTimerRequest : public cMenuRequest {
public:
  cTimerRequest(int ID, const char *ref);
};

// --- cRequestVector ------------------------------------------------------

class cRequestVector : public cVector<cMenuRequest *> {
public:
  cRequestVector(int Allocated = 10) : cVector<cMenuRequest *>(Allocated) {}

  cMenuRequest *FindByHandle(WebviHandle handle);
};

#endif
