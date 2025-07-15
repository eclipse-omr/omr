#!/bin/bash -f
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

allFiles=`git diff -C --diff-filter=ACM --name-only origin/master HEAD --`
if [ x"$allFiles" = x ] ; then
    echo "There are no files to check for code formatting."
else
    echo "allFiles:$allFiles"
    badFiles=
    for file in $allFiles ; do
        echo "file:$file"
        if [[ $file == compiler/*.c ]] || [[ $file == compiler/*.cpp ]] || [[ $file == compiler/*.h ]] || [[ $file == compiler/*.hpp ]] ; then
            docker run --rm -v $WORKSPACE:/src clang-format:latest clang-format -style=file:compiler/.clang-format /src/$file > $file.copy
            if [ "$(diff $file $file.copy)" != "" ] ; then
                echo "ERROR - clang-format should be a NOP: '$file'"
                badFiles="$badFiles $file"
                rm -f $file.copy
            fi
        fi
    done

    hashes='###################################'
    if [ x"$badFiles" != x ] ; then
        echo "$hashes"
        echo "The following files were modified and have incorrect code formatting:"
        for file in $badFiles ; do
            echo "$file"
        done
            echo "$hashes"
        exit 1
    else
        echo "All modified files appear to have correct code formatting."
    fi
fi