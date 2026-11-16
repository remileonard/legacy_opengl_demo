#!smake -J 1
#-------------------------------------------------------------------#
#-- Makefile for "perfly" the basic demonstration program         --#
#-------------------------------------------------------------------#
#-- RCS version information                                       --#
#--   $Revision: 1.53 $                                           --#
#--   $Date: 1996/04/09 05:26:44 $                                --#
#-------------------------------------------------------------------#

#--
#--	definitions
#--

#-- provide a list of alternate locations for file searches
UNIQUE	= ..
COMMON	= ../../common

#-- alternate locatins for included files
LINCS	= \
	-I${UNIQUE} \
	-I${COMMON}
LCINCS  = $(LINCS)


#if !defined(PFSTYLE)
PFSTYLE = 32
#endif
#if $(PFSTYLE) == "64"
OBJECT_STYLE = 64
LIBBITSUF=64
PFRELEASE=N64
#endif
#if $(PFSTYLE) == "N32"
OBJECT_STYLE = N32_M3
LIBBITSUF=32
PFRELEASE=N32
#endif
#if $(PFSTYLE) == "32"
OBJECT_STYLE = 32
LIBBITSUF=
PFRELEASE=O32
#endif

include $(ROOT)/usr/include/make/commondefs


LIBIGL	= -ignore_unresolved -lgl 
LIBOGL  = -ignore_unresolved -lGLU -lGL -lXext

#if $(RELEASE) < 6.2
IRIXREL=-DIRIX5
#else
IRIXREL=-DIRIX6
LIBIGL	+= -lXsgivc
LIBOGL  += -lXsgivc
#endif


SYSTEM_IRISGL = \
	-limage \
	-lfm \
	${LIBIGL} \
	-lXmu \
	-lX11 \
	-lm \
	-lfpe \
	-lmalloc \
	-lC

SYSTEM_OPENGL = \
	-limage \
	-lfm \
	${LIBOGL} \
	-lXmu \
	-lX11 \
	-lfpe \
	-lm \
	-lmalloc \
	-lC

#if $(PFSTYLE) == "64"
SYSTEM_OPENGL = \
	-limage \
	${LIBOGL} \
        -lXmu \
	-lX11 \
	-lm \
	-lC
#endif

#if $(PFSTYLE) == "N32"
SYSTEM_IRISGL = \
	-limage \
	${LIBIGL} \
	-lXmu \
	-lX11 \
	-lm \
	-lfm \
	-lC

SYSTEM_OPENGL = \
	-limage \
	${LIBOGL} \
	-lXmu \
	-lX11 \
	-lm \
	-lC
#endif

PFROOT ?= $(ROOT)

DSOLINKS = \
	-L$(PFROOT)/usr/lib$(LIBBITSUF) \
	-L$(PFROOT)/usr/lib$(LIBBITSUF)/libpfdb \
	-L$(PFROOT)/lib$(LIBBITSUF)

DDSOLINKS = \
        -L$(PFROOT)/usr/lib$(LIBBITSUF)/Performer/Debug \
        -L$(PFROOT)/usr/lib$(LIBBITSUF)/Performer/Debug/libpfdb \
        -L$(PFROOT)/lib$(LIBBITSUF)

DBGLINKS = \
        -L$(PFROOT)/usr/lib$(LIBBITSUF)/Performer/DebugStatic \
        -L$(PFROOT)/usr/lib$(LIBBITSUF)/Performer/DebugStatic/libpfdb \
        -L$(PFROOT)/lib$(LIBBITSUF)

OPTLINKS = \
        -L$(PFROOT)/usr/lib$(LIBBITSUF)/Performer/Static \
        -L$(PFROOT)/usr/lib$(LIBBITSUF)/Performer/Static/libpfdb \
        -L$(PFROOT)/lib$(LIBBITSUF)

IGLLIB = -lpf_igl -lpfdu_igl -lpfutil_igl -lpfui
OGLLIB = -lpf_ogl -lpfdu_ogl -lpfutil_ogl -lpfui

#if defined(PFSTATIC_CONVERTERS)
IGLLIB += -all $(PFSTATIC_CONVERTERS) -none
OGLLIB += -all $(PFSTATIC_CONVERTERS) -none
#endif


#-- base name of program
TARGET	= perfly

#-- object files from which target built {some are in the common directory}
OBJECTS	= \
	cmdline.o \
	generic.o \
	env.o \
	gui.o \
	keybd.o \
	perfly.o \
	main.o

#-- the debug version is in DBG and both opt and dso are in OPT
DIRS = DBG OPT

#--
#--	generic targets
#--

#-- make optimized dso version of program by default
default: ogldso

