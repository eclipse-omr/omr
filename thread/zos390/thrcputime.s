**********************************************************************
* Copyright (c) 1991, 2016 IBM Corp. and others
* 
* This program and the accompanying materials are made available 
* under the terms of the Eclipse Public License 2.0 which accompanies
* this distribution and is available at 
* https://www.eclipse.org/legal/epl-2.0/ or the Apache License, 
* Version 2.0 which accompanies this distribution and
* is available at https://www.apache.org/licenses/LICENSE-2.0.
*      
* This Source Code may also be made available under the following
* Secondary Licenses when the conditions for such availability set
* forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
* General Public License, version 2 with the GNU Classpath
* Exception [1] and GNU General Public License, version 2 with the
* OpenJDK Assembly Exception [2].
*    
* [1] https://www.gnu.org/software/classpath/license.html
* [2] http://openjdk.java.net/legal/assembly-exception.html
*
* SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
***********************************************************************
         TITLE 'thrcputime.s'

         AIF ('&SYSPARM' EQ 'BIT64').JMP1
_CPUTIME EDCXPRLG BASEREG=8
         LR    3,1
         TIMEUSED ECT=COND,LINKAGE=SYSTEM,STORADR=(3)
         EDCXEPLG
         AGO .JMP2
.JMP1    ANOP
_CPUTIME CELQPRLG BASEREG=8
         LGR    2,1
         SYSSTATE AMODE64=YES
         TIMEUSED ECT=COND,LINKAGE=SYSTEM,STORADR=(2)
         CELQEPLG
.JMP2    ANOP
*
         IHAPSA
         CVT DSECT=YES
         IHAECVT
         END
