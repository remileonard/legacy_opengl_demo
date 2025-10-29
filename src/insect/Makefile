#!smake

include $(ROOT)/usr/include/make/commondefs

LIBTK = ../libtk/libtk.a

LCDEFS = -DUNIX -D_GL_X -Dmips -D_GL_DOUBLE -D__STDC__=1 -U__EXTENSIONS__
LCINCS = -I$(ROOT)/usr/include/GL -I../libtk
LLDLIBS = -lGLU -lGL -lXext -lX11 -lm -lmalloc -lfpe

TARGETS = insect

CFILES = gl42ogl.c insect.c mat.c parts.c

default all: $(TARGETS)

include $(COMMONRULES)

$(TARGETS): $(OBJECTS) $(LIBTK)
	$(CCF) -o $@ $(OBJECTS) $(LIBTK) $(LDFLAGS)
