###############################################################################
# Copyright (c) 2015, 2016 IBM Corp. and others
#
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at http://eclipse.org/legal/epl-2.0
# or the Apache License, Version 2.0 which accompanies this distribution
# and is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# This Source Code may also be made available under the following Secondary
# Licenses when the conditions for such availability set forth in the
# Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
# version 2 with the GNU Classpath Exception [1] and GNU General Public
# License, version 2 with the OpenJDK Assembly Exception [2].
#
# [1] https://www.gnu.org/software/classpath/license.html
# [2] http://openjdk.java.net/legal/assembly-exception.html
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
###############################################################################

GLUE_OBJECTS := \
  argmain

GLUE_OBJECTS := $(addsuffix $(OBJEXT),$(GLUE_OBJECTS))
OBJECTS += $(GLUE_OBJECTS)

MODULE_INCLUDES += $(OMR_GTEST_INCLUDES) ../util
MODULE_INCLUDES += $(OMR_IPATH)
MODULE_CXXFLAGS += $(OMR_GTEST_CXXFLAGS)

vpath argmain.cpp $(top_srcdir)/fvtest/omrGtestGlue

MODULE_STATIC_LIBS += \
  j9prtstatic \
  j9thrstatic \
  omrutil \
  j9avl \
  j9hashtable \
  j9omr \
  j9pool \
  omrtrace \
  omrGtest \
  testutil \
  omrgcbase \
  omrgcstructs \
  omrgcstats \
  omrgcstandard \
  omrgcstartup \
  omrgcverbose \
  omrgcverbosehandlerstandard \
  omrvmstartup \
  j9hookstatic \
  omrglue

ifeq (gcc,$(OMR_TOOLCHAIN))
  MODULE_SHARED_LIBS += stdc++
endif
ifeq (linux,$(OMR_HOST_OS))
  MODULE_SHARED_LIBS += rt pthread
endif
ifeq (osx,$(OMR_HOST_OS))
  MODULE_SHARED_LIBS += iconv pthread
endif
ifeq (aix,$(OMR_HOST_OS))
  MODULE_SHARED_LIBS += iconv perfstat
endif
ifeq (win,$(OMR_HOST_OS))
  MODULE_SHARED_LIBS += ws2_32 # socket library
  MODULE_SHARED_LIBS += shell32 Iphlpapi psapi pdh
endif
