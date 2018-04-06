# Copyright 2018-present Alex Malone, Ziften Technologies, Inc. All Rights Reserved.
# Apache-2 license.

f = STDOUT
f.puts "#!/usr/bin/ruby"
f.puts  "STRINGS_FILE_CONTENTS=<<-EOS321"
f.puts File.read(ARGV[0])
f.puts "EOS321"
f.puts
f.puts "MODULES_FILE_CONTENTS=<<-EOS321"
f.puts File.read(ARGV[1])
f.puts "EOS321"

script_path = File.dirname(__FILE__)
f.puts File.read(File.join(script_path, 'clog_deobfuscate.rb'))
