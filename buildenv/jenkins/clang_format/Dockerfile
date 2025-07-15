###############################################################################
# Copyright IBM Corp. and others 2025
#
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at https://www.eclipse.org/legal/epl-2.0/
# or the Apache License, Version 2.0 which accompanies this distribution and
# is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# This Source Code may also be made available under the following
# Secondary Licenses when the conditions for such availability set
# forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
# General Public License, version 2 with the GNU Classpath
# Exception [1] and GNU General Public License, version 2 with the
# OpenJDK Assembly Exception [2].
#
# [1] https://www.gnu.org/software/classpath/license.html
# [2] https://openjdk.org/legal/assembly-exception.html
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
###############################################################################

FROM registry.access.redhat.com/ubi8/ubi-minimal:latest

RUN set -eux; \
    microdnf install -y \
       wget tar xz ncurses-compat-libs \
    && microdnf update -y \
    && microdnf clean all; \
    cd /tmp; \
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/clang+llvm-18.1.8-x86_64-linux-gnu-ubuntu-18.04.tar.xz \
    && tar -xvf clang+llvm-18.1.8-x86_64-linux-gnu-ubuntu-18.04.tar.xz \
    && mkdir -p /usr/bin \
    && cp clang+llvm-18.1.8-x86_64-linux-gnu-ubuntu-18.04/bin/clang-format /usr/bin; \
    rm -rf clang+llvm-18.1.8-x86_64-linux-gnu-ubuntu-18.04*; \
    microdnf remove wget tar xz

ENV PATH=/usr/bin:$PATH

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

WORKDIR /src

CMD [""]
