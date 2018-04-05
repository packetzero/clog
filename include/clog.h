#ifndef _CLOG_BASIC_H_
#define _CLOG_BASIC_H_

#include <stdio.h>
#include <stdint.h>
#include <string>

#include "clog_defs.h"

#ifndef CLL_DEFAULT
#define CLL_DEFAULT CLL_WARN
#endif

#ifdef LOCAL_CLOG_FILEID
#define CLOG_FILEID LOCAL_CLOG_FILEID
#else
#pragma message ("Warning: You need to (#define LOCAL_CLOG_FILEID actualid) in your file before including clog header files.  Otherwise, symbol lookup for hidden string will not be possible.")
#define CLOG_FILEID 0
#endif

#define CLOG_ENABLED(PTR_OR_MODULEID,LEVEL) CLog::isEnabled((int)PTR_OR_MODULEID, LEVEL)

// hidden strings are inline when _DEBUG is defined
#ifndef NDEBUG
  #define CLOG_LOG(PTR_OR_MODULEID,LEVEL,HIDDENMSG,...) if (CLog::isEnabled(PTR_OR_MODULEID,LEVEL)) CLog::log(PTR_OR_MODULEID, LEVEL, CLOG_FILEID, __LINE__, __FILE__, #HIDDENMSG, CLog::render(__VA_ARGS__))
#else
  #define CLOG_LOG(PTR_OR_MODULEID,LEVEL,HIDDENMSG,...) if (CLog::isEnabled(PTR_OR_MODULEID,LEVEL)) CLog::log(PTR_OR_MODULEID, LEVEL, CLOG_FILEID, __LINE__, "", "", CLog::render(__VA_ARGS__))
#endif // _DEBUG

#ifdef WIN32
#define LINEENDING "\r\n"
#include <Windows.h>
#else
#include <libgen.h>
#include <sys/time.h>
#include <unistd.h> // getpid
#include <pthread.h>
#define LINEENDING "\n"
#endif

struct CLogApp {
  /*
   * return an application specific name for module
   */
  virtual std::string getModuleName(int moduleId) = 0;

  /*
   * return STDOUT, STDERR, or other, causing onLogLine() to be called
   */
  virtual int getDest() { return 1; /* STDOUT */}

  virtual void onLogLine(const char *str, int len) { }
};

class CLog {
public:

  static void log(int moduleId, int level, uint64_t fileId, uint32_t lineNum, const std::string file, const std::string hiddenMsg, const std::string msg) {
    State &_state = getState();

    std::string extra = "";
    std::string filename = file;

#ifdef WIN32
    uint64_t threadId = (uint64_t)GetCurrentThreadId();
#else
    uint64_t threadId = (uint64_t)pthread_self();
    if (filename.length() > 0) {
      filename = std::string(basename((char *)filename.c_str()));
    }
#endif

    char labelId[64]="";
    char moduleStr[64]="";
    if (file.length() > 0 && _state.backend != 0L) {
      snprintf(moduleStr,sizeof(moduleStr),"%s", _state.backend->getModuleName(moduleId).c_str());
      snprintf(labelId,sizeof(labelId),":%d", lineNum);
      extra = " [" + filename + std::string(labelId) + "]";
      labelId[0] = 0;
    } else {
      snprintf(moduleStr,sizeof(moduleStr),"%02d", moduleId);
      snprintf(labelId,sizeof(labelId),"%llx:%d", fileId, lineNum);
    }

    // int timestamp output in release, more fancy in debug
    uint64_t ts = getTime();
    char timestamp[64]="";
#ifndef NDEBUG
#ifdef WIN32
    snprintf(timestamp,sizeof(timestamp), "%lld", ts);
#else
    time_t now = (time_t)(ts / 10000000);
    uint32_t subseconds = (ts % 10000000) / 10;
    struct tm *tm = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S.", tm);
    size_t tslen = strlen(timestamp);
    snprintf(timestamp + tslen, sizeof(timestamp)-tslen,"%06d", subseconds);
    strftime(timestamp + strlen(timestamp),6,  "%z", tm);
#endif // WIN32
#else
    snprintf(timestamp,sizeof(timestamp), "%lld", ts);
#endif

    // put it all together
    char *entryStr=0L;
    int entryLen = asprintf(&entryStr, "%s %c %s %s P:%05d T:%04x %s %s%s%s", timestamp, levelChar(level), moduleStr, labelId, _state.pid, (uint32_t)(0x0FFFF & threadId), (file.length() > 0 ? hiddenMsg.c_str() : ""), msg.c_str(), extra.c_str(), LINEENDING);
    if (entryStr == 0L) {
      // malloc error
      return;
    }

    FILE *outfile = stdout;
    if (_state.backend != 0L) {
      switch(_state.backend->getDest()) {
        case 1: outfile = stdout; break;
        case 2: outfile = stderr; break;
        default:
          outfile = NULL;
      }
    }

    if (NULL == outfile) {
      _state.backend->onLogLine(entryStr, entryLen);
    } else {
      fputs(entryStr, outfile);
      fflush(outfile);
    }

    free(entryStr);
  }

