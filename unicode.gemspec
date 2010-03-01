Gem::Specification.new { |s|
  s.name             = %q{unicode}
  s.version          = %q{0.2.0}
  s.date             = %q{2009-12-29}
  s.summary          = %q{Unicode normalization library.}
  s.require_paths    = %q{.}
  s.author           = %q{Yoshida Masato}
  s.email            = %q{yoshidam@yoshidam.net}
  s.homepage         = %q{http://www.yoshidam.net/Ruby.html#unicode}
  s.files            = %w[
    extconf.rb unicode.c ustring.c ustring.h wstring.c wstring.h
    README test.rb tools/mkunidata.rb tools/README unidata.map
  ]
  s.extra_rdoc_files = %w[README]
  s.extensions       = %w[extconf.rb]
}
