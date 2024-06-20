/*
 * Unicode Library version 0.4.3
 * Aug  8, 2012: version 0.4
 * Oct 14, 2010: version 0.4
 * Feb 26, 2010: version 0.3
 * Dec 29, 2009: version 0.2
 * Nov 23, 1999 yoshidam
 *
 */

#define UNICODE_VERSION "0.4.3"

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
  // deprecated: taintedness turned out to be a wrong idea
  //if (OBJ_TAINTED(src))
  //  OBJ_TAINT(obj);
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
static VALUE catname_long[c_Cn+1];
static VALUE catname_abbr[c_Cn+1];

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

VALUE
get_unidata(int ucs) {
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));
  if (!NIL_P(ch))
    return ch;
#ifdef CJK_IDEOGRAPH_EXTENSION_A_FIRST
  else if (ucs >= CJK_IDEOGRAPH_EXTENSION_A_FIRST &&
           ucs <= CJK_IDEOGRAPH_EXTENSION_A_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(CJK_IDEOGRAPH_EXTENSION_A_FIRST));
#endif
#ifdef CJK_IDEOGRAPH_FIRST
  else if (ucs >= CJK_IDEOGRAPH_FIRST &&
           ucs <= CJK_IDEOGRAPH_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(CJK_IDEOGRAPH_FIRST));
#endif
#ifdef HANGUL_SYLLABLE_FIRST
  else if (ucs >= HANGUL_SYLLABLE_FIRST &&
           ucs <= HANGUL_SYLLABLE_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(HANGUL_SYLLABLE_FIRST));
#endif
#ifdef NON_PRIVATE_USE_HIGH_SURROGATE_FIRST
  else if (ucs >= NON_PRIVATE_USE_HIGH_SURROGATE_FIRST &&
           ucs <= NON_PRIVATE_USE_HIGH_SURROGATE_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(NON_PRIVATE_USE_HIGH_SURROGATE_FIRST));
#endif
#ifdef PRIVATE_USE_HIGH_SURROGATE_FIRST
  else if (ucs >= PRIVATE_USE_HIGH_SURROGATE_FIRST &&
           ucs <= PRIVATE_USE_HIGH_SURROGATE_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(PRIVATE_USE_HIGH_SURROGATE_FIRST));
#endif
#ifdef LOW_SURROGATE_FIRST
  else if (ucs >= LOW_SURROGATE_FIRST &&
           ucs <= LOW_SURROGATE_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(LOW_SURROGATE_FIRST));
#endif
#ifdef PRIVATE_USE_FIRST
  else if (ucs >= PRIVATE_USE_FIRST &&
           ucs <= PRIVATE_USE_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(PRIVATE_USE_FIRST));
#endif
#ifdef CJK_IDEOGRAPH_EXTENSION_B_FIRST
  else if (ucs >= CJK_IDEOGRAPH_EXTENSION_B_FIRST &&
           ucs <= CJK_IDEOGRAPH_EXTENSION_B_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(CJK_IDEOGRAPH_EXTENSION_B_FIRST));
#endif
#ifdef CJK_IDEOGRAPH_EXTENSION_C_FIRST
  else if (ucs >= CJK_IDEOGRAPH_EXTENSION_C_FIRST &&
           ucs <= CJK_IDEOGRAPH_EXTENSION_C_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(CJK_IDEOGRAPH_EXTENSION_C_FIRST));
#endif
#ifdef CJK_IDEOGRAPH_EXTENSION_D_FIRST
  else if (ucs >= CJK_IDEOGRAPH_EXTENSION_D_FIRST &&
           ucs <= CJK_IDEOGRAPH_EXTENSION_D_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(CJK_IDEOGRAPH_EXTENSION_D_FIRST));
#endif
#ifdef PLANE_15_PRIVATE_USE_FIRST
  else if (ucs >= PLANE_15_PRIVATE_USE_FIRST &&
           ucs <= PLANE_15_PRIVATE_USE_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(PLANE_15_PRIVATE_USE_FIRST));
#endif
#ifdef PLANE_16_PRIVATE_USE_FIRST
  else if (ucs >= PLANE_16_PRIVATE_USE_FIRST &&
           ucs <= PLANE_16_PRIVATE_USE_LAST)
    return rb_hash_aref(unicode_data,
                        INT2FIX(PLANE_16_PRIVATE_USE_FIRST));
#endif
  return Qnil;
}

static int
get_cc(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    return unidata[FIX2INT(ch)].combining_class;
  }
  return 0;
}

