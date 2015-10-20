
#include <tcutil.h>
#include <tchdb.h>
#include "myconf.h"

#define RECBUFSIZ      32                // buffer for records


/* global variables */
const char *g_progname;                  // program name
int g_dbgfd;                             // debugging output


/* function prototypes */
int main(int argc, char **argv);
static void usage(void);
static void printerr(TCHDB *hdb);
static int printdata(const char *ptr, int size, bool px);
static char *hextoobj(const char *str, int *sp);
static int runcreate(int argc, char **argv);
static int runinform(int argc, char **argv);
static int runput(int argc, char **argv);
static int runout(int argc, char **argv);
static int runget(int argc, char **argv);
static int runlist(int argc, char **argv);
static int runoptimize(int argc, char **argv);
static int runversion(int argc, char **argv);
static int proccreate(const char *path, int bnum, int apow, int fpow, int opts);
static int procinform(const char *path, int omode);
static int procput(const char *path, const char *kbuf, int ksiz, const char *vbuf, int vsiz,
                   int omode, int dmode);
static int procout(const char *path, const char *kbuf, int ksiz, int omode);
static int procget(const char *path, const char *kbuf, int ksiz, int omode, bool px, bool pz);
static int proclist(const char *path, int omode, bool pv, bool px);
static int procoptimize(const char *path, int bnum, int apow, int fpow, int opts, int omode);
static int procversion(void);


/* main routine */
int main(int argc, char **argv){
  g_progname = argv[0];
  g_dbgfd = -1;
  const char *ebuf = getenv("TCDBGFD");
  if(ebuf) g_dbgfd = atoi(ebuf);
  if(argc < 2) usage();
  int rv = 0;
  if(!strcmp(argv[1], "create")){
    rv = runcreate(argc, argv);
  } else if(!strcmp(argv[1], "inform")){
    rv = runinform(argc, argv);
  } else if(!strcmp(argv[1], "put")){
    rv = runput(argc, argv);
  } else if(!strcmp(argv[1], "out")){
    rv = runout(argc, argv);
  } else if(!strcmp(argv[1], "get")){
    rv = runget(argc, argv);
  } else if(!strcmp(argv[1], "list")){
    rv = runlist(argc, argv);
  } else if(!strcmp(argv[1], "optimize")){
    rv = runoptimize(argc, argv);
  } else if(!strcmp(argv[1], "version") || !strcmp(argv[1], "--version")){
    rv = runversion(argc, argv);
  } else {
    usage();
  }
  return rv;
}


