


#ifndef _TCHDB_H                         // duplication check
#define _TCHDB_H


#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <tcutil.h>



/*************************************************************************************************
 * API
 *************************************************************************************************/

typedef struct {                         // type of structure for a hash database
  uint8_t type;                          // database type
  uint8_t flags;                         // additional flags
  uint64_t bnum;                         // number of the bucket array
  uint8_t apow;                          // power of record alignment
  uint8_t fpow;                          // power of free block pool number
  uint8_t opts;                          // options
  char *path;                            // path of the database file
  int fd;                                // file descriptor of the database file
  uint32_t omode;                        // connection mode
  uint64_t rnum;                         // number of the records
  uint64_t fsiz;                         // size of the database file
  uint64_t frec;                         // offset of the first record
  uint64_t lrec;                         // offset of the last record
  uint64_t iter;                         // offset of the iterator
  char *map;                             // pointer to the mapped memory
  uint64_t msiz;                         // size of the mapped memory
  uint32_t *ba32;                        // 32-bit bucket array
  uint64_t *ba64;                        // 64-bit bucket array
  uint32_t align;                        // record alignment
  uint32_t runit;                        // record reading unit
  int32_t fbpmax;                        // maximum number of the free block pool
  int32_t fbpsiz;                        // size of the free block pool
  void *fbpool;                          // free block pool
  int32_t fbpnum;                        // number of free block pool
  int32_t fbpmis;                        // number of missing retrieval of free block pool
  TCXSTR *drpool;                        // delayed record pool
  TCXSTR *drpdef;                        // deferred records of delayed record pool
  int ecode;                             // last happened error code
  bool fatal;                            // whether a fatal error occured
  uint64_t inode;                        // inode number
  time_t mtime;                          // modification time
  int dbgfd;                             // file descriptor for debugging
  int64_t cnt_writerec;                  // tesing counter for record write times
  int64_t cnt_reuserec;                  // tesing counter for record reuse times
  int64_t cnt_moverec;                   // tesing counter for record move times
  int64_t cnt_readrec;                   // tesing counter for record read times
  int64_t cnt_searchfbp;                 // tesing counter for FBP search times
  int64_t cnt_insertfbp;                 // tesing counter for FBP insert times
  int64_t cnt_splicefbp;                 // tesing counter for FBP splice times
  int64_t cnt_dividefbp;                 // tesing counter for FBP divide times
  int64_t cnt_mergefbp;                  // tesing counter for FBP merge times
  int64_t cnt_reducefbp;                 // tesing counter for FBP reduce times
  int64_t cnt_appenddrp;                 // tesing counter for DRP append times
  int64_t cnt_deferdrp;                  // tesing counter for DRP defer times
  int64_t cnt_flushdrp;                  // tesing counter for DRP flush times
} TCHDB;

enum {                                   // enumeration for error codes
  TCESUCCESS,                            // success
  TCEINVALID,                            // invalid operation												
  TCENOFILE,                             // file not found													
  TCENOPERM,                             // no permission													
  TCEMETA,                               // invalid meta data
  TCERHEAD,                              // invalid record header
  TCEOPEN,                               // open error													
  TCECLOSE,                              // close error
  TCETRUNC,                              // trunc error
  TCESYNC,                               // sync error
  TCESTAT,                               // stat error								
  TCESEEK,                               // seek error
  TCEREAD,                               // read error
  TCEWRITE,                              // write error
  TCEMMAP,                               // mmap error
  TCELOCK,                               // lock error							
  TCEUNLINK,                             // unlink error
  TCERENAME,                             // rename error
  TCEMKDIR,                              // mkdir error
  TCERMDIR,                              // rmdir error
  TCEKEEP,                               // existing record
  TCENOREC,                              // no record found
  TCEMISC                                // miscellaneous error
};

enum {                                   // enumeration for database type
  HDBTHASH,                              // hash table
  HDBTBTREE                              // B+ tree
};

enum {                                   // enumeration for additional flags
  HDBFOPEN = 1 << 0,                     // whether opened
  HDBFFATAL = 1 << 1                     // whetehr with fatal error
};

enum {                                   // enumeration for tuning options
  HDBTLARGE = 1 << 0,                    // use 64-bit bucket array						
  HDBTDEFLATE = 1 << 1,                  // compress each record with deflate
};

enum {                                   // enumeration for open modes
  HDBOREADER = 1 << 0,                   // open as a reader
  HDBOWRITER = 1 << 1,                   // open as a writer						
  HDBOCREAT = 1 << 2,                    // writer creating						
  HDBOTRUNC = 1 << 3,                    // writer truncating						
  HDBONOLCK = 1 << 4,                    // open without locking					
  HDBOLCKNB = 1 << 5                     // lock without blocking					
};		


/* Get the message string corresponding to an error code.
   `ecode' specifies the error code. */
