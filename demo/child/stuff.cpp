#include "../modules.h"
#define LOCAL_CLOG_FILEID 0x3301
#include <clog.h>

void do_some_stuff(int a, double b, bool c)
{
  CLOG_TRACE(ModMisc, "do_some_stuff() enter","%d %.3f %d", a, b, c);

  CLOG_INFO(ModMisc,"","a midly interesting value:%d", a);

  // looks like string extraction script stops at punctuation (commas, periods). TODO: fix?
  // test with longer strings

  CLOG_VERBOSE(ModMisc,"Lorem ipsum dolor sit amet consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.","Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");

  CLOG_TRACE(ModMisc, "do_some_stuff() exit","");
}