/* print the usage and exit */
static void usage(void){
  fprintf(stderr, "%s: the command line utility of the hash database API\n", g_progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  %s create [-tl] [-td] path [bnum] [apow] [fpow]\n", g_progname);
  fprintf(stderr, "  %s inform [-nl|-nb] path\n", g_progname);
  fprintf(stderr, "  %s put [-nl|-nb] [-sx] [-dk|-dc] path key value\n", g_progname);
  fprintf(stderr, "  %s out [-nl|-nb] [-sx] path key\n", g_progname);
  fprintf(stderr, "  %s get [-nl|-nb] [-sx] [-px] [-nl] path key\n", g_progname);
  fprintf(stderr, "  %s list [-nl|-nb] [-pv] path\n", g_progname);
  fprintf(stderr, "  %s optimize [-tl] [-td] [-tz] [-nl|-nb] path [bnum] [apow] [fpow]\n",
          g_progname);
  fprintf(stderr, "  %s version\n", g_progname);
  fprintf(stderr, "\n");
  exit(1);
}


/* print error information */
static void printerr(TCHDB *hdb){
  const char *path = tchdbpath(hdb);
  int ecode = tchdbecode(hdb);
  fprintf(stderr, "%s: %s: %d: %s\n", g_progname, path ? path : "-", ecode, tchdberrmsg(ecode));
}


/* print record data */
static int printdata(const char *ptr, int size, bool px){
  int len = 0;
  while(size-- > 0){
    if(px){
      if(len > 0) putchar(' ');
      len += printf("%02X", *(unsigned char *)ptr);
    } else {
      putchar(*ptr);
      len++;
    }
    ptr++;
  }
  return len;
}


/* create a binary object from a hexadecimal string */
static char *hextoobj(const char *str, int *sp){
  int len = strlen(str);
  char *buf = tcmalloc(len + 1);
  int j = 0;
  for(int i = 0; i < len; i += 2){
    while(strchr(" \n\r\t\f\v", str[i])){
      i++;
    }
    char mbuf[3];
    if((mbuf[0] = str[i]) == '\0') break;
    if((mbuf[1] = str[i+1]) == '\0') break;
    mbuf[2] = '\0';
    buf[j++] = (char)strtol(mbuf, NULL, 16);
  }
  buf[j] = '\0';
  *sp = j;
  return buf;
}


/* parse arguments of create command */
static int runcreate(int argc, char **argv){
  char *path = NULL;
  char *bstr = NULL;
  char *astr = NULL;
  char *fstr = NULL;
  int opts = 0;
  for(int i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      if(!strcmp(argv[i], "-tl")){
        opts |= HDBTLARGE;
      } else if(!strcmp(argv[i], "-td")){
        opts |= HDBTDEFLATE;
      } else {
        usage();
      }
    } else if(!path){
      path = argv[i];
    } else if(!bstr){
      bstr = argv[i];
    } else if(!astr){
      astr = argv[i];
    } else if(!fstr){
      fstr = argv[i];
    } else {
      usage();
    }
  }
  if(!path) usage();
  int bnum = bstr ? atoi(bstr) : -1;
  int apow = astr ? atoi(astr) : -1;
  int fpow = fstr ? atoi(fstr) : -1;
  int rv = proccreate(path, bnum, apow, fpow, opts);
  return rv;
}


/* parse arguments of inform command */
static int runinform(int argc, char **argv){
  char *path = NULL;
  int omode = 0;
  for(int i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      if(!strcmp(argv[i], "-nl")){
        omode |= HDBONOLCK;
      } else if(!strcmp(argv[i], "-nb")){
        omode |= HDBOLCKNB;
      } else {
        usage();
      }
    } else if(!path){
      path = argv[i];
    } else {
      usage();
    }
  }
  if(!path) usage();
  int rv = procinform(path, omode);
  return rv;
}


/* parse arguments of put command */
static int runput(int argc, char **argv){
  char *path = NULL;
  char *key = NULL;
  char *value = NULL;
  int omode = 0;
  int dmode = 0;
  bool sx = false;
  for(int i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      if(!strcmp(argv[i], "-nl")){
        omode |= HDBONOLCK;
      } else if(!strcmp(argv[i], "-nb")){
        omode |= HDBOLCKNB;
      } else if(!strcmp(argv[i], "-dk")){
        dmode = -1;
      } else if(!strcmp(argv[i], "-dc")){
        dmode = 1;
      } else if(!strcmp(argv[i], "-sx")){
        sx = true;
      } else {
        usage();
      }
    } else if(!path){
      path = argv[i];
    } else if(!key){
      key = argv[i];
    } else if(!value){
      value = argv[i];
    } else {
      usage();
    }
  }
  if(!path || !key || !value) usage();
  int ksiz, vsiz;
  char *kbuf, *vbuf;
  if(sx){
    kbuf = hextoobj(key, &ksiz);
    vbuf = hextoobj(value, &vsiz);
  } else {
    ksiz = strlen(key);
    kbuf = tcmemdup(key, ksiz);
    vsiz = strlen(value);
    vbuf = tcmemdup(value, vsiz);
  }
  int rv = procput(path, kbuf, ksiz, vbuf, vsiz, omode, dmode);
  free(vbuf);
  free(kbuf);
  return rv;
}


