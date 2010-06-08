require "rake/clean"
require "rake/extensiontask"
require "rubygems/package_task"

CLEAN << "pkg" << "tmp"

gem_spec = eval(File.read(File.expand_path("../unicode.gemspec", __FILE__)))

Rake::GemPackageTask.new(gem_spec) {|pkg|}

Rake::ExtensionTask.new('unicode', gem_spec) do |ext|
  ext.cross_compile = true
  ext.cross_platform = ['i386-mswin32', 'i386-mswin32-60']
  ext.ext_dir = "."
end
