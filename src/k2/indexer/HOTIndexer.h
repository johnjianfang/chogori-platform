/*
MIT License

Copyright(c) 2020 Futurewei Cloud

    Permission is hereby granted,
    free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

    The above copyright notice and this permission notice shall be included in all copies
    or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS",
    WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER
    LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

#include "IndexerInterface.h"

#include <k2/common/Common.h>
#include <hot/singlethreaded/HOTSingleThreaded.hpp>
#include <idx/contenthelpers/IdentityKeyExtractor.hpp>

namespace k2
{

template <typename ValueType>
struct KeyValuePair {
    String key;
    std::unique_ptr<VersionedTreeNode<ValueType>> value;
};

template<typename ValueType>
struct KeyValuePairKeyExtractor {
    inline size_t getKeyLength(ValueType const &value) const {
        return value->key.length();
    }
    inline const char* operator()(ValueType const &value) const {
        return value->key.c_str();
    }
};

template <typename ValueType>
class HOTIndexer {
    using KeyValuePairTrieType = hot::singlethreaded::HOTSingleThreaded<KeyValuePair<ValueType>*, KeyValuePairKeyExtractor>;
    KeyValuePairTrieType m_keyValuePairTrie;
public:
 void insert(String key, ValueType value, uint64_t version);

 VersionedTreeNode<ValueType>* find(const String& key, uint64_t version);

 void trim(const String& key, uint64_t version);
};

template <typename ValueType>
inline void HOTIndexer<ValueType>::insert(String key, ValueType value, uint64_t version) {
    std::unique_ptr < VersionedTreeNode<ValueType>> newNode(new VersionedTreeNode<ValueType>());
    newNode->value = std::move(value);
    newNode->version = version;

    KeyValuePair<ValueType>* keyValuePair = new KeyValuePair<ValueType>();
    keyValuePair->key = std::move(key);
    keyValuePair->value = std::move(newNode);

    auto inserted = m_keyValuePairTrie.insert(keyValuePair, keyValuePair->key.length());
    //  Value already exists
    if (!inserted) {
        auto exist = m_keyValuePairTrie.lookup(keyValuePair->key.c_str(), keyValuePair->key.length());
        newNode->next = std::move(exist.mValue->value);
        exist.mValue->value = std::move(newNode);
    }
}

template <typename ValueType>
inline VersionedTreeNode<ValueType>* HOTIndexer<ValueType>::find(const String& key, uint64_t version) {
    //
    // Bug: sometimes find() cannot find existing key
    //
    // auto it = m_keyValuePairTrie.find(key.c_str());
    // if (it == m_keyValuePairTrie.end()) {
    //     return nullptr;
    // }

    // VersionedTreeNode* node = (*it)->value.get();

    auto it = m_keyValuePairTrie.lookup(key.c_str(), key.length());
    if (!it.mIsValid) {
        return nullptr;
    }

    VersionedTreeNode<ValueType>* node = it.mValue->value.get();
    while (node && node->version > version) {
        node = node->next.get();
    }
    return node;
}

template <typename ValueType>
inline void HOTIndexer<ValueType>::trim(const String& key, uint64_t version) {
    if (version == std::numeric_limits<uint64_t>::max()) {
        auto it = m_keyValuePairTrie.lookup(key.c_str(), key.length());
        if (!it.mIsValid) {
            return;
        }
        m_keyValuePairTrie.remove(key.c_str(), key.length());
        return;
    }

    auto it = m_keyValuePairTrie.lookup(key.c_str(), key.length());
    if (!it.mIsValid) {
        return;
    }

    VersionedTreeNode<ValueType>* node = it.mValue->value.get();
    if (node) {
        node = node->next.get();
    }
    while (node && node->version >= version) {
        node = node->next.get();
    }
    node->next = nullptr;
}

}
