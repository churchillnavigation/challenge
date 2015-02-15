# We don't use the built-in rules.
.SUFFIXES:

# Execute all recipes in single shell.
.ONESHELL:

# Delete targets whose recipe fails.
.DELETE_ON_ERROR:

.PHONY: all
all:
ALL_TARGETS :=

.PHONY: clean
clean:
CLEAN_TARGETS :=

# Compiler flags we always require.
override CXXFLAGS += -std=gnu++1y -Wall

# Information about the shared objects we're going to build.
SO_SUFFIX :=
SO_STRIP_SUFFIX :=
SO_FLAGS := -fno-rtti -fno-exceptions
USE_DEF_FILES :=

UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
  SO_SUFFIX := .so
  SO_STRIP_SUFFIX := .strip.so
  SO_FLAGS += -fPIC
  ALL_TARGETS += test_point_search
else ifeq ($(UNAME),MINGW32_NT-6.2)

  # We're using a MSYS/MINGW toolchain on Windows.

  SO_SUFFIX := .dll
  SO_STRIP_SUFFIX := .strip.dll

  # On Windows, we use a def file to control the exposed symbols.
  USE_DEF_FILES := yes
endif

dll_names := single_pass flat_quadtree

dll_tgts := $(dll_names:=$(SO_SUFFIX))
dll_strip_tgts := $(dll_names:=$(SO_STRIP_SUFFIX))
dll_objs := $(dll_names:=.o)

ALL_TARGETS += $(dll_tgts) $(dll_strip_tgts)
CLEAN_TARGETS += $(dll_tgts) $(dll_strip_tgts) $(dll_objs)

$(dll_objs) : %.o : %.cc
	g++ -c -o $@ $< $(SO_FLAGS) $(CXXFLAGS)

$(dll_tgts) : %$(SO_SUFFIX) : %.o $(if $(USE_DEF_FILES),%.def)
	g++ -shared -o $@ $^ $(SO_FLAGS) $(CXXFLAGS)

$(dll_strip_tgts) : %$(SO_STRIP_SUFFIX) : %$(SO_SUFFIX)
	cp $< $@
	strip $@

CLEAN_TARGETS += test_point_search
test_point_search : test_point_search.cc
	g++ -ldl -o $@ $^ $(CXXFLAGS)

CLEAN_TARGETS += unit_test_driver
unit_test_driver : unit_test_driver.cc
	g++ -o $@ $^ $(CXXFLAGS)

.PHONY: test
test: unit_test_driver
	./unit_test_driver

all: $(ALL_TARGETS)

clean:
	rm -rf $(CLEAN_TARGETS)
