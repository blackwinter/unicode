/*
 * Simple wide string library
 * Version 0.1
 * 1999 by yoshidam
 */

#ifndef _WSTRING_H
#define _WSTRING_H

#include "ustring.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WSTR_INITIAL_STRING_LEN 1024
#define WSTR_STRING_EXTEND_LEN 1024

typedef struct _WString {
  int* str;
  int len;
  int size;
} WString;

WString* WStr_alloc(WString* str);
WString* WStr_allocWithUTF8(WString* s, const char* u);
WString* WStr_enlarge(WString* str, int size);
void WStr_free(WString* str);
int WStr_addWChars(WString* s, const int* a, int len);
int WStr_addWChar(WString* s, int a);
int WStr_pushWString(WString* s, const WString* add);
int WStr_addWChar2(WString* s, int a1, int a2);
int WStr_addWChar3(WString* s, int a1, int a2, int a3);
UString* WStr_convertIntoUString(WString* wstr, UString* ustr);
void WStr_dump(WString* s);

#ifdef __cplusplus
}
#endif

#endif
