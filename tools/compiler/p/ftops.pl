#!/bin/perl

###############################################################################
# Copyright (c) 2000, 2016 IBM Corp. and others
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
#   Multiple authors (IBM Corp.) - initial implementation and documentation
###############################################################################

# This program takes as input the file 'compiler/p/codegen/PPCOps.ops' and creates various
# output files.

use strict;
use warnings;

use File::Basename;
use File::Spec;
use Cwd 'abs_path';
my $scriptDir = dirname(__FILE__);
# NOTE: If this script moves, modify the following statement to point to the top-level OMR directory.
my $omrDir = abs_path("$scriptDir/../../..");

my $inFilePath      = "$omrDir/compiler/p/codegen/PPCOps.ops";
my $propsFilePath   = "$omrDir/compiler/p/codegen/OpProperties.cpp";
my $binaryFilePath  = "$omrDir/compiler/p/codegen/OpBinary.cpp";
my $nameFilePath    = "$omrDir/compiler/ras/PPCOpNames.cpp";
my $enumFilePath    = "$omrDir/compiler/p/codegen/OMRInstOpCodeEnum.hpp";

sub writePrefixes
   {
   my $scriptName = basename($0);
   my $scriptRelPath = File::Spec->abs2rel($0, $omrDir);
   my $inFileRelPath = File::Spec->abs2rel($inFilePath, $omrDir);
   my $warning = <<END;
// DO NOT EDIT THIS FILE!!!
// This file was generated by '$scriptName'.
// In order to change the information in this
// file you must edit '\$OMR/$inFileRelPath'
// and then run 'perl \$OMR/$scriptRelPath'.
END

   die("cannot open input file: $inFilePath") unless open(INFILE,"<$inFilePath");
   die("cannot open properties file: $propsFilePath") unless open(PROPSFILE,">$propsFilePath");
   die("cannot open binary file: $binaryFilePath") unless open(BINARYFILE,">$binaryFilePath");
   die("cannot open name file: $nameFilePath") unless open(NAMEFILE,">$nameFilePath");
   die("cannot open enum file: $enumFilePath") unless open(ENUMFILE,">$enumFilePath");

   print PROPSFILE <<END;
$warning
#include <stdint.h>                     // for uint32_t
#include "codegen/InstOpCode.hpp"       // for InstOpCode, etc
#include "p/codegen/PPCOpsDefines.hpp"  // for PPCOpProp_SyncSideEffectFree, etc

const uint32_t OMR::Power::InstOpCode::properties[PPCNumOpCodes] =
   {
END

   print BINARYFILE <<END;
$warning
#include "codegen/InstOpCode.hpp"  // for InstOpCode, etc

const TR_PPCOpCodeBinaryEntry OMR::Power::InstOpCode::binaryEncodings[PPCNumOpCodes] =
   {
END

   print NAMEFILE <<END;
$warning
#ifndef TR_TARGET_POWER
int ppcOpNames;
#else
const char * ppcOpCodeToNameMap[][2] =
   {
END

   print ENUMFILE <<END;
$warning
/*
 * This file will be included within an enum. Only comments and enumerator
 * definitions are permitted.
 */

END
   }

sub writeAppendices
   {
   print BINARYFILE <<END;
   0x00000000   // label
   };
END

   print PROPSFILE <<END;
// label
0
};
END

   print NAMEFILE <<END;
   {"label", "label"}
   };
#endif
END

   print ENUMFILE <<END;
   label,            // Destination of a jump
   PPCLastOp = label,
   PPCNumOpCodes = PPCLastOp + 1,
END

   close INFILE;
   close PROPSFILE;
   close BINARYFILE;
   close NAMEFILE;
   close ENUMFILE;
   }

sub readLine
   {
   my $line = shift;
   $line =~ s/\s+$//; # chomp
   return 0 if !$line or $line =~ /^!/; # skip comments and blank lines
   $line =~ s/\/\// \/\//; # seperate properties from comments
   my ($opcode,$opName,$binary,$properties,@commentArr) = split(' ',$line);
   if ($properties =~ /^\/\//)
      {
      $properties = 0;
      unshift(@commentArr,'//');
      }
   my $comment = join(" ",@commentArr);
   return ($opcode,$opName,$binary,$properties,$comment);
   }

sub printLine
   {
   my ($opcode,$opName,$decbin,$properties,$comment) = (shift,shift,shift,shift,shift);
   my $padding = " " x (17-length($opcode));
   my $hexbin  = sprintf '0x%.8x', $decbin; #convert back to hex

   # singleton cases
   $hexbin = "0x34000000" if $opcode =~ /addi._r/;
   $opName = "addic." if $opcode =~ /addi2_r/;
   ($hexbin,$opName) = ("0x70000000","andi") if $opcode =~ /mr_r/;

   $properties = join(" |\nPPCOpProp_",split(",",$properties));
   $properties = "PPCOpProp_$properties" unless ($properties eq 0);

   print PROPSFILE "// $opcode\n$properties,\n\n";
   print BINARYFILE "   $hexbin,  // $opcode\n";
   print NAMEFILE "   {\"$opcode\", \"$opName\"},\n";
   $opcode = uc $opcode if $opcode =~ /^(and|or|xor)$/;
   print ENUMFILE "   $opcode,$padding$comment\n";
   }

# ------------------------ MAIN ------------------------ #
writePrefixes();

while(<INFILE>)
   {
   my ($opcode,$opName,$binary,$properties,$comment) = readLine($_);
   next if ($opcode eq 0);  # skip comments and blank lines
   printLine($opcode,$opName,hex($binary),$properties,$comment);
   printLine("$opcode\_r","$opName.",hex($binary)+1,$properties,"$comment with record") if ($properties =~ s/HasRecordForm/IsRecordForm/);
   }

writeAppendices();
# -------------------- END OF MAIN --------------------- #
