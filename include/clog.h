/*
 * Copyright 2018-present Alex Malone, Ziften Technologies, Inc. All Rights Reserved.
 * Apache-2 license.
 */
#ifndef _CLOG_BASIC_H_
#define _CLOG_BASIC_H_

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <map>

enum CLogLevels {
  CLL_NONE    = 0,
  CLL_FATAL   = 1,
  CLL_ERROR   = 2,
  CLL_WARN    = 3,
  CLL_INFO    = 4,
  CLL_DEBUG   = 5,
  CLL_TRACE   = 6,
  CLL_VERBOSE = 7,
  CLL_COUNT
};

#define CLOG_MAX_MODULES 64

/*
 * Use these convenience aliases, instead of CLOG_LOG()
 */
#define CLOG_FATAL(MODULEID,HIDDENMSG,...) CLOG_LOG(MODULEID,CLL_FATAL,HIDDENMSG, __VA_ARGS__)
#define CLOG_ERR(MODULEID,HIDDENMSG,...) CLOG_LOG(MODULEID,CLL_ERROR,HIDDENMSG, __VA_ARGS__)
#define CLOG_WARN(MODULEID,HIDDENMSG,...) CLOG_LOG(MODULEID,CLL_WARN,HIDDENMSG, __VA_ARGS__)
#define CLOG_INFO(MODULEID,HIDDENMSG,...) CLOG_LOG(MODULEID,CLL_INFO,HIDDENMSG, __VA_ARGS__)
#define CLOG_DBG(MODULEID,HIDDENMSG,...) CLOG_LOG(MODULEID,CLL_DEBUG,HIDDENMSG, __VA_ARGS__)
#define CLOG_TRACE(MODULEID,HIDDENMSG,...) CLOG_LOG(MODULEID,CLL_TRACE,HIDDENMSG, __VA_ARGS__)
#define CLOG_VERBOSE(MODULEID,HIDDENMSG,...) CLOG_LOG(MODULEID,CLL_VERBOSE,HIDDENMSG, __VA_ARGS__)
#define CLOG_DEBUG CLOG_DBG
#define CLOG_ERROR CLOG_ERR

/*
 * Override default level at compile-time, set CFLAGS="-DCLL_DEFAULT=CLL_INFO"
 */

#ifndef CLL_DEFAULT
#define CLL_DEFAULT CLL_WARN
#endif

/*
 * At the top of each .cpp file using logging, define a LOCAL_CLOG_FILEID
 * with a unique hex value (uint64_t).  This number is output in release
 * logging lines, and used for matching to file name in rehydration.
 *
 * Example:
 * #define LOCAL_CLOG_FILEID 0x12345678
 */
#ifdef LOCAL_CLOG_FILEID
#define CLOG_FILEID LOCAL_CLOG_FILEID
#else
#pragma message ("Warning: You need to (#define LOCAL_CLOG_FILEID actualid) in your file before including clog header files.  Otherwise, symbol lookup for hidden string will not be possible.")
#define CLOG_FILEID 0
#endif

#define CLOG_ENABLED(MODULEID,LEVEL) CLog::isEnabled((int)MODULEID, LEVEL)

// When NDEBUG is defined, __FILE__ and HIDDENMSG are passed to ::log().
#ifndef NDEBUG
  #define CLOG_LOG(MODULEID,LEVEL,HIDDENMSG,...) if (CLog::isEnabled(MODULEID,LEVEL)) CLog::log(MODULEID, LEVEL, CLOG_FILEID, __LINE__, __FILE__, #HIDDENMSG, CLog::render(__VA_ARGS__))
#else
  #define CLOG_LOG(MODULEID,LEVEL,HIDDENMSG,...) if (CLog::isEnabled(MODULEID,LEVEL)) CLog::log(MODULEID, LEVEL, CLOG_FILEID, __LINE__, "", "", CLog::render(__VA_ARGS__))
#endif // _DEBUG

#ifdef _WIN32
#define LINEENDING "\r\n"
#include <Windows.h>
#include <time.h>
#else
#include <libgen.h>
#include <sys/time.h>
#include <unistd.h> // getpid
#include <pthread.h>
#define LINEENDING "\n"
#endif

/*
 * Optional: implement the CLogApp interface and pass instance to
 * CLog::setApp() to add more control.
 * At a minimum, you will want to implement getModuleNameMap() for use
 * when logging NDEBUG is not defined.  Otherwise, the logs will only
 * contain module ids.
 */

typedef std::map<int,std::string> CLogModNameMap;
typedef std::map<int,std::string>::const_iterator CLogModNameMapIter;

struct CLogApp {

  /*
   * A way to detect and manage future versions
   */
  virtual int getVersion() { return 1; }

  /*
   * return an application specific module ids and their names.
   * module ids MUST be > 0.
   */
  virtual const CLogModNameMap& getModuleNameMap() = 0;

  /*
   * return STDOUT, STDERR, or other, causing onLogLine() to be called
   */
  virtual int getDest() { return 1; /* STDOUT */}