/* parse arguments of out command */
static int runout(int argc, char **argv){
  char *path = NULL;
  char *key = NULL;
  int omode = 0;
  bool sx = false;
  for(int i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      if(!strcmp(argv[i], "-nl")){
        omode |= HDBONOLCK;
      } else if(!strcmp(argv[i], "-nb")){
        omode |= HDBOLCKNB;
      } else if(!strcmp(argv[i], "-sx")){
        sx = true;
      } else {
        usage();
      }
    } else if(!path){
      path = argv[i];
    } else if(!key){
      key = argv[i];
    } else {
      usage();
    }
  }
  if(!path || !key) usage();
  int ksiz;
  char *kbuf;
  if(sx){
    kbuf = hextoobj(key, &ksiz);
  } else {
    ksiz = strlen(key);
    kbuf = tcmemdup(key, ksiz);
  }
  int rv = procout(path, kbuf, ksiz, omode);
  free(kbuf);
  return rv;
}


/* parse arguments of get command */
static int runget(int argc, char **argv){
  char *path = NULL;
  char *key = NULL;
  int omode = 0;
  bool sx = false;
  bool px = false;
  bool pz = false;
  for(int i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      if(!strcmp(argv[i], "-nl")){
        omode |= HDBONOLCK;
      } else if(!strcmp(argv[i], "-nb")){
        omode |= HDBOLCKNB;
      } else if(!strcmp(argv[i], "-sx")){
        sx = true;
      } else if(!strcmp(argv[i], "-px")){
        px = true;
      } else if(!strcmp(argv[i], "-pz")){
        pz = true;
      } else {
        usage();
      }
    } else if(!path){
      path = argv[i];
    } else if(!key){
      key = argv[i];
    } else {
      usage();
    }
  }
  if(!path || !key) usage();
  int ksiz;
  char *kbuf;
  if(sx){
    kbuf = hextoobj(key, &ksiz);
  } else {
    ksiz = strlen(key);
    kbuf = tcmemdup(key, ksiz);
  }
  int rv = procget(path, kbuf, ksiz, omode, px, pz);
  free(kbuf);
  return rv;
}


/* parse arguments of list command */
static int runlist(int argc, char **argv){
  char *path = NULL;
  int omode = 0;
  bool pv = false;
  bool px = false;
  for(int i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      if(!strcmp(argv[i], "-nl")){
        omode |= HDBONOLCK;
      } else if(!strcmp(argv[i], "-nb")){
        omode |= HDBOLCKNB;
      } else if(!strcmp(argv[i], "-pv")){
        pv = true;
      } else if(!strcmp(argv[i], "-px")){
        px = true;
      } else {
        usage();
      }
    } else if(!path){
      path = argv[i];
    } else {
      usage();
    }
  }
  if(!path) usage();
  int rv = proclist(path, omode, pv, px);
  return rv;
}


/* parse arguments of optimize command */
static int runoptimize(int argc, char **argv){
  char *path = NULL;
  char *bstr = NULL;
  char *astr = NULL;
  char *fstr = NULL;
  int opts = UINT8_MAX;
  int omode = 0;
  for(int i = 2; i < argc; i++){
    if(argv[i][0] == '-'){
      if(!strcmp(argv[i], "-tl")){
        if(opts == UINT8_MAX) opts = 0;
        opts |= HDBTLARGE;
      } else if(!strcmp(argv[i], "-td")){
        if(opts == UINT8_MAX) opts = 0;
        opts |= HDBTDEFLATE;
      } else if(!strcmp(argv[i], "-tz")){
        if(opts == UINT8_MAX) opts = 0;
      } else if(!strcmp(argv[i], "-nl")){
        omode |= HDBONOLCK;
      } else if(!strcmp(argv[i], "-nb")){
        omode |= HDBOLCKNB;
      } else {
        usage();
      }
    } else if(!path){
      path = argv[i];
    } else if(!bstr){
      bstr = argv[i];
    } else if(!astr){
      astr = argv[i];
    } else if(!fstr){
      fstr = argv[i];
    } else {
      usage();
    }
  }
  if(!path) usage();
  int bnum = bstr ? atoi(bstr) : -1;
  int apow = astr ? atoi(astr) : -1;
  int fpow = fstr ? atoi(fstr) : -1;
  int rv = procoptimize(path, bnum, apow, fpow, opts, omode);
  return rv;
}


