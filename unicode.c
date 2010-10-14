/*
 * Unicode Library version 0.4
 * Oct 14, 2010: version 0.4
 * Feb 26, 2010: version 0.3
 * Dec 29, 2009: version 0.2
 * Nov 23, 1999 yoshidam
 *
 */

#include "ruby.h"
#ifdef HAVE_RUBY_IO_H
#  include "ruby/io.h"
#else
#  include "rubyio.h"
#endif
#include <stdio.h>
#include "wstring.h"
#include "unidata.map"

#ifndef RSTRING_PTR
#  define RSTRING_PTR(s) (RSTRING(s)->ptr)
#  define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

#ifdef HAVE_RUBY_ENCODING_H
static rb_encoding* enc_out;
#  define ENC_(o) (rb_enc_associate(o, enc_out))
#else
#  define ENC_(o) (o)
#endif

inline static VALUE
taintObject(VALUE src, VALUE obj) {
  if (OBJ_TAINTED(src))
    OBJ_TAINT(obj);
  return obj;
}
#define TO_(src, obj) (taintObject(src, obj))

#ifdef HAVE_RUBY_ENCODING_H
#  define CONVERT_TO_UTF8(str) do { \
    int encindex = ENCODING_GET(str); \
    volatile VALUE encobj; \
    if (encindex != rb_utf8_encindex() && \
        encindex != rb_usascii_encindex()) { \
      encobj = rb_enc_from_encoding(enc_out); \
      str = rb_str_encode(str, encobj, 0, Qnil); \
    } \
  } while (0)
#endif

static VALUE mUnicode;
static VALUE unicode_data;
static VALUE composition_table;

/* Hangul */
#define SBASE   (0xac00)
#define LBASE   (0x1100)
#define LCOUNT  (19)
#define VBASE   (0x1161)
#define VCOUNT  (21)
#define TBASE   (0x11a7)
#define TCOUNT  (28)
#define NCOUNT  (VCOUNT * TCOUNT) /* 588 */
#define SCOUNT  (LCOUNT * NCOUNT) /* 11172 */

static int
get_cc(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    return unidata[FIX2INT(ch)].combining_class;
  }
  return 0;
}

static const char*
get_canon(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    return unidata[FIX2INT(ch)].canon;
  }
  return NULL;
}

static const char*
get_compat(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    return unidata[FIX2INT(ch)].compat;
  }
  return NULL;
}

static const char*
get_uppercase(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    return unidata[FIX2INT(ch)].uppercase;
  }
  return NULL;
}

static const char*
get_lowercase(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    return unidata[FIX2INT(ch)].lowercase;
  }
  return NULL;
}

static const char*
get_titlecase(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    return unidata[FIX2INT(ch)].titlecase;
  }
  return NULL;
}

static int
get_composition(const char* str)
{
  VALUE ch = rb_hash_aref(composition_table, rb_str_new2(str));

  if (!NIL_P(ch)) {
    return FIX2INT(ch);
  }
  return -1;
}

static WString*
sort_canonical(WString* ustr)
{
  int i = 1;
  int len = ustr->len;

  if (len < 2) return ustr;

  while (i < len) {
    int last = ustr->str[i - 1];
    int ch = ustr->str[i];
    int last_cc = get_cc(last);
    int cc = get_cc(ch);
    if (cc != 0 && last_cc != 0 && last_cc > cc) {
      ustr->str[i] = last;
      ustr->str[i-1] = ch;
      if (i > 1) i--;
    }
    else {
      i++;
    }
  }
  return ustr;
}

static void
decompose_hangul(int ucs, int* l, int* v, int* t)
{
  int sindex = ucs - SBASE;
  if (sindex < 0 || sindex >= SCOUNT) {
    *l = ucs;
    *v = *t = 0;
    return;
  }
  *l = LBASE + sindex / NCOUNT;
  *v = VBASE + (sindex % NCOUNT) / TCOUNT;
  *t = TBASE + sindex % TCOUNT;
  if (*t == TBASE) *t = 0;
}

/*
 * push decomposed str into result 
 */
