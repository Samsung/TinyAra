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
// vector<bool>

// vector(size_type n, const value_type& x, const allocator_type& a);

#include <vector>
#include <cassert>

#include "test_macros.h"

template <class C>
void
test(typename C::size_type n, const typename C::value_type& x,
     const typename C::allocator_type& a)
{
    C c(n, x, a);
    LIBCPP_ASSERT(c.__invariants());
    assert(a == c.get_allocator());
    assert(c.size() == n);
    for (typename C::const_iterator i = c.cbegin(), e = c.cend(); i != e; ++i)
        assert(*i == x);
}

int main()
{
    test<std::vector<bool> >(50, true, std::allocator<bool>());
}
