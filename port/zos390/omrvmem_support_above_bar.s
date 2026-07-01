***********************************************************************
* Copyright IBM Corp. and others 1991
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
* [2] https://openjdk.org/legal/assembly-exception.html
*
* SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR
* GPL-2.0-only WITH Classpath-exception-2.0 OR
* GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
***********************************************************************

         TITLE 'omrvmem_support_above_bar'

*** Please note: This file contains 2 Macros:
*
* NOTE: Each of these macro definitions start with "MACRO" and end
*       with "MEND"
*
* 1. MYPROLOG. This was needed for the METAL C compiler implementation
*       of omrallocate_large_pages and omrfree_large_pages (implemented
*       at bottom).
* 2. MYEPILOG. See explanation for MYPROLOG
*
*** This file also includes multiple HLASM calls to IARV64 HLASM 
* 		macro
*		- These calls were generated using the METAL-C compiler
*		- See omriarv64.c for details/instructions.
*
         MACRO                                                                 
&NAME    MYPROLOG                                                              
         GBLC  &CCN_PRCN                                                       
         GBLC  &CCN_LITN                                                       
         GBLC  &CCN_BEGIN                                                      
         GBLC  &CCN_ARCHLVL                                                    
         GBLA  &CCN_DSASZ                                                      
         GBLA  &CCN_RLOW                                                       
         GBLA  &CCN_RHIGH                                                      
         GBLB  &CCN_NAB                                                        
         GBLB  &CCN_LP64                                                       
         LARL  15,&CCN_LITN                                                    
         USING &CCN_LITN,15                                                    
         GBLA  &MY_DSASZ                                                       
&MY_DSASZ SETA 0                                                                
         AIF   (&CCN_LP64).LP64_1                                               
         STM   14,12,12(13)                                                     
         AGO   .NEXT_1                                                          
.LP64_1  ANOP                                                                   
         STMG  14,12,8(13)                                                      
.NEXT_1  ANOP                                                                   
         AIF   (&CCN_DSASZ LE 0).DROP                                           
&MY_DSASZ SETA &CCN_DSASZ                                                       
         AIF   (&CCN_DSASZ GT 32767).USELIT                                     
         AIF   (&CCN_LP64).LP64_2                                               
         LHI   0,&CCN_DSASZ                                                     
         AGO   .NEXT_2                                                          
.LP64_2  ANOP                                                                   
         LGHI  0,&CCN_DSASZ                                                     
         AGO   .NEXT_2                                                          
.USELIT  ANOP                                                                   
         AIF   (&CCN_LP64).LP64_3                                               
         L     0,=F'&CCN_DSASZ'                                                 
         AGO   .NEXT_2                                                          
.LP64_3  ANOP                                                                   
         LGF   0,=F'&CCN_DSASZ'                                                 
.NEXT_2  AIF   (NOT &CCN_NAB).GETDSA                                            
&MY_DSASZ SETA &MY_DSASZ+1048576                                                
         LA    1,1                                                              
         SLL   1,20                                                             
         AIF   (&CCN_LP64).LP64_4                                               
         AR    0,1                                                              
         AGO   .GETDSA                                                          
.LP64_4  ANOP                                                                   
         AGR   0,1                                                              
.GETDSA ANOP                                                                    
         STORAGE OBTAIN,LENGTH=(0),BNDRY=PAGE                                   
         AIF   (&CCN_LP64).LP64_5                                               
         LR    15,1                                                             
         ST    15,8(,13)                                                        
         L     1,24(,13)                                                        
         ST    13,4(,15)                                                        
         LR    13,15                                                            
         AGO   .DROP                                                            
.LP64_5  ANOP                                                                   
         LGR   15,1                                                             
         STG   15,136(,13)                                                      
         LG    1,32(,13)                                                        
         STG   13,128(,15)                                                      
         LGR   13,15                                                            
.DROP    ANOP                                                                   
         DROP  15                                                               
         MEND                                                                   
         MACRO                                                                  
&NAME    MYEPILOG                                                               
         GBLC  &CCN_PRCN                                                        
         GBLC  &CCN_LITN                                                        
         GBLC  &CCN_BEGIN                                                       
         GBLC  &CCN_ARCHLVL                                                     
         GBLA  &CCN_DSASZ                                                       
         GBLA  &CCN_RLOW                                                        
         GBLA  &CCN_RHIGH                                                       
         GBLB  &CCN_NAB                                                         
         GBLB  &CCN_LP64                                                        
         GBLA  &MY_DSASZ                                                        
         AIF   (&MY_DSASZ EQ 0).NEXT_1                                          
         AIF   (&CCN_LP64).LP64_1                                               
         LR    1,13                                                             
         AGO   .NEXT_1                                                          
.LP64_1  ANOP                                                                   
         LGR   1,13                                                             
.NEXT_1  ANOP                                                                   
         AIF   (&CCN_LP64).LP64_2                                               
         L     13,4(,13)                                                        
         AGO   .NEXT_2                                                          
.LP64_2  ANOP                                                                   
         LG    13,128(,13)                                                      
.NEXT_2  ANOP                                                                   
         AIF   (&MY_DSASZ EQ 0).NODSA                                           
         AIF   (&CCN_LP64).LP64_3                                               
         ST    15,16(,13)                                                       
         AGO   .NEXT_3                                                          
.LP64_3  ANOP                                                                   
         STG   15,16(,13)                                                       
.NEXT_3  ANOP                                                                   
         LARL  15,&CCN_LITN                                                     
         USING &CCN_LITN,15                                                     
         STORAGE RELEASE,LENGTH=&MY_DSASZ,ADDR=(1)                              
         AIF   (&CCN_LP64).LP64_4                                               
         L     15,16(,13)                                                       
         AGO   .NEXT_4                                                          
.LP64_4  ANOP                                                                   
         LG    15,16(,13)                                                       
.NEXT_4  ANOP                                                                   
.NODSA   ANOP                                                                   
         AIF   (&CCN_LP64).LP64_5                                               
         L     14,12(,13)                                                       
         LM    1,12,24(13)                                                      
         AGO   .NEXT_5                                                          
.LP64_5  ANOP                                                                   
         LG    14,8(,13)                                                        
         LMG   1,12,32(13)                                                      
.NEXT_5  ANOP                                                                   
         BR    14                                                              
         DROP  15                                                               
         MEND
*
**************************************************
* Insert contents of omriarv64.s below
**************************************************
*
         ACONTROL AFPR                                                   000000
OMRIARV64 CSECT                                                          000000
OMRIARV64 AMODE 64                                                       000000
OMRIARV64 RMODE ANY                                                      000000
         GBLA  &CCN_DSASZ              DSA size of the function          000000
         GBLA  &CCN_SASZ               Save Area Size of this function   000000
         GBLA  &CCN_ARGS               Number of fixed parameters        000000
         GBLA  &CCN_RLOW               High GPR on STM/STMG              000000
         GBLA  &CCN_RHIGH              Low GPR for STM/STMG              000000
         GBLB  &CCN_MAIN               True if function is main          000000
         GBLB  &CCN_LP64               True if compiled with LP64        000000
         GBLB  &CCN_NAB                True if NAB needed                000000
.* &CCN_NAB is to indicate if there are called functions that depend on  000000
.* stack space being pre-allocated. When &CCN_NAB is true you'll need    000000
.* to add a generous amount to the size set in &CCN_DSASZ when you       000000
.* obtain the stack space.                                               000000
         GBLB  &CCN_ALTGPR(16)         Altered GPRs by the function      000000
         GBLB  &CCN_SASIG              True to gen savearea signature    000000
         GBLC  &CCN_PRCN               Entry symbol of the function      000000
         GBLC  &CCN_CSECT              CSECT name of the file            000000
         GBLC  &CCN_LITN               Symbol name for LTORG             000000
         GBLC  &CCN_BEGIN              Symbol name for function body     000000
         GBLC  &CCN_ARCHLVL            n in ARCH(n) option               000000
         GBLC  &CCN_ASCM               A=AR mode P=Primary mode          000000
         GBLC  &CCN_NAB_OFFSET         Offset to NAB pointer in DSA      000000
         GBLB  &CCN_NAB_STORED         True if NAB pointer stored        000000
         GBLC  &CCN_PRCN_LONG          Full func name up to 1024 chars   000000
         GBLB  &CCN_STATIC             True if function is static        000000
         GBLB  &CCN_RENT               True if compiled with RENT        000000
         GBLB  &CCN_APARSE             True to parse OS PARM             000000
&CCN_SASIG SETB 1                                                        000000
&CCN_LP64 SETB 1                                                         000000
&CCN_RENT SETB 0                                                         000000
&CCN_APARSE SETB 1                                                       000000
&CCN_CSECT SETC 'OMRIARV64'                                              000000
&CCN_ARCHLVL SETC '10'                                                   000000
         SYSSTATE ARCHLVL=2,AMODE64=YES                                  000000
         IEABRCX DEFINE                                                  000000
.* The HLASM GOFF option is needed to assemble this program              000000
@@CCN@113 ALIAS C'omrdiscard_data'                                       000000
@@CCN@104 ALIAS C'omradd_guard'                                          000000
@@CCN@95 ALIAS C'omrremove_guard'                                        000000
@@CCN@87 ALIAS C'omrfree_memory_above_bar'                               000000
@@CCN@78 ALIAS C'omrallocate_4K_pages_guarded_above_bar'                 000000
@@CCN@69 ALIAS C'omrallocate_4K_pages_above_bar'                         000000
@@CCN@58 ALIAS C'omrallocate_4K_pages_guarded_in_userExtendedPrivateAreX 000000
               a'                                                        000000
