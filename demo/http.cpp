#include "modules.h"
#define LOCAL_CLOG_FILEID 0x2000
#include <clog.h>

int run()
{
  int retval = 1;
  CLOG_TRACE(ModHttp, "run() enter","");

  CLOG_WARN(ModHttp,"oops bad assumption","Error:%d", 33);

  CLOG_TRACE(ModHttp, "run() exit","%d", retval);
  return retval;
}