const char *tchdberrmsg(int ecode);


/* Create a hash database object.
   The return value is the new hash database object. */
TCHDB *tchdbnew(void);


/* Delete a hash database object.
   `hdb' specifies the hash database object.
   If the database is not closed, it is closed implicitly.  Note that the deleted object and its
   derivatives can not be used anymore. */
void tchdbdel(TCHDB *hdb);


/* Get the last happened error code of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the last happened error code. */
int tchdbecode(TCHDB *hdb);



/* Open a database file and connect a hash database object.
   `hdb' specifies the hash database object.
   `path' specifies the path of the database file.
   `omode' specifies the connection mode: `HDBOWRITER' as a writer, `HDBOREADER' as a reader.
   If the mode is `HDBOWRITER', the following may be added by bitwise or: `HDBOCREAT', which
   means it creates a new database if not exist, `HDBOTRUNC', which means it creates a new database
   regardless if one exists.  Both of `HDBOREADER' and `HDBOWRITER' can be added to by
   bitwise or: `HDBONOLOCK', which means it opens the database file without file locking, or
   `HDBOLOCKNB', which means locking is performed without blocking.
   If successful, the return value is true, else, it is false. */
bool tchdbopen(TCHDB *hdb, const char *path, int omode);


/* Close a database object.
   `hdb' specifies the hash database object.
   If successful, the return value is true, else, it is false.
   Updating a database is assured to be written when the database is closed.  If a writer opens
   a database but does not close it appropriately, the database will be broken. */
bool tchdbclose(TCHDB *hdb);


/* Store a record into a hash database object.
   `hdb' specifies the hash database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tchdbput(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz);


/* Store a string record into a hash database object.
   `hdb' specifies the hash database object connected as a writer.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   `over' specifies whether the value of the duplicated record is overwritten or not.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tchdbput2(TCHDB *hdb, const char *kstr, const char *vstr);





/* Retrieve a record in a hash database object.
   `hdb' specifies the hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `sp' specifies the pointer to the variable to which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the
   corresponding record.  `NULL' is returned when no record corresponds.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
char *tchdbget(TCHDB *hdb, const char *kbuf, int ksiz, int *sp);


/* Retrieve a string record in a hash database object.
   `hdb' specifies the hash database object.
   `kstr' specifies the string of the key.
   If successful, the return value is the string of the value of the corresponding record.
   `NULL' is returned when no record corresponds.  Because the region of the return value is
   allocated with the `malloc' call, it should be released with the `free' call when it is no
   longer in use. */
char *tchdbget2(TCHDB *hdb, const char *kstr);





/* Initialize the iterator of a hash database object.
   `hdb' specifies the hash database object.
   If successful, the return value is true, else, it is false.
   The iterator is used in order to access the key of every record stored in a database. */
bool tchdbiterinit(TCHDB *hdb);


/* Get the next key of the iterator of a hash database object.
   `hdb' specifies the hash database object.
   `sp' specifies the pointer to the variable to which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the next key, else, it is
   `NULL'.  `NULL' is returned when no record is to be get out of the iterator.
   Because an additional zero code is appended at the end of the region of the return value, the
   return value can be treated as a character string.  Because the region of the return value is
   allocated with the `malloc' call, it should be released with the `free' call when it is no
   longer in use.  It is possible to access every record by iteration of calling this function.
   It is allowed to update or remove records whose keys are fetched while the iteration.
   However, it is not assured if updating the database is occurred while the iteration.  Besides,
   the order of this traversal access method is arbitrary, so it is not assured that the order of
   storing matches the one of the traversal access. */
char *tchdbiternext(TCHDB *hdb, int *sp);


/* Get the next key string of the iterator of a hash database object.
   `hdb' specifies the hash database object.
   If successful, the return value is the string the next key, else, it is `NULL'.  `NULL' is
   returned when no record is to be get out of the iterator.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use.  It is possible to access every
   record by iteration of calling this function.  However, it is not assured if updating the
   database is occurred while the iteration.  Besides, the order of this traversal access method
   is arbitrary, so it is not assured that the order of storing matches the one of the traversal
   access. */
char *tchdbiternext2(TCHDB *hdb);






/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


/* Set the error code of a hash database object.
   `hdb' specifies the hash database object.
   `ecode' specifies the error code.
   `file' specifies the file name of the code.
   `line' specifies the line number of the code.
   `func' specifies the function name of the code. */
void tchdbsetecode(TCHDB *hdb, int ecode, const char *filename, int line, const char *func);




/* Synchronize updating contents on memory.
   `hdb' specifies the hash database object connected as a writer.
   `phys' specifies whether to synchronize physically.
   If successful, the return value is true, else, it is false. */
bool tchdbmemsync(TCHDB *hdb, bool phys);




#endif                                   // duplication check


// END OF FILE
