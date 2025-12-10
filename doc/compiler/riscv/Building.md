<!--
Copyright IBM Corp. and others 2019

This program and the accompanying materials are made available under
the terms of the Eclipse Public License 2.0 which accompanies this
distribution and is available at https://www.eclipse.org/legal/epl-2.0/
or the Apache License, Version 2.0 which accompanies this distribution
and is available at https://www.apache.org/licenses/LICENSE-2.0.

This Source Code may also be made available under the following Secondary
Licenses when the conditions for such availability set forth in the
Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
version 2 with the GNU Classpath Exception [1] and GNU General Public
License, version 2 with the OpenJDK Assembly Exception [2].

[1] https://www.gnu.org/software/classpath/license.html
[2] https://openjdk.org/legal/assembly-exception.html

SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
-->

# Building Compiler on RISC-V

## Native build

*Note:* Following steps have been tested on Debian system created using
[debian-for-toys](1) scripts. The image built using these scripts should contain everything
needed to build OMR on RISC-V. It should be possible to build OMR on RISC-V
using Fedora images but this has not been tried.

*Note:* You will need `riscv.h` and `riscv-opc.h` headers from GNU binutils. You
can get them using (the image created using [debian-riscv][1] contains them):

    sudo wget "-O/usr/local/include/riscv.h" 'https://sourceware.org/git/gitweb.cgi?p=binutils-gdb.git;a=blob_plain;f=include/opcode/riscv.h;hb=2f973f134d7752cbc662ec65da8ad8bbe4c6fb8f'
    sudo wget "-O/usr/local/include/riscv-opc.h" 'https://sourceware.org/git/gitweb.cgi?p=binutils-gdb.git;a=blob_plain;f=include/opcode/riscv-opc.h;hb=2f973f134d7752cbc662ec65da8ad8bbe4c6fb8f'

### Building

```
git clone https://github.com/eclipse-omr/omr
cd omr
mkdir build
cd build
cmake .. -Wdev -C../cmake/caches/Travis.cmake \
         -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -j4
```

Notes:

 * You *must* return `cmake ...` when you add/remove/rename files (generally, when you change any `CMakeLists.txt`).
 * `-DCMAKE_BUILD_TYPE=Debug` is necessary to produce debug symbols
 * `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` is to produce a `compile_commands.json` used by Clang-based tools. Should not harm.

### Compiler tests

To see what tests are in minimal testsuite run (assume you're in `build` directory)

    cd fvtest/compilertest
    ./compilertest --gtest_list_tests --gtest_filter=MinimalTest*

 To run specific test, say `MeaningOfLife`:

    ./compilertest --gtest_filter=MinimalTest.MeaningOfLife

 To run all tests in minimal test suite:

    ./compilertest --gtest_filter=MinimalTest.*

### TRIL tests

To run selected TRIL tests:

    cd fvtest/compilertriltest
    ./comptest --gtest_filter=*Arithmetic*/Int32*:LogicalTest/Int32*Unary*

To run *ALL* TRIL tests:

    cd fvtest/compilertriltest
    TR_Options=softFailOnAssume ./comptest

*Note*: At the time of writing, some opcodes used in TRIL tests are still unimplemented, causing `TR_ASSERT()` failure. `TR_Options=softFailOnAssume` causes this (and all others) assertion to just abort the compilation. This way, hitting such and unimplemented opcode does not terminate whole test run.

## Cross compiling

*Note:* Following steps are for (Debian 13 "Trixie"). It should work on Debian
Testing ("bullseye"). It may work on other debian-based distributions, perhaps
with some modification. However, it has been tested only on Debian 10 "buster".

### Preparing the cross-compilation environment

 1. Install RISCV64 toolchain:

        sudo apt install crossbuild-essential-riscv64 qemu-user-static

    (Alternatively, if there's no `crossbuild-essential-riscv64` package,
    installing `gcc-riscv64-linux-gnu` and `g++-riscv64-linux-gnu` should
    suffice)

 2. Download required `riscv.h` and `riscv-opc.h` from GNU binutils:

        sudo wget "-O$/usr/local/include/riscv.h" 'https://sourceware.org/git/gitweb.cgi?p=binutils-gdb.git;a=blob_plain;f=include/opcode/riscv.h;hb=2f973f134d7752cbc662ec65da8ad8bbe4c6fb8f'
        sudo wget "-O$/usr/local/include/riscv-opc.h" 'https://sourceware.org/git/gitweb.cgi?p=binutils-gdb.git;a=blob_plain;f=include/opcode/riscv-opc.h;hb=2f973f134d7752cbc662ec65da8ad8bbe4c6fb8f'

 3. Create a "root filesystem" containing all required libraries and tools to link and run OMR. Here we build it into `/usr/gnemul/qemu-riscv64`, modify according your taste):

        sudo apt install mmdebstrap fakeroot
        cd ~
        wget https://raw.githubusercontent.com/janvrany/debian-builders/refs/heads/master/scripts/build-sysroot.sh
        bash build-sysroot.sh -a riscv64 -d /usr/gnemul/qemu-riscv64

*Note*: you may create a complete container/VM image with everything set up
 using ["debian-builders"](2) scripts.

### Building

```
git clone https://github.com/eclipse-omr/omr
cd omr
mkdir build_cross
cmake .. -Wdev -C../cmake/caches/Travis.cmake \
         -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
         -DOMR_DDR=OFF  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/riscv64-linux-cross.cmake \
         "-DOMR_EXE_LAUNCHER=qemu-riscv64-static;-L;/usr/gnemul/qemu-riscv64" "-DCMAKE_SYSROOT=/usr/gnemul/qemu-riscv64"
make -j4
```

### Compiler tests

To see what tests are in minimal testsuite run:

    cd fvtest/compilertest
    qemu-riscv64-static -L /usr/gnemul/qemu-riscv64 ./compilertest --gtest_list_tests --gtest_filter=MinimalTest*

The above command assumes that:

 * you're in `build_cross` directory,
  * Debian RISCV sysroot fs is at `/usr/gnemul/qemu-riscv64` (see section *Preparing the cross-compilation environment*, step 3).

 To run specific test, say `MeaningOfLife`:
7
    ~qemu-riscv64-static -L /usr/gnemul/qemu-riscv64 ./compilertest --gtest_filter=MinimalTest.MeaningOfLife

 To run all tests in minimal test suite:

    qemu-riscv64-static -L /usr/gnemul/qemu-riscv64 ./compilertest --gtest_filter=MinimalTest.*

### TRIL tests

To run selected TRIL tests:

    cd fvtest/compilertriltest
    qemu-riscv64-static -L /usr/gnemul/qemu-riscv64 ./comptest --gtest_filter=*Arithmetic*/Int32*:LogicalTest/Int32*Unary*

[1]: https://github.com/janvrany/debian-for-toys
[2]: https://github.com/janvrany/debian-builders
