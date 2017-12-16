/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

//===----------------------------------------------------------------------===//
// Pragmas
//===----------------------------------------------------------------------===//

// Unreferenced formal parameter
#pragma warning(disable: 4100)
// Hides class member
#pragma warning(disable: 4458)
// std::uninitialized_copy::_Unchecked_iterators::_Deprecate
#pragma warning(disable: 4996)

//===----------------------------------------------------------------------===//
// JUCE
//===----------------------------------------------------------------------===//

#if !defined JUCE_ANDROID
#   define JUCE_USE_FREETYPE_AMALGAMATED 1
#   define JUCE_AMALGAMATED_INCLUDE 1
#endif

#include "JuceHeader.h"

//===----------------------------------------------------------------------===//
// SparsePP
//===----------------------------------------------------------------------===//

#include "../../ThirdParty/SparseHashMap/sparsepp/spp.h"

template <class Key, class T, class HashFcn = spp::spp_hash<Key>, class EqualKey = std::equal_to<Key>>
using SparseHashMap = spp::sparse_hash_map<Key, T, HashFcn, EqualKey>;

template <class Value, class HashFcn = spp::spp_hash<Value>, class EqualKey = std::equal_to<Value>>
using SparseHashSet = spp::sparse_hash_set<Value, HashFcn, EqualKey>;

typedef size_t HashCode;

#if !defined HASH_CODE_MAX
#   define HASH_CODE_MAX SIZE_MAX
#endif

struct StringHash
{
    inline HashCode operator()(const juce::String &key) const noexcept
    {
        return static_cast<HashCode>(key.hashCode()) % HASH_CODE_MAX;
    }
};

//===----------------------------------------------------------------------===//
// Various helpers
//===----------------------------------------------------------------------===//

template <class T>
using UniquePointer = std::unique_ptr<T>;

#if _MSC_VER
inline float roundf(float x)
{
    return (x >= 0.0f) ? floorf(x + 0.5f) : ceilf(x - 0.5f);
}
#endif

#if !defined M_PI
#   define M_PI 3.14159265358979323846
#endif

#if !defined M_PI_2
#   define M_PI_2 1.57079632679489661923
#endif

#if !defined M_2PI
#   define M_2PI 6.283185307179586476
#endif

#if JUCE_ANDROID || JUCE_IOS
#   define HELIO_MOBILE 1
#else
#   define HELIO_DESKTOP 1
#endif

//===----------------------------------------------------------------------===//
// Internationalization
//===----------------------------------------------------------------------===//

#include "TranslationManager.h"

#if defined TRANS
#   undef TRANS
#endif

#define TRANS(stringLiteral) TranslationManager::getInstance().translate(stringLiteral)
#define TRANS_PLURAL(stringLiteral, intValue) TranslationManager::getInstance().translate(stringLiteral, intValue)
