begin
  RUBY_VERSION =~ /(\d+.\d+)/
  require "unicode/#{$1}/unicode_native"
rescue LoadError
  require 'unicode/unicode_native'
end