static WString*
decompose_internal(WString* ustr, WString* result)
{
  int i;
  int len = ustr->len;

  for (i = 0; i < len; i++) {
    int ucs = ustr->str[i];
    if (ucs >= SBASE && ucs < SBASE + SCOUNT) {
      int l, v, t;
      decompose_hangul(ucs, &l, &v, &t);
      WStr_addWChar(result, l);
      if (v) WStr_addWChar(result, v);
      if (t) WStr_addWChar(result, t);
    }
    else {
      const char* dc = get_canon(ucs);
      if (!dc) {
	WStr_addWChar(result, ucs);
      }
      else {
	WString wdc;
	WStr_allocWithUTF8(&wdc, dc);
	decompose_internal(&wdc, result);
	WStr_free(&wdc);
      }
    }
  }
  return result;
}

/*
 * push compatibility decomposed str into result 
 */
static WString*
decompose_compat_internal(WString* ustr, WString* result)
{
  int i;
  int len = ustr->len;

  for (i = 0; i < len; i++) {
    int ucs = ustr->str[i];
    if (ucs >= SBASE && ucs < SBASE + SCOUNT) {
      int l, v, t;
      decompose_hangul(ucs, &l, &v, &t);
      WStr_addWChar(result, l);
      if (v) WStr_addWChar(result, v);
      if (t) WStr_addWChar(result, t);
    }
    else {
      const char* dc = get_compat(ucs);
      if (!dc) {
	WStr_addWChar(result, ucs);
      }
      else {
	WString wdc;
	WStr_allocWithUTF8(&wdc, dc);
	decompose_compat_internal(&wdc, result);
	WStr_free(&wdc);
      }
    }
  }
  return result;
}


#define UCS4toUTF8(p, c) \
  do { \
    if (c < 128) { \
      *p++ = c; \
    } \
    else if (c < 2048) { \
      *p++ = (c >> 6) | 192; \
      *p++ = (c & 63) | 128; \
    } \
    else if (c < 0x10000) { \
      *p++ = (c >> 12) | 224; \
      *p++ = ((c >> 6) & 63) | 128; \
      *p++ = (c & 63) | 128; \
    } \
    else if (c < 0x200000) { \
      *p++ = (c >> 18) | 240; \
      *p++ = ((c >> 12) & 63) | 128; \
      *p++ = ((c >> 6) & 63) | 128; \
      *p++ = (c & 63) | 128; \
    } \
    else if (c < 0x4000000) { \
      *p++ = (c >> 24) | 248; \
      *p++ = ((c >> 18) & 63) | 128; \
      *p++ = ((c >> 12) & 63) | 128; \
      *p++ = ((c >> 6) & 63) | 128; \
      *p++ = (c & 63) | 128; \
    } \
    else if (c < 0x80000000) { \
      *p++ = (c >> 30) | 252; \
      *p++ = ((c >> 24) & 63) | 128; \
      *p++ = ((c >> 18) & 63) | 128; \
      *p++ = ((c >> 12) & 63) | 128; \
      *p++ = ((c >> 6) & 63) | 128; \
      *p++ = (c & 63) | 128; \
    } \
  } while (0)

static int
compose_pair(unsigned int c1, unsigned int c2)
{
  int ret;
  char ustr[13]; /* stored two UTF-8 chars */
  char *p = ustr;

  /* Hangul L + V */
  if (c1 >= LBASE && c1 < LBASE + LCOUNT &&
      c2 >= VBASE && c2 < VBASE + VCOUNT) {
    return SBASE + ((c1 - LBASE) * VCOUNT + (c2 - VBASE)) * TCOUNT;
  }
  /* Hangul LV + T */
  else if (c1 >= SBASE && c1 < SBASE + SCOUNT &&
	   (c1 - SBASE) % TCOUNT == 0 &&
	   c2 >= TBASE && c2 < TBASE + TCOUNT) {
    return c1 + (c2 - TBASE);
  }
  UCS4toUTF8(p, c1);
  UCS4toUTF8(p, c2);
  *p = '\0';
  ret = get_composition(ustr);

  return ret;
}

/*
 * push canonical composed str into result 
 */
