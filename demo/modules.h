#ifndef _MY_MODULES_H_
#define _MY_MODULES_H_

// each application defines ids for modules.  This exact format is important, as
// clog_extract_lines.rb script parses it to map module ids to names.

enum MyModules {
  ModUnknown = 0, // 0 is reserved for detecting invalid configuration
  ModMain = 1,
  ModHttp = 2,
  ModConfig = 3,
  ModMisc = 4,
  ModCount
};

#endif // _MY_MODULES_H_
