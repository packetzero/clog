# build-deobs-ruby.sh will prepend data/embedded_strings.rb with this file
require "csv"

class LogStringParser

  def lookupLogSym key, levelChar
    @logSyms[key]
  end

  def initialize fileContents
    @logSyms={}
    @fileIds={}

    CSV.parse(fileContents) do | row |
      next if row.size == 0
      next if row[0].start_with?"==="

      case (row[0])
      when "L"
        fid=row[1].gsub(/^0x/,"")
        linenum=row[2]
        key = "#{fid}:#{linenum}"
        @logSyms[key] = row
      when "F"
        @fileIds[row[1]] = row
      else
        # ?
      end

#      puts "#{row.inspect}"
    end
  end
end

class ModulesParser
  def initialize lines
    @idToNameMap={}
    lines.each do |line|
      next unless line.include?('=')
      sym,tmp = line.chomp.split('=',2)
      id,tmp = tmp.split(',',2)
      @idToNameMap[id.strip] = sym.strip
    end
  end

  # idstr may be a zero-prefixed string like "02", so try to convert
  # to int to remove prefix, then cast back to string
  def lookupById idstr
    id = idstr.to_i.to_s rescue idstr
    @idToNameMap[id]
  end
end

class RawLogParser
  def initialize modules, strings
    @modules = modules
    @strings = strings
  end

  # in: 15045599201964060
  # out: 2017-09-04 16:18:40.205466-0500
  def format_timestamp tsnanos
    tsec = tsnanos[0,10]  # 1504559920 1964060
    subsec = tsnanos[10,6]

    now = DateTime.now
    tz = now.strftime("%z")
    d = DateTime.strptime(tsec, "%s")

    d = d.new_offset(tz)

    fmt = d.strftime("%Y-%m-%d %H:%M:%S")

    #"#{tsec}.#{subsec}"
    "#{fmt}.#{subsec}#{tz}"
  end

  def run infile
    infile.each do |line|
      if !line.start_with? "15"
        puts line # old format line
        next
      end

      ts,levelChar,moduleId,key,pid,tid,msg = line.chomp.split(' ',7)

      mod = @modules.lookupById moduleId rescue moduleId
      extra = ""
      sym = key

      tmp = @strings.lookupLogSym key,levelChar
      unless tmp.nil?
        sym = tmp[4]
        filename=File.basename(tmp[5]) rescue tmp[5]
        fileId,lineNum = key.split(':')
        extra = "[#{filename}:#{lineNum}]"
      end

      puts "#{format_timestamp(ts)} #{levelChar} #{mod} #{pid} #{tid} #{sym} #{msg} #{extra}"
    end
  end
end


if ARGV.size == 1 && ARGV[0] == "--help"
  puts "USAGE: #{__FILE__} logfile stringsfile modules.h"
  exit 2
end

if ARGV.size > 0
  logFile = File.new(ARGV[0])
else
  logFile = STDIN
end

if (ARGV.size <= 1)

  # embedded
  stringsFileContent = STRINGS_FILE_CONTENTS
  modulesFileLines = MODULES_FILE_CONTENTS.lines

else

  stringsFileContent = File.new(ARGV[1]).read
  modulesFileLines = File.new(ARGV[2]).read.lines
end

modules = ModulesParser.new modulesFileLines #{"10" => "SomeModuleName"}

strings = LogStringParser.new stringsFileContent
logParser = RawLogParser.new modules, strings

logParser.run logFile
