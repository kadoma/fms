

#include "tcutil.h"
#include "myconf.h"


/*************************************************************************************************
 * basic utilities
 *************************************************************************************************/

/* String containing the version information. */
const char *tcversion = _TC_VERSION;


/* Call back function for handling a fatal error. */
void (*tcfatalfunc)(const char *message) = NULL;


/* Allocate a region on memory. */
void *tcmalloc(size_t size){
  assert(size > 0 && size < INT_MAX);
  char *p = malloc(size);
  if(!p) tcmyfatal("out of memory");
  return p;
}

/* Re-allocate a region on memory. */
void *tcrealloc(void *ptr, size_t size){
  assert(size > 0);
  char *p = realloc(ptr, size);
  if(!p) tcmyfatal("out of memory");
  return p;
}

/* Show error message on the standard error output and exit. */
void *tcmyfatal(const char *message){
  assert(message);
  if(tcfatalfunc){
    tcfatalfunc(message);
  } else {
    fprintf(stderr, "fatal error: %s\n", message);
  }
  exit(1);
  return NULL;
}

/* Duplicate a region on memory. */
void *tcmemdup(const void *ptr, size_t size){
  assert(ptr && size >= 0);
  char *p = tcmalloc(size + 1);
  memcpy(p, ptr, size);
  p[size] = '\0';
  return p;
}


/* Duplicate a string on memory. */
char *tcstrdup(const void *str){
  assert(str);
  int size = strlen(str);
  char *p = tcmalloc(size + 1);
  memcpy(p, str, size);
  p[size] = '\0';
  return p;
}


/*************************************************************************************************
 * extensible string
 *************************************************************************************************/


#define TC_XSTRUNIT    12                // allocation unit size of an extensible string


/* Delete an extensible string object. */
void tcxstrdel(TCXSTR *xstr){
  assert(xstr);
  free(xstr->ptr);
  free(xstr);
}



/* Get the larger value of two integers. */
long tclmax(long a, long b){
  return (a > b) ? a : b;
}


/* Get the lesser value of two integers. */
long tclmin(long a, long b){
  return (a < b) ? a : b;
}



/* Get the pointer of the region of an extensible string object. */
const char *tcxstrptr(const TCXSTR *xstr){
  assert(xstr);
  return xstr->ptr;
}


/* Get the size of the region of an extensible string object. */
int tcxstrsize(const TCXSTR *xstr){
  assert(xstr);
  return xstr->size;
}









/* END OF FILE */
