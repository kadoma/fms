


#ifndef _TCUTIL_H                        // duplication check
#define _TCUTIL_H


#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define _TC_VERSION    "0.1.0"
#define _TC_LIBVER 	103
#define _TC_FORMATVER "0.1"


/*************************************************************************************************
 * basic utilities
 *************************************************************************************************/


/* String containing the version information. */
extern const char *tcversion;


/* Pointer to the call back function for handling a fatal error.
   The argument specifies the error message.  The initial value of this variable is `NULL'.
   If the value is `NULL', the default function is called when a fatal error occurs.  A fatal
   error occurs when memory allocation is failed. */
extern void (*tcfatalfunc)(const char *);


/* Show error message on the standard error output and exit.
   `message' specifies an error message.
   This function does not return. */
void *tcmyfatal(const char *message);

/* Allocate a region on memory.
   `size' specifies the size of the region.
   The return value is the pointer to the allocated region.
   This function handles failure of memory allocation implicitly.  Because the region of the
   return value is allocated with the `malloc' call, it should be released with the `free' call
   when it is no longer in use. */
void *tcmalloc(size_t size);



/* Re-allocate a region on memory.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   The return value is the pointer to the re-allocated region.
   This function handles failure of memory allocation implicitly.  Because the region of the
   return value is allocated with the `realloc' call, it should be released with the `free' call
   when it is no longer in use. */
void *tcrealloc(void *ptr, size_t size);


/* Duplicate a region on memory.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   The return value is the pointer to the allocated region of the duplicate.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
void *tcmemdup(const void *ptr, size_t size);


/* Duplicate a string on memory.
   `str' specifies the string.
   The return value is the allocated string equivalent to the specified string.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *tcstrdup(const void *str);





/*************************************************************************************************
 * extensible string
 *************************************************************************************************/


typedef struct {                         // type of structure for an extensible string object
  char *ptr;                             // pointer to the region
  int size;                              // size of the region
  int asize;                             // size of the allocated region
} TCXSTR;



/* Delete an extensible string object.
   `xstr' specifies the extensible string object.
   Note that the deleted object and its derivatives can not be used anymore. */
void tcxstrdel(TCXSTR *xstr);




/* Get the pointer of the region of an extensible string object.
   `xstr' specifies the extensible string object.
   The return value is the pointer of the region of the object.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string. */
const char *tcxstrptr(const TCXSTR *xstr);


/* Get the size of the region of an extensible string object.
   `xstr' specifies the extensible string object.
   The return value is the size of the region of the object. */
int tcxstrsize(const TCXSTR *xstr);

/* Get the larger value of two integers.
   `a' specifies an integer.
   `b' specifies the other integer.
   The return value is the larger value of the two. */
long tclmax(long a, long b);


/* Get the lesser value of two integers.
   `a' specifies an integer.
   `b' specifies the other integer.
   The return value is the lesser value of the two. */
long tclmin(long a, long b);





#endif                                   // duplication check



// END OF FILE
