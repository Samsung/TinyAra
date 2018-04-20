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

// istreambuf_iterator

// template <class charT, class traits>
//   bool operator==(const istreambuf_iterator<charT,traits>& a,
//                   const istreambuf_iterator<charT,traits>& b);

#include <iterator>
#include <sstream>
#include <cassert>

int main()
{
    {
        std::istringstream inf1("abc");
        std::istringstream inf2("def");
        std::istreambuf_iterator<char> i1(inf1);
        std::istreambuf_iterator<char> i2(inf2);
        std::istreambuf_iterator<char> i3;
        std::istreambuf_iterator<char> i4;
        std::istreambuf_iterator<char> i5(nullptr);

        assert( (i1 == i1));
        assert( (i1 == i2));
        assert(!(i1 == i3));
        assert(!(i1 == i4));
        assert(!(i1 == i5));

        assert( (i2 == i1));
        assert( (i2 == i2));
        assert(!(i2 == i3));
        assert(!(i2 == i4));
        assert(!(i2 == i5));

        assert(!(i3 == i1));
        assert(!(i3 == i2));
        assert( (i3 == i3));
        assert( (i3 == i4));
        assert( (i3 == i5));

        assert(!(i4 == i1));
        assert(!(i4 == i2));
        assert( (i4 == i3));
        assert( (i4 == i4));
        assert( (i4 == i5));

        assert(!(i5 == i1));
        assert(!(i5 == i2));
        assert( (i5 == i3));
        assert( (i5 == i4));
        assert( (i5 == i5));
    }
    {
        std::wistringstream inf1(L"abc");
        std::wistringstream inf2(L"def");
        std::istreambuf_iterator<wchar_t> i1(inf1);
        std::istreambuf_iterator<wchar_t> i2(inf2);
        std::istreambuf_iterator<wchar_t> i3;
        std::istreambuf_iterator<wchar_t> i4;
        std::istreambuf_iterator<wchar_t> i5(nullptr);

        assert( (i1 == i1));
        assert( (i1 == i2));
        assert(!(i1 == i3));
        assert(!(i1 == i4));
        assert(!(i1 == i5));

        assert( (i2 == i1));
        assert( (i2 == i2));
        assert(!(i2 == i3));
        assert(!(i2 == i4));
        assert(!(i2 == i5));

        assert(!(i3 == i1));
        assert(!(i3 == i2));
        assert( (i3 == i3));
        assert( (i3 == i4));
        assert( (i3 == i5));

        assert(!(i4 == i1));
        assert(!(i4 == i2));
        assert( (i4 == i3));
        assert( (i4 == i4));
        assert( (i4 == i5));

        assert(!(i5 == i1));
        assert(!(i5 == i2));
        assert( (i5 == i3));
        assert( (i5 == i4));
        assert( (i5 == i5));
    }
}
