begin
  require "unicode/#{RUBY_VERSION[/\d+.\d+/]}/unicode_native"
rescue LoadError => err
  raise if err.respond_to?(:path) && !err.path
  require 'unicode/unicode_native'
end
