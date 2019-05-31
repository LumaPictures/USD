//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
// Wrap one of various unordered set implementations.  The exposed API
// is similar to std::unordered_set but is missing some methods and
// erase(const_iterator) returns nothing.
//
// Wrapping provides a convenient way to switch between implementations.
// The GNU extension (currently) has the best overall performance but
// isn't standard.  Otherwise we use the C++11 standard implementation.

#ifndef TF_HASHSET_H
#define TF_HASHSET_H

#include "pxr/pxr.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

template<class Key, class HashFn = std::hash<Key>,
	 class EqualKey = std::equal_to<Key>,
         class Alloc = std::allocator<Key> >
class TfHashSet :
    private std::unordered_set<Key, HashFn, EqualKey, Alloc> {
    typedef std::unordered_set<Key, HashFn, EqualKey, Alloc> _Base;
public:
    typedef typename _Base::key_type key_type;
    typedef typename _Base::value_type value_type;
    typedef typename _Base::hasher hasher;
    typedef typename _Base::key_equal key_equal;
    typedef typename _Base::size_type size_type;
    typedef typename _Base::difference_type difference_type;
    typedef typename _Base::pointer pointer;
    typedef typename _Base::const_pointer const_pointer;
    typedef typename _Base::reference reference;
    typedef typename _Base::const_reference const_reference;
    typedef typename _Base::iterator iterator;
    typedef typename _Base::const_iterator const_iterator;
    typedef typename _Base::allocator_type allocator_type;
    // No local_iterator nor any methods using them.

    TfHashSet() : _Base() { }
    explicit
    TfHashSet(size_type n, const hasher& hf = hasher(),
              const key_equal& eql = key_equal(),
              const allocator_type& alloc = allocator_type()) :
        _Base(n, hf, eql, alloc) { }
    explicit
    TfHashSet(const allocator_type& alloc) : _Base(alloc) { }
    template<class InputIterator>
    TfHashSet(InputIterator first, InputIterator last,
              size_type n = 0, const hasher& hf = hasher(),
              const key_equal& eql = key_equal(),
              const allocator_type& alloc = allocator_type()) :
        _Base(first, last, n, hf, eql, alloc) { }
    TfHashSet(const TfHashSet& other) : _Base(other) { }

    TfHashSet& operator=(const TfHashSet& rhs) {
        _Base::operator=(rhs);
        return *this;
    }

    iterator begin() { return _Base::begin(); }
    const_iterator begin() const { return _Base::begin(); }
    // using _Base::bucket;
    using _Base::bucket_count;
    using _Base::bucket_size;
    const_iterator cbegin() const { return _Base::cbegin(); }
    const_iterator cend() const { return _Base::cend(); }
    using _Base::clear;
    using _Base::count;
    using _Base::empty;
    iterator end() { return _Base::end(); }
    const_iterator end() const { return _Base::end(); }
    using _Base::equal_range;
    size_type erase(const key_type& key) { return _Base::erase(key); }
    void erase(const_iterator position) { _Base::erase(position); }
    void erase(const_iterator first, const_iterator last) {
        _Base::erase(first, last);
    }
    using _Base::find;
    using _Base::get_allocator;
    using _Base::hash_function;
    std::pair<iterator, bool> insert(const value_type& v) {
        return _Base::insert(v);
    }
    iterator insert(const_iterator hint, const value_type& v) {
        return _Base::insert(hint, v);
    }
    template<class InputIterator>
    void insert(InputIterator first, InputIterator last) {
        _Base::insert(first, last);
    }
    using _Base::key_eq;
    using _Base::load_factor;
    using _Base::max_bucket_count;
    using _Base::max_load_factor;
    // using _Base::max_load_factor;
    using _Base::max_size;
    using _Base::rehash;
    using _Base::reserve;
    using _Base::size;
    void swap(TfHashSet& other) { _Base::swap(other); }

    template<class Key2, class HashFn2, class EqualKey2, class Alloc2>
    friend bool
    operator==(const TfHashSet<Key2, HashFn2, EqualKey2, Alloc2>&,
               const TfHashSet<Key2, HashFn2, EqualKey2, Alloc2>&);
};

template<class Key, class HashFn = std::hash<Key>,
	 class EqualKey = std::equal_to<Key>,
         class Alloc = std::allocator<Key> >
