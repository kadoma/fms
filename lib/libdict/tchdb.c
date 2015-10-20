


#include "tchdb.h"
#include "tcutil.h"
#include "myconf.h"

#define HDBFILEMODE    00644             // permission of created files
#define HDBIOBUFSIZ    8192              // size of an I/O buffer

#define HDBMAGICNUM    "ToKyO CaBiNeT"   // magic number on environments of big endian
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
static bool tcseekwrite(TCHDB *hdb, off_t off, void *buf, size_t size);
static bool tcread(int fd, void *buf, size_t size);
static bool tcseekread(TCHDB *hdb, off_t off, void *buf, size_t size);
static void tcdumpmeta(TCHDB *hdb, char *hbuf);
static void tcloadmeta(TCHDB *hdb, const char *hbuf);
static uint64_t tcgetprime(int num);
static void tchdbclear(TCHDB *hdb);
static int32_t tchdbpadsize(TCHDB *hdb);
static void tchdbsetflag(TCHDB *hdb, int flag, bool sign);
static uint64_t tchdbbidx(TCHDB *hdb, const char *kbuf, int ksiz);
static uint8_t tcrechash(const char *kbuf, int ksiz);
static uint64_t tchdbgetbucket(TCHDB *hdb, uint64_t bidx);
static void tchdbsetbucket(TCHDB *hdb, uint64_t bidx, uint64_t off);
static bool tchdbsavefbp(TCHDB *hdb);
static bool tchdbloadfbp(TCHDB *hdb);
static void tcfbpsortbyoff(HDBFB *fbpool, int fbpnum);
static void tcfbpsortbyrsiz(HDBFB *fbpool, int fbpnum);
static void tchdbfbpmerge(TCHDB *hdb);
static void tchdbfbpinsert(TCHDB *hdb, uint64_t off, uint32_t rsiz);
static bool tchdbfbpsearch(TCHDB *hdb, TCHREC *rec);
static bool tchdbfbpsplice(TCHDB *hdb, TCHREC *rec, uint32_t nsiz);
static bool tchdbwritefb(TCHDB *hdb, uint64_t off, uint32_t rsiz);
static bool tchdbwriterec(TCHDB *hdb, TCHREC *rec, uint64_t bidx, uint64_t entoff);
static bool tchdbreadrec(TCHDB *hdb, TCHREC *rec, char *rbuf);
static bool tchdbreadrecbody(TCHDB *hdb, TCHREC *rec);
static int tcreckeycmp(const char *abuf, int asiz, const char *bbuf, int bsiz);
static bool tchdbflushdrp(TCHDB *hdb);
static bool tchdbputimpl(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz,
                         int over);
static void tchdbdrpappend(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz);
static bool tchdbputasyncimpl(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz);


/* debugging function prototypes */
void tchdbprintmeta(TCHDB *hdb);
void tchdbprintrec(TCHDB *hdb, TCHREC *rec);



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


