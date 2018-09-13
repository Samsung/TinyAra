/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++98, c++03

// <vector>

// vector& operator=(vector&& c);

#include <vector>
#include <cassert>
#include "libcxx_tc_common.h"
#include "test_allocator.h"

int tc_libcxx_containers_vector_bool_assign_move(void)
{
    {
        std::vector<bool, test_allocator<bool> > l(test_allocator<bool>(5));
        std::vector<bool, test_allocator<bool> > lo(test_allocator<bool>(5));
        for (int i = 1; i <= 3; ++i)
        {
            l.push_back(i);
            lo.push_back(i);
        }
        std::vector<bool, test_allocator<bool> > l2(test_allocator<bool>(5));
        l2 = std::move(l);
        TC_ASSERT_EXPR(l2 == lo);
        TC_ASSERT_EXPR(l.empty());
        TC_ASSERT_EXPR(l2.get_allocator() == lo.get_allocator());
    }
    {
        std::vector<bool, test_allocator<bool> > l(test_allocator<bool>(5));
        std::vector<bool, test_allocator<bool> > lo(test_allocator<bool>(5));
        for (int i = 1; i <= 3; ++i)
        {
            l.push_back(i);
            lo.push_back(i);
        }
        std::vector<bool, test_allocator<bool> > l2(test_allocator<bool>(6));
        l2 = std::move(l);
        TC_ASSERT_EXPR(l2 == lo);
        TC_ASSERT_EXPR(!l.empty());
        TC_ASSERT_EXPR(l2.get_allocator() == test_allocator<bool>(6));
    }
    {
        std::vector<bool, other_allocator<bool> > l(other_allocator<bool>(5));
        std::vector<bool, other_allocator<bool> > lo(other_allocator<bool>(5));
        for (int i = 1; i <= 3; ++i)
        {
            l.push_back(i);
            lo.push_back(i);
        }
        std::vector<bool, other_allocator<bool> > l2(other_allocator<bool>(6));
        l2 = std::move(l);
        TC_ASSERT_EXPR(l2 == lo);
        TC_ASSERT_EXPR(l.empty());
        TC_ASSERT_EXPR(l2.get_allocator() == lo.get_allocator());
    }
    TC_SUCCESS_RESULT();
    return 0;
}
