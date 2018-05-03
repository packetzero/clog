# Demo Application for CLog

The app doesn't do anything, but consist of a few separate C++ files containing logging statements, and outputs logs to stdout.  It takes a single string argument that is the log setting string.

## Build

The build script first compiles the C++ application, then extracts the log lines from source code, and creates the rehydration script.

```
sh build.sh
```

The resulting files in the build directory include the debug (**demod**) and release (**demor**) executables, as well as the **rehydrate.rb** script that can be used to augment release logs with debug information.
```
-rwxr-xr-x   1 bob  staff  45936 May  3 13:39 demod
drwxr-xr-x   3 bob  staff    102 Apr  5 17:40 demod.dSYM
-rwxr-xr-x   1 bob  staff  24932 May  3 13:39 demor
-rw-r--r--   1 bob  staff   3720 May  3 13:39 rehydrate.rb
-rw-r--r--   1 bob  staff    452 May  3 13:39 strings.csv
```

## Example - Running with default WARN level in debug and release

```
$ ./build/demod
2018-05-03 13:43:44.814193-0500 W ModHttp  P:52057 T:13c0 "oops bad assumption" Error:33 [http.cpp:10]

$ ./build/demor
15253730271240860 W 02 2000:10 P:52058 T:13c0  Error:33

$ ./build/demor | ruby build/rehydrate.rb
2018-05-03 13:45:00.835322-0500 W ModHttp P:52059 T:13c0 "oops bad assumption" Error:33 [http.cpp:10]

```

## Example - Default level at DEBUG

```
$ ./build/demor D
15253732549815440 W 02 2000:10 P:52078 T:13c0  Error:33
15253732549816540 I 04 3301:9 P:52078 T:13c0  a midly interesting value:254
15253732549816610 D 01 1000:42 P:52078 T:13c0  1
```
## Example - default at INFO, module 4 at VERBOSE
```
$ ./build/demor I,4:V
15253732421419620 W 02 2000:10 P:52077 T:13c0  Error:33
15253732421420330 T 04 3301:7 P:52077 T:13c0  242 3.234 0
15253732421420400 I 04 3301:9 P:52077 T:13c0  a midly interesting value:242
15253732421420460 T 04 3301:11 P:52077 T:13c0
```
## Example - configure using module names
This can be done in either release or debug, but since debug logs include the module names, it's easier to see what we are configuring.
```
$ ./build/demod V
2018-05-03 14:57:48.375629-0500 V Main  P:53185 T:13c0 "Using arg as loglevels" V [main.cpp:29]
2018-05-03 14:57:48.376547-0500 T Http  P:53185 T:13c0 "run() enter"  [http.cpp:8]
2018-05-03 14:57:48.376560-0500 W Http  P:53185 T:13c0 "oops bad assumption" Error:33 [http.cpp:10]
2018-05-03 14:57:48.376569-0500 T Http  P:53185 T:13c0 "run() exit" 1 [http.cpp:12]
2018-05-03 14:57:48.376588-0500 T Misc  P:53185 T:13c0 "do_some_stuff() enter" 468 3.234 0 [child/stuff.cpp:7]
2018-05-03 14:57:48.376598-0500 I Misc  P:53185 T:13c0 "" a midly interesting value:468 [child/stuff.cpp:9]
2018-05-03 14:57:48.376607-0500 T Misc  P:53185 T:13c0 "do_some_stuff() exit"  [child/stuff.cpp:11]
2018-05-03 14:57:48.376616-0500 D Main  P:53185 T:13c0 "exit" 1 [main.cpp:41]

$ ./build/demod V,Misc:I
2018-05-03 14:57:59.455254-0500 V Main  P:53186 T:13c0 "Using arg as loglevels" V,Misc:I [main.cpp:29]
2018-05-03 14:57:59.456041-0500 T Http  P:53186 T:13c0 "run() enter"  [http.cpp:8]
2018-05-03 14:57:59.456054-0500 W Http  P:53186 T:13c0 "oops bad assumption" Error:33 [http.cpp:10]
2018-05-03 14:57:59.456063-0500 T Http  P:53186 T:13c0 "run() exit" 1 [http.cpp:12]
2018-05-03 14:57:59.456078-0500 I Misc  P:53186 T:13c0 "" a midly interesting value:479 [child/stuff.cpp:9]
2018-05-03 14:57:59.456088-0500 D Main  P:53186 T:13c0 "exit" 1 [main.cpp:41]

$ ./build/demod V,Misc:I,Http:W
2018-05-03 14:58:08.551095-0500 V Main  P:53187 T:13c0 "Using arg as loglevels" V,Misc:I,Http:W [main.cpp:29]
2018-05-03 14:58:08.551908-0500 W Http  P:53187 T:13c0 "oops bad assumption" Error:33 [http.cpp:10]
2018-05-03 14:58:08.551927-0500 I Misc  P:53187 T:13c0 "" a midly interesting value:488 [child/stuff.cpp:9]
2018-05-03 14:58:08.551937-0500 D Main  P:53187 T:13c0 "exit" 1 [main.cpp:41]
```

## Configuration Errors
`CLog::setLevels()` will return an empty string on success, otherwise it returns an error string.  The demo application logs it at ERROR level under the 'Config' module id.

```
$ ./build/demod I,08:E,Http:W
2018-05-03 14:56:08.075450-0500 E Config  P:53180 T:13c0 "" CLog::setLevels() parse error at 2. Module Id not in map : '08' [main.cpp:32]

$ ./build/demod R,Misc:E,Http:W
2018-05-03 14:52:11.531879-0500 E Config  P:53091 T:13c0 "" CLog::setLevels() parse error at 0. Invalid Level Char : 'R' [main.cpp:32]

$ ./build/demod FooBar:D
2018-05-03 15:02:01.335040-0500 E Config  P:53194 T:13c0 "" CLog::setLevels() parse error at 0. Module Id not in map : 'FooBar' [main.cpp:32]
```
