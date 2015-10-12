SHELL=/bin/sh
CC=gcc

BASEPATH=.
OUTPUT=$(BASEPATH)

TARGET=fminject

LDFLAGS= -lpthread -ldl -lrt

CFLAGS= -g -O0 -Wall\
	-Wno-unused-function \
    -Wno-unused-variable \
    -Wno-implicit-function-declaration \
	-I ../include\
	-I ./\
	-I ../evt_modules/include\
	-L /usr/lib/fms

all:$(TARGET)

$(TARGET): ./lex.yy.c ./y.tab.c ./fminject.c
	$(CC) -o $(OUTPUT)/$@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(TARGET) *.o