@@CCN@47 ALIAS C'omrallocate_4K_pages_in_userExtendedPrivateArea'        000000
@@CCN@36 ALIAS C'omrallocate_2G_pages'                                   000000
@@CCN@25 ALIAS C'omrallocate_1M_pageable_pages_guarded_above_bar'        000000
@@CCN@14 ALIAS C'omrallocate_1M_pageable_pages_above_bar'                000000
@@CCN@2  ALIAS C'omrallocate_1M_fixed_pages'                             000000
* /********************************************************************  000001
*  * Copyright IBM Corp. and others 1991                                 000002
*  *                                                                     000003
*  * This program and the accompanying materials are made available und  000004
*  * the terms of the Eclipse Public License 2.0 which accompanies this  000005
*  * distribution and is available at https://www.eclipse.org/legal/epl  000006
*  * or the Apache License, Version 2.0 which accompanies this distribu  000007
*  * is available at https://www.apache.org/licenses/LICENSE-2.0.        000008
*  *                                                                     000009
*  * This Source Code may also be made available under the following     000010
*  * Secondary Licenses when the conditions for such availability set    000011
*  * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU      000012
*  * General Public License, version 2 with the GNU Classpath            000013
*  * Exception [1] and GNU General Public License, version 2 with the    000014
*  * OpenJDK Assembly Exception [2].                                     000015
*  *                                                                     000016
*  * [1] https://www.gnu.org/software/classpath/license.html             000017
*  * [2] https://openjdk.org/legal/assembly-exception.html               000018
*  *                                                                     000019
*  * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WIT  000020
*  ********************************************************************  000021
*                                                                        000022
* /*                                                                     000023
*  * This file is used to generate the HLASM corresponding to the C cal  000024
*  * that use the IARV64 macro in omrvmem.c                              000025
*  *                                                                     000026
*  * This file is compiled manually using the METAL-C compiler that was  000027
*  * introduced in z/OS V1R9. The generated output (omriarv64.s) is the  000028
*  * inserted into omrvmem_support_above_bar.s which is compiled by our  000029
*  *                                                                     000030
*  * omrvmem_support_above_bar.s indicates where to put the contents of  000031
*  * Search for:                                                         000032
*  *   Insert contents of omriarv64.s below                              000033
*  *                                                                     000034
*  * *******                                                             000035
*  * NOTE!!!!! You must strip the line numbers from any pragma statemen  000036
*  * *******                                                             000037
*  *                                                                     000038
*  * It should be obvious, however, just to be clear be sure to omit th  000039
*  * first two lines from omriarv64.s which will look something like:    000040
*  *                                                                     000041
*  *          TITLE '5694A01 V1.9 z/OS XL C                              000042
*  *                     ./omriarv64.c'                                  000043
*  *                                                                     000044
*  * To compile:                                                         000045
*  *  xlc -S -qmetal -Wc,lp64 -qlongname omriarv64.c                     000046
*  *                                                                     000047
*  * z/OS V1R9 z/OS V1R9.0 Metal C Programming Guide and Reference:      000048
*  *   http://publibz.boulder.ibm.com/epubs/pdf/ccrug100.pdf             000049
*  */                                                                    000050
*                                                                        000051
* #include "omriarv64.h"                                                 000052
*                                                                        000053
* #pragma prolog(omrallocate_1M_fixed_pages,"MYPROLOG")                  000054
* #pragma epilog(omrallocate_1M_fixed_pages,"MYEPILOG")                  000055
*                                                                        000056
* __asm(" IARV64 PLISTVER=MAX,MF=(L,LGETSTOR)":"DS"(lgetstor));          000057
*                                                                        000058
* /*                                                                     000059
*  * Allocate 1MB fixed pages using IARV64 system macro.                 000060
*  * Memory allocated is freed using omrfree_memory_above_bar().         000061
*  *                                                                     000062
*  * @params[in] numMBSegments Number of 1MB segments to be allocated    000063
*  * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0   000064
*  *                                                                     000065
*  * @return pointer to memory allocated, NULL on failure.               000066
*  */                                                                    000067
* void * omrallocate_1M_fixed_pages(int *numMBSegments, int *userExtend  000068
*  long segments = 0;                                                    000069
*  long origin = 0;                                                      000070
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000071
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000072
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,LGETSTOR)":"DS"(wgetstor));         000073
*                                                                        000074
*  segments = *numMBSegments;                                            000075
*  wgetstor = lgetstor;                                                  000076
*                                                                        000077
*  switch (useMemoryType) {                                              000078
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000079
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,PAG  000080
*     "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\    000081
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000082
*   break;                                                               000083
*  case ZOS64_VMEM_2_TO_32G:                                             000084
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO32G=YES,"\   000085
*     "CONTROL=UNAUTH,PAGEFRAMESIZE=1MEG,"\                              000086
*     "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\    000087
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000088
*   break;                                                               000089
*  case ZOS64_VMEM_2_TO_64G:                                             000090
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO64G=YES,"\   000091
*     "CONTROL=UNAUTH,PAGEFRAMESIZE=1MEG,"\                              000092
*     "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\    000093
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000094
*   break;                                                               000095
*  }                                                                     000096
*                                                                        000097
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000098
*   return (void *)0;                                                    000099
*  }                                                                     000100
*  return (void *)origin;                                                000101
* }                                                                      000102
*                                                                        000103
* #pragma prolog(omrallocate_1M_pageable_pages_above_bar,"MYPROLOG")     000104
* #pragma epilog(omrallocate_1M_pageable_pages_above_bar,"MYEPILOG")     000105
*                                                                        000106
* __asm(" IARV64 PLISTVER=MAX,MF=(L,NGETSTOR)":"DS"(ngetstor));          000107
*                                                                        000108
* /*                                                                     000109
*  * Allocate 1MB pageable pages above 2GB bar using IARV64 system macr  000110
*  * Memory allocated is freed using omrfree_memory_above_bar().         000111
*  *                                                                     000112
*  * @params[in] numMBSegments Number of 1MB segments to be allocated    000113
*  * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0   000114
*  *                                                                     000115
*  * @return pointer to memory allocated, NULL on failure.               000116
*  */                                                                    000117
* void * omrallocate_1M_pageable_pages_above_bar(int *numMBSegments, in  000118
*  long segments = 0;                                                    000119
*  long origin = 0;                                                      000120
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000121
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000122
*                                                                        000123
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,NGETSTOR)":"DS"(wgetstor));         000124
*                                                                        000125
*  segments = *numMBSegments;                                            000126
*  wgetstor = ngetstor;                                                  000127
*                                                                        000128
*  switch (useMemoryType) {                                              000129
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000130
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,"\   000131
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000132
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000133
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000134
*   break;                                                               000135
*  case ZOS64_VMEM_2_TO_32G:                                             000136
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000137
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000138
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000139
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000140
*   break;                                                               000141
*  case ZOS64_VMEM_2_TO_64G:                                             000142
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000143
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000144
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000145
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000146
*   break;                                                               000147
*  }                                                                     000148
*                                                                        000149
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000150
*   return (void *)0;                                                    000151
*  }                                                                     000152
*  return (void *)origin;                                                000153
* }                                                                      000154
*                                                                        000155
* #pragma prolog(omrallocate_1M_pageable_pages_guarded_above_bar,"MYPRO  000156
* #pragma epilog(omrallocate_1M_pageable_pages_guarded_above_bar,"MYEPI  000157
*                                                                        000158
* __asm(" IARV64 PLISTVER=MAX,MF=(L,SGETSTOR)":"DS"(sgetstor));          000159
*                                                                        000160
* /*                                                                     000161
*  * Allocate 1MB pageable guarded pages above 2GB bar using IARV64 sys  000162
*  * Memory allocated is freed using omrfree_memory_above_bar().         000163
*  *                                                                     000164
*  * Note: To stay below MEMLIMIT, GUARDSIZE64 is set equal to the numb  000165
*  * segments, which guards the entire allocated memory region.          000166
*  *                                                                     000167
*  * @params[in] numMBSegments number of 1MB segments to be allocated    000168
*  * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0   000169
*  *                                                                     000170
*  * @return pointer to memory allocated, NULL on failure.               000171
*  */                                                                    000172
* void *omrallocate_1M_pageable_pages_guarded_above_bar(int *numMBSegme  000173
*  long segments = 0;                                                    000174
*  long origin = 0;                                                      000175
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000176
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000177
*                                                                        000178
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,SGETSTOR)":"DS"(wgetstor));         000179
*                                                                        000180
*  segments = *numMBSegments;                                            000181
*  wgetstor = sgetstor;                                                  000182
*                                                                        000183
*  switch (useMemoryType) {                                              000184
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000185
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,"\   000186
*     "GUARDSIZE64=(%2),"\                                               000187
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000188
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000189
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000190
*   break;                                                               000191
*  case ZOS64_VMEM_2_TO_32G:                                             000192
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000193
*     "GUARDSIZE64=(%2),"\                                               000194
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000195
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000196
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000197
*   break;                                                               000198
*  case ZOS64_VMEM_2_TO_64G:                                             000199
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000200
*     "GUARDSIZE64=(%2),"\                                               000201
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000202
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000203
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000204
*   break;                                                               000205
*  }                                                                     000206
*                                                                        000207
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000208
*   return (void *)0;                                                    000209
*  }                                                                     000210
*  return (void *)origin;                                                000211
* }                                                                      000212
*                                                                        000213
* #pragma prolog(omrallocate_2G_pages,"MYPROLOG")                        000214
* #pragma epilog(omrallocate_2G_pages,"MYEPILOG")                        000215
*                                                                        000216
* __asm(" IARV64 PLISTVER=MAX,MF=(L,OGETSTOR)":"DS"(ogetstor));          000217
*                                                                        000218
* /*                                                                     000219
*  * Allocate 2GB fixed pages using IARV64 system macro.                 000220
*  * Memory allocated is freed using omrfree_memory_above_bar().         000221
*  *                                                                     000222
*  * @params[in] num2GBUnits Number of 2GB units to be allocated         000223
*  * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0   000224
*  *                                                                     000225
*  * @return pointer to memory allocated, NULL on failure.               000226
*  */                                                                    000227
* void * omrallocate_2G_pages(int *num2GBUnits, int *userExtendedPrivat  000228
*  long units = 0;                                                       000229
*  long origin = 0;                                                      000230
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000231
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000232
*                                                                        000233
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,OGETSTOR)":"DS"(wgetstor));         000234
*                                                                        000235
*  units = *num2GBUnits;                                                 000236
*  wgetstor = ogetstor;                                                  000237
*                                                                        000238
*  switch (useMemoryType) {                                              000239
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000240
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,"\   000241
*     "PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(%2),"\             000242
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000243
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&units),"r"(&wgetstor),"r"(ttkn  000244
*   break;                                                               000245
*  case ZOS64_VMEM_2_TO_32G:                                             000246
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000247
*     "PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(%2),"\             000248
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000249
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&units),"r"(&wgetstor),"r"(ttkn  000250
*   break;                                                               000251
*  case ZOS64_VMEM_2_TO_64G:                                             000252
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000253
*     "PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(%2),"\             000254
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000255
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&units),"r"(&wgetstor),"r"(ttkn  000256
*   break;                                                               000257
*  }                                                                     000258
*                                                                        000259
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000260
*   return (void *)0;                                                    000261
*  }                                                                     000262
*  return (void *)origin;                                                000263
* }                                                                      000264
*                                                                        000265
* #pragma prolog(omrallocate_4K_pages_in_userExtendedPrivateArea,"MYPRO  000266
* #pragma epilog(omrallocate_4K_pages_in_userExtendedPrivateArea,"MYEPI  000267
*                                                                        000268
* __asm(" IARV64 PLISTVER=MAX,MF=(L,MGETSTOR)":"DS"(mgetstor));          000269
*                                                                        000270
* /*                                                                     000271
*  * Allocate 4KB pages above 2GB bar using IARV64 system macro.         000272
*  * Memory allocated is freed using omrfree_memory_above_bar().         000273
*  *                                                                     000274
*  * @params[in] numMBSegments Number of 1MB segments to be allocated    000275
*  * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0   000276
*  *                                                                     000277
*  * @return pointer to memory allocated, NULL on failure.               000278
*  */                                                                    000279
* void * omrallocate_4K_pages_in_userExtendedPrivateArea(int *numMBSegm  000280
*  long segments = 0;                                                    000281
*  long origin = 0;                                                      000282
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000283
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000284
*                                                                        000285
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,MGETSTOR)":"DS"(wgetstor));         000286
*                                                                        000287
*  segments = *numMBSegments;                                            000288
*  wgetstor = mgetstor;                                                  000289
*                                                                        000290
*  switch (useMemoryType) {                                              000291
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000292
*   break;                                                               000293
*  case ZOS64_VMEM_2_TO_32G:                                             000294
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO32G=YES,"\   000295
*     "CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\                                000296
*     "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\    000297
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000298
*   break;                                                               000299
*  case ZOS64_VMEM_2_TO_64G:                                             000300
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO64G=YES,"\   000301
*     "CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\                                000302
*     "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\    000303
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000304
*   break;                                                               000305
*  }                                                                     000306
*                                                                        000307
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000308
*   return (void *)0;                                                    000309
*  }                                                                     000310
*  return (void *)origin;                                                000311
* }                                                                      000312
*                                                                        000313
* #pragma prolog(omrallocate_4K_pages_guarded_in_userExtendedPrivateAre  000314
* #pragma epilog(omrallocate_4K_pages_guarded_in_userExtendedPrivateAre  000315
*                                                                        000316
* __asm(" IARV64 PLISTVER=MAX,MF=(L,TGETSTOR)":"DS"(tgetstor));          000317
*                                                                        000318
* /*                                                                     000319
*  * Allocate 4KB pages guarded above 2G bar using IARV64 system macro.  000320
*  * Memory allocated is freed using omrfree_memory_above_bar().         000321
*  *                                                                     000322
*  * @params[in] numMBSegments number of 1MB segments to be allocated    000323
*  * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0   000324
*  *                                                                     000325
*  * @return pointer to memory allocated, NULL on failure.               000326
*  */                                                                    000327
* void *omrallocate_4K_pages_guarded_in_userExtendedPrivateArea(int *nu  000328
*  long segments = 0;                                                    000329
*  long origin = 0;                                                      000330
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000331
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000332
*                                                                        000333
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,TGETSTOR)":"DS"(wgetstor));         000334
*                                                                        000335
*  segments = *numMBSegments;                                            000336
*  wgetstor = tgetstor;                                                  000337
*                                                                        000338
*  switch (useMemoryType) {                                              000339
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000340
*   break;                                                               000341
*  case ZOS64_VMEM_2_TO_32G:                                             000342
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000343
*     "GUARDSIZE64=(%2),PAGEFRAMESIZE=4K,SEGMENTS=(%2),"\                000344
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000345
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000346
*   break;                                                               000347
*  case ZOS64_VMEM_2_TO_64G:                                             000348
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000349
*     "GUARDSIZE64=(%2),PAGEFRAMESIZE=4K,SEGMENTS=(%2),"\                000350
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000351
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000352
*   break;                                                               000353
*  }                                                                     000354
*                                                                        000355
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000356
*   return (void *)0;                                                    000357
*  }                                                                     000358
*  return (void *)origin;                                                000359
* }                                                                      000360
*                                                                        000361
* #pragma prolog(omrallocate_4K_pages_above_bar,"MYPROLOG")              000362
* #pragma epilog(omrallocate_4K_pages_above_bar,"MYEPILOG")              000363
*                                                                        000364
* __asm(" IARV64 PLISTVER=MAX,MF=(L,RGETSTOR)":"DS"(rgetstor));          000365
*                                                                        000366
* /*                                                                     000367
*  * Allocate 4KB pages using IARV64 system macro.                       000368
*  * Memory allocated is freed using omrfree_memory_above_bar().         000369
*  *                                                                     000370
*  * @params[in] numMBSegments Number of 1MB segments to be allocated    000371
*  *                                                                     000372
*  * @return pointer to memory allocated, NULL on failure.               000373
*  */                                                                    000374
* void * omrallocate_4K_pages_above_bar(int *numMBSegments, const char   000375
*  long segments = 0;                                                    000376
*  long origin = 0;                                                      000377
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000378
*                                                                        000379
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,RGETSTOR)":"DS"(wgetstor));         000380
*                                                                        000381
*  segments = *numMBSegments;                                            000382
*  wgetstor = rgetstor;                                                  000383
*                                                                        000384
*  __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,"\                   000385
*    "CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\                                 000386
*    "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\     000387
*    ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(tt  000388
*                                                                        000389
*  if (0 != iarv64_rc) {                                                 000390
*   return (void *)0;                                                    000391
*  }                                                                     000392
*  return (void *)origin;                                                000393
* }                                                                      000394
*                                                                        000395
* #pragma prolog(omrallocate_4K_pages_guarded_above_bar,"MYPROLOG")      000396
* #pragma epilog(omrallocate_4K_pages_guarded_above_bar,"MYEPILOG")      000397
*                                                                        000398
* __asm(" IARV64 PLISTVER=MAX,MF=(L,EGETSTOR)":"DS"(egetstor));          000399
*                                                                        000400
* /*                                                                     000401
*  * Allocate 4KB pages guarded using IARV64 system macro.               000402
*  * Memory allocated is freed using omrfree_memory_above_bar().         000403
*  *                                                                     000404
*  * Note: To stay below MEMLIMIT, GUARDSIZE64 is set equal to the numb  000405
*  * segments, which guards the entire allocated memory region.          000406
*  *                                                                     000407
*  * @params[in] numMBSegments number of 1MB segments to be allocated    000408
*  *                                                                     000409
*  * @return pointer to memory allocated, NULL on failure.               000410
*  */                                                                    000411
* void *omrallocate_4K_pages_guarded_above_bar(int *numMBSegments, cons  000412
*  long segments = 0;                                                    000413
*  long origin = 0;                                                      000414
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000415
*                                                                        000416
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,EGETSTOR)":"DS"(wgetstor));         000417
*                                                                        000418
*  segments = *numMBSegments;                                            000419
*  wgetstor = egetstor;                                                  000420
*                                                                        000421
*  __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,"\                   000422
*    "GUARDSIZE=(%2),"\                                                  000423
*    "CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\                                 000424
*    "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\     000425
*    ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(tt  000426
*                                                                        000427
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000428
*   return (void *)0;                                                    000429
*  }                                                                     000430
*  return (void *)origin;                                                000431
* }                                                                      000432
*                                                                        000433
* #pragma prolog(omrfree_memory_above_bar,"MYPROLOG")                    000434
* #pragma epilog(omrfree_memory_above_bar,"MYEPILOG")                    000435
*                                                                        000436
* __asm(" IARV64 PLISTVER=MAX,MF=(L,PGETSTOR)":"DS"(pgetstor));          000437
*                                                                        000438
* /*                                                                     000439
*  * Free memory allocated using IARV64 system macro.                    000440
*  *                                                                     000441
*  * @params[in] address pointer to memory region to be freed            000442
*  *                                                                     000443
*  * @return non-zero if memory is not freed successfully, 0 otherwise.  000444
*  */                                                                    000445
* int omrfree_memory_above_bar(void *address, const char * ttkn){        000446
*  void *xmemobjstart = 0;                                               000447
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000448
*                                                                        000449
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,PGETSTOR)":"DS"(wgetstor));         000450
*                                                                        000451
*  xmemobjstart = address;                                               000452
*  wgetstor = pgetstor;                                                  000453
*                                                                        000454
*  __asm(" IARV64 REQUEST=DETACH,COND=YES,MEMOBJSTART=(%2),TTOKEN=(%3),  000455
*    ::"m"(iarv64_rc),"r"(&wgetstor),"r"(&xmemobjstart),"r"(ttkn));      000456
*  return iarv64_rc;                                                     000457
* }                                                                      000458
*                                                                        000459
* #pragma prolog(omrremove_guard,"MYPROLOG")                             000460
* #pragma epilog(omrremove_guard,"MYEPILOG")                             000461
*                                                                        000462
* __asm(" IARV64 PLISTVER=MAX,MF=(L,FGETSTOR)":"DS"(fgetstor));          000463
*                                                                        000464
* /*                                                                     000465
*  * Remove guard for memory allocated using IARV64 system macro.        000466
*  *                                                                     000467
*  * @params[in] address pointer to memory region to be freed            000468
*  * @params[in] numMBSegments number of 1MB segments to be allocated    000469
*  *                                                                     000470
*  * @return non-zero if memory is not freed successfully, 0 otherwise.  000471
*  *         04 - Operation completed successfully but with exceptions.  000472
*  *              One or more segments in the memory object are already  000473
*  *              in the requested state.                                000474
*  *         08 - Request was rejected because there was insufficient    000475
*  *              storage to satisfy the request.                        000476
*  */                                                                    000477
* int omrremove_guard(void *address, int *numMBSegments){                000478
*  void *memObjConvertStart = 0;                                         000479
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000480
*  long segments = 0;                                                    000481
*                                                                        000482
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,FGETSTOR)":"DS"(wgetstor));         000483
*                                                                        000484
*  memObjConvertStart = address;                                         000485
*  wgetstor = fgetstor;                                                  000486
*  segments = *numMBSegments;                                            000487
*                                                                        000488
*  __asm(" IARV64 REQUEST=CHANGEGUARD,CONVERT=FROMGUARD,COND=YES,"\      000489
*    "CONVERTSTART=(%2),CONVERTSIZE64=(%3),"\                            000490
*    "RETCODE=%0,MF=(E,(%1))"\                                           000491
*    ::"m"(iarv64_rc),"r"(&wgetstor),"r"(&memObjConvertStart),"r"(&segm  000492
*  return iarv64_rc;                                                     000493
* }                                                                      000494
*                                                                        000495
* #pragma prolog(omradd_guard,"MYPROLOG")                                000496
* #pragma epilog(omradd_guard,"MYEPILOG")                                000497
*                                                                        000498
* __asm(" IARV64 PLISTVER=MAX,MF=(L,DGETSTOR)":"DS"(dgetstor));          000499
*                                                                        000500
* /*                                                                     000501
*  * Add guard to memory allocated using IARV64 system macro.            000502
*  *                                                                     000503
*  * @params[in] address pointer to memory region to be freed            000504
*  * @params[in] numMBSegments number of 1MB segments to be allocated    000505
*  *                                                                     000506
*  * @return non-zero if memory is not freed successfully, 0 otherwise.  000507
*  *         04 - Operation completed successfully but with exceptions.  000508
*  *              One or more segments in the memory object are already  000509
*  *              in the requested state.                                000510
*  *         08 - Request was rejected because there was insufficient    000511
*  *              storage to satisfy the request.                        000512
*  */                                                                    000513
* int omradd_guard(void *address, int *numMBSegments) {                  000514
*  void *memObjConvertStart = 0;                                         000515
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000516
*  long segments = 0;                                                    000517
*                                                                        000518
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,DGETSTOR)":"DS"(wgetstor));         000519
*                                                                        000520
*  memObjConvertStart = address;                                         000521
*  wgetstor = dgetstor;                                                  000522
*  segments = *numMBSegments;                                            000523
*                                                                        000524
*  __asm(" IARV64 REQUEST=CHANGEGUARD,CONVERT=TOGUARD,COND=YES,"\        000525
*    "CONVERTSTART=(%2),CONVERTSIZE64=(%3),"\                            000526
*    "RETCODE=%0,MF=(E,(%1))"\                                           000527
*    ::"m"(iarv64_rc),"r"(&wgetstor),"r"(&memObjConvertStart),"r"(&segm  000528
*  return iarv64_rc;                                                     000529
* }                                                                      000530
*                                                                        000531
* #pragma prolog(omrdiscard_data,"MYPROLOG")                             000532
* #pragma epilog(omrdiscard_data,"MYEPILOG")                             000533
*                                                                        000534
* __asm(" IARV64 PLISTVER=MAX,MF=(L,QGETSTOR)":"DS"(qgetstor));          000535
*                                                                        000536
* /* Used to pass parameters to IARV64 DISCARDDATA in omrdiscard_data()  000537
* struct rangeList {                                                     000538
*  void *startAddr;                                                      000539
*  long pageCount;                                                       000540
* };                                                                     000541
*                                                                        000542
* /*                                                                     000543
*  * Discard memory allocated using IARV64 system macro.                 000544
*  *                                                                     000545
*  * @params[in] address pointer to memory region to be discarded        000546
*  * @params[in] numFrames number of frames to be discarded. Frame size  000547
*  *                                                                     000548
*  * @return non-zero if memory is not discarded successfully, 0 otherw  000549
*  */                                                                    000550
* int omrdiscard_data(void *address, int *numFrames) {                   000551
         J     @@CCN@113                                                 000551
@@PFD@@  DC    XL8'00C300C300D50000'   Prefix Data Marker                000551
         DC    CL8'20260629'           Compiled Date YYYYMMDD            000551
         DC    CL6'115622'             Compiled Time HHMMSS              000551
         DC    XL4'42040000'           Compiler Version                  000551
         DC    XL2'0000'               Reserved                          000551
         DC    BL1'00000000'           Flag Set 1                        000551
         DC    BL1'00000000'           Flag Set 2                        000551
         DC    BL1'00000000'           Flag Set 3                        000551
         DC    BL1'00000000'           Flag Set 4                        000551
         DC    XL4'00000000'           Reserved                          000551
         ENTRY @@CCN@113                                                 000551
@@CCN@113 AMODE 64                                                       000551
         DC    0FD                                                       000551
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000551
         DC    A(@@FPB@1-*+8)          Signed offset to FPB              000551
         DC    XL4'00000000'           Reserved                          000551
@@CCN@113 DS   0FD                                                       000551
&CCN_PRCN SETC '@@CCN@113'                                               000551
&CCN_PRCN_LONG SETC 'omrdiscard_data'                                    000551
&CCN_LITN SETC '@@LIT@1'                                                 000551
&CCN_BEGIN SETC '@@BGN@1'                                                000551
&CCN_ASCM SETC 'P'                                                       000551
&CCN_DSASZ SETA 472                                                      000551
&CCN_SASZ SETA 144                                                       000551
&CCN_ARGS SETA 2                                                         000551
&CCN_RLOW SETA 14                                                        000551
&CCN_RHIGH SETA 4                                                        000551
&CCN_NAB SETB  0                                                         000551
&CCN_MAIN SETB 0                                                         000551
&CCN_NAB_STORED SETB 0                                                   000551
&CCN_STATIC SETB 0                                                       000551
&CCN_ALTGPR(1) SETB 1                                                    000551
&CCN_ALTGPR(2) SETB 1                                                    000551
&CCN_ALTGPR(3) SETB 1                                                    000551
&CCN_ALTGPR(4) SETB 1                                                    000551
&CCN_ALTGPR(5) SETB 1                                                    000551
&CCN_ALTGPR(6) SETB 0                                                    000551
&CCN_ALTGPR(7) SETB 0                                                    000551
&CCN_ALTGPR(8) SETB 0                                                    000551
&CCN_ALTGPR(9) SETB 0                                                    000551
&CCN_ALTGPR(10) SETB 0                                                   000551
&CCN_ALTGPR(11) SETB 0                                                   000551
&CCN_ALTGPR(12) SETB 0                                                   000551
&CCN_ALTGPR(13) SETB 0                                                   000551
&CCN_ALTGPR(14) SETB 1                                                   000551
&CCN_ALTGPR(15) SETB 1                                                   000551
&CCN_ALTGPR(16) SETB 1                                                   000551
         MYPROLOG                                                        000551
@@BGN@1  DS    0H                                                        000551
         AIF   (NOT &CCN_SASIG).@@NOSIG1                                 000551
         LLILH 4,X'C6F4'                                                 000551
         OILL  4,X'E2C1'                                                 000551
         ST    4,4(,13)                                                  000551
.@@NOSIG1 ANOP                                                           000551
         USING @@AUTO@1,13                                               000551
         LARL  3,@@LIT@1                                                 000551
         USING @@LIT@1,3                                                 000551
         STG   1,464(0,13)             #SR_PARM_1                        000551
*  struct rangeList range = {0};                                         000552
         MVGHI 176(13),0                                                 000552
         MVI   @117range+8,0                                             000552
         MVC   @117range+9(7),@117range+8                                000552
*  void *rangePtr = 0;                                                   000553
         MVGHI @119rangePtr,0                                            000553
*  int iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                    000554
         MVHI  @120iarv64_rc@67,8                                        000554
*                                                                        000555
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,QGETSTOR)":"DS"(wgetstor));         000556
*                                                                        000557
*  range.startAddr = address;                                            000558
         LG    14,464(0,13)            #SR_PARM_1                        000558
         USING @@PARMD@1,14                                              000558
         LG    14,@114address@65                                         000558
         STG   14,176(0,13)            range_rangeList_startAddr         000558
*  range.pageCount = *numFrames;                                         000559
         LG    14,464(0,13)            #SR_PARM_1                        000559
         LG    14,@115numFrames                                          000559
         LGF   14,0(0,14)              (*)int                            000559
         STG   14,184(0,13)            range_rangeList_pageCount         000559
*  rangePtr = (void *)&range;                                            000560
         LA    14,@117range                                              000560
         STG   14,@119rangePtr                                           000560
*  wgetstor = qgetstor;                                                  000561
         LARL  14,$STATIC                                                000561
         DROP  14                                                        000561
         USING @@STATICD@@,14                                            000561
         MVC   @121wgetstor,@112qgetstor                                 000561
*                                                                        000562
*  __asm(" IARV64 REQUEST=DISCARDDATA,KEEPREAL=NO,"\                     000563
         LA    2,@119rangePtr                                            000563
         DROP  14                                                        000563
         LA    4,@121wgetstor                                            000563
         IARV64 REQUEST=DISCARDDATA,KEEPREAL=NO,RANGLIST=(2),RETCODE=20X 000563
               0(13),MF=(E,(4))                                          000563
*    "RANGLIST=(%1),RETCODE=%0,MF=(E,(%2))"\                             000564
*    ::"m"(iarv64_rc),"r"(&rangePtr),"r"(&wgetstor));                    000565
*                                                                        000566
*  return iarv64_rc;                                                     000567
         LGF   15,@120iarv64_rc@67                                       000567
* }                                                                      000568
@1L37    DS    0H                                                        000568
         DROP                                                            000568
         MYEPILOG                                                        000568
OMRIARV64 CSECT ,                                                        000568
         DS    0FD                                                       000568
@@LIT@1  LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@1  DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111100000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@1)      Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(15)                 Function Name                     000000
         DC    C'omrdiscard_data'                                        000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@1 DSECT                                                           000000
         DS    59FD                                                      000000
         ORG   @@AUTO@1                                                  000000
