


#include "tchdb.h"
#include "tcutil.h"
#include "myconf.h"

#define HDBFILEMODE    00644             // permission of created files						
#define HDBIOBUFSIZ    8192              // size of an I/O buffer

#define HDBMAGICNUM    "Inspur	i-db"    // magic number on environments of big endian
#define HDBHEADSIZ     128               // size of the reagion of the header								
#define HDBTYPEOFF     32                // offset of the region for the database type
#define HDBFLAGSOFF    33                // offset of the region for the additional flags
#define HDBAPOWOFF     34                // offset of the region for the alignment power
#define HDBFPOWOFF     35                // offset of the region for the free block pool power
#define HDBOPTSOFF     36                // offset of the region for the options
#define HDBBNUMOFF     40                // offset of the region for the bucket number
#define HDBRNUMOFF     48                // offset of the region for the record number
#define HDBFSIZOFF     56                // offset of the region for the file size
#define HDBFRECOFF     64                // offset of the region for the first record offset
#define HDBOPAQUEOFF   80                // offset of the region for the opaque field

#define HDBDEFBNUM     16381             // default bucket number
#define HDBDEFAPOW     4                 // default alignment power
#define HDBMAXAPOW     16                // maximum alignment power
#define HDBDEFFPOW     10                // default free block pool power
#define HDBMAXFPOW     20                // maximum free block pool power
#define HDBMINRUNIT    48                // minimum record reading unit
#define HDBMAXHSIZ     32                // minimum record header size
#define HDBFBPBSIZ     64                // base region size of the free block pool					
#define HDBFBPESIZ     4                 // size of each region of the free block pool
#define HDBFBPMGFREQ   256               // frequency to merge the free block pool
#define HDBDRPUNIT     65536             // unit size of the delayed record pool
#define HDBDRPLAT      2048              // latitude size of the delayed record pool

typedef struct {                         // type of structure for a record
  uint64_t off;                          // offset of the record
  uint32_t rsiz;                         // size of the whole record
  uint8_t magic;                         // magic number
  uint8_t hash;                          // second hash value
  uint64_t left;                         // offset of the left child record
  uint64_t right;                        // offset of the right child record
  uint32_t ksiz;                         // size of the key
  uint32_t vsiz;                         // size of the value
  uint16_t psiz;                         // size of the padding
  const char *kbuf;                      // pointer to the key
  const char *vbuf;                      // pointer to the value
  uint64_t boff;                         // offset of the body
  char *bbuf;                            // buffer of the body
} TCHREC;

typedef struct {                         // type of structure for a free block
  uint64_t off;                          // offset of the block
  uint32_t rsiz;                         // size of the block
} HDBFB;

enum {                                   // enumeration for magic numbers
  HDBMAGICREC = 0xc8,                    // for data block
  HDBMAGICFB = 0xb0                      // for free block
};

enum {                                   // enumeration for duplication behavior
  HDBPDOVER,                             // overwrite an existing value
  HDBPDKEEP,                             // keep the existing value
  HDBPDCAT                               // concatenate values
};


/* private function prototypes */
static bool tclock(int fd, bool ex, bool nb);
static bool tcwrite(int fd, const void *buf, size_t size);
static bool tcread(int fd, void *buf, size_t size);

static bool tcseekwrite(TCHDB *hdb, off_t off, void *buf, size_t size);
static bool tcseekread(TCHDB *hdb, off_t off, void *buf, size_t size);
static void tcdumpmeta(TCHDB *hdb, char *hbuf);
static void tcloadmeta(TCHDB *hdb, const char *hbuf);
static void tchdbclear(TCHDB *hdb);
static int32_t tchdbpadsize(TCHDB *hdb);
static void tchdbsetflag(TCHDB *hdb, int flag, bool sign);
static uint64_t tchdbbidx(TCHDB *hdb, const char *kbuf, int ksiz);
static uint8_t tcrechash(const char *kbuf, int ksiz);
static uint64_t tchdbgetbucket(TCHDB *hdb, uint64_t bidx);
static bool tchdbsavefbp(TCHDB *hdb);
static bool tchdbloadfbp(TCHDB *hdb);
static void tcfbpsortbyoff(HDBFB *fbpool, int fbpnum);
static void tcfbpsortbyrsiz(HDBFB *fbpool, int fbpnum);
static void tchdbfbpmerge(TCHDB *hdb);
static bool tchdbfbpsearch(TCHDB *hdb, TCHREC *rec);
static bool tchdbfbpsplice(TCHDB *hdb, TCHREC *rec, uint32_t nsiz);
static void tchdbfbpinsert(TCHDB *hdb, uint64_t off, uint32_t rsiz);
static void tchdbsetbucket(TCHDB *hdb, uint64_t bidx, uint64_t off);

static bool tchdbwritefb(TCHDB *hdb, uint64_t off, uint32_t rsiz);
static bool tchdbwriterec(TCHDB *hdb, TCHREC *rec, uint64_t bidx, uint64_t entoff);
static bool tchdbreadrec(TCHDB *hdb, TCHREC *rec, char *rbuf);
static bool tchdbreadrecbody(TCHDB *hdb, TCHREC *rec);
static int tcreckeycmp(const char *abuf, int asiz, const char *bbuf, int bsiz);
static bool tchdbflushdrp(TCHDB *hdb);
static bool tchdbputimpl(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz,
                         int over);



/*************************************************************************************************
 * API
 *************************************************************************************************/

/* Get the message string corresponding to an error code. */
const char *tchdberrmsg(int ecode){
  switch(ecode){
  case TCESUCCESS: return "sucess";
  case TCEINVALID: return "invalid operation";
  case TCENOFILE: return "file not found";
  case TCENOPERM: return "no permission";
  case TCEMETA: return "invalid meta data";
  case TCERHEAD: return "invalid record header";
  case TCEOPEN: return "open error";
  case TCECLOSE: return "close error";
  case TCETRUNC: return "trunc error";
  case TCESYNC: return "sync error";
  case TCESTAT: return "stat error";
  case TCESEEK: return "seek error";
  case TCEREAD: return "read error";
  case TCEWRITE: return "write error";
  case TCEMMAP: return "mmap error";
  case TCELOCK: return "lock error";
  case TCEUNLINK: return "unlink error";
  case TCERENAME: return "rename error";
  case TCEMKDIR: return "mkdir error";
  case TCERMDIR: return "rmdir error";
  case TCEKEEP: return "existing record";
  case TCENOREC: return "no record found";
  case TCEMISC: return "miscellaneous error";
  }
  return "unknown error";
}


/* Create a hash database object. */
TCHDB *tchdbnew(void){
  TCHDB *hdb = tcmalloc(sizeof(*hdb));
  tchdbclear(hdb);
  return hdb;
}


/* Delete a hash database object. */
void tchdbdel(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd >= 0) tchdbclose(hdb);
  free(hdb);
}


/* Get the last happened error code of a hash database object. */
int tchdbecode(TCHDB *hdb){
  assert(hdb);
  return hdb->ecode;
}