static WString*
compose_internal(WString* ustr, WString* result)
{
  int starterPos = 0;
  int starterCh = ustr->str[0];
  int compPos = 1;
  int lastClass = get_cc(starterCh);
  int oldLen = ustr->len;
  int decompPos;

  if (oldLen == 0) return result;
  if (lastClass != 0) lastClass = 256;
  /* copy string */
  result->len = 0;
  WStr_pushWString(result, ustr);

  for (decompPos = compPos; decompPos < result->len; decompPos++) {
    int ch = result->str[decompPos];
    int chClass = get_cc(ch);
    int composite = compose_pair(starterCh, ch);
    if (composite > 0 && 
        (lastClass < chClass ||lastClass == 0)) {
      result->str[starterPos] = composite;
      starterCh = composite;
    }
    else {
      if (chClass == 0) {
        starterPos = compPos;
        starterCh = ch;
      }
      lastClass = chClass;
      result->str[compPos] = ch;
      if (result->len != oldLen) {
        decompPos += result->len - oldLen;
        oldLen = result->len;
      }
      compPos++;
    }
  }
  result->len = compPos;
  return result;
}
#if 0
static WString*
compose_internal(WString* ustr, WString* result)
{
  int len = ustr->len;
  int starter;
  int startercc;
  int i;

  if (len == 0) return result;

  starter = ustr->str[0];
  startercc = get_cc(starter);
  if (startercc != 0) startercc = 256;
  for (i = 1; i < len; i++) {
    int ch = ustr->str[i];
    int cc = get_cc(ch);
    int composite;

    if (startercc == 0 &&
	(composite = compose_pair(starter, ch)) >= 0) {
      starter = composite;
      startercc = get_cc(composite);
    }
    else {
      WStr_addWChar(result, starter);
      starter = ch;
      startercc = cc;
    }
  }
  WStr_addWChar(result, starter);

  return result;
}
#endif

static WString*
upcase_internal(WString* str, WString* result)
{
  int i;
  int len = str->len;

  for (i = 0; i < len; i++) {
    int ucs = str->str[i];
    const char* c = get_uppercase(ucs);
    if (!c) {
      WStr_addWChar(result, ucs);
    }
    else {
      WString wc;
      WStr_allocWithUTF8(&wc, c);
      WStr_pushWString(result, &wc);
      WStr_free(&wc);
    }
  }
  return result;
}

static WString*
downcase_internal(WString* str, WString* result)
{
  int i;
  int len = str->len;

  for (i = 0; i < len; i++) {
    int ucs = str->str[i];
    const char* c = get_lowercase(ucs);
    if (!c) {
      WStr_addWChar(result, ucs);
    }
    else {
      WString wc;
      WStr_allocWithUTF8(&wc, c);
      WStr_pushWString(result, &wc);
      WStr_free(&wc);
    }
  }
  return result;
}

static WString*
capitalize_internal(WString* str,  WString* result)
{
  int i;
  int len = str->len;

  if (len > 0) {
    const char* c = get_titlecase(str->str[0]);
    if (!c) {
      WStr_addWChar(result, str->str[0]);
    }
    else {
      WString wc;
      WStr_allocWithUTF8(&wc, c);
      WStr_pushWString(result, &wc);
      WStr_free(&wc);
    }
  }
  for (i = 1; i < len; i++) {
    int ucs = str->str[i];
    const char* c = get_lowercase(ucs);
    if (!c) {
      WStr_addWChar(result, ucs);
    }
    else {
      WString wc;
      WStr_allocWithUTF8(&wc, c);
      WStr_pushWString(result, &wc);
      WStr_free(&wc);
    }
  }
  return result;
}

static VALUE
unicode_strcmp(VALUE obj, VALUE str1, VALUE str2)
{
  WString wstr1;
  WString wstr2;
  WString result1;
  WString result2;
  UString ustr1;
  UString ustr2;
  int ret;

  Check_Type(str1, T_STRING);
  Check_Type(str2, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str1);
  CONVERT_TO_UTF8(str2);
#endif
  WStr_allocWithUTF8(&wstr1, RSTRING_PTR(str1));
  WStr_allocWithUTF8(&wstr2, RSTRING_PTR(str2));
  WStr_alloc(&result1);
  WStr_alloc(&result2);
  decompose_internal(&wstr1, &result1);
  decompose_internal(&wstr2, &result2);
  WStr_free(&wstr1);
  WStr_free(&wstr2);
  sort_canonical(&result1);
  sort_canonical(&result2);
  UniStr_alloc(&ustr1);
  UniStr_alloc(&ustr2);
  WStr_convertIntoUString(&result1, &ustr1);
  WStr_convertIntoUString(&result2, &ustr2);
  WStr_free(&result1);
  WStr_free(&result2);
  UniStr_addChar(&ustr1, '\0');
  UniStr_addChar(&ustr2, '\0');
  ret = strcmp((char*)ustr1.str, (char*)ustr2.str);
  UniStr_free(&ustr1);
  UniStr_free(&ustr2);

  return INT2FIX(ret);
}