#GPR_SA_1 DS   18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@1+176                                              000000
@117range DS   XL16                                                      000000
         ORG   @@AUTO@1+192                                              000000
@119rangePtr DS AD                                                       000000
         ORG   @@AUTO@1+200                                              000000
@120iarv64_rc@67 DS F                                                    000000
         ORG   @@AUTO@1+208                                              000000
@121wgetstor DS XL256                                                    000000
         ORG   @@AUTO@1+464                                              000000
#SR_PARM_1 DS  XL8                                                       000000
@@PARMD@1 DSECT                                                          000000
         DS    XL16                                                      000000
         ORG   @@PARMD@1+0                                               000000
         DS    0FD                                                       000000
@114address@65 DS 0XL8                                                   000000
         ORG   @@PARMD@1+8                                               000000
         DS    0FD                                                       000000
@115numFrames DS 0XL8                                                    000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* void * omrallocate_1M_fixed_pages(int *numMBSegments, int *userExtend  000068
         ENTRY @@CCN@2                                                   000068
@@CCN@2  AMODE 64                                                        000068
         DC    0FD                                                       000068
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000068
         DC    A(@@FPB@12-*+8)         Signed offset to FPB              000068
         DC    XL4'00000000'           Reserved                          000068
@@CCN@2  DS    0FD                                                       000068
&CCN_PRCN SETC '@@CCN@2'                                                 000068
&CCN_PRCN_LONG SETC 'omrallocate_1M_fixed_pages'                         000068
&CCN_LITN SETC '@@LIT@12'                                                000068
&CCN_BEGIN SETC '@@BGN@12'                                               000068
&CCN_ASCM SETC 'P'                                                       000068
&CCN_DSASZ SETA 480                                                      000068
&CCN_SASZ SETA 144                                                       000068
&CCN_ARGS SETA 3                                                         000068
&CCN_RLOW SETA 14                                                        000068
&CCN_RHIGH SETA 6                                                        000068
&CCN_NAB SETB  0                                                         000068
&CCN_MAIN SETB 0                                                         000068
&CCN_NAB_STORED SETB 0                                                   000068
&CCN_STATIC SETB 0                                                       000068
&CCN_ALTGPR(1) SETB 1                                                    000068
&CCN_ALTGPR(2) SETB 1                                                    000068
&CCN_ALTGPR(3) SETB 1                                                    000068
&CCN_ALTGPR(4) SETB 1                                                    000068
&CCN_ALTGPR(5) SETB 1                                                    000068
&CCN_ALTGPR(6) SETB 1                                                    000068
&CCN_ALTGPR(7) SETB 1                                                    000068
&CCN_ALTGPR(8) SETB 0                                                    000068
&CCN_ALTGPR(9) SETB 0                                                    000068
&CCN_ALTGPR(10) SETB 0                                                   000068
&CCN_ALTGPR(11) SETB 0                                                   000068
&CCN_ALTGPR(12) SETB 0                                                   000068
&CCN_ALTGPR(13) SETB 0                                                   000068
&CCN_ALTGPR(14) SETB 1                                                   000068
&CCN_ALTGPR(15) SETB 1                                                   000068
&CCN_ALTGPR(16) SETB 1                                                   000068
         MYPROLOG                                                        000068
@@BGN@12 DS    0H                                                        000068
         AIF   (NOT &CCN_SASIG).@@NOSIG12                                000068
         LLILH 6,X'C6F4'                                                 000068
         OILL  6,X'E2C1'                                                 000068
         ST    6,4(,13)                                                  000068
.@@NOSIG12 ANOP                                                          000068
         USING @@AUTO@12,13                                              000068
         LARL  3,@@LIT@12                                                000068
         USING @@LIT@12,3                                                000068
         STG   1,464(0,13)             #SR_PARM_12                       000068
*  long segments = 0;                                                    000069
         MVGHI @7segments,0                                              000069
*  long origin = 0;                                                      000070
         MVGHI @8origin,0                                                000070
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000071
         LG    14,464(0,13)            #SR_PARM_12                       000071
         USING @@PARMD@12,14                                             000071
         LG    14,@4userExtendedPrivateAreaMemoryType                    000071
         LGF   14,0(0,14)              (*)int                            000071
         STG   14,@9useMemoryType                                        000071
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000072
         MVHI  @11iarv64_rc,8                                            000072
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,LGETSTOR)":"DS"(wgetstor));         000073
*                                                                        000074
*  segments = *numMBSegments;                                            000075
         LG    14,464(0,13)            #SR_PARM_12                       000075
         LG    14,@3numMBSegments                                        000075
         LGF   14,0(0,14)              (*)int                            000075
         STG   14,@7segments                                             000075
*  wgetstor = lgetstor;                                                  000076
         LARL  14,$STATIC                                                000076
         DROP  14                                                        000076
         USING @@STATICD@@,14                                            000076
         MVC   @12wgetstor,@1lgetstor                                    000076
*                                                                        000077
*  switch (useMemoryType) {                                              000078
         LG    14,@9useMemoryType                                        000078
         STG   14,472(0,13)            #SW_WORK12                        000078
         CLG   14,=X'0000000000000002'                                   000078
         BRH   @12L68                                                    000078
         LG    14,472(0,13)            #SW_WORK12                        000078
         SLLG  14,14,2                                                   000078
         LGFR  15,14                                                     000078
         LARL  14,@@CONST@AREA@@                                         000000
         LGF   14,0(15,14)                                               000078
         B     0(3,14)                                                   000078
@12L68   DS    0H                                                        000078
         BRU   @12L73                                                    000078
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000079
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,PAG  000080
@12L69   DS    0H                                                        000080
         LA    2,@8origin                                                000080
         DROP  14                                                        000080
         LA    4,@7segments                                              000080
         LA    5,@12wgetstor                                             000080
         LG    14,464(0,13)            #SR_PARM_12                       000080
         USING @@PARMD@12,14                                             000080
         LG    6,@5ttkn                                                  000080
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,PAGEFRX 000080
               AMESIZE=1MEG,SEGMENTS=(4),ORIGIN=(2),TTOKEN=(6),RETCODE=X 000080
               200(13),MF=(E,(5))                                        000080
*     "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\    000081
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000082
*   break;                                                               000083
         BRU   @12L20                                                    000083
*  case ZOS64_VMEM_2_TO_32G:                                             000084
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO32G=YES,"\   000085
@12L70   DS    0H                                                        000085
         LA    2,@8origin                                                000085
         LA    4,@7segments                                              000085
         LA    5,@12wgetstor                                             000085
         LG    14,464(0,13)            #SR_PARM_12                       000085
         LG    6,@5ttkn                                                  000085
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO32G=YES,CONTROX 000085
               L=UNAUTH,PAGEFRAMESIZE=1MEG,SEGMENTS=(4),ORIGIN=(2),TTOKX 000085
               EN=(6),RETCODE=200(13),MF=(E,(5))                         000085
*     "CONTROL=UNAUTH,PAGEFRAMESIZE=1MEG,"\                              000086
*     "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\    000087
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000088
*   break;                                                               000089
         BRU   @12L20                                                    000089
*  case ZOS64_VMEM_2_TO_64G:                                             000090
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO64G=YES,"\   000091
@12L71   DS    0H                                                        000091
         LA    2,@8origin                                                000091
         LA    4,@7segments                                              000091
         LA    5,@12wgetstor                                             000091
         LG    14,464(0,13)            #SR_PARM_12                       000091
         LG    6,@5ttkn                                                  000091
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO64G=YES,CONTROX 000091
               L=UNAUTH,PAGEFRAMESIZE=1MEG,SEGMENTS=(4),ORIGIN=(2),TTOKX 000091
               EN=(6),RETCODE=200(13),MF=(E,(5))                         000091
*     "CONTROL=UNAUTH,PAGEFRAMESIZE=1MEG,"\                              000092
*     "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\    000093
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000094
*   break;                                                               000095
@12L20   DS    0H                                                        000078
@12L73   DS    0H                                                        000000
*  }                                                                     000096
*                                                                        000097
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000098
         LGF   14,@11iarv64_rc                                           000098
         LTR   14,14                                                     000098
         BRE   @12L19                                                    000098
*   return (void *)0;                                                    000099
         LGHI  15,0                                                      000099
         BRU   @12L21                                                    000099
@12L19   DS    0H                                                        000099
*  }                                                                     000100
*  return (void *)origin;                                                000101
         LG    15,@8origin                                               000101
* }                                                                      000102
@12L21   DS    0H                                                        000102
         DROP                                                            000102
         MYEPILOG                                                        000102
OMRIARV64 CSECT ,                                                        000102
         DS    0FD                                                       000102
@@LIT@12 LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@12 DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111111000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@12)     Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(26)                 Function Name                     000000
         DC    C'omrallocate_1M_fixed_pages'                             000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@12 DSECT                                                          000000
         DS    60FD                                                      000000
         ORG   @@AUTO@12                                                 000000
#GPR_SA_12 DS  18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@12+176                                             000000
@7segments DS  FD                                                        000000
         ORG   @@AUTO@12+184                                             000000
@8origin DS    FD                                                        000000
         ORG   @@AUTO@12+192                                             000000
@9useMemoryType DS FD                                                    000000
         ORG   @@AUTO@12+200                                             000000
@11iarv64_rc DS F                                                        000000
         ORG   @@AUTO@12+208                                             000000
@12wgetstor DS XL256                                                     000000
         ORG   @@AUTO@12+464                                             000000
#SR_PARM_12 DS XL8                                                       000000
@@PARMD@12 DSECT                                                         000000
         DS    XL24                                                      000000
         ORG   @@PARMD@12+0                                              000000
         DS    0FD                                                       000000
@3numMBSegments DS 0XL8                                                  000000
         ORG   @@PARMD@12+8                                              000000
         DS    0FD                                                       000000
@4userExtendedPrivateAreaMemoryType DS 0XL8                              000000
         ORG   @@PARMD@12+16                                             000000
         DS    0FD                                                       000000
