;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Copyright (c) 2000, 2016 IBM Corp. and others
;; 
;; This program and the accompanying materials are made available under
;; the terms of the Eclipse Public License 2.0 which accompanies this
;; distribution and is available at https://www.eclipse.org/legal/epl-2.0/
;; or the Apache License, Version 2.0 which accompanies this distribution and
;; is available at https://www.apache.org/licenses/LICENSE-2.0.
;; 
;; This Source Code may also be made available under the following
;; Secondary Licenses when the conditions for such availability set
;; forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
;; General Public License, version 2 with the GNU Classpath 
;; Exception [1] and GNU General Public License, version 2 with the
;; OpenJDK Assembly Exception [2].
;; 
;; [1] https://www.gnu.org/software/classpath/license.html
;; [2] http://openjdk.java.net/legal/assembly-exception.html
;; 
;; SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; VM should define this at some point
OFFSETOF_CPU_CYCLE_COUNT equ 0

ifdef TR_HOST_64BIT

   include x/amd64/runtime/AMD64AsmUtil.inc

public initCycleCounter
public incrementCycleCounter
public getCycles

GetCycles MACRO
        rdtsc
        shl    rdx,32
        or     rax,rdx
ENDM

; void initCycleCounter()
initCycleCounter PROC NEAR
	GetCycles
        mov    qword ptr [rbp + OFFSETOF_CPU_CYCLE_COUNT], rax
        ret
initCycleCounter ENDP

; void incrementCycleCounter(intptr_t cycleCounterPointer)
incrementCycleCounter PROC NEAR
	GetCycles
        sub    rax, qword ptr [rbp + OFFSETOF_CPU_CYCLE_COUNT]
        add    qword ptr [rdi], rax
        ret
incrementCycleCounter ENDP

getCycles PROC NEAR
	GetCycles
	ret
getCycles ENDP

else
   .686p
   assume cs:flat,ds:flat,ss:flat
_DATA           segment para use32 public 'DATA'
AMD64AsmUtil:
   db      00h
_DATA           ends

endif
                end
