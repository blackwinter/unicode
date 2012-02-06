# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{unicode}
  s.version = "0.4.1"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = [%q{Yoshida Masato}]
  s.date = %q{2011-02-03}
  s.email = %q{yoshidam@yoshidam.net}
  s.extensions = [%q{ext/unicode/extconf.rb}]
  s.extra_rdoc_files = [%q{README}]
  s.files = `git ls-files`.split("\n").reject {|f| f =~ /^\./}
  s.homepage = %q{http://www.yoshidam.net/Ruby.html#unicode}
  s.require_paths = [%q{lib}]
  s.rubygems_version = %q{1.8.6}
  s.summary = %q{Unicode normalization library.}
  s.description = %q{Unicode normalization library.}

  if s.respond_to? :specification_version then
    s.specification_version = 3
  end
end

