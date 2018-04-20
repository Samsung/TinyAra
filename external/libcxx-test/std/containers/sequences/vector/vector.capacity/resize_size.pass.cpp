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

// <vector>

// void resize(size_type sz);

#include <vector>
#include <cassert>

#include "test_macros.h"
#include "test_allocator.h"
#include "MoveOnly.h"
#include "asan_testing.h"

int main()
{
    {
        std::vector<int> v(100);
        v.resize(50);
        assert(v.size() == 50);
        assert(v.capacity() == 100);
        assert(is_contiguous_container_asan_correct(v));
        v.resize(200);
        assert(v.size() == 200);
        assert(v.capacity() >= 200);
        assert(is_contiguous_container_asan_correct(v));
    }
    {
        // Add 1 for implementations that dynamically allocate a container proxy.
        std::vector<int, limited_allocator<int, 300 + 1> > v(100);
        v.resize(50);
        assert(v.size() == 50);
        assert(v.capacity() == 100);
        assert(is_contiguous_container_asan_correct(v));
        v.resize(200);
        assert(v.size() == 200);
        assert(v.capacity() >= 200);
        assert(is_contiguous_container_asan_correct(v));
    }
#if TEST_STD_VER >= 11
    {
        std::vector<MoveOnly> v(100);
        v.resize(50);
        assert(v.size() == 50);
        assert(v.capacity() == 100);
        assert(is_contiguous_container_asan_correct(v));
        v.resize(200);
        assert(v.size() == 200);
        assert(v.capacity() >= 200);
        assert(is_contiguous_container_asan_correct(v));
    }
    {
        // Add 1 for implementations that dynamically allocate a container proxy.
        std::vector<MoveOnly, limited_allocator<MoveOnly, 300 + 1> > v(100);
        v.resize(50);
        assert(v.size() == 50);
        assert(v.capacity() == 100);
        assert(is_contiguous_container_asan_correct(v));
        v.resize(200);
        assert(v.size() == 200);
        assert(v.capacity() >= 200);
        assert(is_contiguous_container_asan_correct(v));
    }
#endif
}
