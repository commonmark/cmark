#!/usr/bin/env ruby
require 'ffi'

module CMark
  extend FFI::Library

  class Mem < FFI::Struct
    layout :calloc, :pointer,
           :realloc, :pointer,
           :free, callback([:pointer], :void)
  end

  ffi_lib ['libcmark', 'cmark']
  attach_function :cmark_get_default_mem_allocator, [], :pointer
  attach_function :cmark_markdown_to_html, [:string, :size_t, :int], :pointer
end

def markdown_to_html(s)
  len = s.bytesize
  cstring = CMark::cmark_markdown_to_html(s, len, 0)
  result = cstring.get_string(0)
  mem = CMark::cmark_get_default_mem_allocator
  CMark::Mem.new(mem)[:free].call(cstring)
  result
end

STDOUT.write(markdown_to_html(ARGF.read()))
