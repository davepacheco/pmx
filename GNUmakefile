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
PMX_CSTYLE_SOURCES	 = $(wildcard src/*.c src/*.h include/pmx/*.h)
CPPFLAGS		+= -Iinclude
CFLAGS			+= -Werror -Wall -Wextra -fPIC -fno-omit-frame-pointer

ifeq ($(shell uname -s),Darwin)
	SOFLAGS		+= -Wl,-install_name,$(PMX_SONAME)
else
	SOFLAGS		+= -Wl,-soname=$(PMX_SONAME)
endif

# Definitions used as recipes
MKDIRP			 = mkdir -p $@
COMPILE.c		 = $(CC) -o $@ -c $(CFLAGS) $(CPPFLAGS) $^
MAKESO	 		 = $(CC) -o $@ -shared $(SOFLAGS) $(LDFLAGS) $^

PMX_OBJECTS_ia32	 = $(PMX_SOURCES:%.c=$(PMX_BUILD)/ia32/%.o)
PMX_TARGETS_ia32	 = $(PMX_BUILD)/ia32/libpmx.so
$(PMX_TARGETS_ia32):	 CFLAGS += -m32
$(PMX_TARGETS_ia32):	 SOFLAGS += -m32

PMX_OBJECTS_amd64	 = $(PMX_SOURCES:%.c=$(PMX_BUILD)/amd64/%.o)
PMX_TARGETS_amd64	 = $(PMX_BUILD)/amd64/libpmx.so
$(PMX_TARGETS_amd64):	 CFLAGS += -m64
$(PMX_TARGETS_amd64):	 SOFLAGS += -m64

PMX_ALLTARGETS   	 = $(PMX_TARGETS_ia32) $(PMX_TARGETS_amd64)

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
	$(CSTYLE) $(CSTYLE_FLAGS) $(PMX_CSTYLE_SOURCES)

.PHONY: prepush
prepush: check


# Concrete targets
$(PMX_BUILD)/ia32:
	$(MKDIRP)

$(PMX_BUILD)/ia32/libpmx.so: $(PMX_OBJECTS_ia32)
	$(MAKESO)

$(PMX_BUILD)/ia32/%.o: src/%.c | $(PMX_BUILD)/ia32
	$(COMPILE.c)


$(PMX_BUILD)/amd64/libpmx.so: $(PMX_OBJECTS_amd64)
	$(MAKESO)

$(PMX_BUILD)/amd64/%.o: src/%.c | $(PMX_BUILD)/amd64
	$(COMPILE.c)

$(PMX_BUILD)/amd64:
	$(MKDIRP)
