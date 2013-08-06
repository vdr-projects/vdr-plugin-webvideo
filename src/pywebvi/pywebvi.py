from ctypes import *
import ctypes.util
import weakref

_WEBVIERR_OK = 0

_WEBVI_INVALID_HANDLE = -1

_WEBVIOPT_WRITEFUNC = 0
_WEBVIOPT_READFUNC = 1
_WEBVIOPT_WRITEDATA = 2
_WEBVIOPT_READDATA = 3

_WEBVIINFO_URL = 0
_WEBVIINFO_CONTENT_LENGTH = 1
_WEBVIINFO_CONTENT_TYPE = 2
_WEBVIINFO_STREAM_TITLE = 3

_WEBVI_CONFIG_TEMPLATE_PATH = 0
_WEBVI_CONFIG_DEBUG = 1
_WEBVI_CONFIG_TIMEOUT_CALLBACK = 2
_WEBVI_CONFIG_TIMEOUT_DATA = 3

class WebviState:
    NOT_FINISHED = 0
    FINISHED_OK = 1
    NOT_FOUND = 2
    NETWORK_READ_ERROR = 3
    IO_ERROR = 4
    TIMEDOUT = 5
    INTERNAL_ERROR = 999

class WebviMsg(Structure):
    _fields_ = [("msg", c_int),
                ("handle", c_int),
                ("status_code", c_int),
                ("data", c_char_p)]

class WeakRequestRef(weakref.ref):
    pass

class WebviError(Exception):
    pass

def raise_if_webvi_result_not_ok(value):
    if value != _WEBVIERR_OK:
        raise WebviError('libwebvi function returned error code %d: %s' %
                         (value, strerror(value)))
    return value

def raise_if_request_not_ok(value):
    if value == -1:
        raise WebviError('libwebvi request initialization failed')
    return value

_libc = CDLL(ctypes.util.find_library("c"))
_libc.free.argtypes = [c_void_p]
_libc.free.restype = None

libwebvi = CDLL("libwebvi.so")
libwebvi.webvi_global_init()

libwebvi.webvi_initialize_context.argtypes = []
libwebvi.webvi_initialize_context.restype = c_long
libwebvi.webvi_version.argtypes = []
libwebvi.webvi_version.restype = c_char_p
libwebvi.webvi_strerror.argtypes = [c_int]
libwebvi.webvi_strerror.restype = c_char_p
libwebvi.webvi_new_request.argtypes = [c_long, c_char_p]
libwebvi.webvi_new_request.restype = raise_if_request_not_ok
libwebvi.webvi_delete_request.argtypes = [c_long, c_int]
libwebvi.webvi_delete_request.restype = raise_if_webvi_result_not_ok
libwebvi.webvi_process_some.argtypes = [c_long, c_int]
libwebvi.webvi_process_some.restype = c_int
libwebvi.webvi_get_message.argtypes = [c_long, POINTER(c_int)]
libwebvi.webvi_get_message.restype = POINTER(WebviMsg)
libwebvi.webvi_start_request.argtypes = [c_long, c_int]
libwebvi.webvi_start_request.restype = raise_if_webvi_result_not_ok
libwebvi.webvi_stop_request.argtypes = [c_long, c_int]
libwebvi.webvi_stop_request.restype = raise_if_webvi_result_not_ok

WEBVICALLBACK = CFUNCTYPE(c_ssize_t, c_char_p, c_size_t, c_void_p)
TIMEOUTFUNC = CFUNCTYPE(None, c_long, c_void_p)

def version():
    return string_at(libwebvi.webvi_version())

def strerror(err):
    return string_at(libwebvi.webvi_strerror(err))