static VALUE
unicode_strcmp_compat(VALUE obj, VALUE str1, VALUE str2)
{
  WString wstr1;
  WString wstr2;
  WString result1;
  WString result2;
  UString ustr1;
  UString ustr2;
  int ret;

  Check_Type(str1, T_STRING);
  Check_Type(str2, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str1);
  CONVERT_TO_UTF8(str2);
#endif
  WStr_allocWithUTF8(&wstr1, RSTRING_PTR(str1));
  WStr_allocWithUTF8(&wstr2, RSTRING_PTR(str2));
  WStr_alloc(&result1);
  WStr_alloc(&result2);
  decompose_compat_internal(&wstr1, &result1);
  decompose_compat_internal(&wstr2, &result2);
  WStr_free(&wstr1);
  WStr_free(&wstr2);
  sort_canonical(&result1);
  sort_canonical(&result2);
  UniStr_alloc(&ustr1);
  UniStr_alloc(&ustr2);
  WStr_convertIntoUString(&result1, &ustr1);
  WStr_convertIntoUString(&result2, &ustr2);
  WStr_free(&result1);
  WStr_free(&result2);
  UniStr_addChar(&ustr1, '\0');
  UniStr_addChar(&ustr2, '\0');
  ret = strcmp((char*)ustr1.str, (char*)ustr2.str);
  UniStr_free(&ustr1);
  UniStr_free(&ustr2);

  return INT2FIX(ret);
}

static VALUE
unicode_decompose(VALUE obj, VALUE str)
{
  WString ustr;
  WString result;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8(&ustr, RSTRING_PTR(str));
  WStr_alloc(&result);
  decompose_internal(&ustr, &result);
  WStr_free(&ustr);
  sort_canonical(&result);
  UniStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = TO_(str, ENC_(rb_str_new((char*)ret.str, ret.len)));
  UniStr_free(&ret);

  return vret;
}

static VALUE
unicode_decompose_compat(VALUE obj, VALUE str)
{
  WString ustr;
  WString result;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8(&ustr, RSTRING_PTR(str));
  WStr_alloc(&result);
  decompose_compat_internal(&ustr, &result);
  WStr_free(&ustr);
  sort_canonical(&result);
  UniStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = TO_(str, ENC_(rb_str_new((char*)ret.str, ret.len)));
  UniStr_free(&ret);

  return vret;
}

static VALUE
unicode_compose(VALUE obj, VALUE str)
{
  WString ustr;
  WString result;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8(&ustr, RSTRING_PTR(str));
  sort_canonical(&ustr);
  WStr_alloc(&result);
  compose_internal(&ustr, &result);
  WStr_free(&ustr);
  UniStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = TO_(str, ENC_(rb_str_new((char*)ret.str, ret.len)));
  UniStr_free(&ret);

  return vret;
}

static VALUE
unicode_normalize_C(VALUE obj, VALUE str)
{
  WString ustr1;
  WString ustr2;
  WString result;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8(&ustr1, RSTRING_PTR(str));
  WStr_alloc(&ustr2);
  decompose_internal(&ustr1, &ustr2);
  WStr_free(&ustr1);
  sort_canonical(&ustr2);
  WStr_alloc(&result);
  compose_internal(&ustr2, &result);
  WStr_free(&ustr2);
  UniStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = TO_(str, ENC_(rb_str_new((char*)ret.str, ret.len)));
  UniStr_free(&ret);

  return vret;
}

