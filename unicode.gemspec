Gem::Specification.new { |s|
  s.name             = %q{unicode}
  s.version          = %q{0.3.1}
  s.date             = %q{2010-02-26}
  s.summary          = %q{Unicode normalization library.}
  s.require_paths    = %w[lib]
  s.author           = %q{Yoshida Masato}
  s.email            = %q{yoshidam@yoshidam.net}
  s.homepage         = %q{http://www.yoshidam.net/Ruby.html#unicode}
  s.files            = %w[
    ext/unicode/extconf.rb ext/unicode/unicode.c ext/unicode/ustring.c
    ext/unicode/ustring.h ext/unicode/wstring.c ext/unicode/wstring.h README
    test.rb tools/mkunidata.rb tools/README ext/unicode/unidata.map
    lib/unicode.rb
  ]
  s.extra_rdoc_files = %w[README]
  s.extensions       = %w[ext/unicode/extconf.rb]
}
