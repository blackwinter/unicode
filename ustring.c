/*
 * Simple string library
 * Version 0.2
 * 1999 by yoshidam
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ustring.h"

UString*
UniStr_alloc(UString* str)
{
  str->size = USTR_INITIAL_STRING_LEN;
  str->len = 0;
  if ((str->str = (unsigned char*)malloc(USTR_INITIAL_STRING_LEN)) == NULL) {
    str->size = 0;
    return NULL;
  }

  return str;
}

UString*
UniStr_enlarge(UString* str, int size)
{
  unsigned char* newptr;

  if ((newptr = (unsigned char*)realloc(str->str, str->size + size))
      == NULL) {
    return NULL;
  }
  str->str = newptr;
  str->size += size;

  return str;
}

void
UniStr_free(UString* str)
{
  str->size = 0;
  str->len = 0;
  free(str->str);
}

int
UniStr_addChars(UString* s, const unsigned char* a, int len)
{
  if (s->len + len >= s->size) {
    UniStr_enlarge(s, len + USTR_STRING_EXTEND_LEN);
  }
  memcpy(s->str + s->len, a, len);
  s->len += len;

  return s->len;
}

int
UniStr_addChar(UString* s, unsigned char a)
{
  if (s->len + 1 >= s->size) {
    UniStr_enlarge(s, USTR_STRING_EXTEND_LEN);
  }
  *(s->str + s->len) = a;
  (s->len)++;

  return s->len;
}

int
UniStr_addChar2(UString* s, unsigned char a1, unsigned char a2)
{
  if (s->len + 2 >= s->size) {
    UniStr_enlarge(s, USTR_STRING_EXTEND_LEN);
  }
  *(s->str + s->len) = a1;
  *(s->str + s->len + 1) = a2;
  s->len += 2;

  return s->len;
}

int
UniStr_addChar3(UString* s, unsigned char a1, unsigned char a2, unsigned char a3)
{
  if (s->len + 3 >= s->size) {
    UniStr_enlarge(s, USTR_STRING_EXTEND_LEN);
  }
  *(s->str + s->len) = a1;
  *(s->str + s->len + 1) = a2;
  *(s->str + s->len + 2) = a3;
  s->len += 3;

  return s->len;
}

int
UniStr_addChar4(UString* s, unsigned char a1, unsigned char a2,
	      unsigned char a3, unsigned char a4)
{
  if (s->len + 4 >= s->size) {
    UniStr_enlarge(s, USTR_STRING_EXTEND_LEN);
  }
  *(s->str + s->len) = a1;
  *(s->str + s->len + 1) = a2;
  *(s->str + s->len + 2) = a3;
  *(s->str + s->len + 3) = a4;
  s->len += 4;

  return s->len;
}

int
UniStr_addChar5(UString* s, unsigned char a1, unsigned char a2,
	      unsigned char a3, unsigned char a4, unsigned char a5)
{
  if (s->len + 5 >= s->size) {
    UniStr_enlarge(s, USTR_STRING_EXTEND_LEN);
  }
  *(s->str + s->len) = a1;
  *(s->str + s->len + 1) = a2;
  *(s->str + s->len + 2) = a3;
  *(s->str + s->len + 3) = a4;
  *(s->str + s->len + 4) = a5;
  s->len += 5;

  return s->len;
}

int
UniStr_addChar6(UString* s, unsigned char a1, unsigned char a2,
	      unsigned char a3, unsigned char a4,
	      unsigned char a5, unsigned char a6)
{
  if (s->len + 6 >= s->size) {
    UniStr_enlarge(s, USTR_STRING_EXTEND_LEN);
  }
  *(s->str + s->len) = a1;
  *(s->str + s->len + 1) = a2;
  *(s->str + s->len + 2) = a3;
  *(s->str + s->len + 3) = a4;
  *(s->str + s->len + 4) = a5;
  *(s->str + s->len + 5) = a6;
  s->len += 6;

  return s->len;
}

int
UniStr_addWChar(UString* ustr, unsigned int c)
{
  if (c < 128) {         /* 0x0000-0x00FF */
    UniStr_addChar(ustr, c);
  }
  else if (c < 2048) {        /* 0x0100-0x07FF */
    unsigned char b2 = c & 63;
    unsigned char b1 = c >> 6;
    UniStr_addChar2(ustr, b1 | 192, b2 | 128);

  }
  else if (c < 0x10000) {     /* 0x0800-0xFFFF */
    unsigned char b3 = c & 63;
    unsigned char b2 = (c >> 6) & 63;
    unsigned char b1 = c >> 12;
    UniStr_addChar3(ustr, b1 | 224, b2 | 128, b3 | 128);
  }
  else if (c < 0x200000) {     /* 0x00010000-0x001FFFFF */
    unsigned char b4 = c & 63;
    unsigned char b3 = (c >> 6) & 63;
    unsigned char b2 = (c >> 12) & 63;
    unsigned char b1 = c >> 18;
    UniStr_addChar4(ustr, b1 | 240, b2 | 128, b3 | 128, b4 | 128);
  }
  else if (c < 0x4000000) {     /* 0x00200000-0x03FFFFFF */
    unsigned char b5 = c & 63;
    unsigned char b4 = (c >> 6) & 63;
    unsigned char b3 = (c >> 12) & 63;
    unsigned char b2 = (c >> 18) & 63;
    unsigned char b1 = c >> 24;
    UniStr_addChar5(ustr, b1 | 248, b2 | 128, b3 | 128, b4 | 128, b5 | 128);
  }
  else if (c < 0x80000000) {     /* 0x04000000-0x7FFFFFFF */
    unsigned char b6 = c & 63;
    unsigned char b5 = (c >> 6) & 63;
    unsigned char b4 = (c >> 12) & 63;
    unsigned char b3 = (c >> 18) & 63;
    unsigned char b2 = (c >> 24) & 63;
    unsigned char b1 = (c >> 30) & 63;
    UniStr_addChar6(ustr, b1 | 252, b2 | 128, b3 | 128,
		  b4 | 128, b5 | 128, b6 | 128);
  }

  return ustr->len;
}

void
UniStr_dump(UString* s)
{
  int i;

  printf("[%d/%d] ", s->len, s->size);
  for (i = 0; i < s->len ; i++) {
    printf("%02x ", *(s->str + i));
  }
  printf("\n");
}
