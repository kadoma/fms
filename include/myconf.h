
#ifndef _MYCONF_H                        // duplication check
#define _MYCONF_H



/*************************************************************************************************
 * common settings
 *************************************************************************************************/


#if defined(NDEBUG)
#define TCDODEBUG(TC_expr) \
  do { \
  } while(false)
#else
#define TCDODEBUG(TC_expr) \
  do { \
    TC_expr; \
  } while(false)
#endif

#define TCSWAB16(TC_num) \
  ( \
   ((TC_num & 0x00ffU) << 8) | \
   ((TC_num & 0xff00U) >> 8) \
  )

#define TCSWAB32(TC_num) \
  ( \
   ((TC_num & 0x000000ffUL) << 24) | \
   ((TC_num & 0x0000ff00UL) << 8) | \
   ((TC_num & 0x00ff0000UL) >> 8) | \
   ((TC_num & 0xff000000UL) >> 24) \
  )

#define TCSWAB64(TC_num) \
  ( \
   ((TC_num & 0x00000000000000ffULL) << 56) | \
   ((TC_num & 0x000000000000ff00ULL) << 40) | \
   ((TC_num & 0x0000000000ff0000ULL) << 24) | \
   ((TC_num & 0x00000000ff000000ULL) << 8) | \
   ((TC_num & 0x000000ff00000000ULL) >> 8) | \
   ((TC_num & 0x0000ff0000000000ULL) >> 24) | \
   ((TC_num & 0x00ff000000000000ULL) >> 40) | \
   ((TC_num & 0xff00000000000000ULL) >> 56) \
  )

#if defined(_MYBIGEND) || defined(_MYSWAB)
#define TCBIGEND       1
#define TCHTOIS(TC_num)   TCSWAB16(TC_num)
#define TCHTOIL(TC_num)   TCSWAB32(TC_num)
#define TCHTOILL(TC_num)  TCSWAB64(TC_num)
#define TCITOHS(TC_num)   TCSWAB16(TC_num)
#define TCITOHL(TC_num)   TCSWAB32(TC_num)
#define TCITOHLL(TC_num)  TCSWAB64(TC_num)
#else
#define TCBIGEND       0
#define TCHTOIS(TC_num)   (TC_num)
#define TCHTOIL(TC_num)   (TC_num)
#define TCHTOILL(TC_num)  (TC_num)
#define TCITOHS(TC_num)   (TC_num)
#define TCITOHL(TC_num)   (TC_num)
#define TCITOHLL(TC_num)  (TC_num)
#endif

#if defined(_MYNOZLIB)
#define TCUSEZLIB      0
#else
#define TCUSEZLIB      1
#endif

#define sizeof(a)      ((int)sizeof(a))

int _tc_dummyfunc(void);



/*************************************************************************************************
 * general headers
 *************************************************************************************************/


#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <wchar.h>
#include <wctype.h>
#include <iso646.h>

#include <complex.h>
#include <fenv.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <tgmath.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/times.h>
#include <fcntl.h>
#include <dirent.h>



/*************************************************************************************************
 * notation of filesystems
 *************************************************************************************************/


#define MYPATHCHR       '/'
#define MYPATHSTR       "/"
#define MYEXTCHR        '.'
#define MYEXTSTR        "."
#define MYCDIRSTR       "."
#define MYPDIRSTR       ".."



/*************************************************************************************************
 * for ZLIB
 *************************************************************************************************/


enum {
  _TC_ZMZLIB,
  _TC_ZMRAW,
  _TC_ZMGZIP
};


extern char *(*_tc_deflate)(const char *, int, int *, int);

extern char *(*_tc_inflate)(const char *, int, int *, int);

extern unsigned int (*_tc_getcrc)(const char *, int);



/*************************************************************************************************
 * utilities for implementation
 *************************************************************************************************/


#define TC_NUMBUFSIZ   32                // size of a buffer for a number
#define TC_VNUMBUFSIZ  12                // size of a buffer for variable length number

/* set a buffer for a variable length number */
#define TC_SETVNUMBUF(TC_len, TC_buf, TC_num) \
  do { \
    int _TC_num = (TC_num); \
    if(_TC_num == 0){ \
      ((signed char *)(TC_buf))[0] = 0; \
      (TC_len) = 1; \
    } else { \
      (TC_len) = 0; \
      while(_TC_num > 0){ \
        int _TC_rem = _TC_num & 0x7f; \
        _TC_num >>= 7; \
        if(_TC_num > 0){ \
          ((signed char *)(TC_buf))[(TC_len)] = -_TC_rem - 1; \
        } else { \
          ((signed char *)(TC_buf))[(TC_len)] = _TC_rem; \
        } \
        (TC_len)++; \
      } \
    } \
  } while(false)

/* set a buffer for a variable length number of 64-bit */
#define TC_SETVNUMBUF64(TC_len, TC_buf, TC_num) \
  do { \
    long long int _TC_num = (TC_num); \
    if(_TC_num == 0){ \
      ((signed char *)(TC_buf))[0] = 0; \
      (TC_len) = 1; \
    } else { \
      (TC_len) = 0; \
      while(_TC_num > 0){ \
        int _TC_rem = _TC_num & 0x7f; \
        _TC_num >>= 7; \
        if(_TC_num > 0){ \
          ((signed char *)(TC_buf))[(TC_len)] = -_TC_rem - 1; \
        } else { \
          ((signed char *)(TC_buf))[(TC_len)] = _TC_rem; \
        } \
        (TC_len)++; \
      } \
    } \
  } while(false)

/* read a variable length buffer */
#define TC_READVNUMBUF(TC_buf, TC_num, TC_step) \
  do { \
    TC_num = 0; \
    int _TC_base = 1; \
    int _TC_i = 0; \
    while(true){ \
      if(((signed char *)(TC_buf))[_TC_i] >= 0){ \
        TC_num += ((signed char *)(TC_buf))[_TC_i] * _TC_base; \
        break; \
      } \
      TC_num += _TC_base * (((signed char *)(TC_buf))[_TC_i] + 1) * -1; \
      _TC_base *= 128; \
      _TC_i++; \
    } \
    (TC_step) = _TC_i + 1; \
  } while(false)

/* read a variable length buffer */
#define TC_READVNUMBUF64(TC_buf, TC_num, TC_step) \
  do { \
    TC_num = 0; \
    long long int _TC_base = 1; \
    int _TC_i = 0; \
    while(true){ \
      if(((signed char *)(TC_buf))[_TC_i] >= 0){ \
        TC_num += ((signed char *)(TC_buf))[_TC_i] * _TC_base; \
        break; \
      } \
      TC_num += _TC_base * (((signed char *)(TC_buf))[_TC_i] + 1) * -1; \
      _TC_base *= 128; \
      _TC_i++; \
    } \
    (TC_step) = _TC_i + 1; \
  } while(false)



#endif                                   // duplication check


// END OF FILE
