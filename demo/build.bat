mkdir build
pushd build
set FILES=..\main.cpp ..\http.cpp ..\child\*.cpp
set CCFLAGS=/I..\..\include

cl %CCFLAGS% /O2 /DNDEBUG=1 %FILES% & move main.exe demor.exe

cl %CCFLAGS% /MTd /D_DEBUG %FILES% & move main.exe demod.exe

popd
ruby ..\scripts\clog_extract_lines.rb > build\strings.csv
ruby ..\scripts\clog_build_rehydrate.rb build\strings.csv modules.h > build\rehydrate.rb