@5ttkn   DS    0XL8                                                      000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* void * omrallocate_1M_pageable_pages_above_bar(int *numMBSegments, in  000118
         ENTRY @@CCN@14                                                  000118
@@CCN@14 AMODE 64                                                        000118
         DC    0FD                                                       000118
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000118
         DC    A(@@FPB@11-*+8)         Signed offset to FPB              000118
         DC    XL4'00000000'           Reserved                          000118
@@CCN@14 DS    0FD                                                       000118
&CCN_PRCN SETC '@@CCN@14'                                                000118
&CCN_PRCN_LONG SETC 'omrallocate_1M_pageable_pages_above_bar'            000118
&CCN_LITN SETC '@@LIT@11'                                                000118
&CCN_BEGIN SETC '@@BGN@11'                                               000118
&CCN_ASCM SETC 'P'                                                       000118
&CCN_DSASZ SETA 480                                                      000118
&CCN_SASZ SETA 144                                                       000118
&CCN_ARGS SETA 3                                                         000118
&CCN_RLOW SETA 14                                                        000118
&CCN_RHIGH SETA 6                                                        000118
&CCN_NAB SETB  0                                                         000118
&CCN_MAIN SETB 0                                                         000118
&CCN_NAB_STORED SETB 0                                                   000118
&CCN_STATIC SETB 0                                                       000118
&CCN_ALTGPR(1) SETB 1                                                    000118
&CCN_ALTGPR(2) SETB 1                                                    000118
&CCN_ALTGPR(3) SETB 1                                                    000118
&CCN_ALTGPR(4) SETB 1                                                    000118
&CCN_ALTGPR(5) SETB 1                                                    000118
&CCN_ALTGPR(6) SETB 1                                                    000118
&CCN_ALTGPR(7) SETB 1                                                    000118
&CCN_ALTGPR(8) SETB 0                                                    000118
&CCN_ALTGPR(9) SETB 0                                                    000118
&CCN_ALTGPR(10) SETB 0                                                   000118
&CCN_ALTGPR(11) SETB 0                                                   000118
&CCN_ALTGPR(12) SETB 0                                                   000118
&CCN_ALTGPR(13) SETB 0                                                   000118
&CCN_ALTGPR(14) SETB 1                                                   000118
&CCN_ALTGPR(15) SETB 1                                                   000118
&CCN_ALTGPR(16) SETB 1                                                   000118
         MYPROLOG                                                        000118
@@BGN@11 DS    0H                                                        000118
         AIF   (NOT &CCN_SASIG).@@NOSIG11                                000118
         LLILH 6,X'C6F4'                                                 000118
         OILL  6,X'E2C1'                                                 000118
         ST    6,4(,13)                                                  000118
.@@NOSIG11 ANOP                                                          000118
         USING @@AUTO@11,13                                              000118
         LARL  3,@@LIT@11                                                000118
         USING @@LIT@11,3                                                000118
         STG   1,464(0,13)             #SR_PARM_11                       000118
*  long segments = 0;                                                    000119
         MVGHI @19segments@5,0                                           000119
*  long origin = 0;                                                      000120
         MVGHI @20origin@6,0                                             000120
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000121
         LG    14,464(0,13)            #SR_PARM_11                       000121
         USING @@PARMD@11,14                                             000121
         LG    14,@16userExtendedPrivateAreaMemoryType@2                 000121
         LGF   14,0(0,14)              (*)int                            000121
         STG   14,@21useMemoryType@7                                     000121
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000122
         MVHI  @22iarv64_rc@8,8                                          000122
*                                                                        000123
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,NGETSTOR)":"DS"(wgetstor));         000124
*                                                                        000125
*  segments = *numMBSegments;                                            000126
         LG    14,464(0,13)            #SR_PARM_11                       000126
         LG    14,@15numMBSegments@1                                     000126
         LGF   14,0(0,14)              (*)int                            000126
         STG   14,@19segments@5                                          000126
*  wgetstor = ngetstor;                                                  000127
         LARL  14,$STATIC                                                000127
         DROP  14                                                        000127
         USING @@STATICD@@,14                                            000127
         MVC   @23wgetstor,@13ngetstor                                   000127
*                                                                        000128
*  switch (useMemoryType) {                                              000129
         LG    14,@21useMemoryType@7                                     000129
         STG   14,472(0,13)            #SW_WORK11                        000129
         CLG   14,=X'0000000000000002'                                   000129
         BRH   @11L62                                                    000129
         LG    14,472(0,13)            #SW_WORK11                        000129
         SLLG  14,14,2                                                   000129
         LGFR  15,14                                                     000129
         LARL  14,@@CONST@AREA@@                                         000000
         LGF   14,12(15,14)                                              000129
         B     0(3,14)                                                   000129
@11L62   DS    0H                                                        000129
         BRU   @11L67                                                    000129
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000130
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,"\   000131
@11L63   DS    0H                                                        000131
         LA    2,@20origin@6                                             000131
         DROP  14                                                        000131
         LA    4,@19segments@5                                           000131
         LA    5,@23wgetstor                                             000131
         LG    14,464(0,13)            #SR_PARM_11                       000131
         USING @@PARMD@11,14                                             000131
         LG    6,@17ttkn@3                                               000131
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,PAGEFRX 000131
               AMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(4),ORIGIN=(X 000131
               2),TTOKEN=(6),RETCODE=200(13),MF=(E,(5))                  000131
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000132
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000133
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000134
*   break;                                                               000135
         BRU   @11L22                                                    000135
*  case ZOS64_VMEM_2_TO_32G:                                             000136
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000137
@11L64   DS    0H                                                        000137
         LA    2,@20origin@6                                             000137
         LA    4,@19segments@5                                           000137
         LA    5,@23wgetstor                                             000137
         LG    14,464(0,13)            #SR_PARM_11                       000137
         LG    6,@17ttkn@3                                               000137
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTX 000137
               O32G=YES,PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENX 000137
               TS=(4),ORIGIN=(2),TTOKEN=(6),RETCODE=200(13),MF=(E,(5))   000137
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000138
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000139
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000140
*   break;                                                               000141
         BRU   @11L22                                                    000141
*  case ZOS64_VMEM_2_TO_64G:                                             000142
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000143
@11L65   DS    0H                                                        000143
         LA    2,@20origin@6                                             000143
         LA    4,@19segments@5                                           000143
         LA    5,@23wgetstor                                             000143
         LG    14,464(0,13)            #SR_PARM_11                       000143
         LG    6,@17ttkn@3                                               000143
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTX 000143
               O64G=YES,PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENX 000143
               TS=(4),ORIGIN=(2),TTOKEN=(6),RETCODE=200(13),MF=(E,(5))   000143
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000144
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000145
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000146
*   break;                                                               000147
@11L22   DS    0H                                                        000129
@11L67   DS    0H                                                        000000
*  }                                                                     000148
*                                                                        000149
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000150
         LGF   14,@22iarv64_rc@8                                         000150
         LTR   14,14                                                     000150
         BRE   @11L17                                                    000150
*   return (void *)0;                                                    000151
         LGHI  15,0                                                      000151
         BRU   @11L23                                                    000151
@11L17   DS    0H                                                        000151
*  }                                                                     000152
*  return (void *)origin;                                                000153
         LG    15,@20origin@6                                            000153
* }                                                                      000154
@11L23   DS    0H                                                        000154
         DROP                                                            000154
         MYEPILOG                                                        000154
OMRIARV64 CSECT ,                                                        000154
         DS    0FD                                                       000154
@@LIT@11 LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@11 DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111111000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@11)     Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(39)                 Function Name                     000000
         DC    C'omrallocate_1M_pageable_pages_above_bar'                000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@11 DSECT                                                          000000
         DS    60FD                                                      000000
         ORG   @@AUTO@11                                                 000000
#GPR_SA_11 DS  18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@11+176                                             000000
@19segments@5 DS FD                                                      000000
         ORG   @@AUTO@11+184                                             000000
@20origin@6 DS FD                                                        000000
         ORG   @@AUTO@11+192                                             000000
@21useMemoryType@7 DS FD                                                 000000
         ORG   @@AUTO@11+200                                             000000
@22iarv64_rc@8 DS F                                                      000000
         ORG   @@AUTO@11+208                                             000000
@23wgetstor DS XL256                                                     000000
         ORG   @@AUTO@11+464                                             000000
#SR_PARM_11 DS XL8                                                       000000
@@PARMD@11 DSECT                                                         000000
         DS    XL24                                                      000000
         ORG   @@PARMD@11+0                                              000000
         DS    0FD                                                       000000
@15numMBSegments@1 DS 0XL8                                               000000
         ORG   @@PARMD@11+8                                              000000
         DS    0FD                                                       000000
@16userExtendedPrivateAreaMemoryType@2 DS 0XL8                           000000
         ORG   @@PARMD@11+16                                             000000
         DS    0FD                                                       000000
@17ttkn@3 DS   0XL8                                                      000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* void *omrallocate_1M_pageable_pages_guarded_above_bar(int *numMBSegme  000173
         ENTRY @@CCN@25                                                  000173
@@CCN@25 AMODE 64                                                        000173
         DC    0FD                                                       000173
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000173
         DC    A(@@FPB@10-*+8)         Signed offset to FPB              000173
         DC    XL4'00000000'           Reserved                          000173
@@CCN@25 DS    0FD                                                       000173
&CCN_PRCN SETC '@@CCN@25'                                                000173
&CCN_PRCN_LONG SETC 'omrallocate_1M_pageable_pages_guarded_above_bar'    000173
&CCN_LITN SETC '@@LIT@10'                                                000173
&CCN_BEGIN SETC '@@BGN@10'                                               000173
&CCN_ASCM SETC 'P'                                                       000173
&CCN_DSASZ SETA 480                                                      000173
&CCN_SASZ SETA 144                                                       000173
&CCN_ARGS SETA 3                                                         000173
&CCN_RLOW SETA 14                                                        000173
&CCN_RHIGH SETA 6                                                        000173
&CCN_NAB SETB  0                                                         000173
&CCN_MAIN SETB 0                                                         000173
&CCN_NAB_STORED SETB 0                                                   000173
&CCN_STATIC SETB 0                                                       000173
&CCN_ALTGPR(1) SETB 1                                                    000173
&CCN_ALTGPR(2) SETB 1                                                    000173
&CCN_ALTGPR(3) SETB 1                                                    000173
&CCN_ALTGPR(4) SETB 1                                                    000173
&CCN_ALTGPR(5) SETB 1                                                    000173
&CCN_ALTGPR(6) SETB 1                                                    000173
&CCN_ALTGPR(7) SETB 1                                                    000173
&CCN_ALTGPR(8) SETB 0                                                    000173
&CCN_ALTGPR(9) SETB 0                                                    000173
&CCN_ALTGPR(10) SETB 0                                                   000173
&CCN_ALTGPR(11) SETB 0                                                   000173
&CCN_ALTGPR(12) SETB 0                                                   000173
&CCN_ALTGPR(13) SETB 0                                                   000173
&CCN_ALTGPR(14) SETB 1                                                   000173
&CCN_ALTGPR(15) SETB 1                                                   000173
&CCN_ALTGPR(16) SETB 1                                                   000173
         MYPROLOG                                                        000173
@@BGN@10 DS    0H                                                        000173
         AIF   (NOT &CCN_SASIG).@@NOSIG10                                000173
         LLILH 6,X'C6F4'                                                 000173
         OILL  6,X'E2C1'                                                 000173
         ST    6,4(,13)                                                  000173
.@@NOSIG10 ANOP                                                          000173
         USING @@AUTO@10,13                                              000173
         LARL  3,@@LIT@10                                                000173
         USING @@LIT@10,3                                                000173
         STG   1,464(0,13)             #SR_PARM_10                       000173
*  long segments = 0;                                                    000174
         MVGHI @30segments@13,0                                          000174
*  long origin = 0;                                                      000175
         MVGHI @31origin@14,0                                            000175
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000176
         LG    14,464(0,13)            #SR_PARM_10                       000176
         USING @@PARMD@10,14                                             000176
         LG    14,@27userExtendedPrivateAreaMemoryType@10                000176
         LGF   14,0(0,14)              (*)int                            000176
         STG   14,@32useMemoryType@15                                    000176
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000177
         MVHI  @33iarv64_rc@16,8                                         000177
*                                                                        000178
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,SGETSTOR)":"DS"(wgetstor));         000179
*                                                                        000180
*  segments = *numMBSegments;                                            000181
         LG    14,464(0,13)            #SR_PARM_10                       000181
         LG    14,@26numMBSegments@9                                     000181
         LGF   14,0(0,14)              (*)int                            000181
         STG   14,@30segments@13                                         000181
*  wgetstor = sgetstor;                                                  000182
         LARL  14,$STATIC                                                000182
         DROP  14                                                        000182
         USING @@STATICD@@,14                                            000182
         MVC   @34wgetstor,@24sgetstor                                   000182
*                                                                        000183
*  switch (useMemoryType) {                                              000184
         LG    14,@32useMemoryType@15                                    000184
         STG   14,472(0,13)            #SW_WORK10                        000184
         CLG   14,=X'0000000000000002'                                   000184
         BRH   @10L56                                                    000184
         LG    14,472(0,13)            #SW_WORK10                        000184
         SLLG  14,14,2                                                   000184
         LGFR  15,14                                                     000184
         LARL  14,@@CONST@AREA@@                                         000000
         LGF   14,24(15,14)                                              000184
         B     0(3,14)                                                   000184
@10L56   DS    0H                                                        000184
         BRU   @10L61                                                    000184
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000185
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,"\   000186
@10L57   DS    0H                                                        000186
         LA    2,@31origin@14                                            000186
         DROP  14                                                        000186
         LA    4,@30segments@13                                          000186
         LA    5,@34wgetstor                                             000186
         LG    14,464(0,13)            #SR_PARM_10                       000186
         USING @@PARMD@10,14                                             000186
         LG    6,@28ttkn@11                                              000186
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,GUARDSX 000186
               IZE64=(4),PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMEX 000186
               NTS=(4),ORIGIN=(2),TTOKEN=(6),RETCODE=200(13),MF=(E,(5))  000186
*     "GUARDSIZE64=(%2),"\                                               000187
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000188
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000189
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000190
*   break;                                                               000191
         BRU   @10L24                                                    000191
*  case ZOS64_VMEM_2_TO_32G:                                             000192
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000193
@10L58   DS    0H                                                        000193
         LA    2,@31origin@14                                            000193
         LA    4,@30segments@13                                          000193
         LA    5,@34wgetstor                                             000193
         LG    14,464(0,13)            #SR_PARM_10                       000193
         LG    6,@28ttkn@11                                              000193
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTX 000193
               O32G=YES,GUARDSIZE64=(4),PAGEFRAMESIZE=PAGEABLE1MEG,TYPEX 000193
               =PAGEABLE,SEGMENTS=(4),ORIGIN=(2),TTOKEN=(6),RETCODE=200X 000193
               (13),MF=(E,(5))                                           000193
*     "GUARDSIZE64=(%2),"\                                               000194
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000195
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000196
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000197
*   break;                                                               000198
         BRU   @10L24                                                    000198
*  case ZOS64_VMEM_2_TO_64G:                                             000199
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000200
@10L59   DS    0H                                                        000200
         LA    2,@31origin@14                                            000200
         LA    4,@30segments@13                                          000200
         LA    5,@34wgetstor                                             000200
         LG    14,464(0,13)            #SR_PARM_10                       000200
         LG    6,@28ttkn@11                                              000200
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTX 000200
               O64G=YES,GUARDSIZE64=(4),PAGEFRAMESIZE=PAGEABLE1MEG,TYPEX 000200
               =PAGEABLE,SEGMENTS=(4),ORIGIN=(2),TTOKEN=(6),RETCODE=200X 000200
               (13),MF=(E,(5))                                           000200
*     "GUARDSIZE64=(%2),"\                                               000201
*     "PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\         000202
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000203
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000204
*   break;                                                               000205
@10L24   DS    0H                                                        000184
@10L61   DS    0H                                                        000000
*  }                                                                     000206
*                                                                        000207
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000208
         LGF   14,@33iarv64_rc@16                                        000208
         LTR   14,14                                                     000208
         BRE   @10L15                                                    000208
*   return (void *)0;                                                    000209
         LGHI  15,0                                                      000209
         BRU   @10L25                                                    000209
@10L15   DS    0H                                                        000209
*  }                                                                     000210
*  return (void *)origin;                                                000211
         LG    15,@31origin@14                                           000211
* }                                                                      000212
@10L25   DS    0H                                                        000212
         DROP                                                            000212
         MYEPILOG                                                        000212
OMRIARV64 CSECT ,                                                        000212
         DS    0FD                                                       000212
@@LIT@10 LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@10 DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111111000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@10)     Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(47)                 Function Name                     000000
         DC    C'omrallocate_1M_pageable_pages_guarded_above_bar'        000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@10 DSECT                                                          000000
         DS    60FD                                                      000000
         ORG   @@AUTO@10                                                 000000