static int
get_gencat(int ucs)
{
  VALUE ch = get_unidata(ucs);

  if (!NIL_P(ch)) {
    return unidata[FIX2INT(ch)].general_category;
  }
  return c_Cn; /* Unassigned */
}

static int
get_eawidth(int ucs)
{
  VALUE ch = get_unidata(ucs);

  if (!NIL_P(ch)) {
    return unidata[FIX2INT(ch)].east_asian_width;
  }
  return w_N; /* Neutral */
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
get_canon_ex(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    int i = FIX2INT(ch);
    if (!unidata[i].exclusion)
      return unidata[i].canon;
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
 * push decomposed str into result 
 */
static WString*
decompose_safe_internal(WString* ustr, WString* result)
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
      const char* dc = get_canon_ex(ucs);
      if (!dc) {
	WStr_addWChar(result, ucs);
      }
      else {
	WString wdc;
	WStr_allocWithUTF8(&wdc, dc);
	decompose_safe_internal(&wdc, result);
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
  WStr_allocWithUTF8L(&wstr1, RSTRING_PTR(str1), RSTRING_LEN(str1));
  WStr_allocWithUTF8L(&wstr2, RSTRING_PTR(str2), RSTRING_LEN(str2));
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
  WStr_allocWithUTF8L(&wstr1, RSTRING_PTR(str1), RSTRING_LEN(str1));
  WStr_allocWithUTF8L(&wstr2, RSTRING_PTR(str2), RSTRING_LEN(str2));
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
  WStr_allocWithUTF8L(&ustr, RSTRING_PTR(str), RSTRING_LEN(str));
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
unicode_decompose_safe(VALUE obj, VALUE str)
{
  WString ustr;
  WString result;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8L(&ustr, RSTRING_PTR(str), RSTRING_LEN(str));
  WStr_alloc(&result);
  decompose_safe_internal(&ustr, &result);
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
  WStr_allocWithUTF8L(&ustr, RSTRING_PTR(str), RSTRING_LEN(str));
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
  WStr_allocWithUTF8L(&ustr, RSTRING_PTR(str), RSTRING_LEN(str));
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
  WStr_allocWithUTF8L(&ustr1, RSTRING_PTR(str), RSTRING_LEN(str));
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
unicode_normalize_safe(VALUE obj, VALUE str)
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
  WStr_allocWithUTF8L(&ustr1, RSTRING_PTR(str), RSTRING_LEN(str));
  WStr_alloc(&ustr2);
  decompose_safe_internal(&ustr1, &ustr2);
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
  WStr_allocWithUTF8L(&ustr1, RSTRING_PTR(str), RSTRING_LEN(str));
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
  WStr_allocWithUTF8L(&ustr, RSTRING_PTR(str), RSTRING_LEN(str));
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
  WStr_allocWithUTF8L(&ustr, RSTRING_PTR(str), RSTRING_LEN(str));
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
  WStr_allocWithUTF8L(&ustr, RSTRING_PTR(str), RSTRING_LEN(str));
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

typedef struct _get_categories_param {
  WString* wstr;
  VALUE str;
  VALUE* catname;
} get_categories_param;

static VALUE
get_categories_internal(VALUE param_value)
{
  get_categories_param* param = (get_categories_param*)param_value;
  WString* wstr = param->wstr;
  VALUE str = param->str;
  VALUE* catname = param->catname;
  int pos;
  int block_p = rb_block_given_p();
  volatile VALUE ret = str;

  if (!block_p)
    ret = rb_ary_new();
  for (pos = 0; pos < wstr->len; pos++) {
    int gencat = get_gencat(wstr->str[pos]);
    if (!block_p)
      rb_ary_push(ret, catname[gencat]);
    else {
      rb_yield(catname[gencat]);
    }
  }
 
  return ret;
}

VALUE
get_categories_ensure(VALUE wstr_value)
{
  WString* wstr = (WString*)wstr_value;
  WStr_free(wstr);
  return Qnil;
}

VALUE
unicode_get_categories(VALUE obj, VALUE str)
{
  WString wstr;
  get_categories_param param = { &wstr, str, catname_long };

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8L(&wstr, RSTRING_PTR(str), RSTRING_LEN(str));

  return rb_ensure(get_categories_internal, (VALUE)&param,
                   get_categories_ensure, (VALUE)&wstr);
  /* wstr will be freed in get_text_elements_ensure() */
}


VALUE
unicode_get_abbr_categories(VALUE obj, VALUE str)
{
  WString wstr;
  get_categories_param param = { &wstr, str, catname_abbr };

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8L(&wstr, RSTRING_PTR(str), RSTRING_LEN(str));

  return rb_ensure(get_categories_internal, (VALUE)&param,
                   get_categories_ensure, (VALUE)&wstr);
  /* wstr will be freed in get_text_elements_ensure() */
}

VALUE
unicode_wcswidth(int argc, VALUE* argv, VALUE obj)
{
  WString wstr;
  int i, count;
  int width = 0;
  int cjk_p = 0;
  VALUE str;
  VALUE cjk;

  count = rb_scan_args(argc, argv, "11", &str, &cjk);
  if (count > 1)
    cjk_p = RTEST(cjk);
  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8L(&wstr, RSTRING_PTR(str), RSTRING_LEN(str));
  for (i = 0; i <wstr.len; i++) {
    int c = wstr.str[i];
    int cat = get_gencat(c);
    int eaw = get_eawidth(c);
    if ((c > 0 && c < 32) || (c >= 0x7f && c < 0xa0)) {
      /* Control Characters */
      width = -1;
      break;
    }
    else if (c != 0x00ad && /* SOFT HYPHEN */
             (cat == c_Mn || cat == c_Me || /* Non-spacing Marks */
              cat == c_Cf || /* Format */
              c == 0 || /* NUL */
              (c >= 0x1160 && c <= 0x11ff))) /* HANGUL JUNGSEONG/JONGSEONG */
      /* zero width */ ;
    else if (eaw == w_F || eaw == w_W || /* Fullwidth or Wide */
             (c >= 0x4db6 && c <= 0x4dbf) || /* CJK Reserved */
             (c >= 0x9fcd && c <= 0x9fff) || /* CJK Reserved */
             (c >= 0xfa6e && c <= 0xfa6f) || /* CJK Reserved */
             (c >= 0xfada && c <= 0xfaff) || /* CJK Reserved */
             (c >= 0x2a6d7 && c <= 0x2a6ff) || /* CJK Reserved */
             (c >= 0x2b735 && c <= 0x2b73f) || /* CJK Reserved */
             (c >= 0x2b81e && c <= 0x2f7ff) || /* CJK Reserved */
             (c >= 0x2fa1e && c <= 0x2fffd) || /* CJK Reserved */
             (c >= 0x30000 && c <= 0x3fffd) || /* CJK Reserved */
             (cjk_p && eaw == w_A)) /* East Asian Ambiguous */
      width += 2;
    else
      width++; /* Halfwidth or Neutral */
  }
  WStr_free(&wstr);

  return INT2FIX(width);
}

VALUE
wstring_to_rstring(WString* wstr, int start, int len) {
  UString ret;
  volatile VALUE vret;

  UniStr_alloc(&ret);
  WStr_convertIntoUString2(wstr, start, len, &ret);
  vret = ENC_(rb_str_new((char*)ret.str, ret.len));
  UniStr_free(&ret);

  return vret;
}

typedef struct _get_text_elements_param {
  WString* wstr;
  VALUE str;
} get_text_elements_param;

VALUE
get_text_elements_internal(VALUE param_value)
{
  get_text_elements_param* param = (get_text_elements_param*)param_value;
  WString* wstr = param->wstr;
  VALUE str = param->str;
  int start_pos;
  int block_p = rb_block_given_p();
  volatile VALUE ret = str;

  if (!block_p)
    ret = rb_ary_new();
  for (start_pos = 0; start_pos < wstr->len;) {
    int c0 = wstr->str[start_pos];
    int cat = get_gencat(c0);
    int length = 1;
    int j;

    if (cat == c_Mn || cat == c_Mc || cat == c_Me) {
      volatile VALUE rstr = TO_(str, wstring_to_rstring(wstr, start_pos, length));
      if (!block_p)
        rb_ary_push(ret, rstr);
      else
        rb_yield(rstr);
      start_pos++;
      continue;
    }

    for (j = start_pos + 1; j < wstr->len; j++) {
      int c1 = wstr->str[j];
      int cat = get_gencat(c1);
      if (c0 >= LBASE && c0 < LBASE + LCOUNT &&
          j + 1 < wstr->len &&
          c1 >= VBASE && c1 < VBASE + VCOUNT &&
          wstr->str[j+1] >= TBASE && wstr->str[j+1] < TBASE + TCOUNT) {
        /* Hangul L+V+T */
        length += 2;
        j++;
      }
      else if (c0 >= LBASE && c0 < LBASE + LCOUNT &&
               c1 >= VBASE && c1< VBASE + VCOUNT) {
        /* Hangul L+V */
        length++;
      }
      else if (c0 >= SBASE && c0 < SBASE + SCOUNT &&
               (c0 - SBASE) % TCOUNT == 0 &&
               c1 >= TBASE && c1 < TBASE + TCOUNT) {
        /* Hangul LV+T */
        length++;
      }
      else if (cat == c_Mn || cat == c_Mc || cat == c_Me) {
        /* Mark */
        length++;
      }
      else {
        volatile VALUE rstr = TO_(str, wstring_to_rstring(wstr, start_pos, length));
        if (!block_p)
          rb_ary_push(ret, rstr);
        else
          rb_yield(rstr);
        length = 0;
        break;
      }
    }
    if (length > 0) {
      volatile VALUE rstr = TO_(str, wstring_to_rstring(wstr, start_pos, length));
      if (!block_p)
        rb_ary_push(ret, rstr);
      else
        rb_yield(rstr);
    }
    start_pos = j;
  }
  return ret;
}

VALUE
get_text_elements_ensure(VALUE wstr_value)
{
  WString* wstr = (WString*)wstr_value;
  WStr_free(wstr);
  return Qnil;
}

VALUE
unicode_get_text_elements(VALUE obj, VALUE str)
{
  WString wstr;
  get_text_elements_param param = { &wstr, str };

  Check_Type(str, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  CONVERT_TO_UTF8(str);
#endif
  WStr_allocWithUTF8L(&wstr, RSTRING_PTR(str), RSTRING_LEN(str));

  return rb_ensure(get_text_elements_internal, (VALUE)&param,
                   get_text_elements_ensure, (VALUE)&wstr);
  /* wstr will be freed in get_text_elements_ensure() */
}

void
Init_unicode_native()
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

  for (i = 0; i < c_Cn + 1; i++) {
    catname_abbr[i] = ID2SYM(rb_intern(gencat_abbr[i]));
    catname_long[i] = ID2SYM(rb_intern(gencat_long[i]));
    rb_global_variable(&catname_abbr[i]);
    rb_global_variable(&catname_long[i]);
  }

  rb_define_module_function(mUnicode, "strcmp",
			    unicode_strcmp, 2);
  rb_define_module_function(mUnicode, "strcmp_compat",
			    unicode_strcmp_compat, 2);

  rb_define_module_function(mUnicode, "decompose",
			    unicode_decompose, 1);
  rb_define_module_function(mUnicode, "decompose_safe",
			    unicode_decompose_safe, 1);
  rb_define_module_function(mUnicode, "decompose_compat",
			    unicode_decompose_compat, 1);
  rb_define_module_function(mUnicode, "compose",
			    unicode_compose, 1);

  rb_define_module_function(mUnicode, "normalize_D",
			    unicode_decompose, 1);
  rb_define_module_function(mUnicode, "normalize_D_safe",
			    unicode_decompose_safe, 1);
  rb_define_module_function(mUnicode, "normalize_KD",
			    unicode_decompose_compat, 1);
  rb_define_module_function(mUnicode, "normalize_C",
			    unicode_normalize_C, 1);
  rb_define_module_function(mUnicode, "normalize_C_safe",
			    unicode_normalize_safe, 1);
  rb_define_module_function(mUnicode, "normalize_KC",
			    unicode_normalize_KC, 1);

  /* aliases */
  rb_define_module_function(mUnicode, "nfd",
			    unicode_decompose, 1);
  rb_define_module_function(mUnicode, "nfd_safe",
			    unicode_decompose_safe, 1);
  rb_define_module_function(mUnicode, "nfkd",
			    unicode_decompose_compat, 1);
  rb_define_module_function(mUnicode, "nfc",
			    unicode_normalize_C, 1);
  rb_define_module_function(mUnicode, "nfc_safe",
			    unicode_normalize_safe, 1);
  rb_define_module_function(mUnicode, "nfkc",
			    unicode_normalize_KC, 1);

  rb_define_module_function(mUnicode, "upcase",
			    unicode_upcase, 1);
  rb_define_module_function(mUnicode, "downcase",
			    unicode_downcase, 1);
  rb_define_module_function(mUnicode, "capitalize",
			    unicode_capitalize, 1);

  rb_define_module_function(mUnicode, "categories",
			    unicode_get_categories, 1);
  rb_define_module_function(mUnicode, "abbr_categories",
			    unicode_get_abbr_categories, 1);
  rb_define_module_function(mUnicode, "width",
			    unicode_wcswidth, -1);
  rb_define_module_function(mUnicode, "text_elements",
			    unicode_get_text_elements, 1);

  rb_define_const(mUnicode, "VERSION",
		  rb_str_new2(UNICODE_VERSION));
}