static VALUE
unicode_normalize_KC(VALUE obj, VALUE str)
{
  WString ustr1;
  WString ustr2;
  WString result;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8(&ustr1, RSTRING_PTR(str));
  WStr_alloc(&ustr2);
  decompose_compat_internal(&ustr1, &ustr2);
  WStr_free(&ustr1);
  sort_canonical(&ustr2);
  WStr_alloc(&result);
  compose_internal(&ustr2, &result);
  WStr_free(&ustr2);
  UniStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = TO_(str, ENC_(rb_str_new((char*)ret.str, ret.len)));
  UniStr_free(&ret);

  return vret;
}

static VALUE
unicode_upcase(VALUE obj, VALUE str)
{
  WString ustr;
  WString result;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8(&ustr, RSTRING_PTR(str));
  WStr_alloc(&result);
  upcase_internal(&ustr, &result);
  //sort_canonical(&result);
  WStr_free(&ustr);
  UniStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = TO_(str, ENC_(rb_str_new((char*)ret.str, ret.len)));
  UniStr_free(&ret);

  return vret;
}

static VALUE
unicode_downcase(VALUE obj, VALUE str)
{
  WString ustr;
  WString result;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8(&ustr, RSTRING_PTR(str));
  WStr_alloc(&result);
  downcase_internal(&ustr, &result);
  //sort_canonical(&result);
  WStr_free(&ustr);
  UniStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = TO_(str, ENC_(rb_str_new((char*)ret.str, ret.len)));
  UniStr_free(&ret);

  return vret;
}

#ifdef HAVE_RUBY_ENCODING_H


#endif

static VALUE
unicode_capitalize(VALUE obj, VALUE str)
{
  WString ustr;
  WString result;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8(&ustr, RSTRING_PTR(str));
  WStr_alloc(&result);
  capitalize_internal(&ustr, &result);
  //sort_canonical(&result);
  WStr_free(&ustr);
  UniStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = TO_(str, ENC_(rb_str_new((char*)ret.str, ret.len)));
  UniStr_free(&ret);

  return vret;
}

void
Init_unicode()
{
  int i;

#ifdef HAVE_RUBY_ENCODING_H
  enc_out = rb_utf8_encoding();
#endif

  mUnicode = rb_define_module("Unicode");
  unicode_data = rb_hash_new();
  composition_table = rb_hash_new();

  rb_global_variable(&unicode_data);
  rb_global_variable(&composition_table);

  for (i = 0; unidata[i].code != -1; i++) {
    int code = unidata[i].code;
    const char* canon = unidata[i].canon;
    int exclusion = unidata[i].exclusion;

    rb_hash_aset(unicode_data, INT2FIX(code), INT2FIX(i));
    if (canon && exclusion == 0) {
      rb_hash_aset(composition_table, rb_str_new2(canon), INT2FIX(code));
    }
  }

  rb_define_module_function(mUnicode, "strcmp",
			    unicode_strcmp, 2);
  rb_define_module_function(mUnicode, "strcmp_compat",
			    unicode_strcmp_compat, 2);

  rb_define_module_function(mUnicode, "decompose",
			    unicode_decompose, 1);
  rb_define_module_function(mUnicode, "decompose_compat",
			    unicode_decompose_compat, 1);
  rb_define_module_function(mUnicode, "compose",
			    unicode_compose, 1);

  rb_define_module_function(mUnicode, "normalize_D",
			    unicode_decompose, 1);
  rb_define_module_function(mUnicode, "normalize_KD",
			    unicode_decompose_compat, 1);
  rb_define_module_function(mUnicode, "normalize_C",
			    unicode_normalize_C, 1);
  rb_define_module_function(mUnicode, "normalize_KC",
			    unicode_normalize_KC, 1);

  /* aliases */
  rb_define_module_function(mUnicode, "nfd",
			    unicode_decompose, 1);
  rb_define_module_function(mUnicode, "nfkd",
			    unicode_decompose_compat, 1);
  rb_define_module_function(mUnicode, "nfc",
			    unicode_normalize_C, 1);
  rb_define_module_function(mUnicode, "nfkc",
			    unicode_normalize_KC, 1);

  rb_define_module_function(mUnicode, "upcase",
			    unicode_upcase, 1);
  rb_define_module_function(mUnicode, "downcase",
			    unicode_downcase, 1);
  rb_define_module_function(mUnicode, "capitalize",
			    unicode_capitalize, 1);
}
