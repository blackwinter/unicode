/*
 * Simple string library
 * Version 0.2
 * 1999 by yoshidam
 */

#ifndef _USTRING_H
#define _USTRING_H

#ifdef __cplusplus
extern "C" {
#endif

#define USTR_INITIAL_STRING_LEN 1024
#define USTR_STRING_EXTEND_LEN 1024

/*#define malloc(s) xmalloc(s)*/
/*#define relloc(p, s) xrelloc(p, s)*/

typedef struct _UString {
  unsigned char* str;
  int len;
  int size;
} UString;

UString* UStr_alloc(UString* str);
UString* UStr_enlarge(UString* str, int size);
void UStr_free(UString* str);
int UStr_addUhars(UString* s, const unsigned char* a, int len);
int UStr_addChar(UString* s, unsigned char a);
int UStr_addChar2(UString* s, unsigned char a1, unsigned char a2);
int UStr_addChar3(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3);
int UStr_addChar4(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3, unsigned char a4);
int UStr_addChar5(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3, unsigned char a4, unsigned char a5);
int UStr_addChar6(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3, unsigned char a4,
		  unsigned char a5, unsigned char a6);
int UStr_addWChar(UString* s, int c);
void UStr_dump(UString* s);

#ifdef __cplusplus
}
#endif

#endif