/* Open a database file and connect a hash database object. */
bool tchdbopen(TCHDB *hdb, const char *path, int omode){
  assert(hdb && path);
  if(hdb->fd >= 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  int mode = O_RDONLY;
  if(omode & HDBOWRITER){
    mode = O_RDWR;
    if(omode & HDBOCREAT) mode |= O_CREAT;
  }
  int fd = open(path, mode, HDBFILEMODE);
  if(fd < 0){
    int ecode = TCEOPEN;
    switch(errno){
    case EACCES: ecode = TCENOPERM; break;
    case ENOENT: ecode = TCENOFILE; break;
    }
    tchdbsetecode(hdb, ecode, __FILE__, __LINE__, __func__);
    return NULL;
  }
  if(!(omode & HDBONOLCK)){
    if(!tclock(fd, omode & HDBOWRITER, omode & HDBOLCKNB)){
      tchdbsetecode(hdb, TCELOCK, __FILE__, __LINE__, __func__);
      close(fd);
      return false;
    }
  }
  if((omode & HDBOWRITER) && (omode & HDBOTRUNC)){
    if(ftruncate(fd, 0) == -1){
      close(fd);
      tchdbsetecode(hdb, TCETRUNC, __FILE__, __LINE__, __func__);
      return false;
    }
  }
  struct stat sbuf;
  if(fstat(fd, &sbuf) == -1 || !S_ISREG(sbuf.st_mode)){
    close(fd);
    tchdbsetecode(hdb, TCESTAT, __FILE__, __LINE__, __func__);
    return false;
  }
  char hbuf[HDBHEADSIZ];
  if((omode & HDBOWRITER) && sbuf.st_size < 1){
    uint32_t fbpmax = 1 << hdb->fpow;
    uint32_t fbpsiz = HDBFBPBSIZ + fbpmax * HDBFBPESIZ;
    int besiz = (hdb->opts & HDBTLARGE) ? sizeof(int64_t) : sizeof(int32_t);
    hdb->align = 1 << hdb->apow;
    hdb->fsiz = HDBHEADSIZ + besiz * hdb->bnum + fbpsiz;
    uint64_t psiz = tchdbpadsize(hdb);
    hdb->fsiz += psiz;
    hdb->frec = hdb->fsiz;
    tcdumpmeta(hdb, hbuf);
    psiz += besiz * hdb->bnum + fbpsiz;
    char pbuf[HDBIOBUFSIZ];
    memset(pbuf, 0, HDBIOBUFSIZ);
    bool err = false;
    if(!tcwrite(fd, hbuf, HDBHEADSIZ)) err = true;
    while(psiz > 0){
      if(psiz > HDBIOBUFSIZ){
        if(!tcwrite(fd, pbuf, HDBIOBUFSIZ)) err = true;
        psiz -= HDBIOBUFSIZ;
      } else {
        if(!tcwrite(fd, pbuf, psiz)) err = true;
        psiz = 0;
      }
    }
    if(err){
      tchdbsetecode(hdb, TCEWRITE, __FILE__, __LINE__, __func__);
      close(fd);
      return false;
    }
    sbuf.st_size = hdb->fsiz;
  }
  hdb->fd = fd;
  if(!tcseekread(hdb, 0, hbuf, HDBHEADSIZ)){
    close(fd);
    hdb->fd = -1;
    return false;
  }
  tcloadmeta(hdb, hbuf);
  uint32_t fbpmax = 1 << hdb->fpow;
  uint32_t fbpsiz = HDBFBPBSIZ + fbpmax * HDBFBPESIZ;
  if(!(omode & HDBONOLCK) && (memcmp(hbuf, HDBMAGICNUM, strlen(HDBMAGICNUM)) ||
                              (hdb->type != HDBTHASH && hdb->type != HDBTBTREE) ||
                              hdb->frec < HDBHEADSIZ + fbpsiz || hdb->frec > hdb->fsiz ||
                              sbuf.st_size != hdb->fsiz)){
    tchdbsetecode(hdb, TCEMETA, __FILE__, __LINE__, __func__);
    close(fd);
    hdb->fd = -1;
    return false;
  }
  if((hdb->opts & HDBTDEFLATE) && !_tc_deflate){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    close(fd);
    hdb->fd = -1;
    return false;
  }
  int besiz = (hdb->opts & HDBTLARGE) ? sizeof(int64_t) : sizeof(int32_t);
  size_t msiz = HDBHEADSIZ + hdb->bnum * besiz;
  void *map = mmap(0, msiz, PROT_READ | ((omode & HDBOWRITER) ? PROT_WRITE : 0),
                   MAP_SHARED, fd, 0);
  if(map == MAP_FAILED){
    tchdbsetecode(hdb, TCEMMAP, __FILE__, __LINE__, __func__);
    close(fd);
    hdb->fd = -1;
    return NULL;
  }
  hdb->fbpmax = fbpmax;
  hdb->fbpsiz = fbpsiz;
  hdb->fbpool = (omode & HDBOWRITER) ? tcmalloc(fbpmax * sizeof(HDBFB)) : NULL;
  hdb->fbpnum = 0;
  hdb->fbpmis = 0;
  hdb->drpool = NULL;
  hdb->drpdef = NULL;
  hdb->path = tcstrdup(path);
  hdb->omode = omode;
  hdb->map = map;
  hdb->msiz = msiz;
  if(hdb->opts & HDBTLARGE){
    hdb->ba32 = NULL;
    hdb->ba64 = (uint64_t *)((char *)map + HDBHEADSIZ);
  } else {
    hdb->ba32 = (uint32_t *)((char *)map + HDBHEADSIZ);
    hdb->ba64 = NULL;
  }
  hdb->align = 1 << hdb->apow;
  hdb->runit = tclmin(tclmax(hdb->align, HDBMINRUNIT), HDBIOBUFSIZ);
  hdb->ecode = TCESUCCESS;
  hdb->fatal = false;
  if((hdb->omode & HDBOWRITER) && !(hdb->flags & HDBFOPEN) && !tchdbloadfbp(hdb)){
    munmap(hdb->map, hdb->msiz);
    close(fd);
    hdb->fd = -1;
    return NULL;
  }
  if(hdb->omode & HDBOWRITER) tchdbsetflag(hdb, HDBFOPEN, true);
  hdb->inode = (uint64_t)sbuf.st_ino;
  hdb->mtime = sbuf.st_mtime;
  return true;
}


/* Close a database object. */
bool tchdbclose(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  bool err = false;
  if(hdb->omode & HDBOWRITER){
    if(hdb->drpool && !tchdbflushdrp(hdb)) err = true;
    if(!tchdbsavefbp(hdb)) err = true;
    free(hdb->fbpool);
    tchdbsetflag(hdb, HDBFOPEN, false);
  }
  free(hdb->path);
  if((hdb->omode & HDBOWRITER) && !tchdbmemsync(hdb, false)) err = true;
  if(munmap(hdb->map, hdb->msiz) == -1){
    tchdbsetecode(hdb, TCEMMAP, __FILE__, __LINE__, __func__);
    err = true;
  }
  if(close(hdb->fd) == -1){
    tchdbsetecode(hdb, TCECLOSE, __FILE__, __LINE__, __func__);
    err = true;
  }
  hdb->path = NULL;
  hdb->fd = -1;
  return err ? false : true;
}



/* Store a record into a hash database object. */
bool tchdbput(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz){
  assert(hdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(hdb->fd < 0 || !(hdb->omode & HDBOWRITER)){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  if(hdb->drpool && !tchdbflushdrp(hdb)) return false;
  if(hdb->opts & HDBTDEFLATE){
    char *zbuf = _tc_deflate(vbuf, vsiz, &vsiz, _TC_ZMRAW);
    if(!zbuf){
      tchdbsetecode(hdb, TCEMISC, __FILE__, __LINE__, __func__);
      return false;
    }
    bool rv = tchdbputimpl(hdb, kbuf, ksiz, zbuf, vsiz, HDBPDOVER);
    free(zbuf);
    return rv;
  }
  return tchdbputimpl(hdb, kbuf, ksiz, vbuf, vsiz, HDBPDOVER);
}



/* Store a string record into a hash database object. */
bool tchdbput2(TCHDB *hdb, const char *kstr, const char *vstr){
  assert(hdb && kstr && vstr);
  return tchdbput(hdb, kstr, strlen(kstr), vstr, strlen(vstr));
}



/* Retrieve a record in a hash database object. */
char *tchdbget(TCHDB *hdb, const char *kbuf, int ksiz, int *sp){
  assert(hdb && kbuf && ksiz >= 0 && sp);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  if(hdb->drpool && !tchdbflushdrp(hdb)) return false;
  uint64_t bidx = tchdbbidx(hdb, kbuf, ksiz);
  uint64_t off = tchdbgetbucket(hdb, bidx);
  uint8_t hash = tcrechash(kbuf, ksiz);
  TCHREC rec;
  char rbuf[HDBIOBUFSIZ];
  while(off > 0){
    rec.off = off;
    if(!tchdbreadrec(hdb, &rec, rbuf)) return NULL;
    if(hash > rec.hash){
      off = rec.left;
    } else if(hash < rec.hash){
      off = rec.right;
    } else {
      if(!rec.kbuf && !tchdbreadrecbody(hdb, &rec)) return NULL;
      int kcmp = tcreckeycmp(kbuf, ksiz, rec.kbuf, rec.ksiz);
      if(kcmp > 0){
        off = rec.left;
        free(rec.bbuf);
        rec.kbuf = NULL;
        rec.bbuf = NULL;
      } else if(kcmp < 0){
        off = rec.right;
        free(rec.bbuf);
        rec.kbuf = NULL;
        rec.bbuf = NULL;
      } else {
        if(!rec.vbuf && !tchdbreadrecbody(hdb, &rec)) return NULL;
        if(hdb->opts & HDBTDEFLATE){
          int zsiz;
          char *zbuf = _tc_inflate(rec.vbuf, rec.vsiz, &zsiz, _TC_ZMRAW);
          free(rec.bbuf);
          if(!zbuf){
            tchdbsetecode(hdb, TCEMISC, __FILE__, __LINE__, __func__);
            return NULL;
          }
          *sp = zsiz;
          return zbuf;
        }
        if(rec.bbuf){
          memmove(rec.bbuf, rec.vbuf, rec.vsiz);
          rec.bbuf[rec.vsiz] = '\0';
          *sp = rec.vsiz;
          return rec.bbuf;
        }
        *sp = rec.vsiz;
        return tcmemdup(rec.vbuf, rec.vsiz);
      }
    }
  }
  tchdbsetecode(hdb, TCENOREC, __FILE__, __LINE__, __func__);
  return NULL;
}


/* Retrieve a string record in a hash database object. */
char *tchdbget2(TCHDB *hdb, const char *kstr){
  assert(hdb && kstr);
  int vsiz;
  return tchdbget(hdb, kstr, strlen(kstr), &vsiz);
}


/* Initialize the iterator of a hash database object. */
bool tchdbiterinit(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return NULL;
  }
  if(hdb->drpool && !tchdbflushdrp(hdb)) return false;
  hdb->iter = hdb->frec;
  return true;
}


/* Get the next key of the iterator of a hash database object. */
char *tchdbiternext(TCHDB *hdb, int *sp){
  assert(hdb && sp);
  if(hdb->fd < 0 || hdb->iter < 1){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return NULL;
  }
  if(hdb->drpool && !tchdbflushdrp(hdb)) return false;
  TCHREC rec;
  char rbuf[HDBIOBUFSIZ];
  while(hdb->iter < hdb->fsiz){
    rec.off = hdb->iter;
    if(!tchdbreadrec(hdb, &rec, rbuf)) return NULL;
    hdb->iter += rec.rsiz;
    if(rec.magic == HDBMAGICREC){
      if(rec.kbuf){
        *sp = rec.ksiz;
        return tcmemdup(rec.kbuf, rec.ksiz);
      }
      if(!tchdbreadrecbody(hdb, &rec)) return NULL;
      rec.bbuf[rec.ksiz] = '\0';
      *sp = rec.ksiz;
      return rec.bbuf;
    }
  }
  tchdbsetecode(hdb, TCENOREC, __FILE__, __LINE__, __func__);
  return NULL;
}


/* Get the next key string of the iterator of a hash database object. */
char *tchdbiternext2(TCHDB *hdb){
  assert(hdb);
  int vsiz;
  return tchdbiternext(hdb, &vsiz);
}


/*************************************************************************************************
 * features for experts
 *************************************************************************************************/

/* Set the error code of a hash database object. */
void tchdbsetecode(TCHDB *hdb, int ecode, const char *filename, int line, const char *func){
  assert(hdb && filename && line >= 1 && func);
  if(!hdb->fatal) hdb->ecode = ecode;
  if(ecode != TCEINVALID && ecode != TCEKEEP && ecode != TCENOREC){
    hdb->fatal = true;
    if(hdb->omode & HDBOWRITER) tchdbsetflag(hdb, HDBFFATAL, true);
  }
  if(hdb->dbgfd >= 0){
    char obuf[HDBIOBUFSIZ];
    int osiz = sprintf(obuf, "ERROR:%s:%d:%s:%s:%d:%s\n", filename, line, func,
                       hdb->path ? hdb->path : "-", ecode, tchdberrmsg(ecode));
    tcwrite(hdb->dbgfd, obuf, osiz);
  }
}


/* Synchronize updating contents on memory. */
bool tchdbmemsync(TCHDB *hdb, bool phys){
  assert(hdb);
  if(hdb->fd < 0 || !(hdb->omode & HDBOWRITER)){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  bool err = false;
  char hbuf[HDBHEADSIZ];
  tcdumpmeta(hdb, hbuf);
  memcpy(hdb->map, hbuf, HDBHEADSIZ);
  if(phys){
    if(msync(hdb->map, hdb->msiz, MS_SYNC) == -1){
      tchdbsetecode(hdb, TCEMMAP, __FILE__, __LINE__, __func__);
      err = true;
    }
    if(fsync(hdb->fd) == -1){
      tchdbsetecode(hdb, TCESYNC, __FILE__, __LINE__, __func__);
      err = true;
    }
  }
  return err ? false : true;
}



/*************************************************************************************************
 * private features
 *************************************************************************************************/

/* Lock a file.
   `fd' specifies the file descriptor.
   `ex' specifies whether an exclusive lock or a shared lock is performed.
   `nb' specifies whether to request with non-blocking.
   The return value is true if successful, else, it is false. */
static bool tclock(int fd, bool ex, bool nb){
  assert(fd >= 0);
  struct flock lock;
  memset(&lock, 0, sizeof(struct flock));
  lock.l_type = ex ? F_WRLCK : F_RDLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = 0;
  while(fcntl(fd, nb ? F_SETLK : F_SETLKW, &lock) == -1){
    if(errno != EINTR) return false;
  }
  return true;
}


/* Write data into a file.
   `fd' specifies the file descriptor.
   `buf' specifies the buffer to be written.
   `size' specifies the size of the buffer.
   The return value is true if successful, else, it is false. */
static bool tcwrite(int fd, const void *buf, size_t size){
  assert(fd >= 0 && buf && size >= 0);
  const char *rp = buf;
  do {
    int wb = write(fd, rp, size);
    switch(wb){
    case -1: if(errno != EINTR) return false;
    case 0: break;
    default:
      rp += wb;
      size -= wb;
      break;
    }
  } while(size > 0);
  return true;
}


/* Seek and read data from a file.
   `hdb' specifies the hash database object.
   `off' specifies the offset of the region to seek.
   `buf' specifies the buffer to store into.
   `size' specifies the size of the buffer.
   The return value is true if successful, else, it is false. */
static bool tcseekread(TCHDB *hdb, off_t off, void *buf, size_t size){
  assert(hdb && off >= 0 && buf && size >= 0);
  if(lseek(hdb->fd, off, SEEK_SET) == -1){
    tchdbsetecode(hdb, TCESEEK, __FILE__, __LINE__, __func__);
    return false;
  }
  if(!tcread(hdb->fd, buf, size)){
    tchdbsetecode(hdb, TCEREAD, __FILE__, __LINE__, __func__);
    return false;
  }
  return true;
}

/* Read data from a file.
   `fd' specifies the file descriptor.
   `buf' specifies the buffer to store into.
   `size' specifies the size of the buffer.
   The return value is true if successful, else, it is false. */
static bool tcread(int fd, void *buf, size_t size){
  assert(fd >= 0 && buf && size >= 0);
  char *wp = buf;
  do {
    int rb = read(fd, wp, size);
    switch(rb){
    case -1: if(errno != EINTR) return false;
    case 0: return size < 1;
    default:
      wp += rb;
      size -= rb;
    }
  } while(size > 0);
  return true;
}


/* Seek and read data from a file.
   `hdb' specifies the hash database object.
   `off' specifies the offset of the region to seek.
   `buf' specifies the buffer to store into.
   `size' specifies the size of the buffer.
   The return value is true if successful, else, it is false. */
static bool tcseekwrite(TCHDB *hdb, off_t off, void *buf, size_t size){
  assert(hdb && off >= 0 && buf && size >= 0);
  if(lseek(hdb->fd, off, SEEK_SET) == -1){
    tchdbsetecode(hdb, TCESEEK, __FILE__, __LINE__, __func__);
    return false;
  }
  if(!tcwrite(hdb->fd, buf, size)){
    tchdbsetecode(hdb, TCEWRITE, __FILE__, __LINE__, __func__);
    return false;
  }
  return true;
}


/* Serialize meta data into a buffer.
   `hdb' specifies the hash database object.
   `hbuf' specifies the buffer. */
static void tcdumpmeta(TCHDB *hdb, char *hbuf){
  memset(hbuf, 0, HDBHEADSIZ);
  sprintf(hbuf, "%s\n%s:%d\n", HDBMAGICNUM, _TC_FORMATVER, _TC_LIBVER);
  memcpy(hbuf + HDBTYPEOFF, &(hdb->type), sizeof(hdb->type));
  memcpy(hbuf + HDBFLAGSOFF, &(hdb->flags), sizeof(hdb->flags));
  memcpy(hbuf + HDBAPOWOFF, &(hdb->apow), sizeof(hdb->apow));
  memcpy(hbuf + HDBFPOWOFF, &(hdb->fpow), sizeof(hdb->fpow));
  memcpy(hbuf + HDBOPTSOFF, &(hdb->opts), sizeof(hdb->opts));
  uint64_t llnum;
  llnum = hdb->bnum;
  llnum = TCHTOILL(llnum);
  memcpy(hbuf + HDBBNUMOFF, &llnum, sizeof(llnum));
  llnum = hdb->rnum;
  llnum = TCHTOILL(llnum);
  memcpy(hbuf + HDBRNUMOFF, &llnum, sizeof(llnum));
  llnum = hdb->fsiz;
  llnum = TCHTOILL(llnum);
  memcpy(hbuf + HDBFSIZOFF, &llnum, sizeof(llnum));
  llnum = hdb->frec;
  llnum = TCHTOILL(llnum);
  memcpy(hbuf + HDBFRECOFF, &llnum, sizeof(llnum));
}


/* Deserialize meta data from a buffer.
   `hdb' specifies the hash database object.
   `hbuf' specifies the buffer. */
static void tcloadmeta(TCHDB *hdb, const char *hbuf){
  memcpy(&(hdb->type), hbuf + HDBTYPEOFF, sizeof(hdb->type));
  memcpy(&(hdb->flags), hbuf + HDBFLAGSOFF, sizeof(hdb->flags));
  memcpy(&(hdb->apow), hbuf + HDBAPOWOFF, sizeof(hdb->apow));
  memcpy(&(hdb->fpow), hbuf + HDBFPOWOFF, sizeof(hdb->fpow));
  memcpy(&(hdb->opts), hbuf + HDBOPTSOFF, sizeof(hdb->opts));
  uint64_t llnum;
  memcpy(&llnum, hbuf + HDBBNUMOFF, sizeof(llnum));
  hdb->bnum = TCITOHLL(llnum);
  memcpy(&llnum, hbuf + HDBRNUMOFF, sizeof(llnum));
  hdb->rnum = TCITOHLL(llnum);
  memcpy(&llnum, hbuf + HDBFSIZOFF, sizeof(llnum));
  hdb->fsiz = TCITOHLL(llnum);
  memcpy(&llnum, hbuf + HDBFRECOFF, sizeof(llnum));
  hdb->frec = TCITOHLL(llnum);
}

/* Clear all members.
   `hdb' specifies the hash database object. */
static void tchdbclear(TCHDB *hdb){
  assert(hdb);
  hdb->type = HDBTHASH;
  hdb->flags = 0;
  hdb->bnum = HDBDEFBNUM;
  hdb->apow = HDBDEFAPOW;
  hdb->fpow = HDBDEFFPOW;
  hdb->opts = 0;
  hdb->path = NULL;
  hdb->fd = -1;
  hdb->omode = 0;
  hdb->rnum = 0;
  hdb->fsiz = 0;
  hdb->frec = 0;
  hdb->iter = 0;
  hdb->map = NULL;
  hdb->msiz = 0;
  hdb->ba32 = NULL;
  hdb->ba64 = NULL;
  hdb->align = 0;
  hdb->runit = 0;
  hdb->fbpmax = 0;
  hdb->fbpsiz = 0;
  hdb->fbpool = NULL;
  hdb->fbpnum = 0;
  hdb->fbpmis = 0;
  hdb->drpool = NULL;
  hdb->drpdef = NULL;
  hdb->ecode = TCESUCCESS;
  hdb->fatal = false;
  hdb->dbgfd = -1;
  hdb->cnt_writerec = -1;
  hdb->cnt_reuserec = -1;
  hdb->cnt_moverec = -1;
  hdb->cnt_readrec = -1;
  hdb->cnt_searchfbp = -1;
  hdb->cnt_insertfbp = -1;
  hdb->cnt_splicefbp = -1;
  hdb->cnt_dividefbp = -1;
  hdb->cnt_mergefbp = -1;
  hdb->cnt_reducefbp = -1;
  hdb->cnt_appenddrp = -1;
  hdb->cnt_deferdrp = -1;
  hdb->cnt_flushdrp = -1;
  TCDODEBUG(hdb->cnt_writerec = 0);
  TCDODEBUG(hdb->cnt_reuserec = 0);
  TCDODEBUG(hdb->cnt_moverec = 0);
  TCDODEBUG(hdb->cnt_readrec = 0);
  TCDODEBUG(hdb->cnt_searchfbp = 0);
  TCDODEBUG(hdb->cnt_insertfbp = 0);
  TCDODEBUG(hdb->cnt_splicefbp = 0);
  TCDODEBUG(hdb->cnt_dividefbp = 0);
  TCDODEBUG(hdb->cnt_mergefbp = 0);
  TCDODEBUG(hdb->cnt_reducefbp = 0);
  TCDODEBUG(hdb->cnt_appenddrp = 0);
  TCDODEBUG(hdb->cnt_deferdrp = 0);
  TCDODEBUG(hdb->cnt_flushdrp = 0);
}


/* Get the padding size to record alignment.
   `hdb' specifies the hash database object.
   The return value is the padding size. */
static int32_t tchdbpadsize(TCHDB *hdb){
  assert(hdb);
  int32_t diff = hdb->fsiz & (hdb->align - 1);
  return diff > 0 ? hdb->align - diff : 0;
}


/* Set the open flag.
   `hdb' specifies the hash database object.
   `flag' specifies the flag value.
   `sign' specifies the sign. */
static void tchdbsetflag(TCHDB *hdb, int flag, bool sign){
  assert(hdb);
  char *fp = (char *)hdb->map + HDBFLAGSOFF;
  if(sign){
    *fp |= (uint8_t)flag;
  } else {
    *fp &= ~(uint8_t)flag;
  }
  hdb->flags = *fp;
}

/* Get the bucket index of a record.
   `hdb' specifies the hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   The return value is the bucket index. */
static uint64_t tchdbbidx(TCHDB *hdb, const char *kbuf, int ksiz){
  assert(hdb && kbuf && ksiz >= 0);
  uint64_t hash = 751;
  for(hash = ksiz; ksiz--;){
    hash = hash * 37 + *(uint8_t *)kbuf++;
  }
  return hash % hdb->bnum;
}



/* Get the second hash value of a record.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   The return value is the second hash value. */
static uint8_t tcrechash(const char *kbuf, int ksiz){
  assert(kbuf && ksiz >= 0);
  kbuf += ksiz - 1;
  uint32_t hash = 19780211;
  for(hash = ksiz; ksiz--;){
    hash = hash * 31 + *(uint8_t *)kbuf--;
  }
  return hash;
}


/* Get the offset of the record of a bucket element.
   `hdb' specifies the hash database object.
   `bidx' specifies the index of the bucket.
   The return value is the offset of the record. */
static uint64_t tchdbgetbucket(TCHDB *hdb, uint64_t bidx){
  assert(hdb && bidx >= 0);
  if(hdb->ba64){
    uint64_t llnum = hdb->ba64[bidx];
    return TCITOHLL(llnum) << hdb->apow;
  }
  uint32_t lnum = hdb->ba32[bidx];
  return (uint64_t)TCITOHL(lnum) << hdb->apow;
}


/* Load the free block pool from the file.
   The return value is true if successful, else, it is false. */
static bool tchdbsavefbp(TCHDB *hdb){
  assert(hdb);
  if(hdb->fbpnum > (hdb->fbpmax >> 1)){
    tchdbfbpmerge(hdb);
  } else if(hdb->fbpnum > 1){
    tcfbpsortbyoff(hdb->fbpool, hdb->fbpnum);
  }
  int bsiz = hdb->fbpsiz;
  char *buf = tcmalloc(bsiz);
  char *wp = buf;
  HDBFB *cur = hdb->fbpool;
  HDBFB *end = cur + hdb->fbpnum;
  uint64_t base = 0;
  bsiz -= sizeof(HDBFB) + sizeof(uint8_t) + sizeof(uint8_t);
  while(cur < end && bsiz > 0){
    uint64_t noff = cur->off >> hdb->apow;
    int step;
    uint64_t llnum = noff - base;
    TC_SETVNUMBUF64(step, wp, llnum);
    wp += step;
    bsiz -= step;
    uint32_t lnum = cur->rsiz >> hdb->apow;
    TC_SETVNUMBUF(step, wp, lnum);
    wp += step;
    bsiz -= step;
    base = noff;
    cur++;
  }
  *(wp++) = '\0';
  *(wp++) = '\0';
  if(!tcseekwrite(hdb, hdb->msiz, buf, wp - buf)){
    free(buf);
    return false;
  }
  free(buf);
  return true;
}


/* Save the free block pool into the file.
   The return value is true if successful, else, it is false. */
static bool tchdbloadfbp(TCHDB *hdb){
  int bsiz = hdb->fbpsiz;
  char *buf = tcmalloc(bsiz);
  if(!tcseekread(hdb, hdb->msiz, buf, bsiz)){
    free(buf);
    return false;
  }
  const char *rp = buf;
  HDBFB *cur = hdb->fbpool;
  HDBFB *end = cur + hdb->fbpmax;
  uint64_t base = 0;
  while(cur < end && *rp != '\0'){
    int step;
    uint64_t llnum;
    TC_READVNUMBUF64(rp, llnum, step);
    base += llnum << hdb->apow;
    cur->off = base;
    rp += step;
    uint32_t lnum;
    TC_READVNUMBUF(rp, lnum, step);
    cur->rsiz = lnum << hdb->apow;
    rp += step;
    cur++;
  }
  hdb->fbpnum = cur - (HDBFB *)hdb->fbpool;
  free(buf);
  return true;
}



/* Sort the free block pool by offset.
   `fbpool' specifies the free block pool.
   `fbpnum' specifies the number of blocks. */
static void tcfbpsortbyoff(HDBFB *fbpool, int fbpnum){
  assert(fbpool && fbpnum >= 0);
  fbpnum--;
  int bottom = fbpnum / 2 + 1;
  int top = fbpnum;
  while(bottom > 0){
    bottom--;
    int mybot = bottom;
    int i = 2 * mybot;
    while(i <= top) {
      if(i < top && fbpool[i+1].off > fbpool[i].off) i++;
      if(fbpool[mybot].off >= fbpool[i].off) break;
      HDBFB swap = fbpool[mybot];
      fbpool[mybot] = fbpool[i];
      fbpool[i] = swap;
      mybot = i;
      i = 2 * mybot;
    }
  }
  while(top > 0){
    HDBFB swap = fbpool[0];
    fbpool[0] = fbpool[top];
    fbpool[top] = swap;
    top--;
    int mybot = bottom;
    int i = 2 * mybot;
    while(i <= top){
      if(i < top && fbpool[i+1].off > fbpool[i].off) i++;
      if(fbpool[mybot].off >= fbpool[i].off) break;
      swap = fbpool[mybot];
      fbpool[mybot] = fbpool[i];
      fbpool[i] = swap;
      mybot = i;
      i = 2 * mybot;
    }
  }
}


/* Merge successive records in the free block pool.
   `hdb' specifies the hash database object. */
static void tchdbfbpmerge(TCHDB *hdb){
  assert(hdb);
  TCDODEBUG(hdb->cnt_mergefbp++);
  int32_t onum = hdb->fbpnum;
  tcfbpsortbyoff(hdb->fbpool, hdb->fbpnum);
  HDBFB *wp = hdb->fbpool;;
  HDBFB *cur = wp;
  HDBFB *end = wp + hdb->fbpnum - 1;
  while(cur < end){
    if(cur->off > 0){
      HDBFB *next = cur + 1;
      if(cur->off + cur->rsiz == next->off){
        cur->rsiz += next->rsiz;
        next->off = 0;
      }
      *(wp++) = *cur;
    }
    cur++;
  }
  if(end->off > 0) *(wp++) = *end;
  hdb->fbpnum = wp - (HDBFB *)hdb->fbpool;
  hdb->fbpmis = (hdb->fbpnum < onum) ? 0 : hdb->fbpnum * -2;
}



/* Read a record from the file.
   `hdb' specifies the hash database object.
   `rec' specifies the record object.
   `bidx' specifies the index of the bucket.
   `entoff' specifies the offset of the tree entry.
   The return value is true if successful, else, it is false. */
static bool tchdbreadrec(TCHDB *hdb, TCHREC *rec, char *rbuf){
  assert(hdb && rec && rbuf);
  TCDODEBUG(hdb->cnt_readrec++);
  int32_t rsiz = tclmin(hdb->fsiz - rec->off, hdb->runit);
  if(rsiz < (sizeof(uint8_t) + sizeof(uint32_t))){
    tchdbsetecode(hdb, TCERHEAD, __FILE__, __LINE__, __func__);
    return false;
  }
  if(!tcseekread(hdb, rec->off, rbuf, rsiz)) return false;
  const char *rp = rbuf;
  rec->magic = *(uint8_t *)(rp++);
  if(rec->magic == HDBMAGICFB){
    uint32_t lnum;
    memcpy(&lnum, rp, sizeof(lnum));
    rec->rsiz = TCITOHL(lnum);
    return true;
  } else if(rec->magic != HDBMAGICREC){
    tchdbsetecode(hdb, TCERHEAD, __FILE__, __LINE__, __func__);
    return false;
  }
  rec->hash = *(uint8_t *)(rp++);
  if(hdb->ba64){
    uint64_t llnum;
    memcpy(&llnum, rp, sizeof(llnum));
    rec->left = TCITOHLL(llnum) << hdb->apow;
    rp += sizeof(llnum);
    memcpy(&llnum, rp, sizeof(llnum));
    rec->right = TCITOHLL(llnum) << hdb->apow;
    rp += sizeof(llnum);
  } else {
    uint32_t lnum;
    memcpy(&lnum, rp, sizeof(lnum));
    rec->left = (uint64_t)TCITOHL(lnum) << hdb->apow;
    rp += sizeof(lnum);
    memcpy(&lnum, rp, sizeof(lnum));
    rec->right = (uint64_t)TCITOHL(lnum) << hdb->apow;
    rp += sizeof(lnum);
  }
  uint16_t snum;
  memcpy(&snum, rp, sizeof(snum));
  rec->psiz = TCITOHS(snum);
  rp += sizeof(snum);
  uint32_t lnum;
  int step;
  TC_READVNUMBUF(rp, lnum, step);
  rec->ksiz = lnum;
  rp += step;
  TC_READVNUMBUF(rp, lnum, step);
  rec->vsiz = lnum;
  rp += step;
  int32_t hsiz = rp - rbuf;
  rec->rsiz = hsiz + rec->ksiz + rec->vsiz + rec->psiz;
  rec->kbuf = NULL;
  rec->vbuf = NULL;
  rec->boff = rec->off + hsiz;
  rec->bbuf = NULL;
  rsiz -= hsiz;
  if(rsiz >= rec->ksiz){
    rec->kbuf = rp;
    rsiz -= rec->ksiz;
    rp += rec->ksiz;
    if(rsiz >= rec->vsiz) rec->vbuf = rp;
  }
  return true;
}


/* Read the body of a record from the file.
   `hdb' specifies the hash database object.
   `rec' specifies the record object.
   The return value is true if successful, else, it is false. */
static bool tchdbreadrecbody(TCHDB *hdb, TCHREC *rec){
  assert(hdb && rec);
  int32_t bsiz = rec->ksiz + rec->vsiz;
  rec->bbuf = tcmalloc(bsiz + 1);
  if(!tcseekread(hdb, rec->boff, rec->bbuf, bsiz)) return false;
  rec->kbuf = rec->bbuf;
  rec->vbuf = rec->bbuf + rec->ksiz;
  return true;
}


/* Compare keys of two records.
   `abuf' specifies the pointer to the region of the former.
   `asiz' specifies the size of the region.
   `bbuf' specifies the pointer to the region of the latter.
   `bsiz' specifies the size of the region.
   The return value is 0 if two equals, positive if the formar is big, else, negative. */
static int tcreckeycmp(const char *abuf, int asiz, const char *bbuf, int bsiz){
  assert(abuf && asiz >= 0 && bbuf && bsiz >= 0);
  if(asiz > bsiz) return 1;
  if(asiz < bsiz) return -1;
  return memcmp(abuf, bbuf, asiz);
}

/* Flush the delayed record pool.
   `hdb' specifies the hash database object.
   The return value is true if successful, else, it is false. */
static bool tchdbflushdrp(TCHDB *hdb){
  assert(hdb);
  TCDODEBUG(hdb->cnt_flushdrp++);
  if(lseek(hdb->fd, 0, SEEK_END) == -1){
    tchdbsetecode(hdb, TCESEEK, __FILE__, __LINE__, __func__);
    return false;
  }
  if(!tcwrite(hdb->fd, tcxstrptr(hdb->drpool), tcxstrsize(hdb->drpool))){
    tchdbsetecode(hdb, TCEWRITE, __FILE__, __LINE__, __func__);
    return false;
  }
  const char *rp = tcxstrptr(hdb->drpdef);
  int size = tcxstrsize(hdb->drpdef);
  while(size > 0){
    int ksiz, vsiz;
    memcpy(&ksiz, rp, sizeof(int));
    rp += sizeof(int);
    memcpy(&vsiz, rp, sizeof(int));
    rp += sizeof(int);
    const char *kbuf = rp;
    rp += ksiz;
    const char *vbuf = rp;
    rp += vsiz;
    if(!tchdbputimpl(hdb, kbuf, ksiz, vbuf, vsiz, HDBPDOVER)){
      tcxstrdel(hdb->drpdef);
      tcxstrdel(hdb->drpool);
      hdb->drpdef = NULL;
      hdb->drpool = NULL;
      return false;
    }
    size -= sizeof(int) * 2 + ksiz + vsiz;
  }
  tcxstrdel(hdb->drpdef);
  tcxstrdel(hdb->drpool);
  hdb->drpdef = NULL;
  hdb->drpool = NULL;
  uint64_t llnum;
  llnum = hdb->rnum;
  llnum = TCHTOILL(llnum);
  memcpy(hdb->map + HDBRNUMOFF, &llnum, sizeof(llnum));
  llnum = hdb->fsiz;
  llnum = TCHTOILL(llnum);
  memcpy(hdb->map + HDBFSIZOFF, &llnum, sizeof(llnum));
  return true;
}


/* Store a record.
   `hdb' specifies the hash database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `over' specifies whether the value of the duplicated record is overwritten or not.
   If successful, the return value is true, else, it is false. */
static bool tchdbputimpl(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz,
                         int over){
  assert(hdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  uint64_t bidx = tchdbbidx(hdb, kbuf, ksiz);
  uint64_t off = tchdbgetbucket(hdb, bidx);
  uint8_t hash = tcrechash(kbuf, ksiz);
  uint64_t entoff = 0;
  TCHREC rec;
  char rbuf[HDBIOBUFSIZ];
  while(off > 0){
    rec.off = off;
    if(!tchdbreadrec(hdb, &rec, rbuf)) return false;
    if(hash > rec.hash){
      off = rec.left;
      entoff = rec.off + (sizeof(uint8_t) + sizeof(uint8_t));
    } else if(hash < rec.hash){
      off = rec.right;
      entoff = rec.off + (sizeof(uint8_t) + sizeof(uint8_t)) +
        (hdb->ba64 ? sizeof(uint64_t) : sizeof(uint32_t));
    } else {
      if(!rec.kbuf && !tchdbreadrecbody(hdb, &rec)) return false;
      int kcmp = tcreckeycmp(kbuf, ksiz, rec.kbuf, rec.ksiz);
      if(kcmp > 0){
        off = rec.left;
        free(rec.bbuf);
        rec.kbuf = NULL;
        rec.bbuf = NULL;
        entoff = rec.off + (sizeof(uint8_t) + sizeof(uint8_t));
      } else if(kcmp < 0){
        off = rec.right;
        free(rec.bbuf);
        rec.kbuf = NULL;
        rec.bbuf = NULL;
        entoff = rec.off + (sizeof(uint8_t) + sizeof(uint8_t)) +
          (hdb->ba64 ? sizeof(uint64_t) : sizeof(uint32_t));
      } else {
        switch(over){
        case HDBPDKEEP:
          free(rec.bbuf);
          tchdbsetecode(hdb, TCEKEEP, __FILE__, __LINE__, __func__);
          return false;
        case HDBPDCAT:
          if(vsiz < 1){
            free(rec.bbuf);
            return true;
          }
          if(!rec.vbuf && !tchdbreadrecbody(hdb, &rec)) return false;
          int nvsiz = rec.vsiz + vsiz;
          if(rec.bbuf){
            rec.bbuf = tcrealloc(rec.bbuf, rec.ksiz + nvsiz);
            memcpy(rec.bbuf + rec.ksiz + rec.vsiz, vbuf, vsiz);
            rec.kbuf = rec.bbuf;
            rec.vbuf = rec.kbuf + rec.ksiz;
            rec.vsiz = nvsiz;
          } else {
            rec.bbuf = tcmalloc(nvsiz);
            memcpy(rec.bbuf, rec.vbuf, rec.vsiz);
            memcpy(rec.bbuf + rec.vsiz, vbuf, vsiz);
            rec.vbuf = rec.bbuf;
            rec.vsiz = nvsiz;
          }
          bool rv = tchdbwriterec(hdb, &rec, bidx, entoff);
          free(rec.bbuf);
          return rv;
        default:
          break;
        }
        free(rec.bbuf);
        rec.ksiz = ksiz;
        rec.vsiz = vsiz;
        rec.kbuf = kbuf;
        rec.vbuf = vbuf;
        return tchdbwriterec(hdb, &rec, bidx, entoff);
      }
    }
  }
  rec.rsiz = HDBMAXHSIZ + ksiz + vsiz;
  if(!tchdbfbpsearch(hdb, &rec)) return false;
  rec.hash = hash;
  rec.left = 0;
  rec.right = 0;
  rec.ksiz = ksiz;
  rec.vsiz = vsiz;
  rec.psiz = 0;
  rec.kbuf = kbuf;
  rec.vbuf = vbuf;
  if(!tchdbwriterec(hdb, &rec, bidx, entoff)) return false;
  hdb->rnum++;
  uint64_t llnum = hdb->rnum;
  llnum = TCHTOILL(llnum);
  memcpy(hdb->map + HDBRNUMOFF, &llnum, sizeof(llnum));
  return true;
}

/* Search the free block pool for the minimum region.
   `hdb' specifies the hash database object.
   `rec' specifies the record object to be stored.
   The return value is true if successful, else, it is false. */
static bool tchdbfbpsearch(TCHDB *hdb, TCHREC *rec){
  assert(hdb && rec);
  TCDODEBUG(hdb->cnt_searchfbp++);
  if(hdb->fbpnum < 1){
    rec->off = hdb->fsiz;
    rec->rsiz = 0;
    return true;
  }
  uint32_t rsiz = rec->rsiz;
  HDBFB *pv = hdb->fbpool;
  HDBFB *ep = pv + hdb->fbpnum;
  while(pv < ep){
    if(pv->rsiz > rsiz){
      if(pv->rsiz > (rsiz << 1)){
        TCDODEBUG(hdb->cnt_dividefbp++);
        uint64_t fsiz = hdb->fsiz;
        hdb->fsiz = pv->off + rsiz;
        uint32_t psiz = tchdbpadsize(hdb);
        hdb->fsiz = fsiz;
        uint64_t noff = pv->off + rsiz + psiz;
        rec->off = pv->off;
        rec->rsiz = noff - pv->off;
        pv->off = noff;
        pv->rsiz -= rec->rsiz;
        return tchdbwritefb(hdb, pv->off, pv->rsiz);
      }
      rec->off = pv->off;
      rec->rsiz = pv->rsiz;
      ep--;
      pv->off = ep->off;
      pv->rsiz = ep->rsiz;
      hdb->fbpnum--;
      return true;
    }
    pv++;
  }
  rec->off = hdb->fsiz;
  rec->rsiz = 0;
  hdb->fbpmis++;
  if(hdb->fbpmis >= HDBFBPMGFREQ){
    tchdbfbpmerge(hdb);
    tcfbpsortbyrsiz(hdb->fbpool, hdb->fbpnum);
  }
  return true;
}

/* Sort the free block pool by record size.
   `fbpool' specifies the free block pool.
   `fbpnum' specifies the number of blocks. */
static void tcfbpsortbyrsiz(HDBFB *fbpool, int fbpnum){
  assert(fbpool && fbpnum >= 0);
  fbpnum--;
  int bottom = fbpnum / 2 + 1;
  int top = fbpnum;
  while(bottom > 0){
    bottom--;
    int mybot = bottom;
    int i = 2 * mybot;
    while(i <= top) {
      if(i < top && fbpool[i+1].rsiz > fbpool[i].rsiz) i++;
      if(fbpool[mybot].rsiz >= fbpool[i].rsiz) break;
      HDBFB swap = fbpool[mybot];
      fbpool[mybot] = fbpool[i];
      fbpool[i] = swap;
      mybot = i;
      i = 2 * mybot;
    }
  }
  while(top > 0){
    HDBFB swap = fbpool[0];
    fbpool[0] = fbpool[top];
    fbpool[top] = swap;
    top--;
    int mybot = bottom;
    int i = 2 * mybot;
    while(i <= top){
      if(i < top && fbpool[i+1].rsiz > fbpool[i].rsiz) i++;
      if(fbpool[mybot].rsiz >= fbpool[i].rsiz) break;
      swap = fbpool[mybot];
      fbpool[mybot] = fbpool[i];
      fbpool[i] = swap;
      mybot = i;
      i = 2 * mybot;
    }
  }
}


/* Write a free block into the file.
   `hdb' specifies the hash database object.
   `off' specifies the offset of the block.
   `rsiz' specifies the size of the block.
   The return value is true if successful, else, it is false. */
static bool tchdbwritefb(TCHDB *hdb, uint64_t off, uint32_t rsiz){
  assert(hdb && off > 0 && rsiz > 0);
  char rbuf[HDBMAXHSIZ];
  char *wp = rbuf;
  *(uint8_t *)(wp++) = HDBMAGICFB;
  uint32_t lnum = TCHTOIL(rsiz);
  memcpy(wp, &lnum, sizeof(lnum));
  wp += sizeof(lnum);
  if(!tcseekwrite(hdb, off, rbuf, wp - rbuf)) return false;
  return true;
}


/* Write a record into the file.
   `hdb' specifies the hash database object.
   `rec' specifies the record object.
   `bidx' specifies the index of the bucket.
   `entoff' specifies the offset of the tree entry.
   The return value is true if successful, else, it is false. */
static bool tchdbwriterec(TCHDB *hdb, TCHREC *rec, uint64_t bidx, uint64_t entoff){
  assert(hdb && rec);
  TCDODEBUG(hdb->cnt_writerec++);
  uint64_t ofsiz = hdb->fsiz;
  char stack[HDBIOBUFSIZ];
  int bsiz = rec->rsiz > 0 ? rec->rsiz : HDBMAXHSIZ + rec->ksiz + rec->vsiz + hdb->align;
  char *rbuf = (bsiz <= HDBIOBUFSIZ) ? stack : tcmalloc(bsiz);
  char *wp = rbuf;
  *(uint8_t *)(wp++) = HDBMAGICREC;
  *(uint8_t *)(wp++) = rec->hash;
  if(hdb->ba64){
    uint64_t llnum;
    llnum = rec->left >> hdb->apow;
    llnum = TCHTOILL(llnum);
    memcpy(wp, &llnum, sizeof(llnum));
    wp += sizeof(llnum);
    llnum = rec->right >> hdb->apow;
    llnum = TCHTOILL(llnum);
    memcpy(wp, &llnum, sizeof(llnum));
    wp += sizeof(llnum);
  } else {
    uint32_t lnum;
    lnum = rec->left >> hdb->apow;
    lnum = TCHTOIL(lnum);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    lnum = rec->right >> hdb->apow;
    lnum = TCHTOIL(lnum);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
  }
  uint16_t snum;
  char *pwp = wp;
  wp += sizeof(snum);
  int step;
  TC_SETVNUMBUF(step, wp, rec->ksiz);
  wp += step;
  TC_SETVNUMBUF(step, wp, rec->vsiz);
  wp += step;
  int32_t hsiz = wp - rbuf;
  int32_t rsiz = hsiz + rec->ksiz + rec->vsiz;
  if(rec->rsiz < 1){
    hdb->fsiz += rsiz;
    uint16_t psiz = tchdbpadsize(hdb);
    rec->rsiz = rsiz + psiz;
    rec->psiz = psiz;
    hdb->fsiz += psiz;
  } else if(rsiz > rec->rsiz){
    if(rbuf != stack) free(rbuf);
    if(tchdbfbpsplice(hdb, rec, rsiz)){
      TCDODEBUG(hdb->cnt_splicefbp++);
      return tchdbwriterec(hdb, rec, bidx, entoff);
    }
    TCDODEBUG(hdb->cnt_moverec++);
    if(!tchdbwritefb(hdb, rec->off, rec->rsiz)) return false;
    tchdbfbpinsert(hdb, rec->off, rec->rsiz);
    rec->rsiz = rsiz;
    if(!tchdbfbpsearch(hdb, rec)) return false;
    return tchdbwriterec(hdb, rec, bidx, entoff);
  } else {
    TCDODEBUG(hdb->cnt_reuserec++);
    uint32_t psiz = rec->rsiz - rsiz;
    if(psiz > UINT16_MAX){
      TCDODEBUG(hdb->cnt_dividefbp++);
      uint64_t fsiz = hdb->fsiz;
      hdb->fsiz = rec->off + rsiz;
      psiz = tchdbpadsize(hdb);
      hdb->fsiz = fsiz;
      uint64_t noff = rec->off + rsiz + psiz;
      uint32_t nsiz = rec->rsiz - rsiz - psiz;
      rec->rsiz = noff - rec->off;
      rec->psiz = psiz;
      if(!tchdbwritefb(hdb, noff, nsiz)) return false;
      tchdbfbpinsert(hdb, noff, nsiz);
    }
    rec->psiz = psiz;
  }
  snum = rec->psiz;
  snum = TCHTOIS(snum);
  memcpy(pwp, &snum, sizeof(snum));
  rsiz = rec->rsiz;
  rsiz -= hsiz;
  memcpy(wp, rec->kbuf, rec->ksiz);
  wp += rec->ksiz;
  rsiz -= rec->ksiz;
  memcpy(wp, rec->vbuf, rec->vsiz);
  wp += rec->vsiz;
  rsiz -= rec->vsiz;
  memset(wp, 0, rsiz);
  if(!tcseekwrite(hdb, rec->off, rbuf, rec->rsiz)){
    if(rbuf != stack) free(rbuf);
    hdb->fsiz = ofsiz;
    return false;
  }
  if(hdb->fsiz != ofsiz){
    uint64_t llnum = hdb->fsiz;
    llnum = TCHTOILL(llnum);
    memcpy(hdb->map + HDBFSIZOFF, &llnum, sizeof(llnum));
  }
  if(rbuf != stack) free(rbuf);
  if(entoff > 0){
    if(hdb->ba64){
      uint64_t llnum = rec->off >> hdb->apow;
      llnum = TCHTOILL(llnum);
      if(!tcseekwrite(hdb, entoff, &llnum, sizeof(uint64_t))) return false;
    } else {
      uint32_t lnum = rec->off >> hdb->apow;
      lnum = TCHTOIL(lnum);
      if(!tcseekwrite(hdb, entoff, &lnum, sizeof(uint32_t))) return false;
    }
  } else {
    tchdbsetbucket(hdb, bidx, rec->off);
  }
  return true;
}

/* Splice the trailing free block
   `hdb' specifies the hash database object.
   `rec' specifies the record object to be stored.
   `nsiz' specifies the needed size.
   The return value is whether splicing succeeded or not. */
static bool tchdbfbpsplice(TCHDB *hdb, TCHREC *rec, uint32_t nsiz){
  assert(hdb && rec && nsiz > 0);
  if(hdb->fbpnum < 1) return false;
  uint64_t off = rec->off + rec->rsiz;
  uint32_t rsiz = rec->rsiz;
  HDBFB *pv = hdb->fbpool;
  HDBFB *ep = pv + hdb->fbpnum;
  while(pv < ep){
    if(pv->off == off && rsiz + pv->rsiz >= nsiz){
      if(hdb->iter == pv->off) hdb->iter += pv->rsiz;
      rec->rsiz += pv->rsiz;
      ep--;
      pv->off = ep->off;
      pv->rsiz = ep->rsiz;
      hdb->fbpnum--;
      return true;
    }
    pv++;
  }
  return false;
}

/* Insert a block into the free block pool.
   `hdb' specifies the hash database object.
   `off' specifies the offset of the block.
   `rsiz' specifies the size of the block. */
static void tchdbfbpinsert(TCHDB *hdb, uint64_t off, uint32_t rsiz){
  assert(hdb && off > 0 && rsiz > 0);
  TCDODEBUG(hdb->cnt_insertfbp++);
  if(hdb->fpow < 1) return;
  HDBFB *pv = hdb->fbpool;
  if(hdb->fbpnum >= hdb->fbpmax){
    tchdbfbpmerge(hdb);
    tcfbpsortbyrsiz(hdb->fbpool, hdb->fbpnum);
    if(hdb->fbpnum >= hdb->fbpmax){
      TCDODEBUG(hdb->cnt_reducefbp++);
      int32_t dnum = (hdb->fbpmax >> 2) + 1;
      memmove(pv, pv + dnum, (hdb->fbpnum - dnum) * sizeof(*pv));
      hdb->fbpnum -= dnum;
    }
    hdb->fbpmis = 0;
  }
  pv = pv + hdb->fbpnum;
  pv->off = off;
  pv->rsiz = rsiz;
  hdb->fbpnum++;
}

/* Get the offset of the record of a bucket element.
   `hdb' specifies the hash database object.
   `bidx' specifies the index of the record.
   `off' specifies the offset of the record. */
static void tchdbsetbucket(TCHDB *hdb, uint64_t bidx, uint64_t off){
  assert(hdb && bidx >= 0);
  if(hdb->ba64){
    uint64_t llnum = off >> hdb->apow;
    hdb->ba64[bidx] = TCHTOILL(llnum);
  } else {
    uint32_t lnum = off >> hdb->apow;
    hdb->ba32[bidx] = TCHTOIL(lnum);
  }
}



/* END OF FILE */
