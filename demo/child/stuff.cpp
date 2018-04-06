#include "../modules.h"
#define LOCAL_CLOG_FILEID 0x3301
#include <clog.h>

void do_some_stuff(int a, double b, bool c)
{
  CLOG_TRACE(ModMisc, "do_some_stuff() enter","%d %.3f %d", a, b, c);

  CLOG_INFO(ModMisc,"","a midly interesting value:%d", a);

  CLOG_TRACE(ModMisc, "do_some_stuff() exit","");
}