/* Set the tuning parameters of a hash database object. */
bool tchdbtune(TCHDB *hdb, int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts){
  assert(hdb);
  if(hdb->fd >= 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  hdb->bnum = (bnum > 0) ? tcgetprime(bnum) : HDBDEFBNUM;
  hdb->apow = (apow >= 0) ? tclmin(apow, HDBMAXAPOW) : HDBDEFAPOW;
  hdb->fpow = (fpow >= 0) ? tclmin(fpow, HDBMAXFPOW) : HDBDEFFPOW;
  hdb->opts = opts;
  if(!_tc_deflate) hdb->opts &= ~HDBTDEFLATE;
  return true;
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


/* Store a new record into a hash database object. */
bool tchdbputkeep(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz){
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
    bool rv = tchdbputimpl(hdb, kbuf, ksiz, zbuf, vsiz, HDBPDKEEP);
    free(zbuf);
    return rv;
  }
  return tchdbputimpl(hdb, kbuf, ksiz, vbuf, vsiz, HDBPDKEEP);
}


/* Store a new string record into a hash database object. */
bool tchdbputkeep2(TCHDB *hdb, const char *kstr, const char *vstr){
  return tchdbputkeep(hdb, kstr, strlen(kstr), vstr, strlen(vstr));
}


/* Concatenate a value at the end of the existing record in a hash database object. */
bool tchdbputcat(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz){
  assert(hdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(hdb->fd < 0 || !(hdb->omode & HDBOWRITER)){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  if(hdb->drpool && !tchdbflushdrp(hdb)) return false;
  if(hdb->opts & HDBTDEFLATE){
    char *zbuf;
    int osiz;
    char *obuf = tchdbget(hdb, kbuf, ksiz, &osiz);
    if(obuf){
      obuf = tcrealloc(obuf, osiz + vsiz + 1);
      memcpy(obuf + osiz, vbuf, vsiz);
      zbuf = _tc_deflate(obuf, osiz + vsiz, &vsiz, _TC_ZMRAW);
      free(obuf);
    } else {
      zbuf = _tc_deflate(vbuf, vsiz, &vsiz, _TC_ZMRAW);
    }
    if(!zbuf){
      tchdbsetecode(hdb, TCEMISC, __FILE__, __LINE__, __func__);
      return false;
    }
    bool rv = tchdbputimpl(hdb, kbuf, ksiz, zbuf, vsiz, HDBPDOVER);
    free(zbuf);
    return rv;
  }
  return tchdbputimpl(hdb, kbuf, ksiz, vbuf, vsiz, HDBPDCAT);
}


/* Concatenate a stirng value at the end of the existing record in a hash database object. */
bool tchdbputcat2(TCHDB *hdb, const char *kstr, const char *vstr){
  assert(hdb && kstr && vstr);
  return tchdbputcat(hdb, kstr, strlen(kstr), vstr, strlen(vstr));
}


/* Store a record into a hash database object in asynchronous fashion. */
bool tchdbputasync(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz){
  assert(hdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(hdb->fd < 0 || !(hdb->omode & HDBOWRITER)){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  if(hdb->opts & HDBTDEFLATE){
    char *zbuf = _tc_deflate(vbuf, vsiz, &vsiz, _TC_ZMRAW);
    if(!zbuf){
      tchdbsetecode(hdb, TCEMISC, __FILE__, __LINE__, __func__);
      return false;
    }
    bool rv = tchdbputasyncimpl(hdb, kbuf, ksiz, zbuf, vsiz);
    free(zbuf);
    return rv;
  }
  return tchdbputasyncimpl(hdb, kbuf, ksiz, vbuf, vsiz);
}


/* Store a string record into a hash database object in asynchronous fashion. */
bool tchdbputasync2(TCHDB *hdb, const char *kstr, const char *vstr){
  assert(hdb && kstr && vstr);
  return tchdbputasync(hdb, kstr, strlen(kstr), vstr, strlen(vstr));
}


/* Remove a record of a hash database object. */
bool tchdbout(TCHDB *hdb, const char *kbuf, int ksiz){
  assert(hdb && kbuf && ksiz >= 0);
  if(hdb->fd < 0 || !(hdb->omode & HDBOWRITER)){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  if(hdb->drpool && !tchdbflushdrp(hdb)) return false;
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
        free(rec.bbuf);
        rec.bbuf = NULL;
        if(!tchdbwritefb(hdb, rec.off, rec.rsiz)) return false;
        tchdbfbpinsert(hdb, rec.off, rec.rsiz);
        uint64_t child;
        if(rec.left > 0 && rec.right < 1){
          child = rec.left;
        } else if(rec.left < 1 && rec.right > 0){
          child = rec.right;
        } else if(rec.left < 1 && rec.left < 1){
          child = 0;
        } else {
          child = rec.left;
          uint64_t right = rec.right;
          rec.right = child;
          while(rec.right > 0){
            rec.off = rec.right;
            if(!tchdbreadrec(hdb, &rec, rbuf)) return false;
          }
          if(hdb->ba64){
            uint64_t toff = rec.off + (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint64_t));
            uint64_t llnum = right >> hdb->apow;
            llnum = TCHTOILL(llnum);
            if(!tcseekwrite(hdb, toff, &llnum, sizeof(uint64_t))) return false;
          } else {
            uint32_t toff = rec.off + (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t));
            uint32_t lnum = right >> hdb->apow;
            lnum = TCHTOIL(lnum);
            if(!tcseekwrite(hdb, toff, &lnum, sizeof(uint32_t))) return false;
          }
        }
        if(entoff > 0){
          if(hdb->ba64){
            uint64_t llnum = child >> hdb->apow;
            llnum = TCHTOILL(llnum);
            if(!tcseekwrite(hdb, entoff, &llnum, sizeof(uint64_t))) return false;
          } else {
            uint32_t lnum = child >> hdb->apow;
            lnum = TCHTOIL(lnum);
            if(!tcseekwrite(hdb, entoff, &lnum, sizeof(uint32_t))) return false;
          }
        } else {
          tchdbsetbucket(hdb, bidx, child);
        }
        hdb->rnum--;
        uint64_t llnum = hdb->rnum;
        llnum = TCHTOILL(llnum);
        memcpy(hdb->map + HDBRNUMOFF, &llnum, sizeof(llnum));
        return true;
      }
    }
  }
  tchdbsetecode(hdb, TCENOREC, __FILE__, __LINE__, __func__);
  return false;
}


/* Remove a string record of a hash database object. */
bool tchdbout2(TCHDB *hdb, const char *kstr){
  assert(hdb && kstr);
  return tchdbout(hdb, kstr, strlen(kstr));
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


/* Retrieve a record in a hash database object and write the value into a buffer. */
int tchdbget3(TCHDB *hdb, const char *kbuf, int ksiz, char *vbuf, int max){
  assert(hdb && kbuf && ksiz >= 0 && vbuf && max >= 0);
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
    if(!tchdbreadrec(hdb, &rec, rbuf)) return -1;
    if(hash > rec.hash){
      off = rec.left;
    } else if(hash < rec.hash){
      off = rec.right;
    } else {
      if(!rec.kbuf && !tchdbreadrecbody(hdb, &rec)) return -1;
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
        if(!rec.vbuf && !tchdbreadrecbody(hdb, &rec)) return -1;
        if(hdb->opts & HDBTDEFLATE){
          int zsiz;
          char *zbuf = _tc_inflate(rec.vbuf, rec.vsiz, &zsiz, _TC_ZMRAW);
          free(rec.bbuf);
          if(!zbuf){
            tchdbsetecode(hdb, TCEMISC, __FILE__, __LINE__, __func__);
            return -1;
          }
          zsiz = tclmin(zsiz, max);
          memcpy(vbuf, zbuf, zsiz);
          free(zbuf);
          return zsiz;
        }
        int vsiz = tclmin(rec.vsiz, max);
        memcpy(vbuf, rec.vbuf, vsiz);
        if(rec.bbuf) free(rec.bbuf);
        return vsiz;
      }
    }
  }
  tchdbsetecode(hdb, TCENOREC, __FILE__, __LINE__, __func__);
  return -1;
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


/* Get the next extensible objects of the iterator of a hash database object. */
bool tchdbiternext3(TCHDB *hdb, TCXSTR *kxstr, TCXSTR *vxstr){
  assert(hdb && kxstr && vxstr);
  if(hdb->fd < 0 || hdb->iter < 1){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return NULL;
  }
  if(hdb->drpool && !tchdbflushdrp(hdb)) return false;
  TCHREC rec;
  char rbuf[HDBIOBUFSIZ];
  while(hdb->iter < hdb->fsiz){
    rec.off = hdb->iter;
    if(!tchdbreadrec(hdb, &rec, rbuf)) return false;
    hdb->iter += rec.rsiz;
    if(rec.magic == HDBMAGICREC){
      if(!rec.vbuf && !tchdbreadrecbody(hdb, &rec)) return false;
      tcxstrclear(kxstr);
      tcxstrcat(kxstr, rec.kbuf, rec.ksiz);
      tcxstrclear(vxstr);
      if(hdb->opts & HDBTDEFLATE){
        int zsiz;
        char *zbuf = _tc_inflate(rec.vbuf, rec.vsiz, &zsiz, _TC_ZMRAW);
        if(!zbuf){
          tchdbsetecode(hdb, TCEMISC, __FILE__, __LINE__, __func__);
          free(rec.bbuf);
          return false;
        }
        tcxstrcat(vxstr, zbuf, zsiz);
        free(zbuf);
      } else {
        tcxstrcat(vxstr, rec.vbuf, rec.vsiz);
      }
      free(rec.bbuf);
      return true;
    }
  }
  tchdbsetecode(hdb, TCENOREC, __FILE__, __LINE__, __func__);
  return false;
}


/* Synchronize updated contents of a hash database object with the file and the device. */
bool tchdbsync(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0 || !(hdb->omode & HDBOWRITER)){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  if(hdb->drpool && !tchdbflushdrp(hdb)) return false;
  return tchdbmemsync(hdb, true);
}


/* Optimize the file of a hash database object. */
bool tchdboptimize(TCHDB *hdb, int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts){
  assert(hdb);
  if(hdb->fd < 0 || !(hdb->omode & HDBOWRITER)){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return false;
  }
  if(hdb->drpool && !tchdbflushdrp(hdb)) return false;
  if(bnum < 1) bnum = tclmax(hdb->rnum * 2 + 1, HDBDEFBNUM);
  if(apow < 0) apow = hdb->apow;
  if(fpow < 0) fpow = hdb->fpow;
  if(opts == UINT8_MAX) opts = hdb->opts;
  char *tpath = tcsprintf("%s%ctmp%c%llu", hdb->path, MYEXTCHR, MYEXTCHR, hdb->inode);
  TCHDB *thdb = tchdbnew();
  tchdbtune(thdb, bnum, apow, fpow, opts);
  if(!tchdbopen(thdb, tpath, HDBOWRITER | HDBOCREAT | HDBOTRUNC)){
    tchdbsetecode(hdb, thdb->ecode, __FILE__, __LINE__, __func__);
    tchdbdel(thdb);
    free(tpath);
    return false;
  }
  bool err = false;
  if(!tchdbiterinit(hdb)) err = true;
  TCXSTR *key = tcxstrnew();
  TCXSTR *val = tcxstrnew();
  while(!err && tchdbiternext3(hdb, key, val)){
    if(!tchdbputkeep(thdb, tcxstrptr(key), tcxstrsize(key), tcxstrptr(val), tcxstrsize(val))){
      tchdbsetecode(hdb, thdb->ecode, __FILE__, __LINE__, __func__);
      err = true;
    }
  }
  tcxstrdel(val);
  tcxstrdel(key);
  if(!tchdbclose(thdb)){
    tchdbsetecode(hdb, thdb->ecode, __FILE__, __LINE__, __func__);
    err = true;
  }
  tchdbdel(thdb);
  if(unlink(hdb->path) == -1){
    tchdbsetecode(hdb, TCEUNLINK, __FILE__, __LINE__, __func__);
    err = true;
  }
  if(rename(tpath, hdb->path) == -1){
    tchdbsetecode(hdb, TCERENAME, __FILE__, __LINE__, __func__);
    err = true;
  }
  free(tpath);
  if(err) return false;
  tpath = tcstrdup(hdb->path);
  int omode = (hdb->omode & ~HDBOCREAT) & ~HDBOTRUNC;
  if(!tchdbclose(hdb)){
    free(tpath);
    return false;
  }
  bool rv = tchdbopen(hdb, tpath, omode);
  free(tpath);
  return rv;
}


/* Get the file path of a hash database object. */
const char *tchdbpath(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return NULL;
  }
  return hdb->path;
}


