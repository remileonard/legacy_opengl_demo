#!smake

include ${ROOT}/usr/include/make/commondefs

TARGETS	= space
LCOPTS  = -float -G 0
LLDLIBS = -lmalloc -limage -lGL -lXext -lX11 -lm

CFILES  = main.c sols.c fract.c ringworld.c god.c objects.c \
	world.c watch.c imlib.c input.c menu.c sound.c events.c \
	matrix.c light.c texture.c
OTHERSOURCE = smooth.o

default all: ${TARGETS}

include ${COMMONRULES}

CLEANOBJECTS = ${OBJECTS}
GDIRT	= ${OBJECTS} a.out core lex.yy.[co] y.tab.[cho] $(_FORCE)

$(TARGETS): ${OBJECTS}
	${CCF} -o $@ ${OBJECTS} ${OTHERSOURCE} ${LDFLAGS}
	${TAG} 0x9051 $@
