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

// <forward_list>

// void swap(forward_list& x);

#include <forward_list>
#include <cassert>

#include "test_allocator.h"

int main()
{
    {
        typedef int T;
        typedef test_allocator<T> A;
        typedef std::forward_list<T, A> C;
        const T t1[] = {0, 1, 2, 3, 4, 5};
        C c1(std::begin(t1), std::end(t1), A(1, 1));
        const T t2[] = {10, 11, 12};
        C c2(std::begin(t2), std::end(t2), A(1, 2));
        c1.swap(c2);

        assert(distance(c1.begin(), c1.end()) == 3);
        assert(*next(c1.begin(), 0) == 10);
        assert(*next(c1.begin(), 1) == 11);
        assert(*next(c1.begin(), 2) == 12);
        assert(c1.get_allocator().get_id() == 1);

        assert(distance(c2.begin(), c2.end()) == 6);
        assert(*next(c2.begin(), 0) == 0);
        assert(*next(c2.begin(), 1) == 1);
        assert(*next(c2.begin(), 2) == 2);
        assert(*next(c2.begin(), 3) == 3);
        assert(*next(c2.begin(), 4) == 4);
        assert(*next(c2.begin(), 5) == 5);
        assert(c2.get_allocator().get_id() == 2);
    }
    {
        typedef int T;
        typedef test_allocator<T> A;
        typedef std::forward_list<T, A> C;
        const T t1[] = {0, 1, 2, 3, 4, 5};
        C c1(std::begin(t1), std::end(t1), A(1, 1));
        C c2(A(1, 2));
        c1.swap(c2);

        assert(distance(c1.begin(), c1.end()) == 0);
        assert(c1.get_allocator().get_id() == 1);

        assert(distance(c2.begin(), c2.end()) == 6);
        assert(*next(c2.begin(), 0) == 0);
        assert(*next(c2.begin(), 1) == 1);
        assert(*next(c2.begin(), 2) == 2);
        assert(*next(c2.begin(), 3) == 3);
        assert(*next(c2.begin(), 4) == 4);
        assert(*next(c2.begin(), 5) == 5);
        assert(c2.get_allocator().get_id() == 2);
    }
    {
        typedef int T;
        typedef test_allocator<T> A;
        typedef std::forward_list<T, A> C;
        C c1(A(1, 1));
        const T t2[] = {10, 11, 12};
        C c2(std::begin(t2), std::end(t2), A(1, 2));
        c1.swap(c2);

        assert(distance(c1.begin(), c1.end()) == 3);
        assert(*next(c1.begin(), 0) == 10);
        assert(*next(c1.begin(), 1) == 11);
        assert(*next(c1.begin(), 2) == 12);
        assert(c1.get_allocator().get_id() == 1);

        assert(distance(c2.begin(), c2.end()) == 0);
        assert(c2.get_allocator().get_id() == 2);
    }
    {
        typedef int T;
        typedef test_allocator<T> A;
        typedef std::forward_list<T, A> C;
        C c1(A(1, 1));
        C c2(A(1, 2));
        c1.swap(c2);

        assert(distance(c1.begin(), c1.end()) == 0);
        assert(c1.get_allocator().get_id() == 1);

        assert(distance(c2.begin(), c2.end()) == 0);
        assert(c2.get_allocator().get_id() == 2);
    }

    {
        typedef int T;
        typedef other_allocator<T> A;
        typedef std::forward_list<T, A> C;
        const T t1[] = {0, 1, 2, 3, 4, 5};
        C c1(std::begin(t1), std::end(t1), A(1));
        const T t2[] = {10, 11, 12};
        C c2(std::begin(t2), std::end(t2), A(2));
        c1.swap(c2);

        assert(distance(c1.begin(), c1.end()) == 3);
        assert(*next(c1.begin(), 0) == 10);
        assert(*next(c1.begin(), 1) == 11);
        assert(*next(c1.begin(), 2) == 12);
        assert(c1.get_allocator() == A(2));

        assert(distance(c2.begin(), c2.end()) == 6);
        assert(*next(c2.begin(), 0) == 0);
        assert(*next(c2.begin(), 1) == 1);
        assert(*next(c2.begin(), 2) == 2);
        assert(*next(c2.begin(), 3) == 3);
        assert(*next(c2.begin(), 4) == 4);
        assert(*next(c2.begin(), 5) == 5);
        assert(c2.get_allocator() == A(1));
    }
    {
        typedef int T;
        typedef other_allocator<T> A;
        typedef std::forward_list<T, A> C;
        const T t1[] = {0, 1, 2, 3, 4, 5};
        C c1(std::begin(t1), std::end(t1), A(1));
        C c2(A(2));
        c1.swap(c2);

        assert(distance(c1.begin(), c1.end()) == 0);
        assert(c1.get_allocator() == A(2));

        assert(distance(c2.begin(), c2.end()) == 6);
        assert(*next(c2.begin(), 0) == 0);
        assert(*next(c2.begin(), 1) == 1);
        assert(*next(c2.begin(), 2) == 2);
        assert(*next(c2.begin(), 3) == 3);
        assert(*next(c2.begin(), 4) == 4);
        assert(*next(c2.begin(), 5) == 5);
        assert(c2.get_allocator() == A(1));
    }
    {
        typedef int T;
        typedef other_allocator<T> A;
        typedef std::forward_list<T, A> C;
        C c1(A(1));
        const T t2[] = {10, 11, 12};
        C c2(std::begin(t2), std::end(t2), A(2));
        c1.swap(c2);

        assert(distance(c1.begin(), c1.end()) == 3);
        assert(*next(c1.begin(), 0) == 10);
        assert(*next(c1.begin(), 1) == 11);
        assert(*next(c1.begin(), 2) == 12);
        assert(c1.get_allocator() == A(2));

        assert(distance(c2.begin(), c2.end()) == 0);
        assert(c2.get_allocator() == A(1));
    }
    {
        typedef int T;
        typedef other_allocator<T> A;
        typedef std::forward_list<T, A> C;
        C c1(A(1));
        C c2(A(2));
        c1.swap(c2);

        assert(distance(c1.begin(), c1.end()) == 0);
        assert(c1.get_allocator() == A(2));

        assert(distance(c2.begin(), c2.end()) == 0);
        assert(c2.get_allocator() == A(1));
    }
}
