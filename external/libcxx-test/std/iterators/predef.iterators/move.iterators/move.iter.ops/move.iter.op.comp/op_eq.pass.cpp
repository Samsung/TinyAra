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

// <iterator>

// move_iterator

// template <InputIterator Iter1, InputIterator Iter2>
//   requires HasEqualTo<Iter1, Iter2>
//   bool
//   operator==(const move_iterator<Iter1>& x, const move_iterator<Iter2>& y);
//
//  constexpr in C++17

#include <iterator>
#include <cassert>

#include "test_macros.h"
#include "test_iterators.h"

template <class It>
void
test(It l, It r, bool x)
{
    const std::move_iterator<It> r1(l);
    const std::move_iterator<It> r2(r);
    assert((r1 == r2) == x);
}

int main()
{
    char s[] = "1234567890";
    test(input_iterator<char*>(s), input_iterator<char*>(s), true);
    test(input_iterator<char*>(s), input_iterator<char*>(s+1), false);
    test(forward_iterator<char*>(s), forward_iterator<char*>(s), true);
    test(forward_iterator<char*>(s), forward_iterator<char*>(s+1), false);
    test(bidirectional_iterator<char*>(s), bidirectional_iterator<char*>(s), true);
    test(bidirectional_iterator<char*>(s), bidirectional_iterator<char*>(s+1), false);
    test(random_access_iterator<char*>(s), random_access_iterator<char*>(s), true);
    test(random_access_iterator<char*>(s), random_access_iterator<char*>(s+1), false);
    test(s, s, true);
    test(s, s+1, false);

#if TEST_STD_VER > 14
    {
    constexpr const char *p = "123456789";
    typedef std::move_iterator<const char *> MI;
    constexpr MI it1 = std::make_move_iterator(p);
    constexpr MI it2 = std::make_move_iterator(p + 5);
    constexpr MI it3 = std::make_move_iterator(p);
    static_assert(!(it1 == it2), "");
    static_assert( (it1 == it3), "");
    static_assert(!(it2 == it3), "");
    }
#endif
}
