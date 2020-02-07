
CURRENTDIR=$(shell pwd)
BASEDIR=$(shell dirname $(CURRENTDIR))
INSTALLDIR=$(BASEDIR)/gcc-install-5.4.0

GCCDIR=$(BASEDIR)/gcc-install-5.4.0/bin
CXX = $(GCCDIR)/g++
CC= $(GCCDIR)/gcc
# -fno-rtti is required for GCC plugins
CXXFLAGS = -std=c++11 -fno-rtti
CXXFLAGS +=  -Wall
# issue between plugin headers and c++11, this is a fix
CXXFLAGS += -Wno-literal-suffix

PLUGINDIR=$(shell $(CXX) -print-file-name=plugin)
CXXFLAGS += -I$(PLUGINDIR)/include

LDFLAGS=-std=c++11

TESTDIR:=$(CURRENTDIR)/tests

# Set 1 if target is 64bits
TARGET_64=1
# if libstdc++ needs to be statically linked
LIBSTD_STATIC=0
# Compilation flags
INSTRULIB_FLAGS=-std=c++11 -shared -fPIC
TEST_FLAGS=-Wstrict-aliasing -fno-stack-protector
OBJCOPY_INPUT=
OBJCOPY_OUTPUT=
ifeq (1, $(LIBSTD_STATIC))
	INSTRULIB_FLAGS += -static-libstdc++
endif
ifeq (1, $(TARGET_64))
	OBJCOPY_INPUT=i386:x86-64
	OBJCOPY_OUTPUT=elf64-x86-64
else
	INSTRULIB_FLAGS += -m32
	TEST_FLAGS += -m32
	OBJCOPY_INPUT=i386
	OBJCOPY_OUTPUT=elf32-i386
	CXXFLAGS += -m32
endif


USER=$(shell whoami)
# Where we want to store the static DFG
DFG_PATH=$(CURRENTDIR)
DFG_VARS_NAME=fg_dfg_vars
DFG_SETS_NAME=fg_dfg_sets
# Where we cd to once the DFG_PATH is given. If no dir is given we set this
# var to '.' to avoid objdump from crashing
CD_DFG_PATH=

# This comparison is make's weird way of checking if DFG_PATH is empty
ifeq ($(DFG_PATH),)
	DFG_VARS=$(DFG_VARS_NAME)
	DFG_SETS=$(DFG_SETS_NAME)
	CD_DFG_PATH=.
else
	DFG_VARS=$(DFG_PATH)/$(DFG_VARS_NAME)
	DFG_SETS=$(DFG_PATH)/$(DFG_SETS_NAME)
	CD_DFG_PATH=$(DFG_PATH)
endif

PROG=fg
OBJS=fg-alias.o fg-defuse.o fg-instru.o fg-serial.o flowguard.o
LIB=libinstru


plugin: $(PROG).so

$(PROG).so: $(OBJS)
	$(CXX) $(LDFLAGS) -shared $^ -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

lib: $(LIB).so

$(LIB).so: instrumentation.cpp $(DFG_VARS).o $(DFG_SETS).o
	$(GCCDIR)/g++ $(INSTRULIB_FLAGS) $^ -o $@

# In the following two directives we cd to the path of the .fg files since
# (1) the symbols are created in the elf taking into account the whole path,
# (2) objcopy --redefine-sym cannot handle dynamic symbols, and
# (3) our library needs _binary_fg_dfg_(vars|sets)_fg_(start|end|size)
# See:
# https://sourceware.org/ml/binutils/2006-03/msg00266.html
# https://stackoverflow.com/questions/15594988/objcopy-prepends-directory-pathname-to-symbol-name
$(DFG_VARS).o: $(DFG_VARS).fg
	cd $(CD_DFG_PATH);	objcopy --input binary --output $(OBJCOPY_OUTPUT) \
		--binary-architecture $(OBJCOPY_INPUT) $(DFG_VARS_NAME).fg \
		$(DFG_VARS_NAME).o

$(DFG_SETS).o: $(DFG_SETS).fg
	cd $(CD_DFG_PATH); objcopy --input binary --output $(OBJCOPY_OUTPUT) \
		--binary-architecture $(OBJCOPY_INPUT) $(DFG_SETS_NAME).fg \
		$(DFG_SETS_NAME).o

clean:
	rm -f *.o *.so *.cc~ *.hpp~ tests/*.o tests/*.cc~ tests/*.hpp~ *.pdf *.dot \
	test_fg *.fg
	rm -f $(DFG_VARS).o $(DFG_SETS).o $(DFG_VARS).fg $(DFG_SETS).fg

test: $(TESTDIR)/test_$(PROG).o $(LIB).so
	$(CXX) -o $(TESTDIR)/test_$(PROG) $(TESTDIR)/test_$(PROG).o -g \
		-fplugin=./$(PROG).so -fplugin-arg-fg-dfgpath=$(DFG_PATH) \
		-Wl,-rpath,./ -L./ -linstru -O2 $(TEST_FLAGS)
	chmod u+x $(TESTDIR)/test_$(PROG)

$(TESTDIR)/test_$(PROG).o: $(PROG).so $(TESTDIR)/test_$(PROG).c
	$(CC) \
	-c -g -fplugin=./$(PROG).so \
	$(TESTDIR)/test_$(PROG).c \
	-o $(TESTDIR)/test_$(PROG).o $(TEST_FLAGS)


all: clean lib plugin test

# General
check-syntax:
	g++ -std=c++11 -I$(PLUGINDIR) -o .nul -S ${CHK_SOURCES}

plugin-path:
	$(CXX) -print-file-name=plugin

.PHONY: clean plugin test