class WebviContext:
    def __init__(self):
        self.handle = libwebvi.webvi_initialize_context()
        self._requests = {}
        self.timeout_callback = None
    
    def __del__(self):
        libwebvi.webvi_cleanup_context(self.handle)
        self.handle = None

    def set_template_path(self, path):
        if self.handle is None:
            return
        
        set_config = libwebvi.webvi_set_config
        set_config.argtypes = [c_long, c_int, c_char_p]
        set_config.restype = raise_if_webvi_result_not_ok
        set_config(self.handle, _WEBVI_CONFIG_TEMPLATE_PATH, path)

    def set_debug(self, enabled):
        if self.handle is None:
            return

        set_config = libwebvi.webvi_set_config
        set_config.argtypes = [c_long, c_int, c_char_p]
        set_config.restype = raise_if_webvi_result_not_ok
        if enabled:
            debug = "1"
        else:
            debug = "0"
        set_config(self.handle, _WEBVI_CONFIG_DEBUG, debug)

    def set_timeout_callback(self, cb):
        def callback_wrapper(timeout, userdata):
            return cb(timeout)

        set_config = libwebvi.webvi_set_config
        set_config.argtypes = [c_long, c_int, TIMEOUTFUNC]
        set_config.restype = raise_if_webvi_result_not_ok
        self.timeout_callback = TIMEOUTFUNC(callback_wrapper)
        set_config(self.handle, _WEBVI_CONFIG_TIMEOUT_CALLBACK,
                   self.timeout_callback)

    def process_some(self, timeout_seconds=0):
        return libwebvi.webvi_process_some(self.handle, int(timeout_seconds))

    def get_finished_request(self):
        remaining = c_int(0)
        msg_ptr = libwebvi.webvi_get_message(self.handle, byref(remaining))

        if not msg_ptr or msg_ptr.contents.msg != 0:
            return None

        reqhandle = msg_ptr.contents.handle
        if reqhandle not in self._requests:
            return None

        request = self._requests[reqhandle]()
        if request is None:
            return None

        return (request,
                msg_ptr.contents.status_code,
                string_at(msg_ptr.contents.data))

    def register_request(self, request):
        def unregister_request(ref):
            del self._requests[ref.handle]
            libwebvi.webvi_delete_request(self.handle, ref.handle)
            
        ref = WeakRequestRef(request, unregister_request)
        ref.handle = request.handle
        self._requests[request.handle] = ref


class WebviRequest:
    def __init__(self, context, href):
        self.context = context
        self.read_callback = None
        self.handle = libwebvi.webvi_new_request(context.handle, href)
        if self.handle == _WEBVI_INVALID_HANDLE:
            raise WebviError('Initializing request failed')
        self.context.register_request(self)

    def start(self):
        libwebvi.webvi_start_request(self.context.handle, self.handle)

    def stop(self):
        libwebvi.webvi_stop_request(self.context.handle, self.handle)

    def set_read_callback(self, cb):
        def callback_wrapper(buf, length, userdata):
            return cb(string_at(buf, length))

        set_opt = libwebvi.webvi_set_opt
        set_opt.argstypes = [c_long, c_int, c_int, WEBVICALLBACK]
        set_opt.restype = raise_if_webvi_result_not_ok
        # Must keep a reference to the callback!
        self.read_callback = WEBVICALLBACK(callback_wrapper)
        set_opt(self.context.handle, self.handle,
                _WEBVIOPT_READFUNC, self.read_callback)

        
    # def set_write_callback(self, cb):
    #     def callback_wrapper(buf, length, userdata):
    #         return cb(string_at(buf, length))

    #     set_opt = libwebvi.webvi_set_opt
    #     set_opt.argstypes = [c_long, c_int, c_int, WEBVICALLBACK]
    #     set_opt.restype = raise_if_webvi_result_not_ok
    #     set_opt(self.context.handle, self.handle, _WEBVIOPT_WRITEFUNC,
    #             WEBVICALLBACK(callback_wrapper))
    
    def get_url(self):
        get_info = libwebvi.webvi_get_info
        get_info.argtypes = [c_long, c_int, c_int, POINTER(c_char_p)]
        get_info.restype = raise_if_webvi_result_not_ok

        output = c_char_p()
        get_info(self.context.handle, self.handle,
                 _WEBVIINFO_URL, pointer(output))
        url = string_at(output)
        _libc.free(output)
        return url
        
    def get_content_length(self):
        get_info = libwebvi.webvi_get_info
        get_info.argtypes = [c_long, c_int, c_int, POINTER(c_long)]
        get_info.restype = raise_if_webvi_result_not_ok

        output = c_long(0)
        get_info(self.context.handle, self.handle,
                 _WEBVIINFO_CONTENT_LENGTH, pointer(output))
        return output.value

    def get_content_type(self):
        get_info = libwebvi.webvi_get_info
        get_info.argtypes = [c_long, c_int, c_int, POINTER(c_char_p)]
        get_info.restype = raise_if_webvi_result_not_ok

        output = c_char_p()
        get_info(self.context.handle, self.handle,
                 _WEBVIINFO_CONTENT_TYPE, pointer(output))
        content_type = string_at(output)
        _libc.free(output)
        return content_type

    def get_stream_title(self):
        get_info = libwebvi.webvi_get_info
        get_info.argtypes = [c_long, c_int, c_int, POINTER(c_char_p)]
        get_info.restype = raise_if_webvi_result_not_ok

        output = c_char_p()
        get_info(self.context.handle, self.handle,
                 _WEBVIINFO_STREAM_TITLE, pointer(output))
        title = string_at(output)
        _libc.free(output)
        return title