#-- make all versions of program
all: ogldbg oglopt ogldso oglddso

#-- clean up directories {remove junk}
clean:
	if test -d DBG.$(PFRELEASE).IRISGL; then cd DBG.$(PFRELEASE).IRISGL ; rm -f ${OBJECTS} core ; cd .. ; fi
	if test -d DBG.$(PFRELEASE).OPENGL; then cd DBG.$(PFRELEASE).OPENGL ; rm -f ${OBJECTS} core ; cd .. ; fi
	if test -d OPT.$(PFRELEASE).IRISGL ; then cd OPT.$(PFRELEASE).IRISGL ; rm -f ${OBJECTS} core ; cd .. ; fi
	if test -d OPT.$(PFRELEASE).OPENGL ; then cd OPT.$(PFRELEASE).OPENGL ; rm -f ${OBJECTS} core ; cd .. ; fi

#-- remove all machine-built files
clobber: clean
	if test -d OPT.$(PFRELEASE).IRISGL ; then rm -rf OPT.$(PFRELEASE).IRISGL ; fi
	if test -d OPT.$(PFRELEASE).OPENGL ; then rm -rf OPT.$(PFRELEASE).OPENGL ; fi
	if test -d DBG.$(PFRELEASE).IRISGL ; then rm -rf DBG.$(PFRELEASE).IRISGL ; fi
	if test -d DBG.$(PFRELEASE).OPENGL ; then rm -rf DBG.$(PFRELEASE).OPENGL ; fi
	rm -f ${TARGET}

#--
#--	library targets
#--

#-- make a debugging version of the program
igldbg: .MAKE
	@ echo "\nmaking IrisGL DBG version of ${TARGET}"
	@ if test ! -d DBG.$(PFRELEASE).IRISGL ; then mkdir -p DBG.$(PFRELEASE).IRISGL ; fi
	@ cd DBG.$(PFRELEASE).IRISGL ; \
	${MAKE} -f ../Makefile OPTIMIZER=-g \
	     "LCDEFS=-DIRISGL $(IRIXREL)" "LCXXDEFS=-DIRISGL $(IRIXREL)"  \
	     LIBRARIES='$(IGLLIB) -Wl,-none ${SYSTEM_IRISGL}' \
	     ${TARGET}.DBG
	@ rm -f ${TARGET}
	ln -s DBG.$(PFRELEASE).IRISGL/${TARGET}.DBG ${TARGET}

#-- make a debugging version of the program
ogldbg: .MAKE
	@ echo "\nmaking OpenGL DBG version of ${TARGET}"
	@ if test ! -d DBG.$(PFRELEASE).OPENGL ; then mkdir -p DBG.$(PFRELEASE).OPENGL ; fi
	@ cd DBG.$(PFRELEASE).OPENGL ; \
	${MAKE} -f ../Makefile OPTIMIZER="-g"\
	     "LCDEFS=$(IRIXREL)" "LCXXDEFS=$(IRIXREL)"\
	     LIBRARIES='$(OGLLIB) -Wl,-none ${SYSTEM_OPENGL}' \
	     ${TARGET}.DBG
	@ rm -f ${TARGET}
	ln -s DBG.$(PFRELEASE).OPENGL/${TARGET}.DBG ${TARGET}

#-- make an optimized version of the program
iglopt: .MAKE
	@ echo "making IrisGL OPT version of ${TARGET}"
	@ if test ! -d OPT.$(PFRELEASE).IRISGL ; then mkdir -p OPT.$(PFRELEASE).IRISGL ; fi
	@ cd OPT.$(PFRELEASE).IRISGL ; ${MAKE} -f ../Makefile OPTIMIZER="-O -Olimit 2000" \
	     "LCDEFS=-DIRISGL $(IRIXREL)" "LCXXDEFS=-DIRISGL $(IRIXREL)"  \
	     LIBRARIES='$(IGLLIB) -Wl,-none ${SYSTEM_IRISGL}' \
	     ${TARGET}.OPT
	@ rm -f ${TARGET}
	ln -s OPT.$(PFRELEASE).IRISGL/${TARGET}.OPT ${TARGET}

#-- make an optimized version of the program
oglopt: .MAKE
	@ echo "making OpenGL OPT version of ${TARGET}"
	@ if test ! -d OPT.$(PFRELEASE).OPENGL ; then mkdir -p OPT.$(PFRELEASE).OPENGL ; fi
	@ cd OPT.$(PFRELEASE).OPENGL ; ${MAKE} -f ../Makefile OPTIMIZER="-O " \
	     "LCDEFS=$(IRIXREL)" "LCXXDEFS=$(IRIXREL)"\
	     LIBRARIES='$(OGLLIB) -Wl,-none ${SYSTEM_OPENGL}' \
	     ${TARGET}.OPT
	@ rm -f ${TARGET}
	ln -s OPT.$(PFRELEASE).OPENGL/${TARGET}.OPT ${TARGET}

