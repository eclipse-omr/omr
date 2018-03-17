###############################################################################
# Copyright (c) 2015, 2018 IBM Corp. and others
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
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
###############################################################################

top_srcdir := .
include $(top_srcdir)/omrmakefiles/configure.mk

######################J9 Lab specific settings#################################
ifneq (,$(findstring linux_ppc,$(SPEC)))
	EXPORT_LIB=true
else
ifneq (,$(findstring linux_390,$(SPEC)))
	EXPORT_LIB=true
endif
endif

ifeq ($(EXPORT_LIB),true)
LIB_PATH:=$(SITE_PATH_STREAMROOT)/../../javatest/lib/googletest/lib/$(SPEC):$(LD_LIBRARY_PATH)
export LD_LIBRARY_PATH=$(LIB_PATH)
$(warning In $(SPEC), set LD_LIBRARY_PATH=$(LIB_PATH))
endif

######################J9 Lab specific settings#################################

all: test

omr_algotest:
	./omralgotest --gtest_output=xml:test_output/omralgotest.xml -avltest:fvtest/algotest/avltest.lst

omr_ddrtest:
	bash $(top_srcdir)/ddr/tools/getmacros tools/ddrgen/test
	./ddrgen ddrgentest --macrolist test/macroList

omr_gctest:
	./omrgctest --gtest_output=xml:test_output/omrgctest.xml --gtest_filter="gcFunctionalTest*"

# jitbuilder can run different sets of tests on linux_x86 and osx than on other platforms
# until we common this up, run "testall" on linux_x86 and osx but run "test" everywhere else
omr_jitbuilderexamples:
ifneq (,$(findstring linux_x86,$(SPEC)))
	make -C jitbuilder/release testall
else
ifneq (,$(findstring osx,$(SPEC)))
	make -C jitbuilder/release testall
else
	make -C jitbuilder/release test
endif
endif

omr_jitbuildertest:
	./omrjitbuildertest --gtest_output=xml:test_output/omrjitbuildertest.xml

omr_jittest:
	./testjit --gtest_output=xml:test_output/testjit.xml
	
omr_porttest:
	./omrporttest --gtest_output=xml:test_output/omrporttest.xml
ifneq (,$(findstring cuda,$(SPEC)))
	./omrporttest --gtest_filter="Cuda*" -earlyExit
endif
	@echo ALL $@ PASSED

omr_rastest:
	./omrrastest --gtest_output=xml:test_output/omrrastest.xml
	./omrsubscribertest --gtest_output=xml:test_output/omrsubscribertest.xml --gtest_filter=-RASSubscriberForkTest.*
	./omrtraceoptiontest --gtest_output=xml:test_output/omrtraceoptiontest.xml
	@echo ALL $@ PASSED
	
omr_subscriberforktest:
	./omrsubscribertest --gtest_output=xml:test_output/omrsubscriberforktest.xml --gtest_filter=RASSubscriberForkTest.*
	
omr_sigtest:
	./omrsigtest --gtest_output=xml:test_output/omrsigtest.xml

omr_threadextendedtest:
	./omrthreadextendedtest --gtest_output=xml:test_output/omrthreadextendedtest.xml

omr_threadtest:
	./omrthreadtest --gtest_output=xml:test_output/omrthreadtest.xml
	./omrthreadtest --gtest_also_run_disabled_tests --gtest_filter=ThreadCreateTest.DISABLED_SetAttrThreadWeight
ifneq (,$(findstring linux,$(SPEC)))
	./omrthreadtest --gtest_output=xml:test_output/omrthreadtest.xml --gtest_filter=ThreadCreateTest.*:$(GTEST_FILTER) -realtime
endif
	@echo ALL $@ PASSED

omr_utiltest:
	./omrutiltest --gtest_output=xml:test_output/omrutiltest.xml
	
omr_vmtest:
	./omrvmtest --gtest_output=xml:test_output/omrvmtest.xml

.NOTPARALLEL:
test: omr_algotest omr_utiltest
ifeq (1,$(OMR_EXAMPLE))
test: omr_vmtest omr_gctest omr_rastest omr_subscriberforktest
endif
ifeq (1,$(OMR_TEST_COMPILER))
test: omr_jittest
endif
ifeq (1,$(OMR_JITBUILDER))
test: omr_jitbuildertest omr_jitbuilderexamples
endif
ifeq (1,$(OMR_PORT))
test: omr_porttest
endif
ifeq (1,$(OMR_THREAD))
test: omr_threadtest omr_threadextendedtest
endif
ifeq (1,$(OMR_OMRSIG))
test: omr_sigtest
endif
ifeq (1,$(ENABLE_DDR))
test: omr_ddrgentest
endif

.PHONY: all test omr_algotest omr_ddrgentest omr_gctest omr_jitbuilderexamples omr_jitbuildertest omr_jittest omr_porttest omr_rastest omr_subscriberforktest omr_sigtest omr_threadextendedtest omr_threadtest omr_utiltest omr_vmtest
