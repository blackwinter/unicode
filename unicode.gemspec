Gem::Specification.new { |s|
  s.name             = %q{unicode}
  s.version          = %q{0.4.0}
  s.date             = %q{2010-10-14}
  s.summary          = %q{Unicode normalization library.}
  s.require_paths    = %w[lib]
  s.author           = %q{Yoshida Masato}
  s.email            = %q{yoshidam@yoshidam.net}
  s.homepage         = %q{http://www.yoshidam.net/Ruby.html#unicode}
  s.files            = `git ls-files`.split("\n").reject {|f| f =~ /^\./}
  s.extra_rdoc_files = %w[README]
  s.extensions       = %w[ext/unicode/extconf.rb]
}
