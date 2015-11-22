#================================================================
# Setting Variables
#================================================================

# Generic settings
SHELL = /bin/sh

# Targets

LIBRARYFILES = libdict.so
LIBOBJFILES = tcutil.o tchdb.o myconf.o
#COMMANDFILES = dict_tool

# Install destinations
prefix = /usr/local
exec_prefix = ${prefix}
INCLUDEDIR = ${prefix}/include
LIBDIR = ${exec_prefix}/lib
BINDIR = ${exec_prefix}/bin
LIBEXECDIR = ${exec_prefix}/libexec
DATADIR = ${prefix}/share/$(PACKAGE)


# Building configuration
CC = gcc
CPPFLAGS = -I. -I$(INCLUDEDIR) -L/root/include -L/usr/local/include -DNDEBUG -D_GNU_SOURCE=1 \
  -D_TC_PREFIX="\"$(prefix)\"" -D_TC_INCLUDEDIR="\"$(INCLUDEDIR)\"" \
  -D_TC_LIBDIR="\"$(LIBDIR)\"" -D_TC_BINDIR="\"$(BINDIR)\"" -D_TC_LIBEXECDIR="\"$(LIBEXECDIR)\""
CFLAGS = -std=c99 -Wall -fPIC -fsigned-char -O2
LDFLAGS = -L. -L$(LIBDIR) -L/root/lib -L/usr/local/lib
LIBS = -lz -lpthread -lm -lc
LDENV = LD_RUN_PATH=/lib:/usr/lib:$(LIBDIR):$(HOME)/lib:/usr/local/lib:$(LIBDIR):.
RUNENV = LD_LIBRARY_PATH=.:/lib:/usr/lib:$(LIBDIR):$(HOME)/lib:/usr/local/lib:$(LIBDIR)
POSTCMD = true

#================================================================
# Suffix rules
#================================================================

.SUFFIXES :
.SUFFIXES : .c .o

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

#================================================================
# Actions
#================================================================

all : $(LIBRARYFILES) $(COMMANDFILES)
	scp libdict.so /usr/lib/fms/

clean :
	rm -rf $(LIBRARYFILES) $(LIBOBJFILES) $(COMMANDFILES) \
	  *.o a.out

libdict.so : $(LIBOBJFILES)
	$(CC) -ldl -lpthread -shared -fpic -Wl,-soname,libdict.so -o $@ $(LIBOBJFILES) $(LDFLAGS) $(LIBS)
#dict_tool : dict_tool.o $(LIBOBJFILES)
##	$(LDENV) $(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -ldict $(LIBS)
myconf.o : myconf.h
tcutil.o : tcutil.h myconf.h
tchdb.o : tchdb.h tcutil.h myconf.h
