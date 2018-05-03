# C++ Logging with per-module levels and string hiding for release-mode

This is a single-file C++ header-only library [clog.h](./include/clog.h).  By default, output goes to STDOUT.
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

## Rehydration

You don't need your release code to contain large strings used for debugging context, `__FILE__` paths, or module names specific to your application.  Those all get dropped on the floor during compilation when NDEBUG is defined.  Provided in this project are some ruby scripts that can be used during the build to extract this information from your .cpp, .cc, .mm files and generate a rehydrate.rb.  The rehydrate.rb file processes release logs to fill in all of the missing information, making release logs every bit as useful as debug logs.  See the example below.

## Configuration

Call `CLog::setLevels()` with a comma-delimited string of module:loglevel values, or just a loglevel to set across the board.  The value is processed in order from left to right, and modules can be specified using the id or name defined by your application.  The demo application takes the first command-line argument and passes it directly to setLevels(), and you can see some examples below.

## Building simple demo - MacOS or Linux
Generates demo/build/demor, demo/build/demod, and demo/build/rehydrate.rb
```
$ cd demo
$ sh build.sh
```

## Building demo - Windows
Generates demo/build/demor.exe, demo/build/demod.exe, and demo/build/rehydrate.rb
```
> cd demo
> build.bat
```

## Example Output - Compiled without NDEBUG defined
Basic non-release format is:
**TimestampString Level ModuleName ProcessId ThreadId HIDDEN_CONTEXT Message [`__FILE__`:`__LINE__` ]**
```
$ ./build/demod V
2018-05-03 15:13:46.318623-0500 V Main  P:53363 T:13c0 "Using arg as loglevels" V, [main.cpp:29]
2018-05-03 15:13:46.319474-0500 T Http  P:53363 T:13c0 "run() enter"  [http.cpp:8]
2018-05-03 15:13:46.319487-0500 W Http  P:53363 T:13c0 "oops bad assumption" Error:33 [http.cpp:10]
2018-05-03 15:13:46.319496-0500 T Http  P:53363 T:13c0 "run() exit" 1 [http.cpp:12]
2018-05-03 15:13:46.319516-0500 T Misc  P:53363 T:13c0 "do_some_stuff() enter" 426 3.234 0 [child/stuff.cpp:7]
2018-05-03 15:13:46.319526-0500 I Misc  P:53363 T:13c0 "" a midly interesting value:426 [child/stuff.cpp:9]
2018-05-03 15:13:46.319535-0500 T Misc  P:53363 T:13c0 "do_some_stuff() exit"  [child/stuff.cpp:11]
2018-05-03 15:13:46.319544-0500 D Main  P:53363 T:13c0 "exit" 1 [main.cpp:40]
```
## Example Output - Compiled with NDEBUG
Basic NDEBUG format is: **Timestamp Level ModuleId ProcessId ThreadId Message**
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
15253784209824950 V 01 1000:29 P:53362 T:13c0  V
15253784209825570 T 02 2000:8 P:53362 T:13c0  
15253784209825640 W 02 2000:10 P:53362 T:13c0  Error:33
15253784209825690 T 02 2000:12 P:53362 T:13c0  1
15253784209825830 T 04 3301:7 P:53362 T:13c0  420 3.234 0
15253784209825890 I 04 3301:9 P:53362 T:13c0  a midly interesting value:420
15253784209825940 T 04 3301:11 P:53362 T:13c0  
15253784209826090 D 01 1000:40 P:53362 T:13c0  1

# release - can specify modules by name as well
$ ./build/demor V,Misc:W
15253784467569020 V 01 1000:29 P:53365 T:13c0  V,Misc:W
15253784467569590 T 02 2000:8 P:53365 T:13c0  
15253784467569650 W 02 2000:10 P:53365 T:13c0  Error:33
15253784467569700 T 02 2000:12 P:53365 T:13c0  1
15253784467569790 D 01 1000:40 P:53365 T:13c0  1
```

## Example Output - Rehydrating output of NDEBUG compiled application
```
# run the release at VERBOSE and rehydrate using strings found in build
$ ./build/demor V | ruby ./build/rehydrate.rb
2018-04-05 18:08:32.899187-0500 V Main P:42499 T:53c0 "Using arg as loglevels" V [main.cpp:31]
2018-04-05 18:08:32.899238-0500 T Http P:42499 T:53c0 "run() enter"  [http.cpp:8]
2018-04-05 18:08:32.899244-0500 W Http P:42499 T:53c0 "oops bad assumption" Error:33 [http.cpp:10]
2018-04-05 18:08:32.899249-0500 T Http P:42499 T:53c0 "run() exit" 1 [http.cpp:12]
2018-04-05 18:08:32.899253-0500 D Main P:42499 T:53c0 "exit" 1 [main.cpp:39]
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
