/*******************************************************************************
 * Copyright IBM Corp. and others 2026
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include <gtest/gtest.h>
#include "AtomicSupport.hpp"

namespace OMR
{
	TEST(TestCAE, Simple8BitSuccess)
	{
		volatile uint8_t value = 0x42;
		uint8_t oldVal = 0x42;
		uint8_t newVal = 0x60;

		uint8_t result = VM_AtomicSupport::lockCompareExchangeU8(&value, oldVal, newVal);

		EXPECT_EQ(result, 0x42);
		EXPECT_EQ(value, 0x60);
	}

	TEST(TestCAE, Simple8BitFailure)
	{
		volatile uint8_t value = 0x42;
		uint8_t wrongOldVal = 0x50;
		uint8_t newVal = 0x60;

		uint8_t result = VM_AtomicSupport::lockCompareExchangeU8(&value, wrongOldVal, newVal);

		EXPECT_EQ(result, 0x42);
		EXPECT_EQ(value, 0x42);
	}

	TEST(TestCAE, Simple16BitSuccess)
	{
		volatile uint16_t value = 0x1234;
		uint16_t oldVal = 0x1234;
		uint16_t newVal = 0x5678;

		uint16_t result = VM_AtomicSupport::lockCompareExchangeU16(&value, oldVal, newVal);

		EXPECT_EQ(result, 0x1234);
		EXPECT_EQ(value, 0x5678);
	}

	TEST(TestCAE, Simple16BitFailure)
	{
		volatile uint16_t value = 0x1234;
		uint16_t wrongOldVal = 0x9999;
		uint16_t newVal = 0x5678;

		uint16_t result = VM_AtomicSupport::lockCompareExchangeU16(&value, wrongOldVal, newVal);

		EXPECT_EQ(result, 0x1234);
		EXPECT_EQ(value, 0x1234);
	}
}