  /**
   * Optionally, applications can receive onLogLine callbacks
   * and output to desired destinations.  Sometimes a destination will
   * have it's own timestamp, so timestampStr is separated from the rest of the
   * log string.
   *
   * timestampStr null-terminated string formatted timestamp. Release is an
   *              integer milliseconds, Debug is human-readable string.
   * str          formatted log line string
   * len          length in bytes of str.
   */
  virtual void onLogLine(const char *timestampStr, const char *str, int len) { }
};

class CLog {
public:
  /*
   * Optionally implement CLogApp and call setApp to provide
   * more output control, and getModuleNameMap().
   */
  static void setApp(CLogApp* ptr) {
    State &_state = getState();
    _state.app = ptr;
  }

  static std::string _parseError(uint64_t offset, std::string msg, std::string item)
  {
    char buf[256];
    snprintf(buf,sizeof(buf),"CLog::setLevels() parse error at %d. %s : '%s'", (int)offset, msg.c_str(), item.c_str());
    return buf;
  }

  /*
   * Set the default level, as well as individual module levels
   * using a comma-delimited string.
   *
   * Examples:
   *  "2:D,4:I"    // sets module 2 to CLL_DEBUG, module 4 to CLL_INFO
   *  "V"          // same as setDefaultLevel(CLL_VERBOSE)
   *
   * @returns Empty string on success, or parse rror message
   */
  static std::string setLevels(const std::string val) {
    State &_state = getState();
    const char *end = val.c_str() + val.length();
    const char *p = val.c_str();

    const CLogModNameMap& _modNameMap = (_state.app ? _state.app->getModuleNameMap() : CLogModNameMap());

    while (p < end) {
      const char *colon = p + 1;
      while (colon < end && *colon != ':' && *colon != ',') colon++;
      if (*colon == ',' || colon >= end) {
        // special case, specifying default level

        if ((colon-p) > 1) {
          return _parseError((p - val.c_str()), "Invalid Level Char", std::string(p,colon));
        }

        int level = levelForChar(*p);
        if (level == CLL_NONE) {
          return _parseError((p - val.c_str()), "Invalid Level Char", std::string(p,p+1));
        }
        setDefaultLevel(level);
        p = colon + 1;
        continue;
      }


      std::string modIdStr = std::string(p, colon);
      int moduleId = _getModuleIdFromName(_modNameMap, modIdStr);

      // first try module name

      if (moduleId < 0) {

        // must be a module number

        moduleId = atoi(modIdStr.c_str());

        // check that the module number is in the map, or they probably made mistake

        if (_modNameMap.size() > 0 && _getModuleName(_modNameMap, moduleId).length() == 0) {
          return _parseError((p - val.c_str()), "Module Id not in map", modIdStr);
        }
      }

      if (moduleId <= 0 || moduleId >= CLOG_MAX_MODULES) {
        return _parseError((p - val.c_str()), "Unknown Module name or id", modIdStr);
      }

      p = colon + 1;
      int level = levelForChar(*p);
      if (level == CLL_NONE) {
        return _parseError((p - val.c_str()), "Invalid Level Char", std::string(p,p+1));
      }

      setModuleLevel(level, moduleId);

      // advance
      while (p < end && *p != ',') p++;
      p++;
    }

    return "";
  }

  /*
   * set level for specific module at runtime.
   */
  static void setModuleLevel(int level, int moduleId) {
    State &_state = getState();
    if (moduleId < 0 || moduleId >= CLOG_MAX_MODULES) return;
    if (level < 0 || level >= CLL_COUNT) return;
    _state.moduleLevels[moduleId] = level;
  }

  /*
   * set level for all modules at runtime.
   */
  static void setDefaultLevel(int level) {
    State &_state = getState();
    for (int i=0; i < CLOG_MAX_MODULES; i++) {
      _state.moduleLevels[i] = level;
    }
  }

  static std::string _getModuleName(const CLogModNameMap &modNameMap, int moduleId) {
    CLogModNameMapIter fit = modNameMap.find(moduleId);
    if (fit != modNameMap.end()) {
      return fit->second;
    }
    return "";
  }

  static int _getModuleIdFromName(const CLogModNameMap &modNameMap, std::string name) {
    for (CLogModNameMapIter it = modNameMap.begin(); it != modNameMap.end(); it++) {
      if (it->second == name) {
        return it->first;
      }
    }
    return -1;
  }

