EXO2=exomizer/exomizer2/src

VPATH=.:$(EXO2)/

DATE:=$(shell date +%FT%T%Z || date)
OS:=$(shell uname -o | tr "/" "_")
EXE=

EXTRA=-Wno-unused-function -s -O3
CC=gcc

ifeq ($(OS),Windows_NT)
        OS:=win
endif

ifeq ($(OS),Cygwin)
        OS:=win
endif

ifeq ($(OS),win)
        CC=i686-w64-mingw32-gcc  -m32 -static
        CXX=i686-w64-mingw32-g++ -m32 -static
        MACHINE=x86
        EXE=.exe
endif

ALL=sqpix$(EXE)
all: $(ALL)
	ls -l $<

include $(EXO2)/Makefile

clean: myclean

CC:=$(CC) -I$(EXO2)/ -I. $(EXTRA)
LDFLAGS=-lm

OBJS = stb.o match.o search.o optimal.o output.o membuf_io.o \
       chunkpool.o radix.o exo_helper.o exodec.o progress.o \
	   exo_util.o vec.o 
	   
%.o: $(EXO2)%.c
	cd $(EXO2) && make CC=$(CC) CFLAGS=$(CFLAGS) \
	$< && cp $< ..
	   
%$(EXE): %.o deps $(OBJS) $(SHARED_OBJS)
	@echo "Linking $@"
	@$(CC) $(LDFLAGS) -o $@ $< $(OBJS) $(SHARED_OBJS)

myclean:
	-@$(RM) $(BIN) $(OBJS) $(SHARED_OBJS) $(ALL:$(EXE)=.o) $(ALL)
	-@cd $(EXO2) && $(MAKE) -f Makefile clean

samples: $(ALL) 
	@for i in samples/*.{png,jpg}; do\
		x=`echo $$i | sed -e s/[\\.].*$$/.sqp/`; \
		./$(ALL) -x -v -b --pgm "$$i" -o "$$x"; \
	done