#-- make an optimized version of the program that uses DSOs
igldso: .MAKE
	@ echo "making IrisGL DSO version of ${TARGET}"
	@ if test ! -d OPT.$(PFRELEASE).IRISGL ; then mkdir -p OPT.$(PFRELEASE).IRISGL ; fi
	@ cd OPT.$(PFRELEASE).IRISGL ; ${MAKE} -f ../Makefile OPTIMIZER="-O -Olimit 2000" \
	     "LCDEFS=-DIRISGL $(IRIXREL)" "LCXXDEFS=-DIRISGL $(IRIXREL)"  \
	    LIBRARIES='$(IGLLIB) -Wl,-none ${SYSTEM_IRISGL}' \
	    ${TARGET}.DSO
	@rm -f ${TARGET}
	ln -s OPT.$(PFRELEASE).IRISGL/${TARGET}.DSO ${TARGET}

#-- make an optimized version of the program that uses DSOs
ogldso: .MAKE
	@ echo "making OpenGL DSO version of ${TARGET}"
	@ if test ! -d OPT.$(PFRELEASE).OPENGL ; then mkdir -p OPT.$(PFRELEASE).OPENGL ; fi
	@ cd OPT.$(PFRELEASE).OPENGL ; ${MAKE} -f ../Makefile OPTIMIZER="-O " \
	    "LCDEFS=$(IRIXREL)" "LCXXDEFS=$(IRIXREL)"\
	    LIBRARIES='$(OGLLIB) -Wl,-none ${SYSTEM_OPENGL}' \
	    ${TARGET}.DSO
	@ rm -f ${TARGET}
	ln -s OPT.$(PFRELEASE).OPENGL/${TARGET}.DSO ${TARGET}

#-- make an optimized version of the program that uses DSOs
iglddso: .MAKE
	@ echo "making IrisGL DDSO version of ${TARGET}"
	@ if test ! -d DBG.$(PFRELEASE).IRISGL ; then mkdir -p DBG.$(PFRELEASE).IRISGL ; fi
	@ cd DBG.$(PFRELEASE).IRISGL ; ${MAKE} -f ../Makefile OPTIMIZER="-g" \
	     "LCDEFS=-DIRISGL $(IRIXREL)" "LCXXDEFS=-DIRISGL $(IRIXREL)"  \
	    LIBRARIES='$(IGLLIB) -Wl,-none ${SYSTEM_IRISGL}' \
	    ${TARGET}.DDSO
	@ rm -f ${TARGET}
	ln -s DBG.$(PFRELEASE).IRISGL/${TARGET}.DDSO ${TARGET}

#-- make an optimized version of the program that uses DSOs
oglddso: .MAKE
	@ echo "making OpenGL DDSO version of ${TARGET}"
	@ if test ! -d DBG.$(PFRELEASE).OPENGL ; then mkdir -p DBG.$(PFRELEASE).OPENGL ; fi
	@  cd DBG.$(PFRELEASE).OPENGL ; ${MAKE} -f ../Makefile OPTIMIZER="-g" \
	    "LCDEFS=$(IRIXREL)" "LCXXDEFS=$(IRIXREL)"\
	    LIBRARIES='$(OGLLIB) -Wl,-none ${SYSTEM_OPENGL}' \
	    ${TARGET}.DDSO
	@ rm -f ${TARGET}
	ln -s DBG.$(PFRELEASE).OPENGL/${TARGET}.DDSO ${TARGET}

dbg:	ogldbg
opt:	oglopt
dso:	ogldso
ddso:   oglddso

#--
#--	internal targets
#--

${TARGET}.DBG: ${OBJECTS}
	${CC} ${CFLAGS} -o $@ ${OBJECTS} $(DBGLINKS) -all ${LIBRARIES}

${TARGET}.OPT: ${OBJECTS}
	${CC} ${CFLAGS} -o $@ ${OBJECTS} $(OPTLINKS) -all ${LIBRARIES}

${TARGET}.DSO: ${OBJECTS}
	${CC} ${CFLAGS} -o $@ ${OBJECTS} $(DSOLINKS) -all ${LIBRARIES}

${TARGET}.DDSO: ${OBJECTS}
	${CC} ${CFLAGS} -o $@ ${OBJECTS} $(DDSOLINKS) -all ${LIBRARIES}

#-- objects are built from either unique or common files
.PATH: ${UNIQUE} ${COMMON}