/* parse arguments of version command */
static int runversion(int argc, char **argv){
  int rv = procversion();
  return rv;
}


/* perform create command */
static int proccreate(const char *path, int bnum, int apow, int fpow, int opts){
  TCHDB *hdb = tchdbnew();
  if(g_dbgfd >= 0) tchdbsetdbgfd(hdb, g_dbgfd);
  if(!tchdbtune(hdb, bnum, apow, fpow, opts)){
    printerr(hdb);
    tchdbdel(hdb);
    return 1;
  }
  if(!tchdbopen(hdb, path, HDBOWRITER | HDBOCREAT | HDBOTRUNC)){
    printerr(hdb);
    tchdbdel(hdb);
    return 1;
  }
  bool err = false;
  if(!tchdbclose(hdb)){
    printerr(hdb);
    err = true;
  }
  tchdbdel(hdb);
  return err ? 1 : 0;
}


/* perform inform command */
static int procinform(const char *path, int omode){
  TCHDB *hdb = tchdbnew();
  if(g_dbgfd >= 0) tchdbsetdbgfd(hdb, g_dbgfd);
  if(!tchdbopen(hdb, path, HDBOREADER | omode)){
    printerr(hdb);
    tchdbdel(hdb);
    return 1;
  }
  bool err = false;
  const char *npath = tchdbpath(hdb);
  if(!npath) npath = "(unknown)";
  printf("path: %s\n", npath);
  const char *type = "(unknown)";
  switch(tchdbtype(hdb)){
  case HDBTHASH: type = "hash"; break;
  case HDBTBTREE: type = "btree"; break;
  }
  printf("database type: %s\n", type);
  uint8_t flags = tchdbflags(hdb);
  printf("additional flags:");
  if(flags & HDBFOPEN) printf(" open");
  if(flags & HDBFFATAL) printf(" fatal");
  printf("\n");
  printf("bucket number: %llu\n", (unsigned long long)tchdbbnum(hdb));
  if(hdb->cnt_writerec >= 0)
    printf("used bucket number: %lld\n", (long long)tchdbbnumused(hdb));
  printf("alignment: %u\n", tchdbalign(hdb));
  printf("free block pool: %u\n", tchdbfbpmax(hdb));
  uint8_t opts = tchdbopts(hdb);
  printf("options:");
  if(opts & HDBTLARGE) printf(" large");
  if(opts & HDBTDEFLATE) printf(" deflate");
  printf("\n");
  printf("record number: %llu\n", (unsigned long long)tchdbrnum(hdb));
  printf("file size: %llu\n", (unsigned long long)tchdbfsiz(hdb));
  if(!tchdbclose(hdb)){
    if(!err) printerr(hdb);
    err = true;
  }
  tchdbdel(hdb);
  return err ? 1 : 0;
}


/* perform put command */
static int procput(const char *path, const char *kbuf, int ksiz, const char *vbuf, int vsiz,
                   int omode, int dmode){
  TCHDB *hdb = tchdbnew();
  if(g_dbgfd >= 0) tchdbsetdbgfd(hdb, g_dbgfd);
  if(!tchdbopen(hdb, path, HDBOWRITER | omode)){
    printerr(hdb);
    tchdbdel(hdb);
    return 1;
  }
  bool err = false;
  switch(dmode){
  case -1:
    if(!tchdbputkeep(hdb, kbuf, ksiz, vbuf, vsiz)){
      printerr(hdb);
      err = true;
    }
    break;
  case 1:
    if(!tchdbputcat(hdb, kbuf, ksiz, vbuf, vsiz)){
      printerr(hdb);
      err = true;
    }
    break;
  default:
    if(!tchdbput(hdb, kbuf, ksiz, vbuf, vsiz)){
      printerr(hdb);
      err = true;
    }
    break;
  }
  if(!tchdbclose(hdb)){
    if(!err) printerr(hdb);
    err = true;
  }
  tchdbdel(hdb);
  return err ? 1 : 0;
}


