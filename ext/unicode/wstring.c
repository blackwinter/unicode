/*
 * Simple wide string Library
 * Version 0.1
 * 1999 by yoshidam
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wstring.h"

WString*
WStr_alloc(WString* str)
{
  str->size = WSTR_INITIAL_STRING_LEN;
  str->len = 0;
  if ((str->str =
       (int*)malloc(WSTR_INITIAL_STRING_LEN * sizeof(int))) == NULL) {
    str->size = 0;
    return NULL;
  }

  return str;
}

WString*
WStr_enlarge(WString* str, int size)
{
  int* newptr;

  if ((newptr = (int*)realloc(str->str, (str->size + size) * sizeof(int)))
      == NULL) {
    return NULL;
  }
  str->str = newptr;
  str->size += size;

  return str;
}

void
WStr_free(WString* str)
{
  str->size = 0;
  str->len = 0;
  free(str->str);
}

int
WStr_addWChars(WString* s, const int* a, int len)
{
  if (s->len + len >= s->size) {
    WStr_enlarge(s, len + WSTR_STRING_EXTEND_LEN);
  }
  memcpy(s->str + s->len, a, len * sizeof(int));
  s->len += len;

  return s->len;
}

int
WStr_addWChar(WString* s, int a)
{
  if (s->len + 1 >= s->size) {
    WStr_enlarge(s, WSTR_STRING_EXTEND_LEN);
  }
  *(s->str + s->len) = a;
  (s->len)++;

  return s->len;
}

int
WStr_pushWString(WString* s, const WString* add)
{
  if (s->len + add->len >= s->size) {
    WStr_enlarge(s, add->len + WSTR_STRING_EXTEND_LEN);
  }
  memcpy(s->str + s->len, add->str, add->len * sizeof(int));
  s->len += add->len;

  return s->len;
}

int
WStr_addWChar2(WString* s, int a1, int a2)
{
  if (s->len + 2 >= s->size) {
    WStr_enlarge(s, WSTR_STRING_EXTEND_LEN);
  }
  *(s->str + s->len) = a1;
  *(s->str + s->len + 1) = a2;
  s->len += 2;

  return s->len;
}

int
WStr_addWChar3(WString* s, int a1, int a2, int a3)
{
  if (s->len + 3 >= s->size) {
    WStr_enlarge(s, WSTR_STRING_EXTEND_LEN);
  }
  *(s->str + s->len) = a1;
  *(s->str + s->len + 1) = a2;
  *(s->str + s->len + 2) = a3;
  s->len += 3;

  return s->len;
}

WString*
WStr_allocWithUTF8(WString* s, const char* in)
{
  int i;
  int u = 0;
  int rest = 0;

  WStr_alloc(s);
  if (in == NULL)
    return s;
  for (i = 0; in[i] != '\0'; i++) {
    unsigned char c = in[i];
    if ((c & 0xc0) == 0x80) {
      if (rest == 0)
	return NULL;
      u = (u << 6) | (c & 63);
      rest--;
      if (rest == 0) {
	WStr_addWChar(s, u);
      }
    }
    else if ((c & 0x80) == 0) {      /* 0b0nnnnnnn (7bit) */
      if (c == 0)
	return NULL;
      WStr_addWChar(s, c);
      rest = 0;
    }
    else if ((c & 0xe0) == 0xc0) {      /* 0b110nnnnn (11bit) */
      rest = 1;
      u = c & 31;
    }
    else if ((c & 0xf0) == 0xe0) {      /* 0b1110nnnn (16bit) */
      rest = 2;
      u = c & 15;
    }
    else if ((c & 0xf8) == 0xf0) {      /* 0b11110nnn (21bit) */
      rest = 3;
      u = c & 7;
    }
    else if ((c & 0xfc) == 0xf8) {      /* 0b111110nn (26bit) */
      rest = 4;
      u = c & 3;
    }
    else if ((c & 0xfe) == 0xfc) {      /* 0b1111110n (31bit) */
      rest = 5;
      u = c & 1;
    }
    else {
      return NULL;
    }
  }

  return s;
}

UString*
WStr_convertIntoUString(WString* wstr, UString* ustr)
{
  int i;

  for (i = 0; i < wstr->len; i++) {
    UniStr_addWChar(ustr, wstr->str[i]);
  }

  return ustr;
}

void
WStr_dump(WString* s)
{
  int i;

  printf("[%d/%d] ", s->len, s->size);
  for (i = 0; i < s->len ; i++) {
    printf("%04x ", *(s->str + i));
  }
  printf("\n");
}
