##############################################################################
# Makefile for SQPIX by Samuel Devulder
##############################################################################

CMD=SQPSHOW.CMD
PIC=HNY2026
PIC=AVATAR
PIC=MONKEY
PIC=BART
PIC=TIGRE
DITH=-x
#  --vac -o8x8 --c5x5b --o3x3

STRIP=strip
WGET=wget --no-check-certificate
SED=sed -e
GIT=git
RM=rm
CP=cp
7Z=7z

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

FLEXFLOPPY=../flextools/flexfloppy/flexfloppy$(EXE)
A09=a09/a09$(EXE)
BIN=sqpix$(EXE)

ALL=$(BIN) $(CMD)

all: $(BIN) $(CMD)
	ls -l $^

include $(EXO2)/Makefile

clean: myclean pic_clean

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
	-@$(RM) $(BIN) $(OBJS) $(SHARED_OBJS) $(ALL:$(EXE)=.o) $(ALL) >/dev/null 2>&1
	-@cd $(EXO2) && $(MAKE) -f Makefile clean >/dev/null 2>&1 

test: $(BIN) 
	-@rm samples/*.sqp.png 2>/dev/null
	@for i in samples/*.{png,jpg,gif}; do\
		x=`echo $$i | sed -e s/[\\.].*$$/.sqp/`; \
		./$(BIN) -z -v $(DITH) --png "$$i" -o "$$x"; \
	done

%.CMD: %.ASM $(A09)
	@echo "Assembling $< to $@..."
	@$(A09) -F$@ -L$*.LST $< #>/dev/null
	
%.BIN: %.ASM $(A09)
	@echo "Assembling $< to $@..."
	@$(A09) -B$@ -L$*.LST $< >/dev/null
	
##############################################################################

$(A09): $(wildcard $(dir $(A09))/*)
	@echo "Building $@..."
	@cd $(dir $@) && export MAKE="make -f Makefile" && $$MAKE BUILDMODE=static CC="$(CC) -static" CFLAGS="$(CFLAGS)"  
	@$(STRIP) "$@"
	@echo

$(FLEXFLOPPY): $(wildcard $(dir $(FLEXFLOPPY))/*.c)
	@echo "Building $@..."
	@cd $(dir $@) && export MAKE="make -f Makefile" && $$MAKE CC="$(CC) -static" CFLAGS="$(CFLAGS)"
	@$(STRIP) "$@"
	@echo

update:
	git submodule update --recursive --remote
	git pull

##############################################################################

FLEX_SYS=NEWFLEX.SYS
ROM=SQ$(PIC).ROM

pic: $(ROM) forth.zip
pic_clean:
	-$(RM) -rf $(ROM) samples/SQ$(PIC).SQP  forth.dir forth.zip

%.zip:
	@echo "Retreiving $@ from system-cfg..."
	@$(WGET) -q https://forum.system-cfg.com/download/file.php?id=43469 -O "$@"

%.imd: %.zip
	@$(7Z) --help >/dev/null || apt-cyg install p7zip || sudo apt-get install p7zip
	@echo "Extracting $@ from $<..."
	@$(7Z) e "$<" "system.imd" -r -so >$@

%.dir/$(FLEX_SYS): %.imd $(FLEXFLOPPY)
	@echo "Extracting $@ from $<..."
	@$(FLEXFLOPPY) --in "$<" --extract "$(dir $@)" >/dev/null

SQ%.ROM: samples/%.SQP $(FLEXFLOPPY) $(CMD) forth.dir/$(FLEX_SYS) Makefile
	@echo "Building $@..."
	@echo >STARTUP.TXT $(CMD) $*.SQP: CAT :
	@$(FLEXFLOPPY) --out $@ --tracks 26 --label HAPPY --number 2026 --new --rompack
	@$(FLEXFLOPPY) --in  "$@" --add forth.dir/$(FLEX_SYS)
	@$(FLEXFLOPPY) --in  "$@" --add STARTUP.TXT
	@$(FLEXFLOPPY) --in  "$@" --add $(CMD)
	@$(FLEXFLOPPY) --in  "$@" --add $< 
	@$(FLEXFLOPPY) --in  "$@" --add forth.dir/CAT.CMD
	@$(FLEXFLOPPY) --in  "$@" --cat
	@$(RM) STARTUP.TXT

samples/%.SQP: samples/%.jpg $(BIN) Makefile
	./$(BIN) $(DITH) $< -o $@ --png -z -v
	
pic_dbg: pic_tst,-debug

pic_tst: pic_tst,-skip_gameinfo

pic_tst,%: pic
	cd ../.. && ./mame squale $* -window -cart "$(shell cygpath -w -s $(PWD)/$(ROM))" &