/* Get the number of elements of the bucket array of a hash database object. */
uint64_t tchdbbnum(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  return hdb->bnum;
}


/* Get the record alignment a hash database object. */
uint32_t tchdbalign(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  return hdb->align;
}


/* Get the maximum number of the free block pool of a a hash database object. */
uint32_t tchdbfbpmax(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  return hdb->fbpmax;
}


/* Get the number of records of a hash database object. */
uint64_t tchdbrnum(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  return hdb->rnum;
}


/* Get the size of the database file of a hash database object. */
uint64_t tchdbfsiz(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  return hdb->fsiz;
}


/* Get the inode number of the database file of a hash database object. */
uint64_t tchdbinode(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  return hdb->inode;
}


/* Get the modification time of the database file of a hash database object. */
time_t tchdbmtime(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  return hdb->mtime;
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


/* Set the type of a hash database object. */
void tchdbsettype(TCHDB *hdb, uint8_t type){
  assert(hdb);
  if(hdb->fd >= 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return;
  }
  hdb->type = type;
}


/* Set the file descriptor for debugging output. */
void tchdbsetdbgfd(TCHDB *hdb, int fd){
  assert(hdb && fd >= 0);
  hdb->dbgfd = fd;
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


/* Get the database type of a hash database object. */
uint8_t tchdbtype(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  return hdb->type;
}


/* Get the additional flags of a hash database object. */
uint8_t tchdbflags(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  return hdb->flags;
}


/* Get the options of a hash database object. */
uint8_t tchdbopts(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  return hdb->opts;
}


/* Get the number of used elements of the bucket array of a hash database object. */
uint64_t tchdbbnumused(TCHDB *hdb){
  assert(hdb);
  if(hdb->fd < 0){
    tchdbsetecode(hdb, TCEINVALID, __FILE__, __LINE__, __func__);
    return 0;
  }
  uint64_t unum = 0;
  if(hdb->ba64){
    uint64_t *buckets = hdb->ba64;
    for(int i = 0; i < hdb->bnum; i++){
      if(buckets[i]) unum++;
    }
  } else {
    uint32_t *buckets = hdb->ba32;
    for(int i = 0; i < hdb->bnum; i++){
      if(buckets[i]) unum++;
    }
  }
  return unum;
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


/* Get a natural prime number not less than a floor number.
   `num' specified the floor number.
   The return value is a prime number not less than the floor number. */
static uint64_t tcgetprime(int num){
  assert(num >= 0);
  uint64_t primes[] = {
    1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 43, 47, 53, 59, 61, 71, 79, 83,
    89, 103, 109, 113, 127, 139, 157, 173, 191, 199, 223, 239, 251, 283, 317, 349,
    383, 409, 443, 479, 509, 571, 631, 701, 761, 829, 887, 953, 1021, 1151, 1279,
    1399, 1531, 1663, 1789, 1913, 2039, 2297, 2557, 2803, 3067, 3323, 3583, 3833,
    4093, 4603, 5119, 5623, 6143, 6653, 7159, 7673, 8191, 9209, 10223, 11261,
    12281, 13309, 14327, 15359, 16381, 18427, 20479, 22511, 24571, 26597, 28669,
    30713, 32749, 36857, 40949, 45053, 49139, 53239, 57331, 61417, 65521, 73727,
    81919, 90107, 98299, 106487, 114679, 122869, 131071, 147451, 163819, 180221,
    196597, 212987, 229373, 245759, 262139, 294911, 327673, 360439, 393209, 425977,
    458747, 491503, 524287, 589811, 655357, 720887, 786431, 851957, 917503, 982981,
    1048573, 1179641, 1310719, 1441771, 1572853, 1703903, 1835003, 1966079,
    2097143, 2359267, 2621431, 2883577, 3145721, 3407857, 3670013, 3932153,
    4194301, 4718579, 5242877, 5767129, 6291449, 6815741, 7340009, 7864301,
    8388593, 9437179, 10485751, 11534329, 12582893, 13631477, 14680063, 15728611,
    16777213, 18874367, 20971507, 23068667, 25165813, 27262931, 29360087, 31457269,
    33554393, 37748717, 41943023, 46137319, 50331599, 54525917, 58720253, 62914549,
    67108859, 75497467, 83886053, 92274671, 100663291, 109051903, 117440509,
    125829103, 134217689, 150994939, 167772107, 184549373, 201326557, 218103799,
    234881011, 251658227, 268435399, 301989881, 335544301, 369098707, 402653171,
    436207613, 469762043, 503316469, 536870909, 603979769, 671088637, 738197503,
    805306357, 872415211, 939524087, 1006632947, 1073741789, 1207959503,
    1342177237, 1476394991, 1610612711, 1744830457, 1879048183, 2013265907,
    2576980349, 3092376431, 3710851741, 4718021527, 6133428047, 7973456459,
    10365493393, 13475141413, 17517683831, 22772988923, 29604885677, 38486351381,
    50032256819, 65041933867, 84554514043, 109920868241, 0
  };
  int i;
  for(i = 0; primes[i] > 0; i++){
    if(num <= primes[i]) return primes[i];
  }
  return primes[i-1];
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


/* Append a record to the delayed record pool.
   `hdb' specifies the hash database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value. */
static void tchdbdrpappend(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz){
  assert(hdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  TCDODEBUG(hdb->cnt_appenddrp++);
  char rbuf[HDBIOBUFSIZ];
  char *wp = rbuf;
  *(uint8_t *)(wp++) = HDBMAGICREC;
  *(uint8_t *)(wp++) = tcrechash(kbuf, ksiz);
  if(hdb->ba64){
    memset(wp, 0, sizeof(uint64_t) * 2);
    wp += sizeof(uint64_t) * 2;
  } else {
    memset(wp, 0, sizeof(uint32_t) * 2);
    wp += sizeof(uint32_t) * 2;
  }
  uint16_t snum;
  char *pwp = wp;
  wp += sizeof(snum);
  int step;
  TC_SETVNUMBUF(step, wp, ksiz);
  wp += step;
  TC_SETVNUMBUF(step, wp, vsiz);
  wp += step;
  int32_t hsiz = wp - rbuf;
  int32_t rsiz = hsiz + ksiz + vsiz;
  hdb->fsiz += rsiz;
  uint16_t psiz = tchdbpadsize(hdb);
  hdb->fsiz += psiz;
  snum = TCHTOIS(psiz);
  memcpy(pwp, &snum, sizeof(snum));
  TCXSTR *drpool = hdb->drpool;
  tcxstrcat(drpool, rbuf, hsiz);
  tcxstrcat(drpool, kbuf, ksiz);
  tcxstrcat(drpool, vbuf, vsiz);
  if(psiz > 0){
    char pbuf[psiz];
    memset(pbuf, 0, psiz);
    tcxstrcat(drpool, pbuf, psiz);
  }
}


/* Store a record in asynchronus fashion.
   `hdb' specifies the hash database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false. */
static bool tchdbputasyncimpl(TCHDB *hdb, const char *kbuf, int ksiz, const char *vbuf, int vsiz){
  assert(hdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(!hdb->drpool){
    hdb->drpool = tcxstrnew3(HDBDRPUNIT + HDBDRPLAT);
    hdb->drpdef = tcxstrnew3(HDBDRPUNIT);
  }
  uint64_t bidx = tchdbbidx(hdb, kbuf, ksiz);
  uint64_t off = tchdbgetbucket(hdb, bidx);
  if(off > 0){
    TCDODEBUG(hdb->cnt_deferdrp++);
    TCXSTR *drpdef = hdb->drpdef;
    tcxstrcat(drpdef, &ksiz, sizeof(ksiz));
    tcxstrcat(drpdef, &vsiz, sizeof(vsiz));
    tcxstrcat(drpdef, kbuf, ksiz);
    tcxstrcat(drpdef, vbuf, vsiz);
    return true;
  }
  tchdbsetbucket(hdb, bidx, hdb->fsiz);
  tchdbdrpappend(hdb, kbuf, ksiz, vbuf, vsiz);
  hdb->rnum++;
  if(tcxstrsize(hdb->drpool) > HDBDRPUNIT && !tchdbflushdrp(hdb)) return false;
  return true;
}



/*************************************************************************************************
 * debugging functions
 *************************************************************************************************/


/* Print meta data of the header into the debugging output.
   `hdb' specifies the hash database object. */
void tchdbprintmeta(TCHDB *hdb){
  assert(hdb);
  if(hdb->dbgfd < 0) return;
  char buf[HDBIOBUFSIZ];
  char *wp = buf;
  wp += sprintf(wp, "META:");
  wp += sprintf(wp, " type=%02X", hdb->type);
  wp += sprintf(wp, " flags=%02X", hdb->flags);
  wp += sprintf(wp, " bnum=%llu", (unsigned long long)hdb->bnum);
  wp += sprintf(wp, " apow=%u", hdb->apow);
  wp += sprintf(wp, " fpow=%u", hdb->fpow);
  wp += sprintf(wp, " opts=%u", hdb->opts);
  wp += sprintf(wp, " path=%s", hdb->path ? hdb->path : "-");
  wp += sprintf(wp, " fd=%u", hdb->fd);
  wp += sprintf(wp, " omode=%u", hdb->omode);
  wp += sprintf(wp, " rnum=%llu", (unsigned long long)hdb->rnum);
  wp += sprintf(wp, " fsiz=%llu", (unsigned long long)hdb->fsiz);
  wp += sprintf(wp, " frec=%llu", (unsigned long long)hdb->frec);
  wp += sprintf(wp, " iter=%llu", (unsigned long long)hdb->iter);
  wp += sprintf(wp, " map=%p", (void *)hdb->map);
  wp += sprintf(wp, " msiz=%llu", (unsigned long long)hdb->msiz);
  wp += sprintf(wp, " ba32=%p", (void *)hdb->ba32);
  wp += sprintf(wp, " ba64=%p", (void *)hdb->ba64);
  wp += sprintf(wp, " align=%u", hdb->align);
  wp += sprintf(wp, " runit=%u", hdb->runit);
  wp += sprintf(wp, " fbpmax=%d", hdb->fbpmax);
  wp += sprintf(wp, " fbpsiz=%d", hdb->fbpsiz);
  wp += sprintf(wp, " fbpool=%p", (void *)hdb->fbpool);
  wp += sprintf(wp, " fbpnum=%d", hdb->fbpnum);
  wp += sprintf(wp, " fbpmis=%d", hdb->fbpmis);
  wp += sprintf(wp, " drpool=%p", (void *)hdb->drpool);
  wp += sprintf(wp, " drpdef=%p", (void *)hdb->drpdef);
  wp += sprintf(wp, " ecode=%d", hdb->ecode);
  wp += sprintf(wp, " fatal=%u", hdb->fatal);
  wp += sprintf(wp, " inode=%llu", (unsigned long long)(uint64_t)hdb->inode);
  wp += sprintf(wp, " mtime=%llu", (unsigned long long)(uint64_t)hdb->mtime);
  wp += sprintf(wp, " dbgfd=%d", hdb->dbgfd);
  wp += sprintf(wp, " cnt_writerec=%lld", (long long)hdb->cnt_writerec);
  wp += sprintf(wp, " cnt_reuserec=%lld", (long long)hdb->cnt_reuserec);
  wp += sprintf(wp, " cnt_moverec=%lld", (long long)hdb->cnt_moverec);
  wp += sprintf(wp, " cnt_readrec=%lld", (long long)hdb->cnt_readrec);
  wp += sprintf(wp, " cnt_searchfbp=%lld", (long long)hdb->cnt_searchfbp);
  wp += sprintf(wp, " cnt_insertfbp=%lld", (long long)hdb->cnt_insertfbp);
  wp += sprintf(wp, " cnt_splicefbp=%lld", (long long)hdb->cnt_splicefbp);
  wp += sprintf(wp, " cnt_dividefbp=%lld", (long long)hdb->cnt_dividefbp);
  wp += sprintf(wp, " cnt_mergefbp=%lld", (long long)hdb->cnt_mergefbp);
  wp += sprintf(wp, " cnt_reducefbp=%lld", (long long)hdb->cnt_reducefbp);
  wp += sprintf(wp, " cnt_appenddrp=%lld", (long long)hdb->cnt_appenddrp);
  wp += sprintf(wp, " cnt_deferdrp=%lld", (long long)hdb->cnt_deferdrp);
  wp += sprintf(wp, " cnt_flushdrp=%lld", (long long)hdb->cnt_flushdrp);
  *(wp++) = '\n';
  tcwrite(hdb->dbgfd, buf, wp - buf);
}


/* Print a record information into the debugging output.
   `hdb' specifies the hash database object.
   `rec' specifies the record. */
void tchdbprintrec(TCHDB *hdb, TCHREC *rec){
  assert(hdb && rec);
  if(hdb->dbgfd < 0) return;
  char buf[HDBIOBUFSIZ];
  char *wp = buf;
  wp += sprintf(wp, "REC:");
  wp += sprintf(wp, " off=%llu", (unsigned long long)rec->off);
  wp += sprintf(wp, " rsiz=%u", rec->rsiz);
  wp += sprintf(wp, " magic=%02X", rec->magic);
  wp += sprintf(wp, " hash=%02X", rec->hash);
  wp += sprintf(wp, " left=%llu", (unsigned long long)rec->left);
  wp += sprintf(wp, " right=%llu", (unsigned long long)rec->right);
  wp += sprintf(wp, " ksiz=%u", rec->ksiz);
  wp += sprintf(wp, " vsiz=%u", rec->vsiz);
  wp += sprintf(wp, " psiz=%u", rec->psiz);
  wp += sprintf(wp, " kbuf=%p", (void *)rec->kbuf);
  wp += sprintf(wp, " vbuf=%p", (void *)rec->vbuf);
  wp += sprintf(wp, " boff=%llu", (unsigned long long)rec->boff);
  wp += sprintf(wp, " bbuf=%p", (void *)rec->bbuf);
  *(wp++) = '\n';
  tcwrite(hdb->dbgfd, buf, wp - buf);
}



/* END OF FILE */
