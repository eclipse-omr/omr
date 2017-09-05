###############################################################################
# Copyright (c) 2013, 2017 IBM Corp. and others
# 
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at https://www.eclipse.org/legal/epl-2.0/
# or the Apache License, Version 2.0 which accompanies this distribution and
# is available at https://www.apache.org/licenses/LICENSE-2.0.
# 
# This Source Code is also Distributed under one or more Secondary Licenses,
# as those terms are defined by the Eclipse Public License, v. 2.0: GNU
# General Public License, version 2 with the GNU Classpath Exception [1]
# and GNU General Public License, version 2 with the OpenJDK Assembly
# Exception [2].
# 
# [1] https://www.gnu.org/software/classpath/license.html
# [2] http://openjdk.java.net/legal/assembly-exception.html
# 
# Contributors:
#   Multiple authors (IBM Corp.) - initial API and implementation and/or initial documentation
###############################################################################
# This is a zLinux helper that stores the STFLE bits
# into the passed memory reference.
#
# The helper returns the number of valid double-words in the STFLE
# bits, minus 1.
#
# The stfle instruction is encoded as 0xb2b00000 in binary, leaving
# it in as such so that we can compile on any platform.
# ===================================================================

.file "j9loadfpc.s"

.text
    .align  8
.globl loadfpc
    .type   loadfpc,@function
loadfpc:
.text
	lgf %r2,1300
	aghi %r2,8212
    larl %r3,.LT0
    mvc 0(4,%r2),.LC0-.LT0(%r3)
    lfpc  .LC0-.LT0(%r3)
    br  %r14

    .align  8
.LT0:
.LC0:
    .long   0x00400000
 
