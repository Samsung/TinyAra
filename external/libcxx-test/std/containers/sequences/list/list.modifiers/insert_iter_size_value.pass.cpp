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

// <list>

// iterator insert(const_iterator position, size_type n, const value_type& x);

// UNSUPPORTED: sanitizer-new-delete

#include <list>
#include <cstdlib>
#include <cassert>

#include "test_macros.h"

template <class List>
void test() {
    int a1[] = {1, 2, 3};
    int a2[] = {1, 4, 4, 4, 4, 4, 2, 3};
    List l1(a1, a1+3);
    typename List::iterator i = l1.insert(next(l1.cbegin()), 5, 4);
    assert(i == next(l1.begin()));
    assert(l1 == List(a2, a2+8));
}

int main()
{
    test<std::list<int> >();
}
