require "rake/clean"
require "rake/extensiontask"
require "rubygems/package_task"

CLEAN << "pkg" << "tmp" << "lib/unicode"

gem_spec = eval(File.read(File.expand_path("../unicode.gemspec", __FILE__)))

Rake::GemPackageTask.new(gem_spec) {|pkg|}

Rake::ExtensionTask.new('unicode_native', gem_spec) do |ext|
  ext.cross_compile = true
  ext.cross_platform = ['x86-mingw32', 'x86-mswin32-60']
  ext.ext_dir = "ext/unicode"
  ext.lib_dir = "lib/unicode"
end

desc "Build native gems for Windows"
task :windows_gem => :clean do
  ENV["RUBY_CC_VERSION"] = "1.8.6:1.9.1"
  sh "rake cross compile"
  sh "rake cross native gem"
end