  static bool isEnabled(int moduleId, int level) {
    State &_state = getState();
    return (_state.moduleLevels[moduleId] >= level);
  }

  static void setApp(CLogApp* ptr) {
    State &_state = getState();
    _state.backend = ptr;
  }

  static std::string render(const char *fmt, ...) {
    va_list va_args;
    va_start(va_args, fmt);
    size_t length = vsnprintf(NULL, 0, fmt, va_args);
    va_end(va_args);

    va_start(va_args, fmt);
    char *temp = new char [length+1];
    vsnprintf(temp, length + 1, fmt, va_args);
    va_end(va_args);

    return std::string(temp);
  }

  static void setModuleLevel(int level, int moduleId) {
    State &_state = getState();
    if (moduleId < 0 || moduleId >= 64) return;
    if (level < 0 || level >= CLL_COUNT) return;
    _state.moduleLevels[moduleId] = level;
  }

  static void setDefaultLevel(int level) {
    State &_state = getState();
    for (int i=0; i < 64; i++) {
      _state.moduleLevels[i] = level;
    }
  }

  // Examples:
  // "2:D,4:I"    // sets module 2 to CLL_DEBUG, module 4 to CLL_INFO
  // return 0 on success, or index of error
  static int setLevels(const std::string val) {
    State &_state = getState();
    const char *end = val.c_str() + val.length();
    const char *p = val.c_str();
    while (p < end) {
      const char *colon = p + 1;
      while (colon < end && *colon != ':' && *colon != ',') colon++;
      //if (colon >= end) return (int)(end - val.c_str());
      if (*colon == ',' || colon >= end) {
        // special case, specifying default level
        int level = *p;
        setDefaultLevel(level);
        p = colon + 1;
        continue;
      }

      std::string modIdStr = std::string(p, colon);
      int moduleId = atoi(modIdStr.c_str());
      if (moduleId < 0 || moduleId >= 64) return (int)(p - val.c_str());

      p = colon + 1;
      int level = levelForChar(*p);

      setModuleLevel(level, moduleId);

      // advance
      while (p < end && *p != ',') p++;
      p++;
    }

    return 0;
  }

private:

  struct State {
    State() {
      backend = 0L;
      pid = getpid();
      for (int i=0; i < 64; i++) moduleLevels[i] = CLL_DEFAULT;
    }
    char      moduleLevels[64];
    CLogApp * backend;
    uint32_t  pid;
  };

  static State& getState() {
    static State _s = State();
    return _s;
  }

  static uint64_t getTime() {
#ifdef WIN32
    	FILETIME ft;
    	GetSystemTimeAsFileTime(&ft);
    	LARGE_INTEGER date, adjust;
    	date.HighPart = ft.dwHighDateTime;
    	date.LowPart = ft.dwLowDateTime;
    	adjust.QuadPart = 11644473600000 * 10000;
    	date.QuadPart -= adjust.QuadPart;
    	return date.QuadPart;
#else
      struct timeval tv;
      gettimeofday(&tv,NULL);
      uint64_t micros = (1000000 * tv.tv_sec + tv.tv_usec);
      return micros * 10;
#endif
  }

  //--------------------------------------------------------------
  // Convenience method that returns 'E' for LVL_ERROR, 'T' for
  // LVL_TRACE, etc.
  // returns '?' if unknown
  //--------------------------------------------------------------
  static char levelChar(int level)
  {
    switch (level) {
    case CLL_FATAL: return 'F';
    case CLL_ERROR: return 'E';
    case CLL_WARN:  return 'W';
    case CLL_INFO:  return 'I';
    case CLL_DEBUG: return 'D';
    case CLL_TRACE: return 'T';
    case CLL_VERBOSE: return 'V';
    default:
      break;
    }
    return '?';
  }

  static int levelForChar(char levelChar)
  {
  switch(levelChar) {
    case 'F': return CLL_FATAL;
    case 'E': return CLL_ERROR;
    case 'W': return CLL_WARN;
    case 'I': return CLL_INFO;
    case 'D': return CLL_DEBUG;
    case 'T': return CLL_TRACE;
    case 'V': return CLL_VERBOSE;
    default:
      break;
  }
  return CLL_INFO;
  }

};



#endif // _CLOG_BASIC_H_
