


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


/* Set the tuning parameters of a hash database object.
   `hdb' specifies the hash database object.
   `bnum' specifies the number of elements of the bucket array.  If it is not more than 0, the
   default value is specified.  The default value is 16381.  Suggested size of the bucket array
   is about from 0.5 to 4 times of the number of all records to be stored.
   `apow' specifies the size of record alignment by power of 2.  If it is negative, the default
   value is specified.  The default value is 4 standing for 2^4=16.
   `fpow' specifies the maximum number of elements of the free block pool by power of 2.  If it
   is negative, the default value is specified.  The default value is 10 standing for 2^10=1024.
   `opts' specifies options by bitwise or: `HDBTLARGE' specifies that the size of the database
   can be larger than 2GB by using 64-bit bucket array, `HDBTDEFLATE' specifies that each record
   is compressed with deflate encoding.
   If successful, the return value is true, else, it is false.
   Note that the tuning parameters of the database should be set before the database is opened. */
bool tchdbtune(TCHDB *hdb, int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts);


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


/* Store a new record into a hash database object.
   `hdb' specifies the hash database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tchdbputkeep(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz);


/* Store a new string record into a hash database object.
   `hdb' specifies the hash database object connected as a writer.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tchdbputkeep2(TCHDB *hdb, const char *kstr, const char *vstr);


/* Concatenate a value at the end of the existing record in a hash database object.
   `hdb' specifies the hash database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tchdbputcat(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz);


/* Concatenate a stirng value at the end of the existing record in a hash database object.
   `hdb' specifies the hash database object connected as a writer.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tchdbputcat2(TCHDB *hdb, const char *kstr, const char *vstr);


/* Store a record into a hash database object in asynchronous fashion.
   `hdb' specifies the hash database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten.  Records passed to
   this function are accumulated into the inner buffer and wrote into the file at a blast. */
bool tchdbputasync(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz);


/* Store a string record into a hash database object in asynchronous fashion.
   `hdb' specifies the hash database object connected as a writer.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten.  Records passed to
   this function are accumulated into the inner buffer and wrote into the file at a blast. */
bool tchdbputasync2(TCHDB *hdb, const char *kstr, const char *vstr);


/* Remove a record of a hash database object.
   `hdb' specifies the hash database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false. */
bool tchdbout(TCHDB *hdb, const char *kbuf, int ksiz);


/* Remove a string record of a hash database object.
   `hdb' specifies the hash database object connected as a writer.
   `kstr' specifies the string of the key.
   If successful, the return value is true, else, it is false. */
bool tchdbout2(TCHDB *hdb, const char *kstr);


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


/* Retrieve a record in a hash database object and write the value into a buffer.
   `hdb' specifies the hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the buffer into which the value of the corresponding record is
   written.
   `max' specifies the size of the buffer.
   If successful, the return value is the size of the written data, else, it is -1.  -1 is
   returned when no record corresponds to the specified key.
   Note that an additional zero code is not appended at the end of the region of the writing
   buffer. */
int tchdbget3(TCHDB *hdb, const char *kbuf, int ksiz, char *vbuf, int max);


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


/* Get the next extensible objects of the iterator of a hash database object.
   `hdb' specifies the hash database object.
   `kxstr' specifies the object into which the next key is wrote down.
   `vxstr' specifies the object into which the next value is wrote down.
   If successful, the return value is true, else, it is false.  False is returned when no record
   is to be get out of the iterator. */
bool tchdbiternext3(TCHDB *hdb, TCXSTR *kxstr, TCXSTR *vxstr);


/* Synchronize updated contents of a hash database object with the file and the device.
   `hdb' specifies the hash database object connected as a writer.
   If successful, the return value is true, else, it is false.
   This function is useful when another process connects the same database file. */
bool tchdbsync(TCHDB *hdb);


/* Optimize the file of a hash database object.
   `hdb' specifies the hash database object connected as a writer.
   `bnum' specifies the number of elements of the bucket array.  If it is not more than 0, the
   default value is specified.  The default value is two times of the number of records.
   `apow' specifies the size of record alignment by power of 2.  If it is negative, the current
   setting is not changed.
   `fpow' specifies the maximum number of elements of the free block pool by power of 2.  If it
   is negative, the current setting is not changed.
   `opts' specifies options by bitwise or: `HDBTLARGE' specifies that the size of the database
   can be larger than 2GB by using 64-bit bucket array, `HDBTDEFLATE' specifies that each record
   is compressed with deflate encoding.  If it is `UINT8_MAX', the default setting is not changed.
   If successful, the return value is true, else, it is false.
   This function is useful to reduce the size of the database file with data fragmentation by
   successive updating. */
bool tchdboptimize(TCHDB *hdb, int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts);


/* Get the file path of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the path of the database file or `NULL' if the object does not connect to
   any database file. */
const char *tchdbpath(TCHDB *hdb);


/* Get the number of elements of the bucket array of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the number of elements of the bucket array or 0 if the object does not
   connect to any database file. */
uint64_t tchdbbnum(TCHDB *hdb);


/* Get the record alignment of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the record alignment a hash database object or 0 if the object does not
   connect to any database file. */
uint32_t tchdbalign(TCHDB *hdb);


/* Get the maximum number of the free block pool of a a hash database object.
   `hdb' specifies the hash database object.
   The return value is the maximum number of the free block pool or 0 if the object does not
   connect to any database file. */
uint32_t tchdbfbpmax(TCHDB *hdb);


/* Get the number of records of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the number of records or 0 if the object does not connect to any database
   file. */
uint64_t tchdbrnum(TCHDB *hdb);


/* Get the size of the database file of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the size of the database file or 0 if the object does not connect to any
   database file. */
uint64_t tchdbfsiz(TCHDB *hdb);


/* Get the inode number of the database file of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the inode number of the database file or 0 the object does not connect to
   any database file. */
uint64_t tchdbinode(TCHDB *hdb);


/* Get the modification time of the database file of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the inode number of the database file or 0 the object does not connect to
   any database file. */
time_t tchdbmtime(TCHDB *hdb);



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


/* Set the type of a hash database object.
   `hdb' specifies the hash database object.
   `type' specifies the database type. */
void tchdbsettype(TCHDB *hdb, uint8_t type);


/* Set the file descriptor for debugging output.
   `hdb' specifies the hash database object.
   `fd' specifies the file descriptor for debugging output. */
void tchdbsetdbgfd(TCHDB *hdb, int fd);


/* Synchronize updating contents on memory.
   `hdb' specifies the hash database object connected as a writer.
   `phys' specifies whether to synchronize physically.
   If successful, the return value is true, else, it is false. */
bool tchdbmemsync(TCHDB *hdb, bool phys);


/* Get the database type of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the database type. */
uint8_t tchdbtype(TCHDB *hdb);


/* Get the additional flags of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the additional flags. */
uint8_t tchdbflags(TCHDB *hdb);


/* Get the options of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the options. */
uint8_t tchdbopts(TCHDB *hdb);


/* Get the number of used elements of the bucket array of a hash database object.
   `hdb' specifies the hash database object.
   The return value is the number of used elements of the bucket array or 0 if the object does not
   connect to any database file. */
uint64_t tchdbbnumused(TCHDB *hdb);



#endif                                   // duplication check


// END OF FILE
