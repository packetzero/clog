mkdir -p ./build
cc -O2 -DNDEBUG -I../include main.cpp http.cpp -o build/demor -lstdc++
cc -g -D_DEBUG -I../include main.cpp http.cpp -o build/demod -lstdc++

ruby ../scripts/clog_extract_lines.rb > build/strings.csv
ruby ../scripts/clog_build_rehydrate.rb build/strings.csv modules.h > build/rehydrate.rb
