#include "modules.h"
#define LOCAL_CLOG_FILEID 0x1000
#include <clog.h>

extern int run();

struct MyLogAppInfo : public CLogApp {
  /*
   * return an application specific id->name map.
   */
  virtual const CLogModNameMap& getModuleNameMap() {
    static CLogModNameMap _map = {    // static init like requires C++11
      {ModMain,   "Main"},
      {ModHttp,   "Http"},
      {ModConfig, "Config"},
      {ModMisc,   "Misc"},
    };
    return _map;
  }
};

int main(int argc, char *argv[])
{
  MyLogAppInfo clogAppInfo = MyLogAppInfo();
  CLog::setApp(&clogAppInfo);

  if (argc > 1) {
    std::string parseErr = CLog::setLevels(argv[1]);
    CLOG_VERBOSE(ModMain, "Using arg as loglevels","%s", argv[1]);
    if (parseErr.length() > 0) {
      CLOG_ERROR(ModConfig, "","%s", parseErr.c_str());
    }
  }

  int status = run();

  extern void do_some_stuff(int a, double b, bool c);
  do_some_stuff(time(NULL) % 1000, 3.234, false);

  CLOG_DBG(ModMain, "exit","%d", status);

  return status;
}