#GPR_SA_10 DS  18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@10+176                                             000000
@30segments@13 DS FD                                                     000000
         ORG   @@AUTO@10+184                                             000000
@31origin@14 DS FD                                                       000000
         ORG   @@AUTO@10+192                                             000000
@32useMemoryType@15 DS FD                                                000000
         ORG   @@AUTO@10+200                                             000000
@33iarv64_rc@16 DS F                                                     000000
         ORG   @@AUTO@10+208                                             000000
@34wgetstor DS XL256                                                     000000
         ORG   @@AUTO@10+464                                             000000
#SR_PARM_10 DS XL8                                                       000000
@@PARMD@10 DSECT                                                         000000
         DS    XL24                                                      000000
         ORG   @@PARMD@10+0                                              000000
         DS    0FD                                                       000000
@26numMBSegments@9 DS 0XL8                                               000000
         ORG   @@PARMD@10+8                                              000000
         DS    0FD                                                       000000
@27userExtendedPrivateAreaMemoryType@10 DS 0XL8                          000000
         ORG   @@PARMD@10+16                                             000000
         DS    0FD                                                       000000
@28ttkn@11 DS  0XL8                                                      000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* void * omrallocate_2G_pages(int *num2GBUnits, int *userExtendedPrivat  000228
         ENTRY @@CCN@36                                                  000228
@@CCN@36 AMODE 64                                                        000228
         DC    0FD                                                       000228
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000228
         DC    A(@@FPB@9-*+8)          Signed offset to FPB              000228
         DC    XL4'00000000'           Reserved                          000228
@@CCN@36 DS    0FD                                                       000228
&CCN_PRCN SETC '@@CCN@36'                                                000228
&CCN_PRCN_LONG SETC 'omrallocate_2G_pages'                               000228
&CCN_LITN SETC '@@LIT@9'                                                 000228
&CCN_BEGIN SETC '@@BGN@9'                                                000228
&CCN_ASCM SETC 'P'                                                       000228
&CCN_DSASZ SETA 480                                                      000228
&CCN_SASZ SETA 144                                                       000228
&CCN_ARGS SETA 3                                                         000228
&CCN_RLOW SETA 14                                                        000228
&CCN_RHIGH SETA 6                                                        000228
&CCN_NAB SETB  0                                                         000228
&CCN_MAIN SETB 0                                                         000228
&CCN_NAB_STORED SETB 0                                                   000228
&CCN_STATIC SETB 0                                                       000228
&CCN_ALTGPR(1) SETB 1                                                    000228
&CCN_ALTGPR(2) SETB 1                                                    000228
&CCN_ALTGPR(3) SETB 1                                                    000228
&CCN_ALTGPR(4) SETB 1                                                    000228
&CCN_ALTGPR(5) SETB 1                                                    000228
&CCN_ALTGPR(6) SETB 1                                                    000228
&CCN_ALTGPR(7) SETB 1                                                    000228
&CCN_ALTGPR(8) SETB 0                                                    000228
&CCN_ALTGPR(9) SETB 0                                                    000228
&CCN_ALTGPR(10) SETB 0                                                   000228
&CCN_ALTGPR(11) SETB 0                                                   000228
&CCN_ALTGPR(12) SETB 0                                                   000228
&CCN_ALTGPR(13) SETB 0                                                   000228
&CCN_ALTGPR(14) SETB 1                                                   000228
&CCN_ALTGPR(15) SETB 1                                                   000228
&CCN_ALTGPR(16) SETB 1                                                   000228
         MYPROLOG                                                        000228
@@BGN@9  DS    0H                                                        000228
         AIF   (NOT &CCN_SASIG).@@NOSIG9                                 000228
         LLILH 6,X'C6F4'                                                 000228
         OILL  6,X'E2C1'                                                 000228
         ST    6,4(,13)                                                  000228
.@@NOSIG9 ANOP                                                           000228
         USING @@AUTO@9,13                                               000228
         LARL  3,@@LIT@9                                                 000228
         USING @@LIT@9,3                                                 000228
         STG   1,464(0,13)             #SR_PARM_9                        000228
*  long units = 0;                                                       000229
         MVGHI @41units,0                                                000229
*  long origin = 0;                                                      000230
         MVGHI @42origin@20,0                                            000230
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000231
         LG    14,464(0,13)            #SR_PARM_9                        000231
         USING @@PARMD@9,14                                              000231
         LG    14,@38userExtendedPrivateAreaMemoryType@17                000231
         LGF   14,0(0,14)              (*)int                            000231
         STG   14,@43useMemoryType@21                                    000231
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000232
         MVHI  @44iarv64_rc@22,8                                         000232
*                                                                        000233
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,OGETSTOR)":"DS"(wgetstor));         000234
*                                                                        000235
*  units = *num2GBUnits;                                                 000236
         LG    14,464(0,13)            #SR_PARM_9                        000236
         LG    14,@37num2GBUnits                                         000236
         LGF   14,0(0,14)              (*)int                            000236
         STG   14,@41units                                               000236
*  wgetstor = ogetstor;                                                  000237
         LARL  14,$STATIC                                                000237
         DROP  14                                                        000237
         USING @@STATICD@@,14                                            000237
         MVC   @45wgetstor,@35ogetstor                                   000237
*                                                                        000238
*  switch (useMemoryType) {                                              000239
         LG    14,@43useMemoryType@21                                    000239
         STG   14,472(0,13)            #SW_WORK9                         000239
         CLG   14,=X'0000000000000002'                                   000239
         BRH   @9L50                                                     000239
         LG    14,472(0,13)            #SW_WORK9                         000239
         SLLG  14,14,2                                                   000239
         LGFR  15,14                                                     000239
         LARL  14,@@CONST@AREA@@                                         000000
         LGF   14,36(15,14)                                              000239
         B     0(3,14)                                                   000239
@9L50    DS    0H                                                        000239
         BRU   @9L55                                                     000239
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000240
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,"\   000241
@9L51    DS    0H                                                        000241
         LA    2,@42origin@20                                            000241
         DROP  14                                                        000241
         LA    4,@41units                                                000241
         LA    5,@45wgetstor                                             000241
         LG    14,464(0,13)            #SR_PARM_9                        000241
         USING @@PARMD@9,14                                              000241
         LG    6,@39ttkn@18                                              000241
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,PAGEFRX 000241
               AMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(4),ORIGIN=(2),TX 000241
               TOKEN=(6),RETCODE=200(13),MF=(E,(5))                      000241
*     "PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(%2),"\             000242
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000243
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&units),"r"(&wgetstor),"r"(ttkn  000244
*   break;                                                               000245
         BRU   @9L26                                                     000245
*  case ZOS64_VMEM_2_TO_32G:                                             000246
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000247
@9L52    DS    0H                                                        000247
         LA    2,@42origin@20                                            000247
         LA    4,@41units                                                000247
         LA    5,@45wgetstor                                             000247
         LG    14,464(0,13)            #SR_PARM_9                        000247
         LG    6,@39ttkn@18                                              000247
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTX 000247
               O32G=YES,PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(X 000247
               4),ORIGIN=(2),TTOKEN=(6),RETCODE=200(13),MF=(E,(5))       000247
*     "PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(%2),"\             000248
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000249
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&units),"r"(&wgetstor),"r"(ttkn  000250
*   break;                                                               000251
         BRU   @9L26                                                     000251
*  case ZOS64_VMEM_2_TO_64G:                                             000252
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000253
@9L53    DS    0H                                                        000253
         LA    2,@42origin@20                                            000253
         LA    4,@41units                                                000253
         LA    5,@45wgetstor                                             000253
         LG    14,464(0,13)            #SR_PARM_9                        000253
         LG    6,@39ttkn@18                                              000253
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTX 000253
               O64G=YES,PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(X 000253
               4),ORIGIN=(2),TTOKEN=(6),RETCODE=200(13),MF=(E,(5))       000253
*     "PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(%2),"\             000254
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000255
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&units),"r"(&wgetstor),"r"(ttkn  000256
*   break;                                                               000257
@9L26    DS    0H                                                        000239
@9L55    DS    0H                                                        000000
*  }                                                                     000258
*                                                                        000259
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000260
         LGF   14,@44iarv64_rc@22                                        000260
         LTR   14,14                                                     000260
         BRE   @9L13                                                     000260
*   return (void *)0;                                                    000261
         LGHI  15,0                                                      000261
         BRU   @9L27                                                     000261
@9L13    DS    0H                                                        000261
*  }                                                                     000262
*  return (void *)origin;                                                000263
         LG    15,@42origin@20                                           000263
* }                                                                      000264
@9L27    DS    0H                                                        000264
         DROP                                                            000264
         MYEPILOG                                                        000264
OMRIARV64 CSECT ,                                                        000264
         DS    0FD                                                       000264
@@LIT@9  LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@9  DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111111000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@9)      Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(20)                 Function Name                     000000
         DC    C'omrallocate_2G_pages'                                   000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@9 DSECT                                                           000000
         DS    60FD                                                      000000
         ORG   @@AUTO@9                                                  000000
#GPR_SA_9 DS   18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@9+176                                              000000
@41units DS    FD                                                        000000
         ORG   @@AUTO@9+184                                              000000
@42origin@20 DS FD                                                       000000
         ORG   @@AUTO@9+192                                              000000
@43useMemoryType@21 DS FD                                                000000
         ORG   @@AUTO@9+200                                              000000
@44iarv64_rc@22 DS F                                                     000000
         ORG   @@AUTO@9+208                                              000000
@45wgetstor DS XL256                                                     000000
         ORG   @@AUTO@9+464                                              000000
#SR_PARM_9 DS  XL8                                                       000000
@@PARMD@9 DSECT                                                          000000
         DS    XL24                                                      000000
         ORG   @@PARMD@9+0                                               000000
         DS    0FD                                                       000000
@37num2GBUnits DS 0XL8                                                   000000
         ORG   @@PARMD@9+8                                               000000
         DS    0FD                                                       000000
@38userExtendedPrivateAreaMemoryType@17 DS 0XL8                          000000
         ORG   @@PARMD@9+16                                              000000
         DS    0FD                                                       000000
@39ttkn@18 DS  0XL8                                                      000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* void * omrallocate_4K_pages_in_userExtendedPrivateArea(int *numMBSegm  000280
         ENTRY @@CCN@47                                                  000280
@@CCN@47 AMODE 64                                                        000280
         DC    0FD                                                       000280
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000280
         DC    A(@@FPB@8-*+8)          Signed offset to FPB              000280
         DC    XL4'00000000'           Reserved                          000280
@@CCN@47 DS    0FD                                                       000280
&CCN_PRCN SETC '@@CCN@47'                                                000280
&CCN_PRCN_LONG SETC 'omrallocate_4K_pages_in_userExtendedPrivateArea'    000280
&CCN_LITN SETC '@@LIT@8'                                                 000280
&CCN_BEGIN SETC '@@BGN@8'                                                000280
&CCN_ASCM SETC 'P'                                                       000280
&CCN_DSASZ SETA 480                                                      000280
&CCN_SASZ SETA 144                                                       000280
&CCN_ARGS SETA 3                                                         000280
&CCN_RLOW SETA 14                                                        000280
&CCN_RHIGH SETA 6                                                        000280
&CCN_NAB SETB  0                                                         000280
&CCN_MAIN SETB 0                                                         000280
&CCN_NAB_STORED SETB 0                                                   000280
&CCN_STATIC SETB 0                                                       000280
&CCN_ALTGPR(1) SETB 1                                                    000280
&CCN_ALTGPR(2) SETB 1                                                    000280
&CCN_ALTGPR(3) SETB 1                                                    000280
&CCN_ALTGPR(4) SETB 1                                                    000280
&CCN_ALTGPR(5) SETB 1                                                    000280
&CCN_ALTGPR(6) SETB 1                                                    000280
&CCN_ALTGPR(7) SETB 1                                                    000280
&CCN_ALTGPR(8) SETB 0                                                    000280
&CCN_ALTGPR(9) SETB 0                                                    000280
&CCN_ALTGPR(10) SETB 0                                                   000280
&CCN_ALTGPR(11) SETB 0                                                   000280
&CCN_ALTGPR(12) SETB 0                                                   000280
&CCN_ALTGPR(13) SETB 0                                                   000280
&CCN_ALTGPR(14) SETB 1                                                   000280
&CCN_ALTGPR(15) SETB 1                                                   000280
&CCN_ALTGPR(16) SETB 1                                                   000280
         MYPROLOG                                                        000280
@@BGN@8  DS    0H                                                        000280
         AIF   (NOT &CCN_SASIG).@@NOSIG8                                 000280
         LLILH 6,X'C6F4'                                                 000280
         OILL  6,X'E2C1'                                                 000280
         ST    6,4(,13)                                                  000280
.@@NOSIG8 ANOP                                                           000280
         USING @@AUTO@8,13                                               000280
         LARL  3,@@LIT@8                                                 000280
         USING @@LIT@8,3                                                 000280
         STG   1,464(0,13)             #SR_PARM_8                        000280
*  long segments = 0;                                                    000281
         MVGHI @52segments@27,0                                          000281
*  long origin = 0;                                                      000282
         MVGHI @53origin@28,0                                            000282
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000283
         LG    14,464(0,13)            #SR_PARM_8                        000283
         USING @@PARMD@8,14                                              000283
         LG    14,@49userExtendedPrivateAreaMemoryType@24                000283
         LGF   14,0(0,14)              (*)int                            000283
         STG   14,@54useMemoryType@29                                    000283
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000284
         MVHI  @55iarv64_rc@30,8                                         000284
*                                                                        000285
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,MGETSTOR)":"DS"(wgetstor));         000286
*                                                                        000287
*  segments = *numMBSegments;                                            000288
         LG    14,464(0,13)            #SR_PARM_8                        000288
         LG    14,@48numMBSegments@23                                    000288
         LGF   14,0(0,14)              (*)int                            000288
         STG   14,@52segments@27                                         000288
*  wgetstor = mgetstor;                                                  000289
         LARL  14,$STATIC                                                000289
         DROP  14                                                        000289
         USING @@STATICD@@,14                                            000289
         MVC   @56wgetstor,@46mgetstor                                   000289
*                                                                        000290
*  switch (useMemoryType) {                                              000291
         LG    14,@54useMemoryType@29                                    000291
         STG   14,472(0,13)            #SW_WORK8                         000291
         CLG   14,=X'0000000000000002'                                   000291
         BRH   @8L44                                                     000291
         LG    14,472(0,13)            #SW_WORK8                         000291
         SLLG  14,14,2                                                   000291
         LGFR  15,14                                                     000291
         LARL  14,@@CONST@AREA@@                                         000000
         LGF   14,48(15,14)                                              000291
         B     0(3,14)                                                   000291
@8L44    DS    0H                                                        000291
         BRU   @8L49                                                     000291
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000292
*   break;                                                               000293
@8L45    DS    0H                                                        000293
         BRU   @8L28                                                     000293
*  case ZOS64_VMEM_2_TO_32G:                                             000294
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO32G=YES,"\   000295
@8L46    DS    0H                                                        000295
         LA    2,@53origin@28                                            000295
         DROP  14                                                        000295
         LA    4,@52segments@27                                          000295
         LA    5,@56wgetstor                                             000295
         LG    14,464(0,13)            #SR_PARM_8                        000295
         USING @@PARMD@8,14                                              000295
         LG    6,@50ttkn@25                                              000295
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO32G=YES,CONTROX 000295
               L=UNAUTH,PAGEFRAMESIZE=4K,SEGMENTS=(4),ORIGIN=(2),TTOKENX 000295
               =(6),RETCODE=200(13),MF=(E,(5))                           000295
*     "CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\                                000296
*     "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\    000297
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000298
*   break;                                                               000299
         BRU   @8L28                                                     000299
*  case ZOS64_VMEM_2_TO_64G:                                             000300
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO64G=YES,"\   000301
@8L47    DS    0H                                                        000301
         LA    2,@53origin@28                                            000301
         LA    4,@52segments@27                                          000301
         LA    5,@56wgetstor                                             000301
         LG    14,464(0,13)            #SR_PARM_8                        000301
         LG    6,@50ttkn@25                                              000301
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO64G=YES,CONTROX 000301
               L=UNAUTH,PAGEFRAMESIZE=4K,SEGMENTS=(4),ORIGIN=(2),TTOKENX 000301
               =(6),RETCODE=200(13),MF=(E,(5))                           000301
*     "CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\                                000302
*     "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\    000303
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000304
*   break;                                                               000305
@8L28    DS    0H                                                        000291
@8L49    DS    0H                                                        000000
*  }                                                                     000306
*                                                                        000307
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000308
         LGF   14,@55iarv64_rc@30                                        000308
         LTR   14,14                                                     000308
         BRE   @8L11                                                     000308
*   return (void *)0;                                                    000309
         LGHI  15,0                                                      000309
         BRU   @8L29                                                     000309
