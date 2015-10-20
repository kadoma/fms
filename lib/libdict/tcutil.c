

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


/* Allocate a nullified region on memory. */
void *tccalloc(size_t nmemb, size_t size){
  assert(nmemb > 0 && size < INT_MAX && size > 0 && size < INT_MAX);
  char *p = tccalloc(nmemb, size);
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


/* Free a region on memory. */
void tcfree(void *ptr){
  free(ptr);
}



/*************************************************************************************************
 * extensible string
 *************************************************************************************************/


#define TC_XSTRUNIT    12                // allocation unit size of an extensible string


/* private function prototypes */
static void tcvxstrprintf(TCXSTR *xstr, const char *format, va_list ap);


/* Create an extensible string object. */
TCXSTR *tcxstrnew(void){
  TCXSTR *xstr = tcmalloc(sizeof(*xstr));
  xstr->ptr = tcmalloc(TC_XSTRUNIT);
  xstr->size = 0;
  xstr->asize = TC_XSTRUNIT;
  xstr->ptr[0] = '\0';
  return xstr;
}


/* Create an extensible string object from a character string. */
TCXSTR *tcxstrnew2(const char *str){
  assert(str);
  TCXSTR *xstr;
  xstr = tcmalloc(sizeof(*xstr));
  int size = strlen(str);
  int asize = tclmax(size + 1, TC_XSTRUNIT);
  xstr->ptr = tcmalloc(asize);
  xstr->size = size;
  xstr->asize = asize;
  memcpy(xstr->ptr, str, size + 1);
  return xstr;
}


/* Create an extensible string object with the initial allocation size. */
TCXSTR *tcxstrnew3(int asiz){
  assert(asiz >= 0);
  asiz = tclmax(asiz, TC_XSTRUNIT);
  TCXSTR *xstr = tcmalloc(sizeof(*xstr));
  xstr->ptr = tcmalloc(asiz);
  xstr->size = 0;
  xstr->asize = asiz;
  xstr->ptr[0] = '\0';
  return xstr;
}


/* Copy an extensible string object. */
TCXSTR *tcxstrdup(const TCXSTR *xstr){
  assert(xstr);
  TCXSTR *nxstr;
  nxstr = tcmalloc(sizeof(*nxstr));
  int asize = tclmax(xstr->size + 1, TC_XSTRUNIT);
  nxstr->ptr = tcmalloc(asize);
  nxstr->size = xstr->size;
  nxstr->asize = asize;
  memcpy(nxstr->ptr, xstr->ptr, xstr->size + 1);
  return nxstr;
}


/* Delete an extensible string object. */
void tcxstrdel(TCXSTR *xstr){
  assert(xstr);
  free(xstr->ptr);
  free(xstr);
}


/* Concatenate a region to the end of an extensible string object. */
void tcxstrcat(TCXSTR *xstr, const void *ptr, int size){
  assert(xstr && ptr && size >= 0);
  int nsize = xstr->size + size + 1;
  if(xstr->asize < nsize){
    while(xstr->asize < nsize){
      xstr->asize *= 2;
      if(xstr->asize < nsize) xstr->asize = nsize;
    }
    xstr->ptr = tcrealloc(xstr->ptr, xstr->asize);
  }
  memcpy(xstr->ptr + xstr->size, ptr, size);
  xstr->size += size;
  xstr->ptr[xstr->size] = '\0';
}


/* Concatenate a character string to the end of an extensible string object. */
void tcxstrcat2(TCXSTR *xstr, const char *str){
  assert(xstr && str);
  int size = strlen(str);
  int nsize = xstr->size + size + 1;
  if(xstr->asize < nsize){
    while(xstr->asize < nsize){
      xstr->asize *= 2;
      if(xstr->asize < nsize) xstr->asize = nsize;
    }
    xstr->ptr = tcrealloc(xstr->ptr, xstr->asize);
  }
  memcpy(xstr->ptr + xstr->size, str, size + 1);
  xstr->size += size;
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


/* Clear an extensible string object. */
void tcxstrclear(TCXSTR *xstr){
  assert(xstr);
  xstr->size = 0;
  xstr->ptr[0] = '\0';
}


/* Convert an extensible string object into a usual allocated region. */
char *tcxstrtomalloc(TCXSTR *xstr){
  assert(xstr);
  char *ptr;
  ptr = xstr->ptr;
  free(xstr);
  return ptr;
}


/* Create an extensible string object from an allocated region. */
TCXSTR *tcxstrfrommalloc(char *ptr, int size){
  TCXSTR *xstr = tcmalloc(sizeof(*xstr));
  xstr->ptr = tcrealloc(ptr, size + 1);
  xstr->ptr[size] = '\0';
  xstr->size = size;
  xstr->asize = size;
  return xstr;
}


/* Perform formatted output into an extensible string object. */
void tcxstrprintf(TCXSTR *xstr, const char *format, ...){
  assert(xstr && format);
  va_list ap;
  va_start(ap, format);
  tcvxstrprintf(xstr, format, ap);
  va_end(ap);
}


/* Allocate a formatted string on memory. */
char *tcsprintf(const char *format, ...){
  assert(format);
  TCXSTR *xstr = tcxstrnew();
  va_list ap;
  va_start(ap, format);
  tcvxstrprintf(xstr, format, ap);
  va_end(ap);
  return tcxstrtomalloc(xstr);
}


/* Perform formatted output into an extensible string object. */
static void tcvxstrprintf(TCXSTR *xstr, const char *format, va_list ap){
  assert(xstr && format);
  while(*format != '\0'){
    if(*format == '%'){
      char cbuf[TC_NUMBUFSIZ];
      cbuf[0] = '%';
      int cblen = 1;
      int lnum = 0;
      format++;
      while(strchr("0123456789 .+-hlLz", *format) && *format != '\0' &&
            cblen < TC_NUMBUFSIZ - 1){
        if(*format == 'l' || *format == 'L') lnum++;
        cbuf[cblen++] = *(format++);
      }
      cbuf[cblen++] = *format;
      cbuf[cblen] = '\0';
      int tlen;
      char *tmp, tbuf[TC_NUMBUFSIZ*2];
      switch(*format){
      case 's':
        tmp = va_arg(ap, char *);
        if(!tmp) tmp = "(null)";
        tcxstrcat2(xstr, tmp);
        break;
      case 'd':
        tlen = sprintf(tbuf, cbuf, va_arg(ap, int));
        tcxstrcat(xstr, tbuf, tlen);
        break;
      case 'o': case 'u': case 'x': case 'X': case 'c':
        if(lnum >= 2){
          tlen = sprintf(tbuf, cbuf, va_arg(ap, unsigned long long));
        } else if(lnum >= 1){
          tlen = sprintf(tbuf, cbuf, va_arg(ap, unsigned long));
        } else {
          tlen = sprintf(tbuf, cbuf, va_arg(ap, unsigned int));
        }
        tcxstrcat(xstr, tbuf, tlen);
        break;
      case 'e': case 'E': case 'f': case 'g': case 'G':
        if(lnum >= 1){
          tlen = sprintf(tbuf, cbuf, va_arg(ap, long double));
        } else {
          tlen = sprintf(tbuf, cbuf, va_arg(ap, double));
        }
        tcxstrcat(xstr, tbuf, tlen);
        break;
      case '@':
        tmp = va_arg(ap, char *);
        if(!tmp) tmp = "(null)";
        while(*tmp){
          switch(*tmp){
          case '&': tcxstrcat(xstr, "&amp;", 5); break;
          case '<': tcxstrcat(xstr, "&lt;", 4); break;
          case '>': tcxstrcat(xstr, "&gt;", 4); break;
          case '"': tcxstrcat(xstr, "&quot;", 6); break;
          default:
            if(!((*tmp >= 0 && *tmp <= 0x8) || (*tmp >= 0x0e && *tmp <= 0x1f)))
              tcxstrcat(xstr, tmp, 1);
            break;
          }
          tmp++;
        }
        break;
      case '?':
        tmp = va_arg(ap, char *);
        if(!tmp) tmp = "(null)";
        while(*tmp){
          unsigned char c = *(unsigned char *)tmp;
          if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
             (c >= '0' && c <= '9') || (c != '\0' && strchr("_-.", c))){
            tcxstrcat(xstr, tmp, 1);
          } else {
            tlen = sprintf(tbuf, "%%%02X", c);
            tcxstrcat(xstr, tbuf, tlen);
          }
          tmp++;
        }
        break;
      case '%':
        tcxstrcat(xstr, "%", 1);
        break;
      }
    } else {
      tcxstrcat(xstr, format, 1);
    }
    format++;
  }
}



/*************************************************************************************************
 * array list
 *************************************************************************************************/


#define TC_LISTUNIT    64                // allocation unit number of a list handle


/* private function prototypes */
static int tclistelemcmp(const void *a, const void *b);
static int tclistelemcmpci(const void *a, const void *b);


/* Create a list object. */
TCLIST *tclistnew(void){
  TCLIST *list = tcmalloc(sizeof(*list));
  list->anum = TC_LISTUNIT;
  list->array = tcmalloc(sizeof(list->array[0]) * list->anum);
  list->start = 0;
  list->num = 0;
  return list;
}


/* Create a list object. */
TCLIST *tclistnew2(int anum){
  TCLIST *list = tcmalloc(sizeof(*list));
  if(anum < 1) anum = 1;
  list->anum = anum;
  list->array = tcmalloc(sizeof(list->array[0]) * list->anum);
  list->start = 0;
  list->num = 0;
  return list;
}


/* Copy a list object. */
TCLIST *tclistdup(const TCLIST *list){
  assert(list);
  int num = list->num;
  if(num < 1) tclistnew();
  const TCLISTDATUM *array = list->array + list->start;
  TCLIST *nlist = tcmalloc(sizeof(*nlist));
  TCLISTDATUM *narray = tcmalloc(sizeof(list->array[0]) * tclmax(num, 1));
  for(int i = 0; i < num; i++){
    int size = array[i].size;
    narray[i].ptr = tcmalloc(tclmax(size + 1, TC_XSTRUNIT));
    memcpy(narray[i].ptr, array[i].ptr, size + 1);
    narray[i].size = array[i].size;
  }
  nlist->anum = num;
  nlist->array = narray;
  nlist->start = 0;
  nlist->num = num;
  return nlist;
}


/* Delete a list object. */
void tclistdel(TCLIST *list){
  assert(list);
  TCLISTDATUM *array = list->array;
  int end = list->start + list->num;
  for(int i = list->start; i < end; i++){
    free(array[i].ptr);
  }
  free(list->array);
  free(list);
}


/* Get the number of elements of a list object. */
int tclistnum(const TCLIST *list){
  assert(list);
  return list->num;
}


/* Get the pointer to the region of an element of a list object. */
const char *tclistval(const TCLIST *list, int index, int *sp){
  assert(list && index >= 0 && sp);
  if(index >= list->num) return NULL;
  index += list->start;
  *sp = list->array[index].size;
  return list->array[index].ptr;
}


/* Get the string of an element of a list object. */
const char *tclistval2(const TCLIST *list, int index){
  assert(list && index >= 0);
  if(index >= list->num) return NULL;
  index += list->start;
  return list->array[index].ptr;
}


/* Add an element at the end of a list object. */
void tclistpush(TCLIST *list, const char *ptr, int size){
  assert(list && ptr && size >= 0);
  int index = list->start + list->num;
  if(index >= list->anum){
    list->anum += list->num + 1;
    list->array = tcrealloc(list->array, list->anum * sizeof(list->array[0]));
  }
  TCLISTDATUM *array = list->array;
  array[index].ptr = tcmalloc(tclmax(size + 1, TC_XSTRUNIT));
  memcpy(array[index].ptr, ptr, size);
  array[index].ptr[size] = '\0';
  array[index].size = size;
  list->num++;
}


/* Add a string element at the end of a list object. */
void tclistpush2(TCLIST *list, const char *str){
  assert(list && str);
  int index = list->start + list->num;
  if(index >= list->anum){
    list->anum += list->num + 1;
    list->array = tcrealloc(list->array, list->anum * sizeof(list->array[0]));
  }
  int size = strlen(str);
  TCLISTDATUM *array = list->array;
  array[index].ptr = tcmalloc(tclmax(size + 1, TC_XSTRUNIT));
  memcpy(array[index].ptr, str, size + 1);
  array[index].size = size;
  list->num++;
}


/* Add an allocated element at the end of a list object. */
void tclistpushmalloc(TCLIST *list, char *ptr, int size){
  assert(list && ptr && size >= 0);
  int index = list->start + list->num;
  if(index >= list->anum){
    list->anum += list->num + 1;
    list->array = tcrealloc(list->array, list->anum * sizeof(list->array[0]));
  }
  TCLISTDATUM *array = list->array;
  array[index].ptr = tcrealloc(ptr, size + 1);
  array[index].ptr[size] = '\0';
  array[index].size = size;
  list->num++;
}


/* Remove an element of the end of a list object. */
char *tclistpop(TCLIST *list, int *sp){
  assert(list && sp);
  if(list->num < 1) return NULL;
  int index = list->start + list->num - 1;
  list->num--;
  *sp = list->array[index].size;
  return list->array[index].ptr;
}


/* Remove a string element of the end of a list object. */
char *tclistpop2(TCLIST *list){
  assert(list);
  if(list->num < 1) return NULL;
  int index = list->start + list->num - 1;
  list->num--;
  return list->array[index].ptr;
}


/* Add an element at the top of a list object. */
void tclistunshift(TCLIST *list, const void *ptr, int size){
  assert(list && ptr && size >= 0);
  if(list->start < 1){
    if(list->start + list->num >= list->anum){
      list->anum += list->num + 1;
      list->array = tcrealloc(list->array, list->anum * sizeof(list->array[0]));
    }
    list->start = list->anum - list->num;
    memmove(list->array + list->start, list->array, list->num * sizeof(list->array[0]));
  }
  int index = list->start - 1;
  list->array[index].ptr = tcmalloc(tclmax(size + 1, TC_XSTRUNIT));
  memcpy(list->array[index].ptr, ptr, size);
  list->array[index].ptr[size] = '\0';
  list->array[index].size = size;
  list->start--;
  list->num++;
}


/* Add a string element at the top of a list object. */
void tclistunshift2(TCLIST *list, const char *str){
  assert(list && str);
  if(list->start < 1){
    if(list->start + list->num >= list->anum){
      list->anum += list->num + 1;
      list->array = tcrealloc(list->array, list->anum * sizeof(list->array[0]));
    }
    list->start = list->anum - list->num;
    memmove(list->array + list->start, list->array, list->num * sizeof(list->array[0]));
  }
  int index = list->start - 1;
  int size = strlen(str);
  list->array[index].ptr = tcmalloc(tclmax(size + 1, TC_XSTRUNIT));
  memcpy(list->array[index].ptr, str, size + 1);
  list->array[index].size = size;
  list->start--;
  list->num++;
}


/* Remove an element of the top of a list object. */
char *tclistshift(TCLIST *list, int *sp){
  assert(list && sp);
  if(list->num < 1) return NULL;
  int index = list->start;
  list->start++;
  list->num--;
  *sp = list->array[index].size;
  return list->array[index].ptr;
}


/* Remove a string element of the top of a list object. */
char *tclistshift2(TCLIST *list){
  assert(list);
  if(list->num < 1) return NULL;
  int index = list->start;
  list->start++;
  list->num--;
  return list->array[index].ptr;
}


/* Add an element at the specified location of a list object. */
void tclistinsert(TCLIST *list, int index, const void *ptr, int size){
  assert(list && index >= 0 && ptr && size >= 0);
  if(index > list->num) return;
  index += list->start;
  if(list->start + list->num >= list->anum){
    list->anum += list->num + 1;
    list->array = tcrealloc(list->array, list->anum * sizeof(list->array[0]));
  }
  memmove(list->array + index + 1, list->array + index,
          sizeof(list->array[0]) * (list->start + list->num - index));
  list->array[index].ptr = tcmalloc(tclmax(size + 1, TC_XSTRUNIT));
  memcpy(list->array[index].ptr, ptr, size);
  list->array[index].ptr[size] = '\0';
  list->array[index].size = size;
  list->num++;
}


/* Add a string element at the specified location of a list object. */
void tclistinsert2(TCLIST *list, int index, const char *str){
  assert(list && index >= 0 && str);
  if(index > list->num) return;
  index += list->start;
  if(list->start + list->num >= list->anum){
    list->anum += list->num + 1;
    list->array = tcrealloc(list->array, list->anum * sizeof(list->array[0]));
  }
  memmove(list->array + index + 1, list->array + index,
          sizeof(list->array[0]) * (list->start + list->num - index));
  int size = strlen(str);
  list->array[index].ptr = tcmalloc(tclmax(size + 1, TC_XSTRUNIT));
  memcpy(list->array[index].ptr, str, size);
  list->array[index].ptr[size] = '\0';
  list->array[index].size = size;
  list->num++;
}


/* Remove an element at the specified location of a list object. */
char *tclistremove(TCLIST *list, int index, int *sp){
  assert(list && index >= 0 && sp);
  if(index >= list->num) return NULL;
  index += list->start;
  char *ptr = list->array[index].ptr;
  *sp = list->array[index].size;
  list->num--;
  memmove(list->array + index, list->array + index + 1,
          sizeof(list->array[0]) * (list->start + list->num - index));
  return ptr;
}


/* Remove a string element at the specified location of a list object. */
char *tclistremove2(TCLIST *list, int index){
  assert(list && index >= 0);
  if(index >= list->num) return NULL;
  index += list->start;
  char *ptr = list->array[index].ptr;
  list->num--;
  memmove(list->array + index, list->array + index + 1,
          sizeof(list->array[0]) * (list->start + list->num - index));
  return ptr;
}


/* Overwrite an element at the specified location of a list object. */
void tclistover(TCLIST *list, int index, const void *ptr, int size){
  assert(list && index >= 0 && ptr && size >= 0);
  if(index >= list->num) return;
  index += list->start;
  if(size > list->array[index].size)
    list->array[index].ptr = tcrealloc(list->array[index].ptr, size + 1);
  memcpy(list->array[index].ptr, ptr, size);
  list->array[index].size = size;
  list->array[index].ptr[size] = '\0';
}


/* Overwrite a string element at the specified location of a list object. */
void tclistover2(TCLIST *list, int index, const char *str){
  assert(list && index >= 0 && str);
  if(index >= list->num) return;
  index += list->start;
  int size = strlen(str);
  if(size > list->array[index].size)
    list->array[index].ptr = tcrealloc(list->array[index].ptr, size + 1);
  memcpy(list->array[index].ptr, str, size + 1);
  list->array[index].size = size;
}


/* Sort elements of a list object in lexical order. */
void tclistsort(TCLIST *list){
  assert(list);
  qsort(list->array + list->start, list->num, sizeof(list->array[0]), tclistelemcmp);
}


/* Sort elements of a list object in case-insensitive lexical order. */
void tclistsortci(TCLIST *list){
  assert(list);
  qsort(list->array + list->start, list->num, sizeof(list->array[0]), tclistelemcmpci);
}


/* Search a list object for an element using liner search. */
int tclistlsearch(const TCLIST *list, const char *ptr, int size){
  assert(list && ptr && size >= 0);
  int end = list->start + list->num;
  for(int i = list->start; i < end; i++){
    if(list->array[i].size == size && !memcmp(list->array[i].ptr, ptr, size))
      return i - list->start;
  }
  return -1;
}


/* Search a list object for an element using binary search. */
int tclistbsearch(const TCLIST *list, const char *ptr, int size){
  assert(list && ptr && size >= 0);
  TCLISTDATUM key;
  key.ptr = (char *)ptr;
  key.size = size;
  TCLISTDATUM *res = bsearch(&key, list->array + list->start,
                             list->num, sizeof(list->array[0]), tclistelemcmp);
  return res ? res - list->array - list->start : -1;
}


/* Clear a list object. */
void tclistclear(TCLIST *list){
  assert(list);
  TCLISTDATUM *array = list->array;
  int end = list->start + list->num;
  for(int i = list->start; i < end; i++){
    free(array[i].ptr);
  }
  list->start = 0;
  list->num = 0;
}


/* Serialize a list object into a byte array. */
char *tclistdump(const TCLIST *list, int *sp){
  assert(list && sp);
  const TCLISTDATUM *array = list->array;
  int end = list->start + list->num;
  int tsiz = 0;
  for(int i = list->start; i < end; i++){
    tsiz += array[i].size + sizeof(int);
  }
  char *buf = tcmalloc(tsiz + 1);
  char *wp = buf;
  for(int i = list->start; i < end; i++){
    int step;
    TC_SETVNUMBUF(step, wp, array[i].size);
    wp += step;
    memcpy(wp, array[i].ptr, array[i].size);
    wp += array[i].size;
  }
  *sp = wp - buf;
  return buf;
}


/* Create a list object from a serialized byte array. */
TCLIST *tclistload(const char *ptr, int size){
  assert(ptr && size >= 0);
  TCLIST *list = tcmalloc(sizeof(*list));
  int anum = size / sizeof(int) + 1;
  TCLISTDATUM *array = tcmalloc(sizeof(array[0]) * anum);
  int num = 0;
  const char *rp = ptr;
  const char *ep = ptr + size;
  while(rp < ep){
    int step, vsiz;
    TC_READVNUMBUF(rp, vsiz, step);
    rp += step;
    if(num >= anum){
      anum *= 2;
      array = tcrealloc(array, anum * sizeof(array[0]));
    }
    array[num].ptr = tcmalloc(tclmax(vsiz + 1, TC_XSTRUNIT));
    memcpy(array[num].ptr, rp, vsiz);
    array[num].ptr[vsiz] = '\0';
    array[num].size = vsiz;
    num++;
    rp += vsiz;
  }
  list->anum = anum;
  list->array = array;
  list->start = 0;
  list->num = num;
  return list;
}


/* Compare two list elements in lexical order.
   `a' specifies the pointer to one element.
   `b' specifies the pointer to the other element.
   The return value is positive if the former is big, negative if the latter is big, 0 if both
   are equivalent. */
static int tclistelemcmp(const void *a, const void *b){
  assert(a && b);
  char *ao = ((TCLISTDATUM *)a)->ptr;
  char *bo = ((TCLISTDATUM *)b)->ptr;
  int size = (((TCLISTDATUM *)a)->size < ((TCLISTDATUM *)b)->size) ?
    ((TCLISTDATUM *)a)->size : ((TCLISTDATUM *)b)->size;
  for(int i = 0; i < size; i++){
    if(ao[i] > bo[i]) return 1;
    if(ao[i] < bo[i]) return -1;
  }
  return ((TCLISTDATUM *)a)->size - ((TCLISTDATUM *)b)->size;
}


/* Compare two list elements in case-insensitive lexical order..
   `a' specifies the pointer to one element.
   `b' specifies the pointer to the other element.
   The return value is positive if the former is big, negative if the latter is big, 0 if both
   are equivalent. */
static int tclistelemcmpci(const void *a, const void *b){
  assert(a && b);
  TCLISTDATUM *ap = (TCLISTDATUM *)a;
  TCLISTDATUM *bp = (TCLISTDATUM *)b;
  char *ao = ap->ptr;
  char *bo = bp->ptr;
  int size = (ap->size < bp->size) ? ap->size : bp->size;
  for(int i = 0; i < size; i++){
    int ac = ao[i];
    bool ab = false;
    if(ac >= 'A' && ac <= 'Z'){
      ac += 'a' - 'A';
      ab = true;
    }
    int bc = bo[i];
    bool bb = false;
    if(bc >= 'A' && bc <= 'Z'){
      bc += 'a' - 'A';
      bb = true;
    }
    if(ac > bc) return 1;
    if(ac < bc) return -1;
    if(!ab && bb) return 1;
    if(ab && !bb) return -1;
  }
  return ap->size - bp->size;
}



/*************************************************************************************************
 * hash map
 *************************************************************************************************/


#define TC_MAPBNUM     4093              // allocation unit number of a list handle
#define TC_MAPCSUNIT   52                // small allocation unit size of map concatenation
#define TC_MAPCBUNIT   252               // big allocation unit size of map concatenation

/* get the first hash value */
#define TC_MAPHASH1(TC_res, TC_kbuf, TC_ksiz) \
  do { \
    const unsigned char *_TC_p = (const unsigned char *)(TC_kbuf); \
    int _TC_ksiz = TC_ksiz; \
    for((TC_res) = 19780211; _TC_ksiz--;){ \
      (TC_res) = (TC_res) * 37 + *(_TC_p)++; \
    } \
  } while(false)

/* get the second hash value */
#define TC_MAPHASH2(TC_res, TC_kbuf, TC_ksiz) \
  do { \
    const unsigned char *_TC_p = (const unsigned char *)(TC_kbuf) + TC_ksiz - 1; \
    int _TC_ksiz = TC_ksiz; \
    for((TC_res) = 0x13579bdf; _TC_ksiz--;){ \
      (TC_res) = (TC_res) * 31 + *(_TC_p)--; \
    } \
  } while(false)

/* get the size of padding bytes for pointer alignment */
#define TC_ALIGNPAD(TC_hsiz) \
  (((TC_hsiz | ~-(int)sizeof(void *)) + 1) - TC_hsiz)

/* compare two keys */
#define TC_KEYCMP(TC_abuf, TC_asiz, TC_bbuf, TC_bsiz) \
  ((TC_asiz > TC_bsiz) ? 1 : (TC_asiz < TC_bsiz) ? -1 : memcmp(TC_abuf, TC_bbuf, TC_asiz))


/* Create a map object. */
TCMAP *tcmapnew(void){
  return tcmapnew2(TC_MAPBNUM);
}


/* Create a map object with specifying the number of the buckets. */
TCMAP *tcmapnew2(int bnum){
  if(bnum < 1) bnum = 1;
  TCMAP *map = tcmalloc(sizeof(*map));
  TCMAPREC **buckets = tcmalloc(sizeof(map->buckets[0]) * bnum);
  for(int i = 0; i < bnum; i++){
    buckets[i] = NULL;
  }
  map->buckets = buckets;
  map->first = NULL;
  map->last = NULL;
  map->cur = NULL;
  map->bnum = bnum;
  map->rnum = 0;
  return map;
}


/* Copy a map object. */
TCMAP *tcmapdup(const TCMAP *map){
  assert(map);
  TCMAP *nmap = tcmapnew2(tclmax(tclmax(map->bnum, map->rnum), TC_MAPBNUM));
  TCMAPREC *cur = map->cur;
  const char *kbuf;
  int ksiz;
  ((TCMAP *)map)->cur = map->first;
  while((kbuf = tcmapiternext((TCMAP *)map, &ksiz)) != NULL){
    int vsiz;
    const char *vbuf = tcmapiterval(kbuf, &vsiz);
    tcmapputkeep(nmap, kbuf, ksiz, vbuf, vsiz);
  }
  ((TCMAP *)map)->cur = cur;
  return nmap;
}


/* Close a map object. */
void tcmapdel(TCMAP *map){
  assert(map);
  TCMAPREC *rec = map->first;
  while(rec){
    TCMAPREC *next = rec->next;
    free(rec);
    rec = next;
  }
  free(map->buckets);
  free(map);
}


/* Store a record into a map object. */
void tcmapput(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(map && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  unsigned int hash;
  TC_MAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TC_MAPHASH2(hash, kbuf, ksiz);
  while(rec){
    if(hash > rec->hash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rec->hash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TC_KEYCMP(kbuf, ksiz, dbuf, rec->ksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        int psiz = TC_ALIGNPAD(ksiz);
        if(vsiz > rec->vsiz){
          TCMAPREC *old = rec;
          rec = tcrealloc(rec, sizeof(*rec) + ksiz + psiz + vsiz + 1);
          if(rec != old){
            if(map->first == old) map->first = rec;
            if(map->last == old) map->last = rec;
            if(map->cur == old) map->cur = rec;
            if(*entp == old) *entp = rec;
            if(rec->prev) rec->prev->next = rec;
            if(rec->next) rec->next->prev = rec;
            dbuf = (char *)rec + sizeof(*rec);
          }
        }
        memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
        dbuf[ksiz+psiz+vsiz] = '\0';
        rec->vsiz = vsiz;
        return;
      }
    }
  }
  int psiz = TC_ALIGNPAD(ksiz);
  rec = tcmalloc(sizeof(*rec) + ksiz + psiz + vsiz + 1);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz;
  memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
  dbuf[ksiz+psiz+vsiz] = '\0';
  rec->vsiz = vsiz;
  rec->hash = hash;
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
}


/* Store a string record into a map object. */
void tcmapput2(TCMAP *map, const char *kstr, const char *vstr){
  assert(map && kstr && vstr);
  tcmapput(map, kstr, strlen(kstr), vstr, strlen(vstr));
}


/* Store a new record into a map object. */
bool tcmapputkeep(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(map && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  unsigned int hash;
  TC_MAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TC_MAPHASH2(hash, kbuf, ksiz);
  while(rec){
    if(hash > rec->hash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rec->hash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TC_KEYCMP(kbuf, ksiz, dbuf, rec->ksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        return false;
      }
    }
  }
  int psiz = TC_ALIGNPAD(ksiz);
  rec = tcmalloc(sizeof(*rec) + ksiz + psiz + vsiz + 1);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz;
  memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
  dbuf[ksiz+psiz+vsiz] = '\0';
  rec->vsiz = vsiz;
  rec->hash = hash;
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
  return true;
}


/* Store a new string record into a map object.
   `map' specifies the map object.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tcmapputkeep2(TCMAP *map, const char *kstr, const char *vstr){
  assert(map && kstr && vstr);
  return tcmapputkeep(map, kstr, strlen(kstr), vstr, strlen(vstr));
}


/* Concatenate a value at the end of the value of the existing record in a map object. */
void tcmapputcat(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(map && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  unsigned int hash;
  TC_MAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TC_MAPHASH2(hash, kbuf, ksiz);
  while(rec){
    if(hash > rec->hash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rec->hash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TC_KEYCMP(kbuf, ksiz, dbuf, rec->ksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        int psiz = TC_ALIGNPAD(ksiz);
        int asiz = sizeof(*rec) + ksiz + psiz + rec->vsiz + vsiz + 1;
        int unit = (asiz <= TC_MAPCSUNIT) ? TC_MAPCSUNIT : TC_MAPCBUNIT;
        asiz = (asiz - 1) + unit - (asiz - 1) % unit;
        TCMAPREC *old = rec;
        rec = tcrealloc(rec, asiz);
        if(rec != old){
          if(map->first == old) map->first = rec;
          if(map->last == old) map->last = rec;
          if(map->cur == old) map->cur = rec;
          if(*entp == old) *entp = rec;
          if(rec->prev) rec->prev->next = rec;
          if(rec->next) rec->next->prev = rec;
          dbuf = (char *)rec + sizeof(*rec);
        }
        memcpy(dbuf + ksiz + psiz + rec->vsiz, vbuf, vsiz);
        dbuf[ksiz+psiz+rec->vsiz+vsiz] = '\0';
        rec->vsiz += vsiz;
        return;
      }
    }
  }
  int psiz = TC_ALIGNPAD(ksiz);
  int asiz = sizeof(*rec) + ksiz + psiz + vsiz + 1;
  int unit = (asiz <= TC_MAPCSUNIT) ? TC_MAPCSUNIT : TC_MAPCBUNIT;
  asiz = (asiz - 1) + unit - (asiz - 1) % unit;
  rec = tcmalloc(asiz);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz;
  memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
  dbuf[ksiz+psiz+vsiz] = '\0';
  rec->vsiz = vsiz;
  rec->hash = hash;
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
}


/* Concatenate a string value at the end of the value of the existing record in a map object. */
void tcmapputcat2(TCMAP *map, const char *kstr, const char *vstr){
  assert(map && kstr && vstr);
  tcmapputcat(map, kstr, strlen(kstr), vstr, strlen(vstr));
}


/* Remove a record of a map object. */
bool tcmapout(TCMAP *map, const void *kbuf, int ksiz){
  assert(map && kbuf && ksiz >= 0);
  unsigned int hash;
  TC_MAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TC_MAPHASH2(hash, kbuf, ksiz);
  while(rec){
    if(hash > rec->hash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rec->hash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TC_KEYCMP(kbuf, ksiz, dbuf, rec->ksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        if(rec->prev) rec->prev->next = rec->next;
        if(rec->next) rec->next->prev = rec->prev;
        if(rec == map->first) map->first = rec->next;
        if(rec == map->last) map->last = rec->prev;
        if(rec == map->cur) map->cur = rec->next;
        if(rec->left && !rec->right){
          *entp = rec->left;
        } else if(!rec->left && rec->right){
          *entp = rec->right;
        } else if(!rec->left && !rec->left){
          *entp = NULL;
        } else {
          *entp = rec->left;
          TCMAPREC *tmp = *entp;
          while(tmp->right){
            tmp = tmp->right;
          }
          tmp->right = rec->right;
        }
        free(rec);
        map->rnum--;
        return true;
      }
    }
  }
  return false;
}


/* Remove a string record of a map object. */
bool tcmapout2(TCMAP *map, const void *kstr){
  assert(map && kstr);
  return tcmapout(map, kstr, strlen(kstr));
}


/* Retrieve a record in a map object. */
const char *tcmapget(const TCMAP *map, const void *kbuf, int ksiz, int *sp){
  assert(map && kbuf && ksiz >= 0 && sp);
  unsigned int hash;
  TC_MAPHASH1(hash, kbuf, ksiz);
  TCMAPREC *rec = map->buckets[hash%map->bnum];
  TC_MAPHASH2(hash, kbuf, ksiz);
  while(rec){
    if(hash > rec->hash){
      rec = rec->left;
    } else if(hash < rec->hash){
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TC_KEYCMP(kbuf, ksiz, dbuf, rec->ksiz);
      if(kcmp < 0){
        rec = rec->left;
      } else if(kcmp > 0){
        rec = rec->right;
      } else {
        *sp = rec->vsiz;
        return dbuf + rec->ksiz + TC_ALIGNPAD(rec->ksiz);
      }
    }
  }
  return NULL;
}


/* Retrieve a string record in a map object. */
const char *tcmapget2(const TCMAP *map, const char *kstr){
  assert(map && kstr);
  int ksiz = strlen(kstr);
  unsigned int hash;
  TC_MAPHASH1(hash, kstr, ksiz);
  TCMAPREC *rec = map->buckets[hash%map->bnum];
  TC_MAPHASH2(hash, kstr, ksiz);
  while(rec){
    if(hash > rec->hash){
      rec = rec->left;
    } else if(hash < rec->hash){
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TC_KEYCMP(kstr, ksiz, dbuf, rec->ksiz);
      if(kcmp < 0){
        rec = rec->left;
      } else if(kcmp > 0){
        rec = rec->right;
      } else {
        return dbuf + rec->ksiz + TC_ALIGNPAD(rec->ksiz);
      }
    }
  }
  return NULL;
}


/* Move a record to the edge of a map object. */
bool tcmapmove(TCMAP *map, const void *kbuf, int ksiz, bool head){
  assert(map && kbuf && ksiz >= 0);
  unsigned int hash;
  TC_MAPHASH1(hash, kbuf, ksiz);
  TCMAPREC *rec = map->buckets[hash%map->bnum];
  TC_MAPHASH2(hash, kbuf, ksiz);
  while(rec){
    if(hash > rec->hash){
      rec = rec->left;
    } else if(hash < rec->hash){
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TC_KEYCMP(kbuf, ksiz, dbuf, rec->ksiz);
      if(kcmp < 0){
        rec = rec->left;
      } else if(kcmp > 0){
        rec = rec->right;
      } else {
        if(head){
          if(map->first == rec) return true;
          if(map->last == rec) map->last = rec->prev;
          if(rec->prev) rec->prev->next = rec->next;
          if(rec->next) rec->next->prev = rec->prev;
          rec->prev = NULL;
          rec->next = map->first;
          map->first->prev = rec;
          map->first = rec;
        } else {
          if(map->last == rec) return true;
          if(map->first == rec) map->first = rec->next;
          if(rec->prev) rec->prev->next = rec->next;
          if(rec->next) rec->next->prev = rec->prev;
          rec->prev = map->last;
          rec->next = NULL;
          map->last->next = rec;
          map->last = rec;
        }
        return true;
      }
    }
  }
  return false;
}


/* Move a string record to the edge of a map object. */
bool tcmapmove2(TCMAP *map, const char *kstr, bool head){
  assert(map && kstr);
  return tcmapmove(map, kstr, strlen(kstr), head);
}


/* Initialize the iterator of a map object. */
void tcmapiterinit(TCMAP *map){
  assert(map);
  map->cur = map->first;
}


/* Get the next key of the iterator of a map object. */
const char *tcmapiternext(TCMAP *map, int *sp){
  assert(map && sp);
  TCMAPREC *rec;
  if(!map->cur) return NULL;
  rec = map->cur;
  map->cur = rec->next;
  *sp = rec->ksiz;
  return (char *)rec + sizeof(*rec);
}


/* Get the next key string of the iterator of a map object. */
const char *tcmapiternext2(TCMAP *map){
  assert(map);
  TCMAPREC *rec;
  if(!map->cur) return NULL;
  rec = map->cur;
  map->cur = rec->next;
  return (char *)rec + sizeof(*rec);
}


/* Get the value binded to the key fetched from the iterator of a map object. */
const char *tcmapiterval(const char *kbuf, int *sp){
  assert(kbuf && sp);
  TCMAPREC *rec = (TCMAPREC *)(kbuf - sizeof(*rec));
  *sp = rec->vsiz;
  return kbuf + rec->ksiz + TC_ALIGNPAD(rec->ksiz);
}


/* Get the value string binded to the key fetched from the iterator of a map object. */
const char *tcmapiterval2(const char *kstr){
  assert(kstr);
  TCMAPREC *rec = (TCMAPREC *)(kstr - sizeof(*rec));
  return kstr + rec->ksiz + TC_ALIGNPAD(rec->ksiz);
}


/* Get the number of records stored in a map object. */
int tcmaprnum(const TCMAP *map){
  assert(map);
  return map->rnum;
}


/* Create a list object containing all keys in a map object. */
TCLIST *tcmapkeys(const TCMAP *map){
  assert(map);
  TCLIST *list = tclistnew2(map->rnum);
  TCMAPREC *cur = map->cur;
  const char *kbuf;
  int ksiz;
  ((TCMAP *)map)->cur = map->first;
  while((kbuf = tcmapiternext((TCMAP *)map, &ksiz)) != NULL){
    tclistpush(list, kbuf, ksiz);
  }
  ((TCMAP *)map)->cur = cur;
  return list;
}


/* Create a list object containing all values in a map object. */
TCLIST *tcmapvals(const TCMAP *map){
  assert(map);
  TCLIST *list = tclistnew2(map->rnum);
  TCMAPREC *cur = map->cur;
  const char *kbuf;
  int ksiz;
  ((TCMAP *)map)->cur = map->first;
  while((kbuf = tcmapiternext((TCMAP *)map, &ksiz)) != NULL){
    int vsiz;
    const char *vbuf = tcmapiterval(kbuf, &vsiz);
    tclistpush(list, vbuf, vsiz);
  }
  ((TCMAP *)map)->cur = cur;
  return list;
}


/* Clear a list object. */
void tcmapclear(TCMAP *map){
  assert(map);
  TCMAPREC *rec = map->first;
  while(rec){
    TCMAPREC *next = rec->next;
    free(rec);
    rec = next;
  }
  TCMAPREC **buckets = map->buckets;
  int bnum = map->bnum;
  for(int i = 0; i < bnum; i++){
    buckets[i] = NULL;
  }
  map->first = NULL;
  map->last = NULL;
  map->cur = NULL;
  map->rnum = 0;
}


/* Serialize map list object into a byte array. */
char *tcmapdump(const TCMAP *map, int *sp){
  assert(map && sp);
  TCMAPREC *cur = map->cur;
  int tsiz = 0;
  const char *kbuf, *vbuf;
  int ksiz, vsiz;
  ((TCMAP *)map)->cur = map->first;
  while((kbuf = tcmapiternext((TCMAP *)map, &ksiz)) != NULL){
    vbuf = tcmapiterval(kbuf, &vsiz);
    tsiz += ksiz + vsiz + sizeof(int) * 2;
  }
  char *buf = tcmalloc(tsiz + 1);
  char *wp = buf;
  ((TCMAP *)map)->cur = map->first;
  while((kbuf = tcmapiternext((TCMAP *)map, &ksiz)) != NULL){
    vbuf = tcmapiterval(kbuf, &vsiz);
    int step;
    TC_SETVNUMBUF(step, wp, ksiz);
    wp += step;
    memcpy(wp, kbuf, ksiz);
    wp += ksiz;
    TC_SETVNUMBUF(step, wp, vsiz);
    wp += step;
    memcpy(wp, vbuf, vsiz);
    wp += vsiz;
  }
  ((TCMAP *)map)->cur = cur;
  *sp = wp - buf;
  return buf;
}


/* Create a map object from a serialized byte array. */
TCMAP *tcmapload(const char *ptr, int size){
  assert(ptr && size >= 0);
  TCMAP *map = tcmapnew();
  const char *rp = ptr;
  const char *ep = ptr + size;
  while(rp < ep){
    int step, ksiz, vsiz;
    TC_READVNUMBUF(rp, ksiz, step);
    rp += step;
    const char *kbuf = rp;
    rp += ksiz;
    TC_READVNUMBUF(rp, vsiz, step);
    rp += step;
    tcmapputkeep(map, kbuf, ksiz, rp, vsiz);
    rp += vsiz;
  }
  return map;
}


/* Extract a map record from a serialized byte array. */
char *tcmaploadone(const char *ptr, int size, const char *kbuf, int ksiz, int *sp){
  assert(ptr && size >= 0 && kbuf && ksiz >= 0 && sp);
  const char *rp = ptr;
  const char *ep = ptr + size;
  while(rp < ep){
    int step, rsiz;
    TC_READVNUMBUF(rp, rsiz, step);
    rp += step;
    if(rsiz == ksiz && !memcmp(kbuf, rp, rsiz)){
      rp += rsiz;
      TC_READVNUMBUF(rp, rsiz, step);
      rp += step;
      *sp = rsiz;
      return tcmemdup(rp, rsiz);
    }
    rp += rsiz;
    TC_READVNUMBUF(rp, rsiz, step);
    rp += step;
    rp += rsiz;
  }
  return NULL;
}



/*************************************************************************************************
 * memory pool
 *************************************************************************************************/


#define TC_MPOOLUNIT   128               // allocation unit size of memory pool elements


/* Global memory pool object */
TCMPOOL *tcglobalmemorypool = NULL;


/* private function prototypes */
static void tcmpooldelglobal(void);


/* Create a memory pool object. */
TCMPOOL *tcmpoolnew(void){
  TCMPOOL *mpool = tcmalloc(sizeof(*mpool));
  mpool->anum = TC_MPOOLUNIT;
  mpool->elems = tcmalloc(sizeof(mpool->elems[0]) * mpool->anum);
  mpool->num = 0;
  return mpool;
}


/* Delete a memory pool object. */
void tcmpooldel(TCMPOOL *mpool){
  assert(mpool);
  TCMPELEM *elems = mpool->elems;
  for(int i = mpool->num - 1; i >= 0; i--){
    elems[i].del(elems[i].ptr);
  }
  free(elems);
  free(mpool);
}


/* Relegate an arbitrary object to a memory pool object. */
void tcmpoolput(TCMPOOL *mpool, void *ptr, void (*del)(void *)){
  assert(mpool && ptr && del);
  int num = mpool->num;
  if(num >= mpool->anum){
    mpool->anum *= 2;
    mpool->elems = tcrealloc(mpool->elems, mpool->anum * sizeof(mpool->elems[0]));
  }
  mpool->elems[num].ptr = ptr;
  mpool->elems[num].del = del;
  mpool->num++;
}


/* Relegate an allocated region to a memory pool object. */
void tcmpoolputptr(TCMPOOL *mpool, void *ptr){
  assert(mpool && ptr);
  tcmpoolput(mpool, ptr, (void (*)(void *))free);
}


/* Relegate an extensible string object to a memory pool object. */
void tcmpoolputxstr(TCMPOOL *mpool, TCXSTR *xstr){
  assert(mpool && xstr);
  tcmpoolput(mpool, xstr, (void (*)(void *))tcxstrdel);
}


/* Relegate a list object to a memory pool object. */
void tcmpoolputlist(TCMPOOL *mpool, TCLIST *list){
  assert(mpool && list);
  tcmpoolput(mpool, list, (void (*)(void *))tclistdel);
}


/* Relegate a map object to a memory pool object. */
void tcmpoolputmap(TCMPOOL *mpool, TCMAP *map){
  assert(mpool && map);
  tcmpoolput(mpool, map, (void (*)(void *))tcmapdel);
}


/* Allocate a region relegated to a memory pool object. */
void *tcmpoolmalloc(TCMPOOL *mpool, size_t size){
  assert(mpool && size > 0);
  void *ptr = tcmalloc(size);
  tcmpoolput(mpool, ptr, (void (*)(void *))free);
  return ptr;
}


/* Create an extensible string object relegated to a memory pool object. */
TCXSTR *tcmpoolxstrnew(TCMPOOL *mpool){
  assert(mpool);
  TCXSTR *xstr = tcxstrnew();
  tcmpoolput(mpool, xstr, (void (*)(void *))tcxstrdel);
  return xstr;
}


/* Create a list object relegated to a memory pool object. */
TCLIST *tcmpoollistnew(TCMPOOL *mpool){
  assert(mpool);
  TCLIST *list = tclistnew();
  tcmpoolput(mpool, list, (void (*)(void *))tclistdel);
  return list;
}


/* Create a map object relegated to a memory pool object. */
TCMAP *tcmpoolmapnew(TCMPOOL *mpool){
  assert(mpool);
  TCMAP *map = tcmapnew();
  tcmpoolput(mpool, map, (void (*)(void *))tcmapdel);
  return map;
}


/* Get the global memory pool object. */
TCMPOOL *tcmpoolglobal(void){
  if(tcglobalmemorypool) return tcglobalmemorypool;
  tcglobalmemorypool = tcmpoolnew();
  atexit(tcmpooldelglobal);
  return tcglobalmemorypool;
}


/* Detete global memory pool object. */
static void tcmpooldelglobal(void){
  if(tcglobalmemorypool) tcmpooldel(tcglobalmemorypool);
}



/*************************************************************************************************
 * miscellaneous utilities
 *************************************************************************************************/


/* Get the larger value of two integers. */
long tclmax(long a, long b){
  return (a > b) ? a : b;
}


/* Get the lesser value of two integers. */
long tclmin(long a, long b){
  return (a < b) ? a : b;
}


/* Compare two strings with case insensitive evaluation. */
int tcstricmp(const char *astr, const char *bstr){
  assert(astr && bstr);
  while(*astr != '\0'){
    if(*bstr == '\0') return 1;
    int ac = (*astr >= 'A' && *astr <= 'Z') ? *astr + ('a' - 'A') : *(unsigned char *)astr;
    int bc = (*bstr >= 'A' && *bstr <= 'Z') ? *bstr + ('a' - 'A') : *(unsigned char *)bstr;
    if(ac != bc) return ac - bc;
    astr++;
    bstr++;
  }
  return (*bstr == '\0') ? 0 : -1;
}


/* Check whether a string begins with a key. */
bool tcstrfwm(const char *str, const char *key){
  assert(str && key);
  while(*key != '\0'){
    if(*str != *key || *str == '\0') return false;
    key++;
    str++;
  }
  return true;
}


/* Check whether a string begins with a key with case insensitive evaluation. */
bool tcstrifwm(const char *str, const char *key){
  assert(str && key);
  while(*key != '\0'){
    if(*str == '\0') return false;
    int sc = *str;
    if(sc >= 'A' && sc <= 'Z') sc += 'a' - 'A';
    int kc = *key;
    if(kc >= 'A' && kc <= 'Z') kc += 'a' - 'A';
    if(sc != kc) return false;
    key++;
    str++;
  }
  return true;
}


/* Check whether a string ends with a key. */
bool tcstrbwm(const char *str, const char *key){
  assert(str && key);
  int slen = strlen(str);
  int klen = strlen(key);
  for(int i = 1; i <= klen; i++){
    if(i > slen || str[slen-i] != key[klen-i]) return false;
  }
  return true;
}


/* Check whether a string ends with a key with case insensitive evaluation. */
bool tcstribwm(const char *str, const char *key){
  assert(str && key);
  int slen = strlen(str);
  int klen = strlen(key);
  for(int i = 1; i <= klen; i++){
    if(i > slen) return false;
    int sc = str[slen-i];
    if(sc >= 'A' && sc <= 'Z') sc += 'a' - 'A';
    int kc = key[klen-i];
    if(kc >= 'A' && kc <= 'Z') kc += 'a' - 'A';
    if(sc != kc) return false;
  }
  return true;
}


/* Convert the letters of a string into upper case. */
char *tcstrtoupper(char *str){
  assert(str);
  char *wp = str;
  while(*wp != '\0'){
    if(*wp >= 'a' && *wp <= 'z') *wp -= 'a' - 'A';
    wp++;
  }
  return str;
}


/* Convert the letters of a string into lower case. */
char *tcstrtolower(char *str){
  assert(str);
  char *wp = str;
  while(*wp != '\0'){
    if(*wp >= 'A' && *wp <= 'Z') *wp += 'a' - 'A';
    wp++;
  }
  return str;
}


/* Cut space characters at head or tail of a string. */
char *tcstrtrim(char *str){
  assert(str);
  const char *rp = str;
  char *wp = str;
  bool head = true;
  while(*rp != '\0'){
    if(*rp > '\0' && *rp <= ' '){
      if(!head) *(wp++) = *rp;
    } else {
      *(wp++) = *rp;
      head = false;
    }
    rp++;
  }
  *wp = '\0';
  while(wp > str && wp[-1] > '\0' && wp[-1] <= ' '){
    *(--wp) = '\0';
  }
  return str;
}


/* Squeeze space characters in a string and trim it. */
char *tcstrsqzspc(char *str){
  assert(str);
  char *rp = str;
  char *wp = str;
  bool spc = true;
  while(*rp != '\0'){
    if(*rp > 0 && *rp <= ' '){
      if(!spc) *(wp++) = *rp;
      spc = true;
    } else {
      *(wp++) = *rp;
      spc = false;
    }
    rp++;
  }
  *wp = '\0';
  for(wp--; wp >= str; wp--){
    if(*wp > 0 && *wp <= ' '){
      *wp = '\0';
    } else {
      break;
    }
  }
  return str;
}


/* Substitute characters in a string. */
char *tcstrsubchr(char *str, const char *rstr, const char *sstr){
  assert(str && rstr && sstr);
  int slen = strlen(sstr);
  char *wp = str;
  for(int i = 0; str[i] != '\0'; i++){
    const char *p = strchr(rstr, str[i]);
    if(p){
      int idx = p - rstr;
      if(idx < slen) *(wp++) = sstr[idx];
    } else {
      *(wp++) = str[i];
    }
  }
  *wp = '\0';
  return str;
}


/* Count the number of characters in a string of UTF-8. */
int tcstrcntutf(const char *str){
  assert(str);
  const unsigned char *rp = (unsigned char *)str;
  int cnt = 0;
  while(*rp != '\0'){
    if((*rp & 0x80) == 0x00 || (*rp & 0xe0) == 0xc0 ||
       (*rp & 0xf0) == 0xe0 || (*rp & 0xf8) == 0xf0) cnt++;
    rp++;
  }
  return cnt;
}


/* Cut a string of UTF-8 at the specified number of characters. */
char *tcstrcututf(char *str, int num){
  assert(str && num >= 0);
  unsigned char *wp = (unsigned char *)str;
  int cnt = 0;
  while(*wp != '\0'){
    if((*wp & 0x80) == 0x00 || (*wp & 0xe0) == 0xc0 ||
       (*wp & 0xf0) == 0xe0 || (*wp & 0xf8) == 0xf0){
      cnt++;
      if(cnt > num){
        *wp = '\0';
        break;
      }
    }
    wp++;
  }
  return str;
}


/* Create a list object by splitting a string. */
TCLIST *tcstrsplit(const char *str, const char *delim){
  assert(str && delim);
  TCLIST *list = tclistnew();
  while(true){
    const char *sp = str;
    while(*str != '\0' && !strchr(delim, *str)){
      str++;
    }
    tclistpush(list, sp, str - sp);
    if(*str == '\0') break;
    str++;
  }
  return list;
}


/* Get the time of day in milliseconds. */
double tctime(void){
  struct timeval tv;
  struct timezone tz;
  if(gettimeofday(&tv, &tz) == -1) return 0.0;
  return (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
}



/*************************************************************************************************
 * filesystem utilities
 *************************************************************************************************/


#define TC_FILEMODE    00644             // permission of a creating file
#define TC_IOBUFSIZ    16384             // size of an I/O buffer


/* Read whole data of a file. */
char *tcreadfile(const char *path, int limit, int *sp){
  int fd = path ? open(path, O_RDONLY, TC_FILEMODE) : 0;
  if(fd == -1) return NULL;
  if(fd == 0){
    TCXSTR *xstr = tcxstrnew();
    char buf[TC_IOBUFSIZ];
    limit = limit > 0 ? tclmin(TC_IOBUFSIZ, limit) : TC_IOBUFSIZ;
    int rsiz;
    while((rsiz = read(fd, buf, limit)) > 0){
      tcxstrcat(xstr, buf, rsiz);
      if(limit > 0 && tcxstrsize(xstr) >= limit) break;
    }
    *sp = tcxstrsize(xstr);
    return tcxstrtomalloc(xstr);
  }
  struct stat sbuf;
  if(fstat(fd, &sbuf) == -1 || !S_ISREG(sbuf.st_mode)){
    close(fd);
    return NULL;
  }
  limit = limit > 0 ? tclmin((int)sbuf.st_size, limit) : sbuf.st_size;
  char *buf = tcmalloc(sbuf.st_size + 1);
  char *wp = buf;
  int rsiz;
  while((rsiz = read(fd, wp, limit - (wp - buf))) > 0){
    wp += rsiz;
  }
  *wp = '\0';
  close(fd);
  *sp = wp - buf;
  return buf;
}


/* Read every line of a file. */
TCLIST *tcreadfilelines(const char *path){
  int fd = path ? open(path, O_RDONLY, TC_FILEMODE) : 0;
  if(fd == -1) return NULL;
  TCLIST *list = tclistnew();
  TCXSTR *xstr = tcxstrnew();
  char buf[TC_IOBUFSIZ];
  int rsiz;
  while((rsiz = read(fd, buf, TC_IOBUFSIZ)) > 0){
    for(int i = 0; i < rsiz; i++){
      switch(buf[i]){
      case '\r':
        break;
      case '\n':
        tclistpush(list, tcxstrptr(xstr), tcxstrsize(xstr));
        tcxstrclear(xstr);
        break;
      default:
        tcxstrcat(xstr, buf + i, 1);
        break;
      }
    }
  }
  tclistpush(list, tcxstrptr(xstr), tcxstrsize(xstr));
  tcxstrdel(xstr);
  if(path) close(fd);
  return list;
}


/* Read names of files in a directory. */
TCLIST *tcreaddir(const char *path){
  assert(path);
  DIR *DD;
  struct dirent *dp;
  if(!(DD = opendir(path))) return NULL;
  TCLIST *list = tclistnew();
  while((dp = readdir(DD)) != NULL){
    if(!strcmp(dp->d_name, MYCDIRSTR) || !strcmp(dp->d_name, MYPDIRSTR)) continue;
    tclistpush(list, dp->d_name, strlen(dp->d_name));
  }
  closedir(DD);
  return list;
}


/* Remove a file or a directory and its sub ones recursively. */
bool tcremovelink(const char *path){
  assert(path);
  printf("[%s]\n", path);
  struct stat sbuf;
  if(lstat(path, &sbuf) == -1) return false;
  if(unlink(path) == 0) return true;
  TCLIST *list;
  if(!S_ISDIR(sbuf.st_mode) || !(list = tcreaddir(path))) return false;
  bool tail = path[0] != '\0' && path[strlen(path)-1] == MYPATHCHR;
  for(int i = 0; i < tclistnum(list); i++){
    const char *elem = tclistval2(list, i);
    if(!strcmp(MYCDIRSTR, elem) || !strcmp(MYPDIRSTR, elem)) continue;
    char *cpath;
    if(tail){
      cpath = tcsprintf("%s%s", path, elem);
    } else {
      cpath = tcsprintf("%s%c%s", path, MYPATHCHR, elem);
    }
    tcremovelink(cpath);
    free(cpath);
  }
  tclistdel(list);
  return rmdir(path) == 0 ? true : false;
}



/*************************************************************************************************
 * encoding utilities
 *************************************************************************************************/


#define TC_URLELBNUM   31                // bucket number of URL elements
#define TC_ENCBUFSIZ   32                // size of a buffer for encoding name
#define TC_XMLATBNUM   31                // bucket number of XML attributes


/* Encode a serial object with URL encoding. */
char *tcurlencode(const char *ptr, int size){
  assert(ptr && size >= 0);
  char *buf = tcmalloc(size * 3 + 1);
  char *wp = buf;
  for(int i = 0; i < size; i++){
    int c = ((unsigned char *)ptr)[i];
    if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
       (c >= '0' && c <= '9') || (c != '\0' && strchr("_-.!~*'()", c))){
      *(wp++) = c;
    } else {
      wp += sprintf(wp, "%%%02X", c);
    }
  }
  *wp = '\0';
  return buf;
}


/* Decode a string encoded with URL encoding. */
char *tcurldecode(const char *str, int *sp){
  assert(str && sp);
  char *buf = tcstrdup(str);
  char *wp = buf;
  while(*str != '\0'){
    if(*str == '%'){
      str++;
      if(((str[0] >= '0' && str[0] <= '9') || (str[0] >= 'A' && str[0] <= 'F') ||
          (str[0] >= 'a' && str[0] <= 'f')) &&
         ((str[1] >= '0' && str[1] <= '9') || (str[1] >= 'A' && str[1] <= 'F') ||
          (str[1] >= 'a' && str[1] <= 'f'))){
        unsigned char c = *str;
        if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if(c >= 'a' && c <= 'z'){
          *wp = c - 'a' + 10;
        } else {
          *wp = c - '0';
        }
        *wp *= 0x10;
        str++;
        c = *str;
        if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if(c >= 'a' && c <= 'z'){
          *wp += c - 'a' + 10;
        } else {
          *wp += c - '0';
        }
        str++;
        wp++;
      } else {
        break;
      }
    } else if(*str == '+'){
      *wp = ' ';
      str++;
      wp++;
    } else {
      *wp = *str;
      str++;
      wp++;
    }
  }
  *wp = '\0';
  *sp = wp - buf;
  return buf;
}


/* Break up a URL into elements. */
TCMAP *tcurlbreak(const char *str){
  assert(str);
  TCMAP *map = tcmapnew2(TC_URLELBNUM);
  char *tmp = tcstrdup(str);
  const char *rp = tcstrtrim(tmp);
  tcmapput2(map, "self", rp);
  bool serv = false;
  if(tcstrifwm(rp, "http://")){
    tcmapput2(map, "scheme", "http");
    rp += 7;
    serv = true;
  } else if(tcstrifwm(rp, "https://")){
    tcmapput2(map, "scheme", "https");
    rp += 8;
    serv = true;
  } else if(tcstrifwm(rp, "ftp://")){
    tcmapput2(map, "scheme", "ftp");
    rp += 6;
    serv = true;
  } else if(tcstrifwm(rp, "sftp://")){
    tcmapput2(map, "scheme", "sftp");
    rp += 7;
    serv = true;
  } else if(tcstrifwm(rp, "ftps://")){
    tcmapput2(map, "scheme", "ftps");
    rp += 7;
    serv = true;
  } else if(tcstrifwm(rp, "tftp://")){
    tcmapput2(map, "scheme", "tftp");
    rp += 7;
    serv = true;
  } else if(tcstrifwm(rp, "ldap://")){
    tcmapput2(map, "scheme", "ldap");
    rp += 7;
    serv = true;
  } else if(tcstrifwm(rp, "ldaps://")){
    tcmapput2(map, "scheme", "ldaps");
    rp += 8;
    serv = true;
  } else if(tcstrifwm(rp, "file://")){
    tcmapput2(map, "scheme", "file");
    rp += 7;
    serv = true;
  }
  char *ep;
  if((ep = strchr(rp, '#')) != NULL){
    tcmapput2(map, "fragment", ep + 1);
    *ep = '\0';
  }
  if((ep = strchr(rp, '?')) != NULL){
    tcmapput2(map, "query", ep + 1);
    *ep = '\0';
  }
  if(serv){
    if((ep = strchr(rp, '/')) != NULL){
      tcmapput2(map, "path", ep);
      *ep = '\0';
    } else {
      tcmapput2(map, "path", "/");
    }
    if((ep = strchr(rp, '@')) != NULL){
      *ep = '\0';
      if(rp[0] != '\0') tcmapput2(map, "authority", rp);
      rp = ep + 1;
    }
    if((ep = strchr(rp, ':')) != NULL){
      if(ep[1] != '\0') tcmapput2(map, "port", ep + 1);
      *ep = '\0';
    }
    if(rp[0] != '\0') tcmapput2(map, "host", rp);
  } else {
    tcmapput2(map, "path", rp);
  }
  free(tmp);
  if((rp = tcmapget2(map, "path")) != NULL){
    if((ep = strrchr(rp, '/')) != NULL){
      if(ep[1] != '\0') tcmapput2(map, "file", ep + 1);
    } else {
      tcmapput2(map, "file", rp);
    }
  }
  if((rp = tcmapget2(map, "file")) != NULL && (!strcmp(rp, ".") || !strcmp(rp, "..")))
    tcmapout2(map, "file");
  return map;
}


/* Resolve a relative URL with an absolute URL. */
char *tcurlresolve(const char *base, const char *target){
  assert(base && target);
  const char *vbuf, *path;
  char *tmp, *wp, *enc;
  while(*base > '\0' && *base <= ' '){
    base++;
  }
  while(*target > '\0' && *target <= ' '){
    target++;
  }
  if(*target == '\0') target = base;
  TCXSTR *rbuf = tcxstrnew();
  TCMAP *telems = tcurlbreak(target);
  int port = 80;
  TCMAP *belems = tcurlbreak(tcmapget2(telems, "scheme") ? target : base);
  if((vbuf = tcmapget2(belems, "scheme")) != NULL){
    tcxstrcat2(rbuf, vbuf);
    tcxstrcat(rbuf, "://", 3);
    if(!tcstricmp(vbuf, "https")){
      port = 443;
    } else if(!tcstricmp(vbuf, "ftp")){
      port = 21;
    } else if(!tcstricmp(vbuf, "sftp")){
      port = 115;
    } else if(!tcstricmp(vbuf, "ftps")){
      port = 22;
    } else if(!tcstricmp(vbuf, "tftp")){
      port = 69;
    } else if(!tcstricmp(vbuf, "ldap")){
      port = 389;
    } else if(!tcstricmp(vbuf, "ldaps")){
      port = 636;
    }
  } else {
    tcxstrcat2(rbuf, "http://");
  }
  int vsiz;
  if((vbuf = tcmapget2(belems, "authority")) != NULL){
    if((wp = strchr(vbuf, ':')) != NULL){
      *wp = '\0';
      tmp = tcurldecode(vbuf, &vsiz);
      enc = tcurlencode(tmp, vsiz);
      tcxstrcat2(rbuf, enc);
      free(enc);
      free(tmp);
      tcxstrcat(rbuf, ":", 1);
      wp++;
      tmp = tcurldecode(wp, &vsiz);
      enc = tcurlencode(tmp, vsiz);
      tcxstrcat2(rbuf, enc);
      free(enc);
      free(tmp);
    } else {
      tmp = tcurldecode(vbuf, &vsiz);
      enc = tcurlencode(tmp, vsiz);
      tcxstrcat2(rbuf, enc);
      free(enc);
      free(tmp);
    }
    tcxstrcat(rbuf, "@", 1);
  }
  if((vbuf = tcmapget2(belems, "host")) != NULL){
    tmp = tcurldecode(vbuf, &vsiz);
    tcstrtolower(tmp);
    enc = tcurlencode(tmp, vsiz);
    tcxstrcat2(rbuf, enc);
    free(enc);
    free(tmp);
  } else {
    tcxstrcat(rbuf, "localhost", 9);
  }
  int num;
  char numbuf[TC_NUMBUFSIZ];
  if((vbuf = tcmapget2(belems, "port")) != NULL && (num = atoi(vbuf)) != port && num > 0){
    sprintf(numbuf, ":%d", num);
    tcxstrcat2(rbuf, numbuf);
  }
  if(!(path = tcmapget2(telems, "path"))) path = "/";
  if(path[0] == '\0' && (vbuf = tcmapget2(belems, "path")) != NULL) path = vbuf;
  if(path[0] == '\0') path = "/";
  TCLIST *bpaths = tclistnew();
  TCLIST *opaths;
  if(path[0] != '/' && (vbuf = tcmapget2(belems, "path")) != NULL){
    opaths = tcstrsplit(vbuf, "/");
  } else {
    opaths = tcstrsplit("/", "/");
  }
  free(tclistpop2(opaths));
  for(int i = 0; i < tclistnum(opaths); i++){
    vbuf = tclistval(opaths, i, &vsiz);
    if(vsiz < 1 || !strcmp(vbuf, ".")) continue;
    if(!strcmp(vbuf, "..")){
      free(tclistpop2(bpaths));
    } else {
      tclistpush(bpaths, vbuf, vsiz);
    }
  }
  tclistdel(opaths);
  opaths = tcstrsplit(path, "/");
  for(int i = 0; i < tclistnum(opaths); i++){
    vbuf = tclistval(opaths, i, &vsiz);
    if(vsiz < 1 || !strcmp(vbuf, ".")) continue;
    if(!strcmp(vbuf, "..")){
      free(tclistpop2(bpaths));
    } else {
      tclistpush(bpaths, vbuf, vsiz);
    }
  }
  tclistdel(opaths);
  for(int i = 0; i < tclistnum(bpaths); i++){
    vbuf = tclistval2(bpaths, i);
    if(strchr(vbuf, '%')){
      tmp = tcurldecode(vbuf, &vsiz);
    } else {
      tmp = tcstrdup(vbuf);
    }
    enc = tcurlencode(tmp, strlen(tmp));
    tcxstrcat(rbuf, "/", 1);
    tcxstrcat2(rbuf, enc);
    free(enc);
    free(tmp);
  }
  if(tcstrbwm(path, "/")) tcxstrcat(rbuf, "/", 1);
  tclistdel(bpaths);
  if((vbuf = tcmapget2(telems, "query")) != NULL ||
     (*target == '#' && (vbuf = tcmapget2(belems, "query")) != NULL)){
    tcxstrcat(rbuf, "?", 1);
    TCLIST *qelems = tcstrsplit(vbuf, "&;");
    for(int i = 0; i < tclistnum(qelems); i++){
      vbuf = tclistval2(qelems, i);
      if(i > 0) tcxstrcat(rbuf, "&", 1);
      if((wp = strchr(vbuf, '=')) != NULL){
        *wp = '\0';
        tmp = tcurldecode(vbuf, &vsiz);
        enc = tcurlencode(tmp, vsiz);
        tcxstrcat2(rbuf, enc);
        free(enc);
        free(tmp);
        tcxstrcat(rbuf, "=", 1);
        wp++;
        tmp = tcurldecode(wp, &vsiz);
        enc = tcurlencode(tmp, strlen(tmp));
        tcxstrcat2(rbuf, enc);
        free(enc);
        free(tmp);
      } else {
        tmp = tcurldecode(vbuf, &vsiz);
        enc = tcurlencode(tmp, vsiz);
        tcxstrcat2(rbuf, enc);
        free(enc);
        free(tmp);
      }
    }
    tclistdel(qelems);
  }
  if((vbuf = tcmapget2(telems, "fragment")) != NULL){
    tmp = tcurldecode(vbuf, &vsiz);
    enc = tcurlencode(tmp, vsiz);
    tcxstrcat(rbuf, "#", 1);
    tcxstrcat2(rbuf, enc);
    free(enc);
    free(tmp);
  }
  tcmapdel(belems);
  tcmapdel(telems);
  return tcxstrtomalloc(rbuf);
}


/* Encode a serial object with Base64 encoding. */
char *tcbaseencode(const char *ptr, int size){
  assert(ptr && size >= 0);
  char *tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const unsigned char *obj = (const unsigned char *)ptr;
  char *buf = tcmalloc(4 * (size + 2) / 3 + 1);
  char *wp = buf;
  for(int i = 0; i < size; i += 3){
    switch(size - i){
    case 1:
      *wp++ = tbl[obj[0] >> 2];
      *wp++ = tbl[(obj[0] & 3) << 4];
      *wp++ = '=';
      *wp++ = '=';
      break;
    case 2:
      *wp++ = tbl[obj[0] >> 2];
      *wp++ = tbl[((obj[0] & 3) << 4) + (obj[1] >> 4)];
      *wp++ = tbl[(obj[1] & 0xf) << 2];
      *wp++ = '=';
      break;
    default:
      *wp++ = tbl[obj[0] >> 2];
      *wp++ = tbl[((obj[0] & 3) << 4) + (obj[1] >> 4)];
      *wp++ = tbl[((obj[1] & 0xf) << 2) + (obj[2] >> 6)];
      *wp++ = tbl[obj[2] & 0x3f];
      break;
    }
    obj += 3;
  }
  *wp = '\0';
  return buf;
}


/* Decode a string encoded with Base64 encoding. */
char *tcbasedecode(const char *str, int *sp){
  assert(str && sp);
  int cnt = 0;
  int bpos = 0;
  int eqcnt = 0;
  int len = strlen(str);
  unsigned char *obj = tcmalloc(len + 4);
  unsigned char *wp = obj;
  while(bpos < len && eqcnt == 0){
    int bits = 0;
    int i;
    for(i = 0; bpos < len && i < 4; bpos++){
      if(str[bpos] >= 'A' && str[bpos] <= 'Z'){
        bits = (bits << 6) | (str[bpos] - 'A');
        i++;
      } else if(str[bpos] >= 'a' && str[bpos] <= 'z'){
        bits = (bits << 6) | (str[bpos] - 'a' + 26);
        i++;
      } else if(str[bpos] >= '0' && str[bpos] <= '9'){
        bits = (bits << 6) | (str[bpos] - '0' + 52);
        i++;
      } else if(str[bpos] == '+'){
        bits = (bits << 6) | 62;
        i++;
      } else if(str[bpos] == '/'){
        bits = (bits << 6) | 63;
        i++;
      } else if(str[bpos] == '='){
        bits <<= 6;
        i++;
        eqcnt++;
      }
    }
    if(i == 0 && bpos >= len) continue;
    switch(eqcnt){
    case 0:
      *wp++ = (bits >> 16) & 0xff;
      *wp++ = (bits >> 8) & 0xff;
      *wp++ = bits & 0xff;
      cnt += 3;
      break;
    case 1:
      *wp++ = (bits >> 16) & 0xff;
      *wp++ = (bits >> 8) & 0xff;
      cnt += 2;
      break;
    case 2:
      *wp++ = (bits >> 16) & 0xff;
      cnt += 1;
      break;
    }
  }
  obj[cnt] = '\0';
  *sp = cnt;
  return (char *)obj;
}


/* Encode a serial object with quoted-printable encoding. */
char *tcquoteencode(const char *ptr, int size){
  assert(ptr && size >= 0);
  const unsigned char *rp = (const unsigned char *)ptr;
  char *buf = tcmalloc(size * 3 + 1);
  char *wp = buf;
  int cols = 0;
  for(int i = 0; i < size; i++){
    if(rp[i] == '=' || (rp[i] < 0x20 && rp[i] != '\r' && rp[i] != '\n' && rp[i] != '\t') ||
       rp[i] > 0x7e){
      wp += sprintf(wp, "=%02X", rp[i]);
      cols += 3;
    } else {
      *(wp++) = rp[i];
      cols++;
    }
  }
  *wp = '\0';
  return buf;
}


/* Decode a string encoded with quoted-printable encoding. */
char *tcquotedecode(const char *str, int *sp){
  assert(str && sp);
  char *buf = tcmalloc(strlen(str) + 1);
  char *wp = buf;
  for(; *str != '\0'; str++){
    if(*str == '='){
      str++;
      if(*str == '\0'){
        break;
      } else if(str[0] == '\r' && str[1] == '\n'){
        str++;
      } else if(str[0] != '\n' && str[0] != '\r'){
        if(*str >= 'A' && *str <= 'Z'){
          *wp = (*str - 'A' + 10) * 16;
        } else if(*str >= 'a' && *str <= 'z'){
          *wp = (*str - 'a' + 10) * 16;
        } else {
          *wp = (*str - '0') * 16;
        }
        str++;
        if(*str == '\0') break;
        if(*str >= 'A' && *str <= 'Z'){
          *wp += *str - 'A' + 10;
        } else if(*str >= 'a' && *str <= 'z'){
          *wp += *str - 'a' + 10;
        } else {
          *wp += *str - '0';
        }
        wp++;
      }
    } else {
      *wp = *str;
      wp++;
    }
  }
  *wp = '\0';
  *sp = wp - buf;
  return buf;
}


/* Encode a string with MIME encoding. */
char *tcmimeencode(const char *str, const char *encname, bool base){
  assert(str && encname);
  int len = strlen(str);
  char *buf = tcmalloc(len * 3 + strlen(encname) + 16);
  char *wp = buf;
  wp += sprintf(wp, "=?%s?%c?", encname, base ? 'B' : 'Q');
  char *enc = base ? tcbaseencode(str, len) : tcquoteencode(str, len);
  wp += sprintf(wp, "%s?=", enc);
  free(enc);
  return buf;
}


/* Decode a string encoded with MIME encoding. */
char *tcmimedecode(const char *str, char *enp){
  assert(str);
  if(enp) sprintf(enp, "US-ASCII");
  char *buf = tcmalloc(strlen(str) + 1);
  char *wp = buf;
  while(*str != '\0'){
    if(tcstrfwm(str, "=?")){
      str += 2;
      const char *pv = str;
      const char *ep = strchr(str, '?');
      if(!ep) continue;
      if(enp && ep - pv < TC_ENCBUFSIZ){
        memcpy(enp, pv, ep - pv);
        enp[ep-pv] = '\0';
      }
      pv = ep + 1;
      bool quoted = (*pv == 'Q' || *pv == 'q');
      if(*pv != '\0') pv++;
      if(*pv != '\0') pv++;
      if(!(ep = strchr(pv, '?'))) continue;
      char *tmp = tcmemdup(pv, ep - pv);
      int len;
      char *dec = quoted ? tcquotedecode(tmp, &len) : tcbasedecode(tmp, &len);
      wp += sprintf(wp, "%s", dec);
      free(dec);
      free(tmp);
      str = ep + 1;
      if(*str != '\0') str++;
    } else {
      *(wp++) = *str;
      str++;
    }
  }
  *wp = '\0';
  return buf;
}


/* Compress a serial object with deflate encoding. */
char *tcdeflate(const char *ptr, int size, int *sp){
  assert(ptr && sp);
  if(!_tc_deflate) return NULL;
  return _tc_deflate(ptr, size, sp, _TC_ZMZLIB);
}


/* Decompress a serial object compressed with deflate encoding. */
char *tcinflate(const char *ptr, int size, int *sp){
  assert(ptr && size >= 0);
  if(!_tc_inflate) return NULL;
  return _tc_inflate(ptr, size, sp, _TC_ZMZLIB);
}


/* Compress a serial object with GZIP encoding. */
char *tcgzipencode(const char *ptr, int size, int *sp){
  assert(ptr && sp);
  if(!_tc_deflate) return NULL;
  return _tc_deflate(ptr, size, sp, _TC_ZMGZIP);
}


/* Decompress a serial object compressed with GZIP encoding. */
char *tcgzipdecode(const char *ptr, int size, int *sp){
  assert(ptr && size >= 0);
  if(!_tc_inflate) return NULL;
  return _tc_inflate(ptr, size, sp, _TC_ZMGZIP);
}


/* Get the CRC32 checksum of a serial object. */
unsigned int tcgetcrc(const char *ptr, int size){
  assert(ptr && size >= 0);
  if(!_tc_getcrc) return 0;
  return _tc_getcrc(ptr, size);
}


/* Escape meta characters in a string with the entity references of XML. */
char *tcxmlescape(const char *str){
  assert(str);
  const char *rp = str;
  int bsiz = 0;
  while(*rp != '\0'){
    switch(*rp){
    case '&':
      bsiz += 5;
      break;
    case '<':
      bsiz += 4;
      break;
    case '>':
      bsiz += 4;
      break;
    case '"':
      bsiz += 6;
      break;
    default:
      bsiz++;
      break;
    }
    rp++;
  }
  char *buf = tcmalloc(bsiz + 1);
  char *wp = buf;
  while(*str != '\0'){
    switch(*str){
    case '&':
      memcpy(wp, "&amp;", 5);
      wp += 5;
      break;
    case '<':
      memcpy(wp, "&lt;", 4);
      wp += 4;
      break;
    case '>':
      memcpy(wp, "&gt;", 4);
      wp += 4;
      break;
    case '"':
      memcpy(wp, "&quot;", 6);
      wp += 6;
      break;
    default:
      *(wp++) = *str;
      break;
    }
    str++;
  }
  *wp = '\0';
  return buf;
}


/* Unescape entity references in a string of XML. */
char *tcxmlunescape(const char *str){
  assert(str);
  char *buf = tcmalloc(strlen(str) + 1);
  char *wp = buf;
  while(*str != '\0'){
    if(*str == '&'){
      if(tcstrfwm(str, "&amp;")){
        *(wp++) = '&';
        str += 5;
      } else if(tcstrfwm(str, "&lt;")){
        *(wp++) = '<';
        str += 4;
      } else if(tcstrfwm(str, "&gt;")){
        *(wp++) = '>';
        str += 4;
      } else if(tcstrfwm(str, "&quot;")){
        *(wp++) = '"';
        str += 6;
      } else {
        *(wp++) = *(str++);
      }
    } else {
      *(wp++) = *(str++);
    }
  }
  *wp = '\0';
  return buf;
}


/* Split an XML string into tags and text sections. */
TCLIST *tcxmlbreak(const char *str){
  assert(str);
  TCLIST *list = tclistnew();
  int i = 0;
  int pv = 0;
  bool tag = false;
  char *ep;
  while(true){
    if(str[i] == '\0'){
      if(i > pv) tclistpush(list, str + pv, i - pv);
      break;
    } else if(!tag && str[i] == '<'){
      if(str[i+1] == '!' && str[i+2] == '-' && str[i+3] == '-'){
        if(i > pv) tclistpush(list, str + pv, i - pv);
        if((ep = strstr(str + i, "-->")) != NULL){
          tclistpush(list, str + i, ep - str - i + 3);
          i = ep - str + 2;
          pv = i + 1;
        }
      } else if(str[i+1] == '!' && str[i+2] == '[' && tcstrifwm(str + i, "<![CDATA[")){
        if(i > pv) tclistpush(list, str + pv, i - pv);
        if((ep = strstr(str + i, "]]>")) != NULL){
          i += 9;
          TCXSTR *xstr = tcxstrnew();
          while(str + i < ep){
            if(str[i] == '&'){
              tcxstrcat(xstr, "&amp;", 5);
            } else if(str[i] == '<'){
              tcxstrcat(xstr, "&lt;", 4);
            } else if(str[i] == '>'){
              tcxstrcat(xstr, "&gt;", 4);
            } else {
              tcxstrcat(xstr, str + i, 1);
            }
            i++;
          }
          if(tcxstrsize(xstr) > 0) tclistpush(list, tcxstrptr(xstr), tcxstrsize(xstr));
          tcxstrdel(xstr);
          i = ep - str + 2;
          pv = i + 1;
        }
      } else {
        if(i > pv) tclistpush(list, str + pv, i - pv);
        tag = true;
        pv = i;
      }
    } else if(tag && str[i] == '>'){
      if(i > pv) tclistpush(list, str + pv, i - pv + 1);
      tag = false;
      pv = i + 1;
    }
    i++;
  }
  return list;
}


/* Get the map of attributes of an XML tag. */
TCMAP *tcxmlattrs(const char *str){
  assert(str);
  TCMAP *map = tcmapnew2(TC_XMLATBNUM);
  const unsigned char *rp = (unsigned char *)str;
  while(*rp == '<' || *rp == '/' || *rp == '?' || *rp == '!' || *rp == ' '){
    rp++;
  }
  const unsigned char *key = rp;
  while(*rp > 0x20 && *rp != '/' && *rp != '>'){
    rp++;
  }
  tcmapputkeep(map, "", 0, (char *)key, rp - key);
  while(*rp != '\0'){
    while(*rp != '\0' && (*rp <= 0x20 || *rp == '/' || *rp == '?' || *rp == '>')){
      rp++;
    }
    key = rp;
    while(*rp > 0x20 && *rp != '/' && *rp != '>' && *rp != '='){
      rp++;
    }
    int ksiz = rp - key;
    while(*rp != '\0' && (*rp == '=' || *rp <= 0x20)){
      rp++;
    }
    const unsigned char *val;
    int vsiz;
    if(*rp == '"'){
      rp++;
      val = rp;
      while(*rp != '\0' && *rp != '"'){
        rp++;
      }
      vsiz = rp - val;
    } else if(*rp == '\''){
      rp++;
      val = rp;
      while(*rp != '\0' && *rp != '\''){
        rp++;
      }
      vsiz = rp - val;
    } else {
      val = rp;
      while(*rp > 0x20 && *rp != '"' && *rp != '\'' && *rp != '>'){
        rp++;
      }
      vsiz = rp - val;
    }
    if(*rp != '\0') rp++;
    if(ksiz > 0){
      char *copy = tcmemdup(val, vsiz);
      char *raw = tcxmlunescape(copy);
      tcmapputkeep(map, (char *)key, ksiz, raw, strlen(raw));
      free(raw);
      free(copy);
    }
  }
  return map;
}



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


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



/* END OF FILE */
