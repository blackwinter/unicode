#! /usr/local/bin/ruby -KU

if $KCODE != 'UTF8'
  raise "$KCODE must be UTF8"
end

HEAD=<<EOS
/*
 * UnicodeData
 * 1999 by yoshidam
 *
 */

#ifndef _UNIDATA_MAP
#define _UNIDATA_MAP

struct unicode_data {
  const int code;
  const int combining_class;
  const int exclusion;
  const char* const canon;
  const char* const compat;
  const int uppercase;
  const int lowercase;
  const int titlecase;
};

const static struct unicode_data unidata[] = {
EOS

TAIL=<<EOS
};

#endif
EOS

def hex2str(hex)
  if hex.nil? || hex == ''
    return [nil, nil]
  end
  canon = ""
  compat = ""
  chars = hex.split(" ")
  if chars[0] =~ /^[0-9A-F]{4}$/
    chars.each do |c|
      canon << [c.hex].pack("U")
    end
    compat = canon
  elsif chars[0] =~ /^<.+>$/
    chars.shift
    chars.each do |c|
      compat << [c.hex].pack("U")
    end
    canon = nil
  else
    raise "unknown value: " + hex
  end
  [canon, compat]
end

def hex_or_nil(str)
  return "-1" if str.nil?
  return format("0x%04x", str.hex)
end

def printstr(str)
  return "NULL" if !str
  ret = ""
  str.each_byte do |c|
    if c >= 32 && c < 127 && c != 34 && c != 92
      ret << c
    else
      ret << format("\\%03o", c)
    end
  end
  return '"' + ret + '"'
end

## scan Composition Exclusions
exclusion = {}
open(ARGV[1]) do |f|
  while l = f.gets
    next if l =~ /^\#/ || l =~ /^$/
    code, = l.split(/\s/)
    code = code.hex
    exclusion[code] = true
  end
end

## scan UnicodeData
udata = {}
open(ARGV[0]) do |f|
  while l = f.gets
    l.chomp!
    code, charname, gencat, ccclass, bidicat,decomp,
      dec, digit, num, mirror, uni1_0, comment, upcase,
      lowcase, titlecase = l.split(";");
    code = code.hex
    ccclass = ccclass.to_i
    canon, compat = hex2str(decomp)
    upcase = hex_or_nil(upcase)
    lowcase = hex_or_nil(lowcase)
    titlecase = hex_or_nil(titlecase)
    udata[code] = [ccclass, canon, compat, upcase, lowcase, titlecase]
  end
end

print HEAD
udata.sort.each do |code, data|
  ccclass, canon, compat, upcase, lowcase, titlecase = data
  ## Exclusions
  ex = 0
  if exclusion[code]  ## Script-specifics or Post Composition Version
    ex = 1
  elsif canon =~ /^.$/ ## Singltons
    ex = 2
  elsif !canon.nil?
    starter = canon.unpack("U*")[0]
    if udata[starter][0] != 0 ## Non-stater decompositions
      ex = 3
    end
  end
  printf("  { 0x%04x, %d, %d, %s, %s, %s, %s, %s }, \n",
         code, ccclass, ex, printstr(canon),
         printstr(compat), upcase, lowcase, titlecase)
end
printf("  { -1, 0, 0, NULL, NULL, -1, -1, -1 }\n")
print TAIL
