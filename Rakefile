require "rake/clean"
require "rake/extensiontask"
require "rubygems/package_task"

CLEAN << "pkg" << "tmp"

gem_spec = eval(File.read(File.expand_path("../unicode.gemspec", __FILE__)))

Rake::GemPackageTask.new(gem_spec) {|pkg|}

Rake::ExtensionTask.new('unicode_native', gem_spec) do |ext|
  ext.cross_compile = true
  ext.cross_platform = ['x86-mingw32', 'x86-mswin32-60']
  ext.ext_dir = "ext/unicode"
  ext.lib_dir = "lib/unicode"
end
