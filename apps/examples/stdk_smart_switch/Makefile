###########################################################################
#
# Copyright 2020 Samsung Electronics All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
###########################################################################
############################################################################
#
#   Copyright (C) 2008, 2010-2013 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

-include $(TOPDIR)/.config
-include $(TOPDIR)/Make.defs
include $(APPDIR)/Make.defs


APPNAME = smart_switch
FUNCNAME = $(APPNAME)_main
THREADEXEC = TASH_EXECMD_ASYNC

STDK_DIR = $(TOPDIR)/../external/stdk
STDK_SRC_DIR = $(STDK_DIR)/st-device-sdk-c/src
ST_APP_PATH=$(TOPDIR)/../apps/examples/stdk_smart_switch
DEVICE_JSON_FILE=$(ST_APP_PATH)/device_info.json
DEVICE_HEADER_FILE=$(ST_APP_PATH)/device_info.h
DEVICE_HEADER_BACKUP_FILE=$(DEVICE_HEADER_FILE).bak
PRODUCT_JSON_FILE=$(ST_APP_PATH)/onboarding_config.json
PRODUCT_HEADER_FILE=$(ST_APP_PATH)/onboarding_config.h
PRODUCT_HEADER_BACKUP_FILE=$(PRODUCT_HEADER_FILE).bak

CFLAGS+=-I$(STDK_SRC_DIR)/include/
CFLAGS+=-I$(STDK_SRC_DIR)/include/bsp
CFLAGS+=-I$(STDK_SRC_DIR)/include/os
CFLAGS+=-I$(STDK_SRC_DIR)/include/mqtt
CFLAGS+=-I$(STDK_SRC_DIR)/port/os/tizenrt

ASRCS =
CSRCS =
MAINSRC = smart_switch.c device_control.c

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

ifneq ($(CONFIG_BUILD_KERNEL),y)
  OBJS += $(MAINOBJ)
endif

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  BIN = ..\..\libapps$(LIBEXT)
else
ifeq ($(WINTOOL),y)
  BIN = ..\\..\\libapps$(LIBEXT)
else
  BIN = ../../libapps$(LIBEXT)
endif
endif

ifeq ($(WINTOOL),y)
  INSTALL_DIR = "${shell cygpath -w $(BIN_DIR)}"
else
  INSTALL_DIR = $(BIN_DIR)
endif

CONFIG_EXAMPLES_PROGNAME ?= stdk_smart_switch$(EXEEXT)
PROGNAME = $(CONFIG_EXAMPLES_PROGNAME)

ROOTDEPPATH = --dep-path .

# Common build

VPATH =

all: .built
.PHONY: clean depend distclean

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS) $(MAINOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

.built: $(OBJS)
	$(call ARCHIVE, $(BIN), $(OBJS))
	@touch .built

ifeq ($(CONFIG_BUILD_KERNEL),y)
$(BIN_DIR)$(DELIM)$(PROGNAME): $(OBJS) $(MAINOBJ)
	@echo "LD: $(PROGNAME)"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) -o $(INSTALL_DIR)$(DELIM)$(PROGNAME) $(ARCHCRT0OBJ) $(MAINOBJ) $(LDLIBS)
	$(Q) $(NM) -u  $(INSTALL_DIR)$(DELIM)$(PROGNAME)

install: $(BIN_DIR)$(DELIM)$(PROGNAME)

else
install:

endif

ifeq ($(CONFIG_BUILTIN_APPS)$(CONFIG_EXAMPLES_STDK_SMART_SWITCH),yy)
$(BUILTIN_REGISTRY)$(DELIM)$(FUNCNAME).bdat: $(DEPCONFIG) Makefile
	$(Q) $(call REGISTER,$(APPNAME),$(FUNCNAME),$(THREADEXEC),$(PRIORITY),$(STACKSIZE))

context: $(BUILTIN_REGISTRY)$(DELIM)$(FUNCNAME).bdat
####################################
# Convert device & product profile
#####################################
	$(Q) cp -n $(STDK_SRC_DIR)/include/profile/json/device_info_example.json $(DEVICE_JSON_FILE)
	$(Q) xxd -i $(DEVICE_JSON_FILE) > $(DEVICE_HEADER_FILE); echo $$?;
	$(Q) sed -i.bak 's/_.*json/device_info/g' $(DEVICE_HEADER_FILE)
	$(Q) rm $(DEVICE_HEADER_BACKUP_FILE)
	$(Q) cp -n $(STDK_SRC_DIR)/include/profile/json/onboarding_config_example.json $(PRODUCT_JSON_FILE)
	$(Q) xxd -i $(PRODUCT_JSON_FILE) > $(PRODUCT_HEADER_FILE); echo $$?;
	$(Q) sed -i.bak 's/_.*json/onboarding_config/g' $(PRODUCT_HEADER_FILE)
	$(Q) rm $(PRODUCT_HEADER_BACKUP_FILE)

else
context:

endif

.depend: Makefile $(SRCS)
	@$(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(SRCS) >Make.dep
	@touch $@

depend: .depend

clean:
	$(call DELFILE, .built)
	$(call CLEAN)
	$(call DELFILE, $(DEVICE_HEADER_FILE))
	$(call DELFILE, $(PRODUCT_HEADER_FILE))

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

-include Make.dep
.PHONY: preconfig
preconfig:
