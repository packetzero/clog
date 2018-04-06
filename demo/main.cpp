#include "modules.h"
#define LOCAL_CLOG_FILEID 0x1000
#include <clog.h>

extern int run();

struct MyLogAppInfo : public CLogApp {
  /*
   * return an application specific name for module
   */
  virtual std::string getModuleName(int moduleId) {
    switch(moduleId) {
      case ModMain: return "ModMain";
      case ModHttp: return "ModHttp";
      case ModConfig: return "ModConfig";
      case ModMisc: return "ModMisc";
      default:
      break;
    }
    return "";
  }
};

int main(int argc, char *argv[])
{
  MyLogAppInfo clogAppInfo = MyLogAppInfo();
  CLog::setApp(&clogAppInfo);

  if (argc > 1) {
    int errIndex = CLog::setLevels(argv[1]);
    CLOG_VERBOSE(ModMain, "Using arg as loglevels","%s", argv[1]);
    if (errIndex != 0) {
      CLOG_ERROR(ModConfig, "","Error in log level config setting at index %d", errIndex);
    }
  }

  int status = run();

  extern void do_some_stuff(int a, double b, bool c);
  do_some_stuff(time(NULL) % 1000, 3.234, false);

  CLOG_DBG(ModMain, "exit","%d", status);

  return status;
}