/* perform out command */
static int procout(const char *path, const char *kbuf, int ksiz, int omode){
  TCHDB *hdb = tchdbnew();
  if(g_dbgfd >= 0) tchdbsetdbgfd(hdb, g_dbgfd);
  if(!tchdbopen(hdb, path, HDBOWRITER | omode)){
    printerr(hdb);
    tchdbdel(hdb);
    return 1;
  }
  bool err = false;
  if(!tchdbout(hdb, kbuf, ksiz)){
    printerr(hdb);
    err = true;
  }
  if(!tchdbclose(hdb)){
    if(!err) printerr(hdb);
    err = true;
  }
  tchdbdel(hdb);
  return err ? 1 : 0;
}


/* perform get command */
static int procget(const char *path, const char *kbuf, int ksiz, int omode, bool px, bool pz){
  TCHDB *hdb = tchdbnew();
  if(g_dbgfd >= 0) tchdbsetdbgfd(hdb, g_dbgfd);
  if(!tchdbopen(hdb, path, HDBOWRITER | omode)){
    printerr(hdb);
    tchdbdel(hdb);
    return 1;
  }
  bool err = false;
  int vsiz;
  char *vbuf = tchdbget(hdb, kbuf, ksiz, &vsiz);
  if(vbuf){
    printdata(vbuf, vsiz, px);
    if(!pz) putchar('\n');
    free(vbuf);
  } else {
    printerr(hdb);
    err = true;
  }
  if(!tchdbclose(hdb)){
    if(!err) printerr(hdb);
    err = true;
  }
  tchdbdel(hdb);
  return err ? 1 : 0;
}


/* perform list command */
static int proclist(const char *path, int omode, bool pv, bool px){
  TCHDB *hdb = tchdbnew();
  if(g_dbgfd >= 0) tchdbsetdbgfd(hdb, g_dbgfd);
  if(!tchdbopen(hdb, path, HDBOREADER | omode)){
    printerr(hdb);
    tchdbdel(hdb);
    return 1;
  }
  bool err = false;
  if(!tchdbiterinit(hdb)){
    printerr(hdb);
    err = true;
  }
  TCXSTR *key = tcxstrnew();
  TCXSTR *val = tcxstrnew();
  while(tchdbiternext3(hdb, key, val)){
    printdata(tcxstrptr(key), tcxstrsize(key), px);
    if(pv){
      putchar('\t');
      printdata(tcxstrptr(val), tcxstrsize(val), px);
    }
    putchar('\n');
  }
  tcxstrdel(val);
  tcxstrdel(key);
  if(!tchdbclose(hdb)){
    if(!err) printerr(hdb);
    err = true;
  }
  tchdbdel(hdb);
  return err ? 1 : 0;
}


/* perform optimize command */
static int procoptimize(const char *path, int bnum, int apow, int fpow, int opts, int omode){
  TCHDB *hdb = tchdbnew();
  if(g_dbgfd >= 0) tchdbsetdbgfd(hdb, g_dbgfd);
  if(!tchdbopen(hdb, path, HDBOWRITER | omode)){
    printerr(hdb);
    tchdbdel(hdb);
    return 1;
  }
  bool err = false;
  if(!tchdboptimize(hdb, bnum, apow, fpow, opts)){
    printerr(hdb);
    err = true;
  }
  if(!tchdbclose(hdb)){
    if(!err) printerr(hdb);
    err = true;
  }
  tchdbdel(hdb);
  return err ? 1 : 0;
}


/* perform version command */
static int procversion(void){
  printf("Tokyo Cabinet version %s (%d:%s)\n", tcversion, _TC_LIBVER, _TC_FORMATVER);
  printf("Copyright (C) 2006-2007 Mikio Hirabayashi\n");
  return 0;
}
