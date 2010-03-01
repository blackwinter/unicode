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

UString* UniStr_alloc(UString* str);
UString* UniStr_enlarge(UString* str, int size);
void UniStr_free(UString* str);
int UniStr_addChars(UString* s, const unsigned char* a, int len);
int UniStr_addChar(UString* s, unsigned char a);
int UniStr_addChar2(UString* s, unsigned char a1, unsigned char a2);
int UniStr_addChar3(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3);
int UniStr_addChar4(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3, unsigned char a4);
int UniStr_addChar5(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3, unsigned char a4, unsigned char a5);
int UniStr_addChar6(UString* s, unsigned char a1, unsigned char a2,
		  unsigned char a3, unsigned char a4,
		  unsigned char a5, unsigned char a6);
int UniStr_addWChar(UString* s, unsigned int c);
void UniStr_dump(UString* s);

#ifdef __cplusplus
}
#endif

#endif
