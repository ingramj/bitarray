# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{bitarray}
  s.version = "0.1.1"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["James E. Ingram"]
  s.date = %q{2009-05-23}
  s.description = %q{A bit array class for Ruby, implemented as a C extension. Includes methods for setting and clearing individual bits, and all bits at once. Also has the standard array access methods, [] and []=, and it mixes in Enumerable.}
  s.email = %q{ingramj@gmail.com}
  s.extensions = ["ext/extconf.rb"]
  s.extra_rdoc_files = [
    "LICENSE",
     "README"
  ]
  s.files = [
    ".gitignore",
     "LICENSE",
     "README",
     "Rakefile",
     "TODO",
     "VERSION",
     "bitarray.gemspec",
     "ext/bitarray.c",
     "ext/extconf.rb",
     "test/bitfield.rb",
     "test/bm.rb",
     "test/test.rb"
  ]
  s.has_rdoc = true
  s.homepage = %q{http://github.com/ingramj/bitarray}
  s.rdoc_options = ["--charset=UTF-8", "--exclude", "ext/Makefile", "--title", " BitArray Documentation"]
  s.require_paths = ["ext"]
  s.required_ruby_version = Gem::Requirement.new(">= 1.9.1")
  s.rubygems_version = %q{1.3.1}
  s.summary = %q{A bitarray class for Ruby, implemented as a C extension.}
  s.test_files = [
    "test/bitfield.rb",
     "test/test.rb",
     "test/bm.rb"
  ]

  if s.respond_to? :specification_version then
    current_version = Gem::Specification::CURRENT_SPECIFICATION_VERSION
    s.specification_version = 2

    if Gem::Version.new(Gem::RubyGemsVersion) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
