#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

#
# Copyright (c) 2016, Joyent, Inc.
#

#
# Very basic Makefile for the pmx library.  This Makefile is based on the one
# used in mdb_v8.
#

# Configuration for builders
PMX_BUILD		 = build
PMX_SONAME		 = libpmx.so
CSTYLE			 = cstyle
CSTYLE_FLAGS		+= -cCp

# Configuration for developers
PMX_SOURCES		 = pmx_subr.c
PMXEMIT_SOURCES		 = pmxemit.c
PMX_CSTYLE_SOURCES	 = $(wildcard \
				include/pmx/*.h \
				src/libpmx/*.c \
				src/libpmx/*.h \
				src/pmxemit/*.c)
CPPFLAGS		+= -Iinclude
CFLAGS			+= -Werror -Wall -Wextra -fPIC -fno-omit-frame-pointer
CFLAGS			+= -std=c99 -D_POSIX_C_SOURCE=200112L

ifeq ($(shell uname -s),Darwin)
	SOFLAGS		+= -Wl,-install_name,$(PMX_SONAME)
else
	SOFLAGS		+= -Wl,-soname=$(PMX_SONAME)
endif

# Definitions used as recipes
MKDIRP			 = mkdir -p $@
COMPILE.c		 = $(CC) -o $@ -c $(CFLAGS) $(CPPFLAGS) $^
MAKESO	 		 = $(CC) -o $@ -shared $(SOFLAGS) $(LDFLAGS) $^
MAKEEXEC		 = $(CC) -o $@ $(LDFLAGS) $^

PMX_OBJECTS_ia32	 = $(PMX_SOURCES:%.c=$(PMX_BUILD)/ia32/%.o)
PMX_TARGETS_ia32	 = $(PMX_BUILD)/ia32/libpmx.so
$(PMX_TARGETS_ia32):	 CFLAGS += -m32
$(PMX_TARGETS_ia32):	 SOFLAGS += -m32

PMX_OBJECTS_amd64	 = $(PMX_SOURCES:%.c=$(PMX_BUILD)/amd64/%.o)
PMX_TARGETS_amd64	 = $(PMX_BUILD)/amd64/libpmx.so
$(PMX_TARGETS_amd64):	 CFLAGS += -m64
$(PMX_TARGETS_amd64):	 SOFLAGS += -m64

PMX_PMXEMIT		 = $(PMX_BUILD)/ia32/pmxemit
PMX_PMXEMIT_OBJECTS	 = $(PMXEMIT_SOURCES:%.c=$(PMX_BUILD)/ia32/%.o)
$(PMX_PMXEMIT_OBJECTS):	 CFLAGS += -m32
$(PMX_PMXEMIT):		 LDFLAGS += -m32 -L$(PMX_BUILD)/ia32 -lpmx

PMX_ALLTARGETS   	 = $(PMX_TARGETS_ia32) \
			    $(PMX_TARGETS_amd64) \
			    $(PMX_PMXEMIT)
$(PMX_ALLTARGETS):	 CPPFLAGS += -Isrc


#
# libpmx uses a set of routines for emitting JSON.  We keep these in a directory
# called libjsonemitter and pretend they're a totally separate library for the
# purpose of hygiene, and potentially spinning it into a truly separate library
# in the future.  But for now, we build them just like the rest of libpmx and
# link the resulting objects into libpmx.so.
#
JSON_SOURCES		 = jsonemitter.c
JSON_CSTYLE_SOURCES      = $(wildcard src/libjsonemitter/*.[ch])
JSON_OBJECTS_ia32	 = $(JSON_SOURCES:%.c=$(PMX_BUILD)/ia32/%.o)
JSON_OBJECTS_amd64	 = $(JSON_SOURCES:%.c=$(PMX_BUILD)/amd64/%.o)

JSON_JSONEMITEXAMPLE_SOURCES	 = json-emit-example.c
JSON_JSONEMITEXAMPLE_OBJECTS	 = \
    $(JSON_JSONEMITEXAMPLE_SOURCES:%.c=$(PMX_BUILD)/ia32/%.o)
JSON_JSONEMITEXAMPLE	 	 = $(PMX_BUILD)/ia32/json-emit-example
$(JSON_JSONEMITEXAMPLE_OBJECTS): CFLAGS  += -m32
$(JSON_JSONEMITEXAMPLE)	:	 LDFLAGS += -m32

PMX_ALLTARGETS		+= $(JSON_OBJECTS_ia32) $(JSON_OBJECTS_amd64) \
			   $(JSON_JSONEMITEXAMPLE)
LDFLAGS			+= -lm

# Phony targets for convenience
.PHONY: all
all: $(PMX_ALLTARGETS)

CLEAN_FILES += $(PMX_BUILD)

.PHONY: clean
clean:
	rm -rf $(CLEAN_FILES)

.PHONY: check
check: check-cstyle

.PHONY: check-cstyle
check-cstyle:
	$(CSTYLE) $(CSTYLE_FLAGS) $(PMX_CSTYLE_SOURCES) $(JSON_CSTYLE_SOURCES)

.PHONY: prepush
prepush: check


# Concrete targets
$(PMX_BUILD)/ia32:
	$(MKDIRP)

$(PMX_BUILD)/ia32/libpmx.so: $(PMX_OBJECTS_ia32) $(JSON_OBJECTS_ia32)
	$(MAKESO)

$(PMX_BUILD)/ia32/%.o: src/libpmx/%.c | $(PMX_BUILD)/ia32
	$(COMPILE.c)

$(PMX_BUILD)/ia32/%.o: src/pmxemit/%.c | $(PMX_BUILD)/ia32
	$(COMPILE.c)

$(PMX_BUILD)/ia32/%.o: src/libjsonemitter/%.c | $(PMX_BUILD)/ia32
	$(COMPILE.c)


$(PMX_BUILD)/amd64/libpmx.so: $(PMX_OBJECTS_amd64) $(JSON_OBJECTS_amd64)
	$(MAKESO)

$(PMX_BUILD)/amd64/%.o: src/libpmx/%.c | $(PMX_BUILD)/amd64
	$(COMPILE.c)

$(PMX_BUILD)/amd64/%.o: src/libjsonemitter/%.c | $(PMX_BUILD)/amd64
	$(COMPILE.c)

$(PMX_BUILD)/amd64:
	$(MKDIRP)


$(PMX_PMXEMIT): $(PMX_PMXEMIT_OBJECTS) | $(PMX_BUILD)/ia32
	$(MAKEEXEC)

$(JSON_JSONEMITEXAMPLE): $(JSON_OBJECTS_ia32) $(JSON_JSONEMITEXAMPLE_OBJECTS) | $(PMX_BUILD)/ia32
	$(MAKEEXEC)
