#!/usr/bin/ruby

require "date"
require 'find'

class SourceGrepParser

  def initialize
    @fileIdToNameMap={}
    @fileNameToIdMap={}
    @currentFileId=nil
    @ztracelabels=[]
    @zlogsyms=[]
  end

  # #define LOCAL_CLOG_FILEID 0x1b79d7299       // this needs to be unique across source tree
  def handle_fileid_line filename, linenum, sourceline
    a,b = sourceline.split("LOCAL_CLOG_FILEID",2)
    return if b.nil?
    id,therest = b.strip.split(' ')

    @fileIdToNameMap[id] = filename
    @fileNameToIdMap[filename] = id
    @currentFileId = id

#    puts "F,#{id},#{filename}"
  end

  # normally first character in INFO,ERR,DBG,WARN,TRACE,VERBOSE,etc.
  def get_level_char(lvl)
    return lvl[0];
  end

  def handle_log_line filename, linenum, sourceline
    pre,content = sourceline.split("CLOG_", 2)
    if content.nil?
      puts "ERROR PARSING LiNE #{sourceline} #{__LINE__}"
      return
    end
    # TODO: check content, etc.
    a,sym,therest = content.split(",", 3)

    # ignore false positives such as constants that start with CLOG_
    return if a.start_with?('ALL_MOD')

    levelChar = get_level_char(a)
    return if sym.nil?
    sym = sym.strip
    return if sym.length <= 1   # signifies 'no symbol'

    fileid = @currentFileId
    @zlogsyms.push "L,#{fileid},#{linenum},#{levelChar},\"#{sym.strip}\",#{filename}"
  end

  def dump io
    io.puts "=== files"
    @fileIdToNameMap.each do |id,name|
      io.puts "F,#{id},#{name}"
    end
    io.puts

    io.puts "=== Log line symbols"
    @zlogsyms.each do |entry|
      io.puts entry
    end
  end
end

parser = SourceGrepParser.new


# reads each line of file and if a line matches search_string,
# will add an entry to results[]
#
# @param file_path string path to file
# @param search_string string or regex to find in line
# @param results array destination to add output
# @return void
#
# Example result line:
# ./src/blah/somefile.cpp:223:   CLOG_WARN(ModX,"","Some message=%s", s.c_str() );
#
def find_matching_lines file_path, search_string, results
  #puts file_path
  begin
    f = File.new file_path
    i=0
    f.each do |line|
      i += 1
      unless line.index(search_string).nil?
        # match
        results.push "#{file_path}:#{i}: #{line}"
      end
    end
  rescue
    # ignore
  end
end

# Find files in repo_path that end in any of suffixes in extensions[]
# Calls find_matching_lines(search_string) for each file
# @return array of strings - 'grep -n' format
def find_grep(repo_path, extensions, search_string)
  results = []
  Find.find(repo_path) do |path|
    next if FileTest.directory?(path)
    extensions.each do |ext|
      if (path.downcase.end_with?(ext))
        find_matching_lines path, search_string, results
#        puts path
      end
    end
  end
  return results
end

rawresults = find_grep(".", [".cpp",".cc", ".mm"], "CLOG_" )
#puts rawresults.inspect

  rawresults.each do |line|

    next if line.include? "/clog_/"

    line.force_encoding(Encoding::ISO_8859_1) # get rid of some binary chars that will cause exception

    filename,linenum,sourceline = line.split(':',3)
    next if sourceline.nil?

    filename = filename.strip.gsub(/^\.\//,"")

    if sourceline.include? "LOCAL_CLOG_FILEID"
      parser.handle_fileid_line filename, linenum, sourceline
    else
      parser.handle_log_line filename, linenum, sourceline
    end
  end

parser.dump STDOUT
