/*
 * Unicode Library version 0.1
 * Nov 23, 1999 yoshidam
 *
 */

#include "ruby.h"
#include "rubyio.h"
#include <stdio.h>
#include "wstring.h"
#include "unidata.map"

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

static const int
get_uppercase(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    int uc = unidata[FIX2INT(ch)].uppercase;
    if (uc > 0) return uc;
  }
  return ucs;
}

static int
get_lowercase(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    int lc = unidata[FIX2INT(ch)].lowercase;
    if (lc > 0) return lc;
  }
  return ucs;
}

static int
get_titlecase(int ucs)
{
  VALUE ch = rb_hash_aref(unicode_data, INT2FIX(ucs));

  if (!NIL_P(ch)) {
    int tc = unidata[FIX2INT(ch)].titlecase;
    if (tc > 0) return tc;
  }
  return ucs;
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
compose_pair(int c1, int c2)
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

static WString*
upcase_internal(WString* str)
{
  int i;

  for (i = 0; i < str->len; i++) {
    int uc = get_uppercase(str->str[i]);
    if (uc > 0) str->str[i] = uc;
  }

  return str;
}

static WString*
downcase_internal(WString* str)
{
  int i;

  for (i = 0; i < str->len; i++) {
    int lc = get_lowercase(str->str[i]);
    if (lc > 0) str->str[i] = lc;
  }

  return str;
}

static WString*
capitalize_internal(WString* str)
{
  int i;

  if (str->len > 1) {
    int tc = get_titlecase(str->str[0]);
    if (tc > 0) str->str[0] = tc;
  }
  for (i = 1; i < str->len; i++) {
    int lc = get_lowercase(str->str[i]);
    if (lc > 0) str->str[i] = lc;
  }

  return str;
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
  WStr_allocWithUTF8(&wstr1, RSTRING(str1)->ptr);
  WStr_allocWithUTF8(&wstr2, RSTRING(str2)->ptr);
  WStr_alloc(&result1);
  WStr_alloc(&result2);
  decompose_internal(&wstr1, &result1);
  decompose_internal(&wstr2, &result2);
  WStr_free(&wstr1);
  WStr_free(&wstr2);
  sort_canonical(&result1);
  sort_canonical(&result2);
  UStr_alloc(&ustr1);
  UStr_alloc(&ustr2);
  WStr_convertIntoUString(&result1, &ustr1);
  WStr_convertIntoUString(&result2, &ustr2);
  WStr_free(&result1);
  WStr_free(&result2);
  UStr_addChar(&ustr1, '\0');
  UStr_addChar(&ustr2, '\0');
  ret = strcmp(ustr1.str, ustr2.str);
  UStr_free(&ustr1);
  UStr_free(&ustr2);

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
  WStr_allocWithUTF8(&wstr1, RSTRING(str1)->ptr);
  WStr_allocWithUTF8(&wstr2, RSTRING(str2)->ptr);
  WStr_alloc(&result1);
  WStr_alloc(&result2);
  decompose_compat_internal(&wstr1, &result1);
  decompose_compat_internal(&wstr2, &result2);
  WStr_free(&wstr1);
  WStr_free(&wstr2);
  sort_canonical(&result1);
  sort_canonical(&result2);
  UStr_alloc(&ustr1);
  UStr_alloc(&ustr2);
  WStr_convertIntoUString(&result1, &ustr1);
  WStr_convertIntoUString(&result2, &ustr2);
  WStr_free(&result1);
  WStr_free(&result2);
  UStr_addChar(&ustr1, '\0');
  UStr_addChar(&ustr2, '\0');
  ret = strcmp(ustr1.str, ustr2.str);
  UStr_free(&ustr1);
  UStr_free(&ustr2);

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
  WStr_allocWithUTF8(&ustr, RSTRING(str)->ptr);
  WStr_alloc(&result);
  decompose_internal(&ustr, &result);
  WStr_free(&ustr);
  sort_canonical(&result);
  UStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = rb_str_new(ret.str, ret.len);
  UStr_free(&ret);

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
  WStr_allocWithUTF8(&ustr, RSTRING(str)->ptr);
  WStr_alloc(&result);
  decompose_compat_internal(&ustr, &result);
  WStr_free(&ustr);
  sort_canonical(&result);
  UStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = rb_str_new(ret.str, ret.len);
  UStr_free(&ret);

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
  WStr_allocWithUTF8(&ustr, RSTRING(str)->ptr);
  sort_canonical(&ustr);
  WStr_alloc(&result);
  compose_internal(&ustr, &result);
  WStr_free(&ustr);
  UStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = rb_str_new(ret.str, ret.len);
  UStr_free(&ret);

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
  WStr_allocWithUTF8(&ustr1, RSTRING(str)->ptr);
  WStr_alloc(&ustr2);
  decompose_internal(&ustr1, &ustr2);
  WStr_free(&ustr1);
  sort_canonical(&ustr2);
  WStr_alloc(&result);
  compose_internal(&ustr2, &result);
  WStr_free(&ustr2);
  UStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = rb_str_new(ret.str, ret.len);
  UStr_free(&ret);

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
  WStr_allocWithUTF8(&ustr1, RSTRING(str)->ptr);
  WStr_alloc(&ustr2);
  decompose_compat_internal(&ustr1, &ustr2);
  WStr_free(&ustr1);
  sort_canonical(&ustr2);
  WStr_alloc(&result);
  compose_internal(&ustr2, &result);
  WStr_free(&ustr2);
  UStr_alloc(&ret);
  WStr_convertIntoUString(&result, &ret);
  WStr_free(&result);
  vret = rb_str_new(ret.str, ret.len);
  UStr_free(&ret);

  return vret;
}

static VALUE
unicode_upcase(VALUE obj, VALUE str)
{
  WString ustr;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
  WStr_allocWithUTF8(&ustr, RSTRING(str)->ptr);
  upcase_internal(&ustr);
  UStr_alloc(&ret);
  WStr_convertIntoUString(&ustr, &ret);
  WStr_free(&ustr);
  vret = rb_str_new(ret.str, ret.len);
  UStr_free(&ret);

  return vret;
}

static VALUE
unicode_downcase(VALUE obj, VALUE str)
{
  WString ustr;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
  WStr_allocWithUTF8(&ustr, RSTRING(str)->ptr);
  downcase_internal(&ustr);
  UStr_alloc(&ret);
  WStr_convertIntoUString(&ustr, &ret);
  WStr_free(&ustr);
  vret = rb_str_new(ret.str, ret.len);
  UStr_free(&ret);

  return vret;
}

static VALUE
unicode_capitalize(VALUE obj, VALUE str)
{
  WString ustr;
  UString ret;
  VALUE vret;

  Check_Type(str, T_STRING);
  WStr_allocWithUTF8(&ustr, RSTRING(str)->ptr);
  capitalize_internal(&ustr);
  UStr_alloc(&ret);
  WStr_convertIntoUString(&ustr, &ret);
  WStr_free(&ustr);
  vret = rb_str_new(ret.str, ret.len);
  UStr_free(&ret);

  return vret;
}

void
Init_unicode()
{
  int i;

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

  rb_define_module_function(mUnicode, "upcase",
			    unicode_upcase, 1);
  rb_define_module_function(mUnicode, "downcase",
			    unicode_downcase, 1);
  rb_define_module_function(mUnicode, "capitalize",
			    unicode_capitalize, 1);
}