@8L11    DS    0H                                                        000309
*  }                                                                     000310
*  return (void *)origin;                                                000311
         LG    15,@53origin@28                                           000311
* }                                                                      000312
@8L29    DS    0H                                                        000312
         DROP                                                            000312
         MYEPILOG                                                        000312
OMRIARV64 CSECT ,                                                        000312
         DS    0FD                                                       000312
@@LIT@8  LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@8  DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111111000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@8)      Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(47)                 Function Name                     000000
         DC    C'omrallocate_4K_pages_in_userExtendedPrivateArea'        000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@8 DSECT                                                           000000
         DS    60FD                                                      000000
         ORG   @@AUTO@8                                                  000000
#GPR_SA_8 DS   18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@8+176                                              000000
@52segments@27 DS FD                                                     000000
         ORG   @@AUTO@8+184                                              000000
@53origin@28 DS FD                                                       000000
         ORG   @@AUTO@8+192                                              000000
@54useMemoryType@29 DS FD                                                000000
         ORG   @@AUTO@8+200                                              000000
@55iarv64_rc@30 DS F                                                     000000
         ORG   @@AUTO@8+208                                              000000
@56wgetstor DS XL256                                                     000000
         ORG   @@AUTO@8+464                                              000000
#SR_PARM_8 DS  XL8                                                       000000
@@PARMD@8 DSECT                                                          000000
         DS    XL24                                                      000000
         ORG   @@PARMD@8+0                                               000000
         DS    0FD                                                       000000
@48numMBSegments@23 DS 0XL8                                              000000
         ORG   @@PARMD@8+8                                               000000
         DS    0FD                                                       000000
@49userExtendedPrivateAreaMemoryType@24 DS 0XL8                          000000
         ORG   @@PARMD@8+16                                              000000
         DS    0FD                                                       000000
@50ttkn@25 DS  0XL8                                                      000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* void *omrallocate_4K_pages_guarded_in_userExtendedPrivateArea(int *nu  000328
         ENTRY @@CCN@58                                                  000328
@@CCN@58 AMODE 64                                                        000328
         DC    0FD                                                       000328
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000328
         DC    A(@@FPB@7-*+8)          Signed offset to FPB              000328
         DC    XL4'00000000'           Reserved                          000328
@@CCN@58 DS    0FD                                                       000328
&CCN_PRCN SETC '@@CCN@58'                                                000328
&CCN_PRCN_LONG SETC 'omrallocate_4K_pages_guarded_in_userExtendedPrivatX 000328
               eArea'                                                    000328
&CCN_LITN SETC '@@LIT@7'                                                 000328
&CCN_BEGIN SETC '@@BGN@7'                                                000328
&CCN_ASCM SETC 'P'                                                       000328
&CCN_DSASZ SETA 480                                                      000328
&CCN_SASZ SETA 144                                                       000328
&CCN_ARGS SETA 3                                                         000328
&CCN_RLOW SETA 14                                                        000328
&CCN_RHIGH SETA 6                                                        000328
&CCN_NAB SETB  0                                                         000328
&CCN_MAIN SETB 0                                                         000328
&CCN_NAB_STORED SETB 0                                                   000328
&CCN_STATIC SETB 0                                                       000328
&CCN_ALTGPR(1) SETB 1                                                    000328
&CCN_ALTGPR(2) SETB 1                                                    000328
&CCN_ALTGPR(3) SETB 1                                                    000328
&CCN_ALTGPR(4) SETB 1                                                    000328
&CCN_ALTGPR(5) SETB 1                                                    000328
&CCN_ALTGPR(6) SETB 1                                                    000328
&CCN_ALTGPR(7) SETB 1                                                    000328
&CCN_ALTGPR(8) SETB 0                                                    000328
&CCN_ALTGPR(9) SETB 0                                                    000328
&CCN_ALTGPR(10) SETB 0                                                   000328
&CCN_ALTGPR(11) SETB 0                                                   000328
&CCN_ALTGPR(12) SETB 0                                                   000328
&CCN_ALTGPR(13) SETB 0                                                   000328
&CCN_ALTGPR(14) SETB 1                                                   000328
&CCN_ALTGPR(15) SETB 1                                                   000328
&CCN_ALTGPR(16) SETB 1                                                   000328
         MYPROLOG                                                        000328
@@BGN@7  DS    0H                                                        000328
         AIF   (NOT &CCN_SASIG).@@NOSIG7                                 000328
         LLILH 6,X'C6F4'                                                 000328
         OILL  6,X'E2C1'                                                 000328
         ST    6,4(,13)                                                  000328
.@@NOSIG7 ANOP                                                           000328
         USING @@AUTO@7,13                                               000328
         LARL  3,@@LIT@7                                                 000328
         USING @@LIT@7,3                                                 000328
         STG   1,464(0,13)             #SR_PARM_7                        000328
*  long segments = 0;                                                    000329
         MVGHI @63segments@35,0                                          000329
*  long origin = 0;                                                      000330
         MVGHI @64origin@36,0                                            000330
*  long useMemoryType = *userExtendedPrivateAreaMemoryType;              000331
         LG    14,464(0,13)            #SR_PARM_7                        000331
         USING @@PARMD@7,14                                              000331
         LG    14,@60userExtendedPrivateAreaMemoryType@32                000331
         LGF   14,0(0,14)              (*)int                            000331
         STG   14,@65useMemoryType@37                                    000331
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000332
         MVHI  @66iarv64_rc@38,8                                         000332
*                                                                        000333
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,TGETSTOR)":"DS"(wgetstor));         000334
*                                                                        000335
*  segments = *numMBSegments;                                            000336
         LG    14,464(0,13)            #SR_PARM_7                        000336
         LG    14,@59numMBSegments@31                                    000336
         LGF   14,0(0,14)              (*)int                            000336
         STG   14,@63segments@35                                         000336
*  wgetstor = tgetstor;                                                  000337
         LARL  14,$STATIC                                                000337
         DROP  14                                                        000337
         USING @@STATICD@@,14                                            000337
         MVC   @67wgetstor,@57tgetstor                                   000337
*                                                                        000338
*  switch (useMemoryType) {                                              000339
         LG    14,@65useMemoryType@37                                    000339
         STG   14,472(0,13)            #SW_WORK7                         000339
         CLG   14,=X'0000000000000002'                                   000339
         BRH   @7L38                                                     000339
         LG    14,472(0,13)            #SW_WORK7                         000339
         SLLG  14,14,2                                                   000339
         LGFR  15,14                                                     000339
         LARL  14,@@CONST@AREA@@                                         000000
         LGF   14,60(15,14)                                              000339
         B     0(3,14)                                                   000339
@7L38    DS    0H                                                        000339
         BRU   @7L43                                                     000339
*  case ZOS64_VMEM_ABOVE_BAR_GENERAL:                                    000340
*   break;                                                               000341
@7L39    DS    0H                                                        000341
         BRU   @7L30                                                     000341
*  case ZOS64_VMEM_2_TO_32G:                                             000342
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000343
@7L40    DS    0H                                                        000343
         LA    2,@64origin@36                                            000343
         DROP  14                                                        000343
         LA    4,@63segments@35                                          000343
         LA    5,@67wgetstor                                             000343
         LG    14,464(0,13)            #SR_PARM_7                        000343
         USING @@PARMD@7,14                                              000343
         LG    6,@61ttkn@33                                              000343
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTX 000343
               O32G=YES,GUARDSIZE64=(4),PAGEFRAMESIZE=4K,SEGMENTS=(4),OX 000343
               RIGIN=(2),TTOKEN=(6),RETCODE=200(13),MF=(E,(5))           000343
*     "GUARDSIZE64=(%2),PAGEFRAMESIZE=4K,SEGMENTS=(%2),"\                000344
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000345
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000346
*   break;                                                               000347
         BRU   @7L30                                                     000347
*  case ZOS64_VMEM_2_TO_64G:                                             000348
*   __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE  000349
@7L41    DS    0H                                                        000349
         LA    2,@64origin@36                                            000349
         LA    4,@63segments@35                                          000349
         LA    5,@67wgetstor                                             000349
         LG    14,464(0,13)            #SR_PARM_7                        000349
         LG    6,@61ttkn@33                                              000349
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTX 000349
               O64G=YES,GUARDSIZE64=(4),PAGEFRAMESIZE=4K,SEGMENTS=(4),OX 000349
               RIGIN=(2),TTOKEN=(6),RETCODE=200(13),MF=(E,(5))           000349
*     "GUARDSIZE64=(%2),PAGEFRAMESIZE=4K,SEGMENTS=(%2),"\                000350
*     "ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\                  000351
*     ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(t  000352
*   break;                                                               000353
@7L30    DS    0H                                                        000339
@7L43    DS    0H                                                        000000
*  }                                                                     000354
*                                                                        000355
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000356
         LGF   14,@66iarv64_rc@38                                        000356
         LTR   14,14                                                     000356
         BRE   @7L9                                                      000356
*   return (void *)0;                                                    000357
         LGHI  15,0                                                      000357
         BRU   @7L31                                                     000357
@7L9     DS    0H                                                        000357
*  }                                                                     000358
*  return (void *)origin;                                                000359
         LG    15,@64origin@36                                           000359
* }                                                                      000360
@7L31    DS    0H                                                        000360
         DROP                                                            000360
         MYEPILOG                                                        000360
OMRIARV64 CSECT ,                                                        000360
         DS    0FD                                                       000360
@@LIT@7  LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@7  DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111111000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@7)      Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(55)                 Function Name                     000000
         DC    C'omrallocate_4K_pages_guarded_in_userExtendedPrivat'     000000
         DC    C'eArea'                                                  000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@7 DSECT                                                           000000
         DS    60FD                                                      000000
         ORG   @@AUTO@7                                                  000000
#GPR_SA_7 DS   18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@7+176                                              000000
@63segments@35 DS FD                                                     000000
         ORG   @@AUTO@7+184                                              000000
@64origin@36 DS FD                                                       000000
         ORG   @@AUTO@7+192                                              000000
@65useMemoryType@37 DS FD                                                000000
         ORG   @@AUTO@7+200                                              000000
@66iarv64_rc@38 DS F                                                     000000
         ORG   @@AUTO@7+208                                              000000
@67wgetstor DS XL256                                                     000000
         ORG   @@AUTO@7+464                                              000000
#SR_PARM_7 DS  XL8                                                       000000
@@PARMD@7 DSECT                                                          000000
         DS    XL24                                                      000000
         ORG   @@PARMD@7+0                                               000000
         DS    0FD                                                       000000
@59numMBSegments@31 DS 0XL8                                              000000
         ORG   @@PARMD@7+8                                               000000
         DS    0FD                                                       000000
@60userExtendedPrivateAreaMemoryType@32 DS 0XL8                          000000
         ORG   @@PARMD@7+16                                              000000
         DS    0FD                                                       000000
@61ttkn@33 DS  0XL8                                                      000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* void * omrallocate_4K_pages_above_bar(int *numMBSegments, const char   000375
         ENTRY @@CCN@69                                                  000375
@@CCN@69 AMODE 64                                                        000375
         DC    0FD                                                       000375
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000375
         DC    A(@@FPB@6-*+8)          Signed offset to FPB              000375
         DC    XL4'00000000'           Reserved                          000375
@@CCN@69 DS    0FD                                                       000375
&CCN_PRCN SETC '@@CCN@69'                                                000375
&CCN_PRCN_LONG SETC 'omrallocate_4K_pages_above_bar'                     000375
&CCN_LITN SETC '@@LIT@6'                                                 000375
&CCN_BEGIN SETC '@@BGN@6'                                                000375
&CCN_ASCM SETC 'P'                                                       000375
&CCN_DSASZ SETA 464                                                      000375
&CCN_SASZ SETA 144                                                       000375
&CCN_ARGS SETA 2                                                         000375
&CCN_RLOW SETA 14                                                        000375
&CCN_RHIGH SETA 6                                                        000375
&CCN_NAB SETB  0                                                         000375
&CCN_MAIN SETB 0                                                         000375
&CCN_NAB_STORED SETB 0                                                   000375
&CCN_STATIC SETB 0                                                       000375
&CCN_ALTGPR(1) SETB 1                                                    000375
&CCN_ALTGPR(2) SETB 1                                                    000375
&CCN_ALTGPR(3) SETB 1                                                    000375
&CCN_ALTGPR(4) SETB 1                                                    000375
&CCN_ALTGPR(5) SETB 1                                                    000375
&CCN_ALTGPR(6) SETB 1                                                    000375
&CCN_ALTGPR(7) SETB 1                                                    000375
&CCN_ALTGPR(8) SETB 0                                                    000375
&CCN_ALTGPR(9) SETB 0                                                    000375
&CCN_ALTGPR(10) SETB 0                                                   000375
&CCN_ALTGPR(11) SETB 0                                                   000375
&CCN_ALTGPR(12) SETB 0                                                   000375
&CCN_ALTGPR(13) SETB 0                                                   000375
&CCN_ALTGPR(14) SETB 1                                                   000375
&CCN_ALTGPR(15) SETB 1                                                   000375
&CCN_ALTGPR(16) SETB 1                                                   000375
         MYPROLOG                                                        000375
@@BGN@6  DS    0H                                                        000375
         AIF   (NOT &CCN_SASIG).@@NOSIG6                                 000375
         LLILH 6,X'C6F4'                                                 000375
         OILL  6,X'E2C1'                                                 000375
         ST    6,4(,13)                                                  000375
.@@NOSIG6 ANOP                                                           000375
         USING @@AUTO@6,13                                               000375
         LARL  3,@@LIT@6                                                 000375
         USING @@LIT@6,3                                                 000375
         STG   1,456(0,13)             #SR_PARM_6                        000375
*  long segments = 0;                                                    000376
         MVGHI @73segments@42,0                                          000376
*  long origin = 0;                                                      000377
         MVGHI @74origin@43,0                                            000377
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000378
         MVHI  @75iarv64_rc@44,8                                         000378
*                                                                        000379
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,RGETSTOR)":"DS"(wgetstor));         000380
*                                                                        000381
*  segments = *numMBSegments;                                            000382
         LG    14,456(0,13)            #SR_PARM_6                        000382
         USING @@PARMD@6,14                                              000382
         LG    14,@70numMBSegments@39                                    000382
         LGF   14,0(0,14)              (*)int                            000382
         STG   14,@73segments@42                                         000382
*  wgetstor = rgetstor;                                                  000383
         LARL  14,$STATIC                                                000383
         DROP  14                                                        000383
         USING @@STATICD@@,14                                            000383
         MVC   @76wgetstor,@68rgetstor                                   000383
*                                                                        000384
*  __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,"\                   000385
         LA    2,@74origin@43                                            000385
         DROP  14                                                        000385
         LA    4,@73segments@42                                          000385
         LA    5,@76wgetstor                                             000385
         LG    14,456(0,13)            #SR_PARM_6                        000385
         USING @@PARMD@6,14                                              000385
         LG    6,@71ttkn@40                                              000385
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,PAGEFRX 000385
               AMESIZE=4K,SEGMENTS=(4),ORIGIN=(2),TTOKEN=(6),RETCODE=19X 000385
               2(13),MF=(E,(5))                                          000385
*    "CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\                                 000386
*    "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\     000387
*    ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(tt  000388
*                                                                        000389
*  if (0 != iarv64_rc) {                                                 000390
         LGF   14,@75iarv64_rc@44                                        000390
         LTR   14,14                                                     000390
         BRE   @6L7                                                      000390
*   return (void *)0;                                                    000391
         LGHI  15,0                                                      000391
         BRU   @6L32                                                     000391
@6L7     DS    0H                                                        000391
*  }                                                                     000392
*  return (void *)origin;                                                000393
         LG    15,@74origin@43                                           000393
* }                                                                      000394
@6L32    DS    0H                                                        000394
         DROP                                                            000394
         MYEPILOG                                                        000394
OMRIARV64 CSECT ,                                                        000394
         DS    0FD                                                       000394
@@LIT@6  LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@6  DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111111000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@6)      Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(30)                 Function Name                     000000
         DC    C'omrallocate_4K_pages_above_bar'                         000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@6 DSECT                                                           000000
         DS    58FD                                                      000000
         ORG   @@AUTO@6                                                  000000
#GPR_SA_6 DS   18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@6+176                                              000000
@73segments@42 DS FD                                                     000000
         ORG   @@AUTO@6+184                                              000000
@74origin@43 DS FD                                                       000000
         ORG   @@AUTO@6+192                                              000000
@75iarv64_rc@44 DS F                                                     000000
         ORG   @@AUTO@6+200                                              000000
@76wgetstor DS XL256                                                     000000
         ORG   @@AUTO@6+456                                              000000
#SR_PARM_6 DS  XL8                                                       000000
@@PARMD@6 DSECT                                                          000000
         DS    XL16                                                      000000
         ORG   @@PARMD@6+0                                               000000
         DS    0FD                                                       000000
