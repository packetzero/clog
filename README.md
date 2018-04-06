# C++ Logging with per-module levels and string hiding for release-mode

This is a single-file C++ header-only library (clog.h).  By default, output goes to STDOUT.
Your application can override to send to STDERR, or anywhere you would like.
If you have ever ifdef'ed logging to add/remove statements between release and debug builds, but wished there was a better way, read on...

Features:
 - Avoid leaving `__FILE__` strings in release code (NDEBUG defined).
 - Trims `__FILE__` when absolute path used by CMake
 - Ability to set level (DEBUG, WARN, etc) for up to 64 different parts of an application
 - Avoid building strings when log level will prevent output. E.g. `if (isEnabled(DEBUG)) log(fmt,a,b,c)`
 - No need to compile-out DEBUG logging, use string hiding for developer context.
 - Can 'rehydrate' release logs with filename, module name, hidden string context.
 - No need to rehydrate when running code compiled without NDEBUG defined.


## Usage
A logging statement macro is defined as follows:
`CLOG_INFO(ModuleId, HIDDEN_CONTEXT, FORMAT, ...)`

The HIDDEN_CONTEXT is a string or symbol that gets ignored by compile when NDEBUG is defined.  Using the rehydration script, you can recover the HIDDEN_CONTEXT for on-demand viewing.

The ModuleId is an int enum defined by the application, and is used to categorize parts of the application.

## Usage example
```
int run()
{
  int retval = 1;
  CLOG_TRACE(ModHttp, "run() enter","");

  CLOG_WARN(ModHttp,"oops bad assumption","Error:%d", 33);

  CLOG_TRACE(ModHttp, "run() exit","%d", retval);
  return retval;
}
```

## Example Output - Compiled without NDEBUG defined

```
$ cd demo
$ sh build.sh

$ ./build/demod V
2018-04-05 17:55:33.638097-0500 V ModMain  P:42430 T:53c0 "Using arg as loglevels" V [main.cpp:31]
2018-04-05 17:55:33.638895-0500 T ModHttp  P:42430 T:53c0 "run() enter"  [http.cpp:8]
2018-04-05 17:55:33.638909-0500 W ModHttp  P:42430 T:53c0 "oops bad assumption" Error:33 [http.cpp:10]
2018-04-05 17:55:33.638920-0500 T ModHttp  P:42430 T:53c0 "run() exit" 1 [http.cpp:12]
2018-04-05 17:55:33.638931-0500 D ModMain  P:42430 T:53c0 "exit" 1 [main.cpp:39]
```
## Example Output - Compiled with NDEBUG
```
# release compiled demo - defaults to WARN level
$ ./build/demor
15229695953988020 W 02 2000:10 P:42493 T:53c0  Error:33

# release - specify TRACE level for module 2
$ ./build/demor 2:T
15229695670875200 T 02 2000:8 P:42492 T:53c0  
15229695670875930 W 02 2000:10 P:42492 T:53c0  Error:33
15229695670875990 T 02 2000:12 P:42492 T:53c0  1

# release - specify VERBOSE level for all modules
$ ./build/demor V
15229695097058400 V 01 1000:31 P:42491 T:53c0  V
15229695097059040 T 02 2000:8 P:42491 T:53c0  
15229695097059110 W 02 2000:10 P:42491 T:53c0  Error:33
15229695097059160 T 02 2000:12 P:42491 T:53c0  1
15229695097059210 D 01 1000:39 P:42491 T:53c0  1
```

## Example Output - Rehydrating output of NDEBUG compiled application
```
# run the release at VERBOSE and rehydrate using strings found in build
$ ./build/demor V | ruby ./build/rehydrate.rb
2018-04-05 18:08:32.899187-0500 V ModMain P:42499 T:53c0 "Using arg as loglevels" V [main.cpp:31]
2018-04-05 18:08:32.899238-0500 T ModHttp P:42499 T:53c0 "run() enter"  [http.cpp:8]
2018-04-05 18:08:32.899244-0500 W ModHttp P:42499 T:53c0 "oops bad assumption" Error:33 [http.cpp:10]
2018-04-05 18:08:32.899249-0500 T ModHttp P:42499 T:53c0 "run() exit" 1 [http.cpp:12]
2018-04-05 18:08:32.899253-0500 D ModMain P:42499 T:53c0 "exit" 1 [main.cpp:39]
```

## Setup - Module IDs
Define your module IDs in an enum, with the values explicitly set, like so:
```
enum MyModules {
  ModMain = 1,
  ModHttp = 2,
  ModConfig = 3,
  ModCount
};
```

## Setup - File IDs
For rehydration to work, every file requires a LOCAL_CLOG_FILEID define, with a unique uint64 hex value.
```
#include "modules.h"
#define LOCAL_CLOG_FILEID 0x2000
#include <clog.h>
```
