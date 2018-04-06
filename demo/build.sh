CCFLAGS="-I../include"

# Uncomment this to compile-in a default level different than CLL_WARN
#CCFLAGS+=" -DCLL_DEFAULT=CLL_DEBUG"

mkdir -p ./build
cc -O2 -DNDEBUG ${CCFLAGS} main.cpp http.cpp child/stuff.cpp -o build/demor -lstdc++ \
  && cc -g -D_DEBUG ${CCFLAGS} main.cpp http.cpp child/stuff.cpp -o build/demod -lstdc++

ruby ../scripts/clog_extract_lines.rb > build/strings.csv
ruby ../scripts/clog_build_rehydrate.rb build/strings.csv modules.h > build/rehydrate.rb
