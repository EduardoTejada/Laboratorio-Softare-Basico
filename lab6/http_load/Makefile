# Makefile for http_load

# CONFIGURE: If you are using a SystemV-based operating system, such as
# Solaris, you will need to uncomment this definition.
#SYSV_LIBS =	-lnsl -lsocket -lresolv

# CONFIGURE: If you want to compile in support for https, uncomment these
# definitions.  You will need to have already built OpenSSL, available at
# http://www.openssl.org/  Make sure the SSL_TREE definition points to the
# tree with your OpenSSL installation - depending on how you installed it,
# it may be in /usr/local instead of /usr/local/ssl.
SSL_TREE =	/usr/lib/ssl
SSL_DEFS =	-DUSE_SSL
SSL_INC =	-I/usr/include/openssl
SSL_LIBS =	-lssl -lcrypto
LDFLAGS = -L/home/feec/alunos/183451/.local/lib64 -lssl -lcrypto
LDLIBS =  -L/home/feec/alunos/183451/.local/lib64 -lssl -lcrypto



BINDIR =	/usr/local/bin
MANDIR =	/usr/local/man/man1
CC =		cc
CFLAGS = -O -DUSE_SSL -I/home/feec/alunos/183451/.local/include -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wno-long-long

all:		http_load

http_load:	http_load.c timers.h port.h timers.o
	$(CC) $(CFLAGS) http_load.c timers.o $(LDFLAGS) -o http_load

timers.o:	timers.c timers.h
	$(CC) $(CFLAGS) -c timers.c

install:	all
	rm -f $(BINDIR)/http_load
	cp http_load $(BINDIR)
	rm -f $(MANDIR)/http_load.1
	cp http_load.1 $(MANDIR)

clean:
	rm -f http_load *.o core core.* *.core

tar:
	@name=`sed -n -e '/#define HTTP_LOAD_VERSION /!d' -e 's,.*http_load ,http_load-,' -e 's,",,p' version.h` ; \
	  rm -rf $$name ; \
	  mkdir $$name ; \
	  tar cf - `cat FILES` | ( cd $$name ; tar xfBp - ) ; \
	  chmod 644 $$name/Makefile ; \
	  tar cf $$name.tar $$name ; \
	  rm -rf $$name ; \
	  gzip $$name.tar
