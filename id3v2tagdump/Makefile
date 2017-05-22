SRCS=id3v2tagdump.cc
PROG=id3v2tagdump
MAN=

CFLAGS+=-I/usr/local/include -L/usr/local/lib
LDADD+=-lc++ -ltag

BINOWN=${USER}
BINGRP=${USER}
BINDIR=${HOME}/bin

.include <bsd.prog.mk>