@70numMBSegments@39 DS 0XL8                                              000000
         ORG   @@PARMD@6+8                                               000000
         DS    0FD                                                       000000
@71ttkn@40 DS  0XL8                                                      000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* void *omrallocate_4K_pages_guarded_above_bar(int *numMBSegments, cons  000412
         ENTRY @@CCN@78                                                  000412
@@CCN@78 AMODE 64                                                        000412
         DC    0FD                                                       000412
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000412
         DC    A(@@FPB@5-*+8)          Signed offset to FPB              000412
         DC    XL4'00000000'           Reserved                          000412
@@CCN@78 DS    0FD                                                       000412
&CCN_PRCN SETC '@@CCN@78'                                                000412
&CCN_PRCN_LONG SETC 'omrallocate_4K_pages_guarded_above_bar'             000412
&CCN_LITN SETC '@@LIT@5'                                                 000412
&CCN_BEGIN SETC '@@BGN@5'                                                000412
&CCN_ASCM SETC 'P'                                                       000412
&CCN_DSASZ SETA 464                                                      000412
&CCN_SASZ SETA 144                                                       000412
&CCN_ARGS SETA 2                                                         000412
&CCN_RLOW SETA 14                                                        000412
&CCN_RHIGH SETA 6                                                        000412
&CCN_NAB SETB  0                                                         000412
&CCN_MAIN SETB 0                                                         000412
&CCN_NAB_STORED SETB 0                                                   000412
&CCN_STATIC SETB 0                                                       000412
&CCN_ALTGPR(1) SETB 1                                                    000412
&CCN_ALTGPR(2) SETB 1                                                    000412
&CCN_ALTGPR(3) SETB 1                                                    000412
&CCN_ALTGPR(4) SETB 1                                                    000412
&CCN_ALTGPR(5) SETB 1                                                    000412
&CCN_ALTGPR(6) SETB 1                                                    000412
&CCN_ALTGPR(7) SETB 1                                                    000412
&CCN_ALTGPR(8) SETB 0                                                    000412
&CCN_ALTGPR(9) SETB 0                                                    000412
&CCN_ALTGPR(10) SETB 0                                                   000412
&CCN_ALTGPR(11) SETB 0                                                   000412
&CCN_ALTGPR(12) SETB 0                                                   000412
&CCN_ALTGPR(13) SETB 0                                                   000412
&CCN_ALTGPR(14) SETB 1                                                   000412
&CCN_ALTGPR(15) SETB 1                                                   000412
&CCN_ALTGPR(16) SETB 1                                                   000412
         MYPROLOG                                                        000412
@@BGN@5  DS    0H                                                        000412
         AIF   (NOT &CCN_SASIG).@@NOSIG5                                 000412
         LLILH 6,X'C6F4'                                                 000412
         OILL  6,X'E2C1'                                                 000412
         ST    6,4(,13)                                                  000412
.@@NOSIG5 ANOP                                                           000412
         USING @@AUTO@5,13                                               000412
         LARL  3,@@LIT@5                                                 000412
         USING @@LIT@5,3                                                 000412
         STG   1,456(0,13)             #SR_PARM_5                        000412
*  long segments = 0;                                                    000413
         MVGHI @82segments@48,0                                          000413
*  long origin = 0;                                                      000414
         MVGHI @83origin@49,0                                            000414
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000415
         MVHI  @84iarv64_rc@50,8                                         000415
*                                                                        000416
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,EGETSTOR)":"DS"(wgetstor));         000417
*                                                                        000418
*  segments = *numMBSegments;                                            000419
         LG    14,456(0,13)            #SR_PARM_5                        000419
         USING @@PARMD@5,14                                              000419
         LG    14,@79numMBSegments@45                                    000419
         LGF   14,0(0,14)              (*)int                            000419
         STG   14,@82segments@48                                         000419
*  wgetstor = egetstor;                                                  000420
         LARL  14,$STATIC                                                000420
         DROP  14                                                        000420
         USING @@STATICD@@,14                                            000420
         MVC   @85wgetstor,@77egetstor                                   000420
*                                                                        000421
*  __asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,"\                   000422
         LA    2,@83origin@49                                            000422
         DROP  14                                                        000422
         LA    4,@82segments@48                                          000422
         LA    5,@85wgetstor                                             000422
         LG    14,456(0,13)            #SR_PARM_5                        000422
         USING @@PARMD@5,14                                              000422
         LG    6,@80ttkn@46                                              000422
         IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,GUARDSIZE=(4),CONTROLX 000422
               =UNAUTH,PAGEFRAMESIZE=4K,SEGMENTS=(4),ORIGIN=(2),TTOKEN=X 000422
               (6),RETCODE=192(13),MF=(E,(5))                            000422
*    "GUARDSIZE=(%2),"\                                                  000423
*    "CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\                                 000424
*    "SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\     000425
*    ::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(tt  000426
*                                                                        000427
*  if (OMRIARV64_SUCCESS != iarv64_rc) {                                 000428
         LGF   14,@84iarv64_rc@50                                        000428
         LTR   14,14                                                     000428
         BRE   @5L5                                                      000428
*   return (void *)0;                                                    000429
         LGHI  15,0                                                      000429
         BRU   @5L33                                                     000429
@5L5     DS    0H                                                        000429
*  }                                                                     000430
*  return (void *)origin;                                                000431
         LG    15,@83origin@49                                           000431
* }                                                                      000432
@5L33    DS    0H                                                        000432
         DROP                                                            000432
         MYEPILOG                                                        000432
OMRIARV64 CSECT ,                                                        000432
         DS    0FD                                                       000432
@@LIT@5  LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@5  DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111111000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@5)      Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(38)                 Function Name                     000000
         DC    C'omrallocate_4K_pages_guarded_above_bar'                 000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@5 DSECT                                                           000000
         DS    58FD                                                      000000
         ORG   @@AUTO@5                                                  000000
#GPR_SA_5 DS   18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@5+176                                              000000
@82segments@48 DS FD                                                     000000
         ORG   @@AUTO@5+184                                              000000
@83origin@49 DS FD                                                       000000
         ORG   @@AUTO@5+192                                              000000
@84iarv64_rc@50 DS F                                                     000000
         ORG   @@AUTO@5+200                                              000000
@85wgetstor DS XL256                                                     000000
         ORG   @@AUTO@5+456                                              000000
#SR_PARM_5 DS  XL8                                                       000000
@@PARMD@5 DSECT                                                          000000
         DS    XL16                                                      000000
         ORG   @@PARMD@5+0                                               000000
         DS    0FD                                                       000000
@79numMBSegments@45 DS 0XL8                                              000000
         ORG   @@PARMD@5+8                                               000000
         DS    0FD                                                       000000
@80ttkn@46 DS  0XL8                                                      000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* int omrfree_memory_above_bar(void *address, const char * ttkn){        000446
         ENTRY @@CCN@87                                                  000446
@@CCN@87 AMODE 64                                                        000446
         DC    0FD                                                       000446
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000446
         DC    A(@@FPB@4-*+8)          Signed offset to FPB              000446
         DC    XL4'00000000'           Reserved                          000446
@@CCN@87 DS    0FD                                                       000446
&CCN_PRCN SETC '@@CCN@87'                                                000446
&CCN_PRCN_LONG SETC 'omrfree_memory_above_bar'                           000446
&CCN_LITN SETC '@@LIT@4'                                                 000446
&CCN_BEGIN SETC '@@BGN@4'                                                000446
&CCN_ASCM SETC 'P'                                                       000446
&CCN_DSASZ SETA 456                                                      000446
&CCN_SASZ SETA 144                                                       000446
&CCN_ARGS SETA 2                                                         000446
&CCN_RLOW SETA 14                                                        000446
&CCN_RHIGH SETA 5                                                        000446
&CCN_NAB SETB  0                                                         000446
&CCN_MAIN SETB 0                                                         000446
&CCN_NAB_STORED SETB 0                                                   000446
&CCN_STATIC SETB 0                                                       000446
&CCN_ALTGPR(1) SETB 1                                                    000446
&CCN_ALTGPR(2) SETB 1                                                    000446
&CCN_ALTGPR(3) SETB 1                                                    000446
&CCN_ALTGPR(4) SETB 1                                                    000446
&CCN_ALTGPR(5) SETB 1                                                    000446
&CCN_ALTGPR(6) SETB 1                                                    000446
&CCN_ALTGPR(7) SETB 0                                                    000446
&CCN_ALTGPR(8) SETB 0                                                    000446
&CCN_ALTGPR(9) SETB 0                                                    000446
&CCN_ALTGPR(10) SETB 0                                                   000446
&CCN_ALTGPR(11) SETB 0                                                   000446
&CCN_ALTGPR(12) SETB 0                                                   000446
&CCN_ALTGPR(13) SETB 0                                                   000446
&CCN_ALTGPR(14) SETB 1                                                   000446
&CCN_ALTGPR(15) SETB 1                                                   000446
&CCN_ALTGPR(16) SETB 1                                                   000446
         MYPROLOG                                                        000446
@@BGN@4  DS    0H                                                        000446
         AIF   (NOT &CCN_SASIG).@@NOSIG4                                 000446
         LLILH 5,X'C6F4'                                                 000446
         OILL  5,X'E2C1'                                                 000446
         ST    5,4(,13)                                                  000446
.@@NOSIG4 ANOP                                                           000446
         USING @@AUTO@4,13                                               000446
         LARL  3,@@LIT@4                                                 000446
         USING @@LIT@4,3                                                 000446
         STG   1,448(0,13)             #SR_PARM_4                        000446
*  void *xmemobjstart = 0;                                               000447
         MVGHI @91xmemobjstart,0                                         000447
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000448
         MVHI  @92iarv64_rc@53,8                                         000448
*                                                                        000449
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,PGETSTOR)":"DS"(wgetstor));         000450
*                                                                        000451
*  xmemobjstart = address;                                               000452
         LG    14,448(0,13)            #SR_PARM_4                        000452
         USING @@PARMD@4,14                                              000452
         LG    14,@88address                                             000452
         STG   14,@91xmemobjstart                                        000452
*  wgetstor = pgetstor;                                                  000453
         LARL  14,$STATIC                                                000453
         DROP  14                                                        000453
         USING @@STATICD@@,14                                            000453
         MVC   @93wgetstor,@86pgetstor                                   000453
*                                                                        000454
*  __asm(" IARV64 REQUEST=DETACH,COND=YES,MEMOBJSTART=(%2),TTOKEN=(%3),  000455
         LA    2,@93wgetstor                                             000455
         DROP  14                                                        000455
         LA    4,@91xmemobjstart                                         000455
         LG    14,448(0,13)            #SR_PARM_4                        000455
         USING @@PARMD@4,14                                              000455
         LG    5,@89ttkn@51                                              000455
         IARV64 REQUEST=DETACH,COND=YES,MEMOBJSTART=(4),TTOKEN=(5),RETCX 000455
               ODE=184(13),MF=(E,(2))                                    000455
*    ::"m"(iarv64_rc),"r"(&wgetstor),"r"(&xmemobjstart),"r"(ttkn));      000456
*  return iarv64_rc;                                                     000457
         LGF   15,@92iarv64_rc@53                                        000457
* }                                                                      000458
@4L34    DS    0H                                                        000458
         DROP                                                            000458
         MYEPILOG                                                        000458
OMRIARV64 CSECT ,                                                        000458
         DS    0FD                                                       000458
@@LIT@4  LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@4  DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111110000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@4)      Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(24)                 Function Name                     000000
         DC    C'omrfree_memory_above_bar'                               000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@4 DSECT                                                           000000
         DS    57FD                                                      000000
         ORG   @@AUTO@4                                                  000000
#GPR_SA_4 DS   18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@4+176                                              000000
@91xmemobjstart DS AD                                                    000000
         ORG   @@AUTO@4+184                                              000000
@92iarv64_rc@53 DS F                                                     000000
         ORG   @@AUTO@4+192                                              000000
@93wgetstor DS XL256                                                     000000
         ORG   @@AUTO@4+448                                              000000
#SR_PARM_4 DS  XL8                                                       000000
@@PARMD@4 DSECT                                                          000000
         DS    XL16                                                      000000
         ORG   @@PARMD@4+0                                               000000
         DS    0FD                                                       000000
@88address DS  0XL8                                                      000000
         ORG   @@PARMD@4+8                                               000000
         DS    0FD                                                       000000
@89ttkn@51 DS  0XL8                                                      000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* int omrremove_guard(void *address, int *numMBSegments){                000478
         ENTRY @@CCN@95                                                  000478
@@CCN@95 AMODE 64                                                        000478
         DC    0FD                                                       000478
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000478
         DC    A(@@FPB@3-*+8)          Signed offset to FPB              000478
         DC    XL4'00000000'           Reserved                          000478
@@CCN@95 DS    0FD                                                       000478
&CCN_PRCN SETC '@@CCN@95'                                                000478
&CCN_PRCN_LONG SETC 'omrremove_guard'                                    000478
&CCN_LITN SETC '@@LIT@3'                                                 000478
&CCN_BEGIN SETC '@@BGN@3'                                                000478
&CCN_ASCM SETC 'P'                                                       000478
&CCN_DSASZ SETA 464                                                      000478
&CCN_SASZ SETA 144                                                       000478
&CCN_ARGS SETA 2                                                         000478
&CCN_RLOW SETA 14                                                        000478
&CCN_RHIGH SETA 5                                                        000478
&CCN_NAB SETB  0                                                         000478
&CCN_MAIN SETB 0                                                         000478
&CCN_NAB_STORED SETB 0                                                   000478
&CCN_STATIC SETB 0                                                       000478
&CCN_ALTGPR(1) SETB 1                                                    000478
&CCN_ALTGPR(2) SETB 1                                                    000478
&CCN_ALTGPR(3) SETB 1                                                    000478
&CCN_ALTGPR(4) SETB 1                                                    000478
&CCN_ALTGPR(5) SETB 1                                                    000478
&CCN_ALTGPR(6) SETB 1                                                    000478
&CCN_ALTGPR(7) SETB 0                                                    000478
&CCN_ALTGPR(8) SETB 0                                                    000478
&CCN_ALTGPR(9) SETB 0                                                    000478
&CCN_ALTGPR(10) SETB 0                                                   000478
&CCN_ALTGPR(11) SETB 0                                                   000478
&CCN_ALTGPR(12) SETB 0                                                   000478
&CCN_ALTGPR(13) SETB 0                                                   000478
&CCN_ALTGPR(14) SETB 1                                                   000478
&CCN_ALTGPR(15) SETB 1                                                   000478
&CCN_ALTGPR(16) SETB 1                                                   000478
         MYPROLOG                                                        000478
@@BGN@3  DS    0H                                                        000478
         AIF   (NOT &CCN_SASIG).@@NOSIG3                                 000478
         LLILH 5,X'C6F4'                                                 000478
         OILL  5,X'E2C1'                                                 000478
         ST    5,4(,13)                                                  000478
.@@NOSIG3 ANOP                                                           000478
         USING @@AUTO@3,13                                               000478
         LARL  3,@@LIT@3                                                 000478
         USING @@LIT@3,3                                                 000478
         STG   1,456(0,13)             #SR_PARM_3                        000478
*  void *memObjConvertStart = 0;                                         000479
         MVGHI @99memObjConvertStart,0                                   000479
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000480
         MVHI  @100iarv64_rc@57,8                                        000480
*  long segments = 0;                                                    000481
         MVGHI @101segments@58,0                                         000481
*                                                                        000482
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,FGETSTOR)":"DS"(wgetstor));         000483
*                                                                        000484
*  memObjConvertStart = address;                                         000485
         LG    14,456(0,13)            #SR_PARM_3                        000485
         USING @@PARMD@3,14                                              000485
         LG    14,@96address@54                                          000485
         STG   14,@99memObjConvertStart                                  000485
*  wgetstor = fgetstor;                                                  000486
         LARL  14,$STATIC                                                000486
         DROP  14                                                        000486
         USING @@STATICD@@,14                                            000486
         MVC   @102wgetstor,@94fgetstor                                  000486
*  segments = *numMBSegments;                                            000487
         LG    14,456(0,13)            #SR_PARM_3                        000487
         DROP  14                                                        000487
         USING @@PARMD@3,14                                              000487
         LG    14,@97numMBSegments@55                                    000487
         LGF   14,0(0,14)              (*)int                            000487
         STG   14,@101segments@58                                        000487
