require "rake/clean"
require "rake/extensiontask"
require "rubygems/package_task"

CLEAN << "pkg" << "tmp" << "lib/unicode"

UPSTREAM_URL = 'http://www.yoshidam.net/unicode-%s.tar.gz'

gem_spec = eval(File.read(File.expand_path("../unicode.gemspec", __FILE__)))

Gem::PackageTask.new(gem_spec) {|pkg|}

Rake::ExtensionTask.new('unicode_native', gem_spec) do |ext|
  ext.cross_compile = true
  ext.cross_platform = ['x86-mingw32', 'x86-mswin32-60']
  ext.ext_dir = "ext/unicode"
  ext.lib_dir = "lib/unicode"
end

desc "Build native gems for Windows"
task :windows_gem => :clean do
  ENV["RUBY_CC_VERSION"] = "1.8.7:1.9.3"
  sh "rake cross compile"
  sh "rake cross native gem"
end

desc "Update from upstream"
task :update, [:version] do |t, args|
  require 'zlib'
  require 'open-uri'
  require 'archive/tar/minitar'

  unless version = args.version || ENV['UPSTREAM_VERSION']
    abort "Please specify UPSTREAM_VERSION. See #{gem_spec.homepage}."
  end

  io = begin
    open(url = UPSTREAM_URL % version)
  rescue OpenURI::HTTPError
    abort "Upstream version not found: #{url}. See #{gem_spec.homepage}."
  end

  Archive::Tar::Minitar.open(Zlib::GzipReader.new(io)) { |tar|
    basedir = File.expand_path('..', __FILE__)

    extract = lambda { |entry, name, dir|
      puts "Extracting `#{name}' to `#{dir || '.'}'..."
      tar.extract_entry(dir ? File.join(basedir, dir) : basedir, entry)
    }

    tar.each { |entry|
      entry.name.sub!(/\Aunicode\//, '')

      case name = entry.full_name
        when /\Atools\/|\.gemspec\z/, 'README'
          extract[entry, name, nil]
        when /\.(?:[ch]|map)\z/, 'extconf.rb'
          extract[entry, name, 'ext/unicode']
        when /\Atest/
          extract[entry, name, 'test']
        else
          puts "Skipping `#{name}'..."
      end
    }
  }
end