  /*
   * Don't call this directly.  Use the CLOG_ERROR(), etc. macros.
   * If setApp() was called to provide a CLogApp instance, and
   * the getDest() returns a value > 2, then CLogApp->onLogLine() will
   * be called with the final string.  Otherwise, the log line will
   * be output to stdout or stderr as indicated.
   *
   * @param moduleId
   * @param level
   * @param fileId  Resolves to LOCAL_CLOG_FILEID defined in each file.
   * @param lineNum __LINE__ macro value.
   * @param file    When NDEBUG is not defined, this is __FILE__ value.
   * @param hiddenMsg The second parameter to CLOG_ERROR(), etc. macro.
   * @param msg     The sprintf'ed (FMT,...) value.
   */
  static void log(int moduleId, int level, uint64_t fileId, uint32_t lineNum,
                  const std::string file, const std::string hiddenMsg, const std::string msg) {
    State &_state = getState();

    std::string extra = "";
    std::string filename = file;

#ifdef WIN32
    uint64_t threadId = (uint64_t)GetCurrentThreadId();
#else
    uint64_t threadId = (uint64_t)pthread_self();

    // CMake will use full path name... strip to just filename
    if (filename.length() > 0 && filename[0] == '/') {
      filename = std::string(basename((char *)filename.c_str()));
    }
#endif

    // string for module (id or name) and label (fileid:line)

    char labelId[64]="";
    char moduleStr[64]="";
    if (file.length() > 0 && _state.app != 0L) {
      snprintf(moduleStr,sizeof(moduleStr),"%s", _getModuleName(_state.app->getModuleNameMap(), moduleId).c_str());
      snprintf(labelId,sizeof(labelId),":%d", lineNum);
      extra = " [" + filename + std::string(labelId) + "]";
      labelId[0] = 0;
    } else {
      snprintf(moduleStr,sizeof(moduleStr),"%02d", moduleId);
      snprintf(labelId,sizeof(labelId),"%llx:%d", fileId, lineNum);
    }

    char timestamp[64]="";
    _formatTimestamp(timestamp, sizeof(timestamp), getTime());

    // put it all together
    char *entryStr=0L;
    int entryLen = asprintf(&entryStr, " %c %s %s P:%05d T:%04x %s %s%s%s", levelChar(level), moduleStr, labelId, _state.pid, (uint32_t)(0x0FFFF & threadId), (file.length() > 0 ? hiddenMsg.c_str() : ""), msg.c_str(), extra.c_str(), LINEENDING);
    if (entryStr == 0L) {
      // malloc error
      return;
    }

    // determine destination

    FILE *outfile = stdout;
    if (_state.app != 0L) {
      switch(_state.app->getDest()) {
        case 1: outfile = stdout; break;
        case 2: outfile = stderr; break;
        default:
          outfile = NULL;
      }
    }

    if (NULL == outfile) {
      _state.app->onLogLine(timestamp, entryStr, entryLen);
    } else {
      fputs(timestamp, outfile);
      fputs(entryStr, outfile);
      fflush(outfile);
    }

    free(entryStr);  // asprintf malloc'ed
  }

  /*
   * don't call directly. Used in CLOG_XX macros
   */
  static bool isEnabled(int moduleId, int level) {
    State &_state = getState();
    return (_state.moduleLevels[moduleId] >= level);
  }

  /*
   * Render the FMT,.. args into a string
   */
  static std::string render(const char *fmt, ...) {
    va_list va_args;
    va_start(va_args, fmt);
    size_t length = vsnprintf(NULL, 0, fmt, va_args);
    va_end(va_args);

    va_start(va_args, fmt);
    char *temp = new char [length+1];
    vsnprintf(temp, length + 1, fmt, va_args);
    va_end(va_args);

    std::string retval = std::string(temp);
    delete [] temp;
    return retval;
  }

private:

  /*
   * State class contains all CLog static variable info.
   */
  struct State {
    State() : app(0L), version(1) {
      app = 0L;
#ifdef _WIN32
      pid = GetCurrentProcessId();
#else
      pid = getpid();
#endif
      for (int i=0; i < CLOG_MAX_MODULES; i++) moduleLevels[i] = CLL_DEFAULT;
    }
    CLogApp * app;
    int       version;
    uint32_t  pid;
    char      moduleLevels[CLOG_MAX_MODULES];
  };

  static State& getState() {
    // Static variable instance in function for header-only implementation,
    // in lieu of static class variables.
    static State _s = State();
    return _s;
  }

  // Returns hundred-nano timestamp
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

  /*
   * int timestamp output in release, more fancy in debug
   */
  static void _formatTimestamp(char *dest, int destlen, uint64_t ts)
  {
#ifndef NDEBUG
#ifdef WIN32
    snprintf(dest,destlen, "%lld", ts);
#else
    time_t now = (time_t)(ts / 10000000);
    uint32_t subseconds = (ts % 10000000) / 10;
    struct tm *tm = localtime(&now);
    strftime(dest, destlen, "%Y-%m-%d %H:%M:%S.", tm);
    size_t tslen = strlen(dest);
    snprintf(dest + tslen, destlen-tslen,"%06d", subseconds);
    strftime(dest + strlen(dest),6,  "%z", tm);
#endif // WIN32
#else
    snprintf(dest,destlen, "%lld", ts);
#endif
  }

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
  return CLL_NONE;
  }

};



#endif // _CLOG_BASIC_H_