*                                                                        000488
*  __asm(" IARV64 REQUEST=CHANGEGUARD,CONVERT=FROMGUARD,COND=YES,"\      000489
         LA    2,@102wgetstor                                            000489
         LA    4,@99memObjConvertStart                                   000489
         LA    5,@101segments@58                                         000489
         IARV64 REQUEST=CHANGEGUARD,CONVERT=FROMGUARD,COND=YES,CONVERTSX 000489
               TART=(4),CONVERTSIZE64=(5),RETCODE=184(13),MF=(E,(2))     000489
*    "CONVERTSTART=(%2),CONVERTSIZE64=(%3),"\                            000490
*    "RETCODE=%0,MF=(E,(%1))"\                                           000491
*    ::"m"(iarv64_rc),"r"(&wgetstor),"r"(&memObjConvertStart),"r"(&segm  000492
*  return iarv64_rc;                                                     000493
         LGF   15,@100iarv64_rc@57                                       000493
* }                                                                      000494
@3L35    DS    0H                                                        000494
         DROP                                                            000494
         MYEPILOG                                                        000494
OMRIARV64 CSECT ,                                                        000494
         DS    0FD                                                       000494
@@LIT@3  LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@3  DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111110000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@3)      Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(15)                 Function Name                     000000
         DC    C'omrremove_guard'                                        000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@3 DSECT                                                           000000
         DS    58FD                                                      000000
         ORG   @@AUTO@3                                                  000000
#GPR_SA_3 DS   18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@3+176                                              000000
@99memObjConvertStart DS AD                                              000000
         ORG   @@AUTO@3+184                                              000000
@100iarv64_rc@57 DS F                                                    000000
         ORG   @@AUTO@3+192                                              000000
@101segments@58 DS FD                                                    000000
         ORG   @@AUTO@3+200                                              000000
@102wgetstor DS XL256                                                    000000
         ORG   @@AUTO@3+456                                              000000
#SR_PARM_3 DS  XL8                                                       000000
@@PARMD@3 DSECT                                                          000000
         DS    XL16                                                      000000
         ORG   @@PARMD@3+0                                               000000
         DS    0FD                                                       000000
@96address@54 DS 0XL8                                                    000000
         ORG   @@PARMD@3+8                                               000000
         DS    0FD                                                       000000
@97numMBSegments@55 DS 0XL8                                              000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
* int omradd_guard(void *address, int *numMBSegments) {                  000514
         ENTRY @@CCN@104                                                 000514
@@CCN@104 AMODE 64                                                       000514
         DC    0FD                                                       000514
         DC    XL8'00C300C300D50100'   Function Entry Point Marker       000514
         DC    A(@@FPB@2-*+8)          Signed offset to FPB              000514
         DC    XL4'00000000'           Reserved                          000514
@@CCN@104 DS   0FD                                                       000514
&CCN_PRCN SETC '@@CCN@104'                                               000514
&CCN_PRCN_LONG SETC 'omradd_guard'                                       000514
&CCN_LITN SETC '@@LIT@2'                                                 000514
&CCN_BEGIN SETC '@@BGN@2'                                                000514
&CCN_ASCM SETC 'P'                                                       000514
&CCN_DSASZ SETA 464                                                      000514
&CCN_SASZ SETA 144                                                       000514
&CCN_ARGS SETA 2                                                         000514
&CCN_RLOW SETA 14                                                        000514
&CCN_RHIGH SETA 5                                                        000514
&CCN_NAB SETB  0                                                         000514
&CCN_MAIN SETB 0                                                         000514
&CCN_NAB_STORED SETB 0                                                   000514
&CCN_STATIC SETB 0                                                       000514
&CCN_ALTGPR(1) SETB 1                                                    000514
&CCN_ALTGPR(2) SETB 1                                                    000514
&CCN_ALTGPR(3) SETB 1                                                    000514
&CCN_ALTGPR(4) SETB 1                                                    000514
&CCN_ALTGPR(5) SETB 1                                                    000514
&CCN_ALTGPR(6) SETB 1                                                    000514
&CCN_ALTGPR(7) SETB 0                                                    000514
&CCN_ALTGPR(8) SETB 0                                                    000514
&CCN_ALTGPR(9) SETB 0                                                    000514
&CCN_ALTGPR(10) SETB 0                                                   000514
&CCN_ALTGPR(11) SETB 0                                                   000514
&CCN_ALTGPR(12) SETB 0                                                   000514
&CCN_ALTGPR(13) SETB 0                                                   000514
&CCN_ALTGPR(14) SETB 1                                                   000514
&CCN_ALTGPR(15) SETB 1                                                   000514
&CCN_ALTGPR(16) SETB 1                                                   000514
         MYPROLOG                                                        000514
@@BGN@2  DS    0H                                                        000514
         AIF   (NOT &CCN_SASIG).@@NOSIG2                                 000514
         LLILH 5,X'C6F4'                                                 000514
         OILL  5,X'E2C1'                                                 000514
         ST    5,4(,13)                                                  000514
.@@NOSIG2 ANOP                                                           000514
         USING @@AUTO@2,13                                               000514
         LARL  3,@@LIT@2                                                 000514
         USING @@LIT@2,3                                                 000514
         STG   1,456(0,13)             #SR_PARM_2                        000514
*  void *memObjConvertStart = 0;                                         000515
         MVGHI @108memObjConvertStart@62,0                               000515
*  int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;                   000516
         MVHI  @109iarv64_rc@63,8                                        000516
*  long segments = 0;                                                    000517
         MVGHI @110segments@64,0                                         000517
*                                                                        000518
*  __asm(" IARV64 PLISTVER=MAX,MF=(L,DGETSTOR)":"DS"(wgetstor));         000519
*                                                                        000520
*  memObjConvertStart = address;                                         000521
         LG    14,456(0,13)            #SR_PARM_2                        000521
         USING @@PARMD@2,14                                              000521
         LG    14,@105address@59                                         000521
         STG   14,@108memObjConvertStart@62                              000521
*  wgetstor = dgetstor;                                                  000522
         LARL  14,$STATIC                                                000522
         DROP  14                                                        000522
         USING @@STATICD@@,14                                            000522
         MVC   @111wgetstor,@103dgetstor                                 000522
*  segments = *numMBSegments;                                            000523
         LG    14,456(0,13)            #SR_PARM_2                        000523
         DROP  14                                                        000523
         USING @@PARMD@2,14                                              000523
         LG    14,@106numMBSegments@60                                   000523
         LGF   14,0(0,14)              (*)int                            000523
         STG   14,@110segments@64                                        000523
*                                                                        000524
*  __asm(" IARV64 REQUEST=CHANGEGUARD,CONVERT=TOGUARD,COND=YES,"\        000525
         LA    2,@111wgetstor                                            000525
         LA    4,@108memObjConvertStart@62                               000525
         LA    5,@110segments@64                                         000525
         IARV64 REQUEST=CHANGEGUARD,CONVERT=TOGUARD,COND=YES,CONVERTSTAX 000525
               RT=(4),CONVERTSIZE64=(5),RETCODE=184(13),MF=(E,(2))       000525
*    "CONVERTSTART=(%2),CONVERTSIZE64=(%3),"\                            000526
*    "RETCODE=%0,MF=(E,(%1))"\                                           000527
*    ::"m"(iarv64_rc),"r"(&wgetstor),"r"(&memObjConvertStart),"r"(&segm  000528
*  return iarv64_rc;                                                     000529
         LGF   15,@109iarv64_rc@63                                       000529
* }                                                                      000530
@2L36    DS    0H                                                        000530
         DROP                                                            000530
         MYEPILOG                                                        000530
OMRIARV64 CSECT ,                                                        000530
         DS    0FD                                                       000530
@@LIT@2  LTORG                                                           000000
@@FPB@   LOCTR                                                           000000
@@FPB@2  DS    0FD                     Function Property Block           000000
         DC    XL2'CCD5'               Eyecatcher                        000000
         DC    BL2'1111110000000011'   Saved GPR Mask                    000000
         DC    A(@@PFD@@-@@FPB@2)      Signed Offset to Prefix Data      000000
         DC    BL1'10000000'           Flag Set 1                        000000
         DC    BL1'10000001'           Flag Set 2                        000000
         DC    BL1'00000000'           Flag Set 3                        000000
         DC    BL1'00000001'           Flag Set 4                        000000
         DC    XL4'00000000'           Reserved                          000000
         DC    XL4'00000000'           Reserved                          000000
         DC    AL2(12)                 Function Name                     000000
         DC    C'omradd_guard'                                           000000
OMRIARV64 LOCTR                                                          000000
         EJECT                                                           000000
@@AUTO@2 DSECT                                                           000000
         DS    58FD                                                      000000
         ORG   @@AUTO@2                                                  000000
#GPR_SA_2 DS   18FD                                                      000000
         DS    FD                                                        000000
         ORG   @@AUTO@2+176                                              000000
@108memObjConvertStart@62 DS AD                                          000000
         ORG   @@AUTO@2+184                                              000000
@109iarv64_rc@63 DS F                                                    000000
         ORG   @@AUTO@2+192                                              000000
@110segments@64 DS FD                                                    000000
         ORG   @@AUTO@2+200                                              000000
@111wgetstor DS XL256                                                    000000
         ORG   @@AUTO@2+456                                              000000
#SR_PARM_2 DS  XL8                                                       000000
@@PARMD@2 DSECT                                                          000000
         DS    XL16                                                      000000
         ORG   @@PARMD@2+0                                               000000
         DS    0FD                                                       000000
@105address@59 DS 0XL8                                                   000000
         ORG   @@PARMD@2+8                                               000000
         DS    0FD                                                       000000
@106numMBSegments@60 DS 0XL8                                             000000
*                                                                        000569
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
@@CONST@AREA@@ DS 0D                                                     000000
         DC    XL16'00000000000000000000000000000000'                    000000
         DC    XL16'00000000000000000000000000000000'                    000000
         DC    XL16'00000000000000000000000000000000'                    000000
         DC    XL16'00000000000000000000000000000000'                    000000
         DC    XL8'0000000000000000'                                     000000
         ORG   @@CONST@AREA@@+0                                          000000
         DC    A(@12L69-@@LIT@12)                                        000000
         DC    A(@12L70-@@LIT@12)                                        000000
         DC    A(@12L71-@@LIT@12)                                        000000
         ORG   @@CONST@AREA@@+12                                         000000
         DC    A(@11L63-@@LIT@11)                                        000000
         DC    A(@11L64-@@LIT@11)                                        000000
         DC    A(@11L65-@@LIT@11)                                        000000
         ORG   @@CONST@AREA@@+24                                         000000
         DC    A(@10L57-@@LIT@10)                                        000000
         DC    A(@10L58-@@LIT@10)                                        000000
         DC    A(@10L59-@@LIT@10)                                        000000
         ORG   @@CONST@AREA@@+36                                         000000
         DC    A(@9L51-@@LIT@9)                                          000000
         DC    A(@9L52-@@LIT@9)                                          000000
         DC    A(@9L53-@@LIT@9)                                          000000
         ORG   @@CONST@AREA@@+48                                         000000
         DC    A(@8L45-@@LIT@8)                                          000000
         DC    A(@8L46-@@LIT@8)                                          000000
         DC    A(@8L47-@@LIT@8)                                          000000
         ORG   @@CONST@AREA@@+60                                         000000
         DC    A(@7L39-@@LIT@7)                                          000000
         DC    A(@7L40-@@LIT@7)                                          000000
         DC    A(@7L41-@@LIT@7)                                          000000
         ORG   ,                                                         000000
         EJECT                                                           000000
OMRIARV64 CSECT ,                                                        000000
$STATIC  DS    0D                                                        000000
         DC    (3072)X'00'                                               000000
         LCLC  &DSMAC                                                    000000
         LCLA  &DSSIZE                                                   000000
         LCLA  &MSIZE                                                    000000
         ORG   $STATIC                                                   000000
@@LAB@1  EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,LGETSTOR)                             000000
@@LAB@1L EQU   *-@@LAB@1                                                 000000
&DSMAC   SETC  '@@LAB@1'                                                 000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@1L                                                  000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@1                                000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@1  ANOP                                                            000000
         ORG   $STATIC+256                                               000000
@@LAB@2  EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,NGETSTOR)                             000000
@@LAB@2L EQU   *-@@LAB@2                                                 000000
&DSMAC   SETC  '@@LAB@2'                                                 000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@2L                                                  000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@2                                000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@2  ANOP                                                            000000
         ORG   $STATIC+512                                               000000
@@LAB@3  EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,SGETSTOR)                             000000
@@LAB@3L EQU   *-@@LAB@3                                                 000000
&DSMAC   SETC  '@@LAB@3'                                                 000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@3L                                                  000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@3                                000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@3  ANOP                                                            000000
         ORG   $STATIC+768                                               000000
@@LAB@4  EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,OGETSTOR)                             000000
@@LAB@4L EQU   *-@@LAB@4                                                 000000
&DSMAC   SETC  '@@LAB@4'                                                 000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@4L                                                  000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@4                                000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@4  ANOP                                                            000000
         ORG   $STATIC+1024                                              000000
@@LAB@5  EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,MGETSTOR)                             000000
@@LAB@5L EQU   *-@@LAB@5                                                 000000
&DSMAC   SETC  '@@LAB@5'                                                 000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@5L                                                  000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@5                                000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@5  ANOP                                                            000000
         ORG   $STATIC+1280                                              000000
@@LAB@6  EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,TGETSTOR)                             000000
@@LAB@6L EQU   *-@@LAB@6                                                 000000
&DSMAC   SETC  '@@LAB@6'                                                 000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@6L                                                  000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@6                                000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@6  ANOP                                                            000000
         ORG   $STATIC+1536                                              000000
@@LAB@7  EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,RGETSTOR)                             000000
@@LAB@7L EQU   *-@@LAB@7                                                 000000
&DSMAC   SETC  '@@LAB@7'                                                 000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@7L                                                  000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@7                                000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@7  ANOP                                                            000000
         ORG   $STATIC+1792                                              000000
@@LAB@8  EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,EGETSTOR)                             000000
@@LAB@8L EQU   *-@@LAB@8                                                 000000
&DSMAC   SETC  '@@LAB@8'                                                 000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@8L                                                  000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@8                                000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@8  ANOP                                                            000000
         ORG   $STATIC+2048                                              000000
@@LAB@9  EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,PGETSTOR)                             000000
@@LAB@9L EQU   *-@@LAB@9                                                 000000
&DSMAC   SETC  '@@LAB@9'                                                 000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@9L                                                  000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@9                                000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@9  ANOP                                                            000000
         ORG   $STATIC+2304                                              000000
@@LAB@10 EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,FGETSTOR)                             000000
@@LAB@10L EQU  *-@@LAB@10                                                000000
&DSMAC   SETC  '@@LAB@10'                                                000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@10L                                                 000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@10                               000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@10 ANOP                                                            000000
         ORG   $STATIC+2560                                              000000
@@LAB@11 EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,DGETSTOR)                             000000
@@LAB@11L EQU  *-@@LAB@11                                                000000
&DSMAC   SETC  '@@LAB@11'                                                000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@11L                                                 000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@11                               000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@11 ANOP                                                            000000
         ORG   $STATIC+2816                                              000000
@@LAB@12 EQU   *                                                         000000
         IARV64 PLISTVER=MAX,MF=(L,QGETSTOR)                             000000
@@LAB@12L EQU  *-@@LAB@12                                                000000
&DSMAC   SETC  '@@LAB@12'                                                000000
&DSSIZE  SETA  256                                                       000000
&MSIZE   SETA  @@LAB@12L                                                 000000
         AIF   (&DSSIZE GE &MSIZE).@@OK@12                               000000
         MNOTE 4,'Expanded size(&MSIZE) from &DSMAC exceeds XL:DS size(X 000000
               &DSSIZE)'                                                 000000
.@@OK@12 ANOP                                                            000000
         EJECT                                                           000000
@@STATICD@@ DSECT                                                        000000
         DS    XL3072                                                    000000
         ORG   @@STATICD@@                                               000000
@1lgetstor DS  XL256                                                     000000
         ORG   @@STATICD@@+256                                           000000
@13ngetstor DS XL256                                                     000000
         ORG   @@STATICD@@+512                                           000000
@24sgetstor DS XL256                                                     000000
         ORG   @@STATICD@@+768                                           000000
@35ogetstor DS XL256                                                     000000
         ORG   @@STATICD@@+1024                                          000000
@46mgetstor DS XL256                                                     000000
         ORG   @@STATICD@@+1280                                          000000
@57tgetstor DS XL256                                                     000000
         ORG   @@STATICD@@+1536                                          000000
@68rgetstor DS XL256                                                     000000
         ORG   @@STATICD@@+1792                                          000000
@77egetstor DS XL256                                                     000000
         ORG   @@STATICD@@+2048                                          000000
@86pgetstor DS XL256                                                     000000
         ORG   @@STATICD@@+2304                                          000000
@94fgetstor DS XL256                                                     000000
         ORG   @@STATICD@@+2560                                          000000
@103dgetstor DS XL256                                                    000000
         ORG   @@STATICD@@+2816                                          000000
@112qgetstor DS XL256                                                    000000
         END   ,(5650ZOS   ,2400,26180)                                  000000