class TfHashMultiSet :
    private std::unordered_multiset<Key, HashFn, EqualKey, Alloc> {
    typedef std::unordered_multiset<Key, HashFn, EqualKey, Alloc> _Base;
public:
    typedef typename _Base::key_type key_type;
    typedef typename _Base::value_type value_type;
    typedef typename _Base::hasher hasher;
    typedef typename _Base::key_equal key_equal;
    typedef typename _Base::size_type size_type;
    typedef typename _Base::difference_type difference_type;
    typedef typename _Base::pointer pointer;
    typedef typename _Base::const_pointer const_pointer;
    typedef typename _Base::reference reference;
    typedef typename _Base::const_reference const_reference;
    typedef typename _Base::iterator iterator;
    typedef typename _Base::const_iterator const_iterator;
    typedef typename _Base::allocator_type allocator_type;
    // No local_iterator nor any methods using them.

    TfHashMultiSet() : _Base() { }
    explicit
    TfHashMultiSet(size_type n, const hasher& hf = hasher(),
                   const key_equal& eql = key_equal(),
                   const allocator_type& alloc = allocator_type()) :
        _Base(n, hf, eql, alloc) { }
    explicit
    TfHashMultiSet(const allocator_type& alloc) : _Base(alloc) { }
    template<class InputIterator>
    TfHashMultiSet(InputIterator first, InputIterator last,
                   size_type n = 0, const hasher& hf = hasher(),
                   const key_equal& eql = key_equal(),
                   const allocator_type& alloc = allocator_type()) :
        _Base(first, last, n, hf, eql, alloc) { }
    TfHashMultiSet(const TfHashMultiSet& other) : _Base(other) { }

    TfHashMultiSet& operator=(const TfHashMultiSet& rhs) {
        _Base::operator=(rhs);
        return *this;
    }

    iterator begin() { return _Base::begin(); }
    const_iterator begin() const { return _Base::begin(); }
    // using _Base::bucket;
    using _Base::bucket_count;
    using _Base::bucket_size;
    const_iterator cbegin() const { return _Base::cbegin(); }
    const_iterator cend() const { return _Base::cend(); }
    using _Base::clear;
    using _Base::count;
    using _Base::empty;
    iterator end() { return _Base::end(); }
    const_iterator end() const { return _Base::end(); }
    using _Base::equal_range;
    size_type erase(const key_type& key) { return _Base::erase(key); }
    void erase(const_iterator position) { _Base::erase(position); }
    void erase(const_iterator first, const_iterator last) {
        _Base::erase(first, last);
    }
    using _Base::find;
    using _Base::get_allocator;
    using _Base::hash_function;
    iterator insert(const value_type& v) {
        return _Base::insert(v);
    }
    iterator insert(const_iterator hint, const value_type& v) {
        return _Base::insert(hint, v);
    }
    template<class InputIterator>
    void insert(InputIterator first, InputIterator last) {
        _Base::insert(first, last);
    }
    using _Base::key_eq;
    using _Base::load_factor;
    using _Base::max_bucket_count;
    using _Base::max_load_factor;
    // using _Base::max_load_factor;
    using _Base::max_size;
    using _Base::rehash;
    using _Base::reserve;
    using _Base::size;
    void swap(TfHashMultiSet& other) { _Base::swap(other); }

    template<class Key2, class HashFn2, class EqualKey2, class Alloc2>
    friend bool
    operator==(const TfHashMultiSet<Key2, HashFn2, EqualKey2, Alloc2>&,
               const TfHashMultiSet<Key2, HashFn2, EqualKey2, Alloc2>&);
};

template<class Key, class HashFn, class EqualKey, class Alloc>
inline void
swap(TfHashSet<Key, HashFn, EqualKey, Alloc>& lhs,
     TfHashSet<Key, HashFn, EqualKey, Alloc>& rhs)
{
    lhs.swap(rhs);
}

template<class Key, class HashFn, class EqualKey, class Alloc>
inline bool
operator==(const TfHashSet<Key, HashFn, EqualKey, Alloc>& lhs,
           const TfHashSet<Key, HashFn, EqualKey, Alloc>& rhs)
{
    typedef typename TfHashSet<Key, HashFn, EqualKey, Alloc>::_Base _Base;
    return static_cast<const _Base&>(lhs) == static_cast<const _Base&>(rhs);
}

template<class Key, class HashFn, class EqualKey, class Alloc>
inline bool
operator!=(const TfHashSet<Key, HashFn, EqualKey, Alloc>& lhs,
           const TfHashSet<Key, HashFn, EqualKey, Alloc>& rhs)
{
    return !(lhs == rhs);
}

template<class Key, class HashFn, class EqualKey, class Alloc>
inline void
swap(TfHashMultiSet<Key, HashFn, EqualKey, Alloc>& lhs,
     TfHashMultiSet<Key, HashFn, EqualKey, Alloc>& rhs)
{
    lhs.swap(rhs);
}

template<class Key, class HashFn, class EqualKey, class Alloc>
inline bool
operator==(const TfHashMultiSet<Key, HashFn, EqualKey, Alloc>& lhs,
           const TfHashMultiSet<Key, HashFn, EqualKey, Alloc>& rhs)
{
    typedef typename TfHashMultiSet<Key, HashFn, EqualKey, Alloc>::_Base _Base;
    return static_cast<const _Base&>(lhs) == static_cast<const _Base&>(rhs);
}

template<class Key, class HashFn, class EqualKey, class Alloc>
inline bool
operator!=(const TfHashMultiSet<Key, HashFn, EqualKey, Alloc>& lhs,
           const TfHashMultiSet<Key, HashFn, EqualKey, Alloc>& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_HASHSET_H
