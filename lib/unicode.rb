begin
  require "unicode/#{RUBY_VERSION[/\d+.\d+/]}/unicode_native"
rescue LoadError => err
  raise unless err.path
  require 'unicode/unicode_native'
end
