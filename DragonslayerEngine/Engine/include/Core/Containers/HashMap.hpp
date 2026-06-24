#pragma once

#include <cstring>

#include "Core/Assert.hpp"
#include "Core/Hash.hpp"
#include "Core/Macros.hpp"
#include "Core/Memory.hpp"
#include "Core/Types.hpp"

/**
 * Unordered Hash Map
 * - Separates Bucket and KeyValuePair to remove the need to call default constructors
 * - Bucket and KeyValuePair are stored interleaved in memory as such:
 * |KeyValuePair|P|Bucket|P|KeyValuePair|P|Bucket|... the bucket is stored after the pair
 * since it is way more likely that the alignment of the pair is larger and thus result in less padding.
 * - Uses open addressing with Robin Hood hashing strategy
 * - Grows when load factor > 0.85
 * - Assumes keys and values are trivially movable or implement move constructor
 * TODO Add const iterator
 * TODO Revisit API, its definitely missing stuff like [] for example
 * TODO Revisit as the current probing logic could be improved
 * TODO Revisit the hash function
 * TODO Implement copy stuff
 */
template<typename KeyType, typename ElementType, typename Allocator, bool KeyIsHash = false>
class HashMap {

public:
    // Potentially non-trivially constructible
    struct KeyValuePair {
        ElementType value; // Element type comes first because it will likely have the largest alignment
        KeyType key;
        uint32 hash;

        template<typename K, typename E>
        KeyValuePair(K&& key, E&& value, uint32 hash)
            : value(std::forward<E>(value)),
              key(std::forward<K>(key)),
              hash(hash)
        {}
    };

private:

    // Intentionally a POD (or trivially constructible) so it can be memset to 0
    struct Bucket {
        bool occupied : 1;
    };

    struct PairBucket {
        KeyValuePair pair;
        Bucket bucket;
    };

    uint8* data; // Holds both Buckets and KeyValuePairs, interleaved as [KeyValuePair, Bucket] for better caching
    size_t paddingAfterPair; // There might be padding between KeyValuePair and Bucket if their alignment is different
    size_t stride; // How many bytes we need to jump between a pair of Bucket and KeyValuePair interleaved

    Allocator* allocator;
    uint32 capacity;
    uint32 size;

    static constexpr float MAX_LOAD_FACTOR = 0.85f;

public:

    HashMap(uint32 initialCapacity, Allocator*allocator) : allocator(allocator), capacity(initialCapacity), size(0)
    {
        ASSERT(initialCapacity > 0);
        paddingAfterPair = offsetof(PairBucket, bucket) - (sizeof(KeyValuePair) + offsetof(PairBucket, pair));
        stride = sizeof(PairBucket);
        data = allocator->Allocate(stride * capacity, alignof(PairBucket));
        memset(data, 0, stride * capacity);
    }

    // Copy constructor
    HashMap(const HashMap& other)
        : paddingAfterPair(other.paddingAfterPair),
          stride(other.stride),
          allocator(other.allocator),
          capacity(other.capacity),
          size(other.size)
    {
        ASSERT(allocator != nullptr);
        data = allocator->Allocate(stride * capacity, alignof(PairBucket));
        memset(data, 0, stride * capacity);

        for (uint32 i = 0; i < other.capacity; ++i) {
            const Bucket* srcBucket = other.GetBucketAt(other.data, i);
            if (!srcBucket->occupied) {
                continue;
            }

            const KeyValuePair* srcPair = other.GetKeyValuePairAt(other.data, i);
            new (GetKeyValuePairAt(i)) KeyValuePair(srcPair->key, srcPair->value, srcPair->hash);
            GetBucketAt(i)->occupied = true;
        }
    }

    // Move constructor
    HashMap(HashMap&& other) noexcept
        : data(other.data),
          paddingAfterPair(other.paddingAfterPair),
          stride(other.stride),
          allocator(other.allocator),
          capacity(other.capacity),
          size(other.size)
    {
        other.data = nullptr;
        other.allocator = nullptr;
        other.capacity = 0;
        other.size = 0;
    }

    // Copy assignment operator
    HashMap& operator=(const HashMap& other) {

        if (this == &other) {
            return *this;
        }

        Reset();

        if (capacity != other.capacity) {
            allocator->Free(data);
            capacity = other.capacity;
            paddingAfterPair = other.paddingAfterPair;
            stride = other.stride;
            data = allocator->Allocate(stride * capacity, alignof(PairBucket));
            memset(data, 0, stride * capacity);
        }
        // Allocator intentionally not propagated, assignment copies content, not ownership.

        for (uint32 i = 0; i < other.capacity; ++i) {
            const Bucket* srcBucket = other.GetBucketAt(other.data, i);
            if (!srcBucket->occupied) {
                continue;
            }

            const KeyValuePair* srcPair = other.GetKeyValuePairAt(other.data, i);
            new (GetKeyValuePairAt(i)) KeyValuePair(srcPair->key, srcPair->value, srcPair->hash);
            GetBucketAt(i)->occupied = true;
        }

        size = other.size;
        return *this;
    }

    // Move assignment operator
    HashMap& operator=(HashMap&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Reset();
        if (data != nullptr) {
            allocator->Free(data);
        }
        data = other.data;
        paddingAfterPair = other.paddingAfterPair;
        stride = other.stride;
        allocator = other.allocator;
        capacity = other.capacity;
        size = other.size;
        other.data = nullptr;
        other.allocator = nullptr;
        other.capacity = 0;
        other.size = 0;
        return *this;
    }

    ~HashMap() {
        Reset();
        if (data != nullptr) {
            allocator->Free(data);
            data = nullptr;
        }
    }

    NO_DISCARD FORCE_INLINE bool IsEmpty() const {
        return size == 0;
    }

    NO_DISCARD FORCE_INLINE uint32 GetSize() const {
        return size;
    }

    NO_DISCARD FORCE_INLINE uint32 GetCapacity() const {
        return capacity;
    }

    void Reset() {

        if constexpr (!std::is_trivially_destructible_v<KeyType> || !std::is_trivially_destructible_v<ElementType>) {
            for (uint32 i = 0; i < capacity; ++i) {

                if (GetBucketAt(i)->occupied) {
                    KeyValuePair& keyValuePair = *GetKeyValuePairAt(i);
                    keyValuePair.key.~KeyType();
                    keyValuePair.value.~ElementType();
                }

            }
        }

        // TODO This memset can probably be avoided
        // if we go into the previous condition, where we can set the occupied to flag to false directly
        memset(data, 0, stride * capacity);
        size = 0;
    }

    FORCE_INLINE uint32 ComputeHash(const KeyType& key) const {
        if constexpr (KeyIsHash) {
            return key;
        } else {
            return std::hash<KeyType>()(key);
        }
    }

    NO_DISCARD FORCE_INLINE ElementType& operator[](const KeyType& key) {
        return *Find(key);
    }

    NO_DISCARD FORCE_INLINE const ElementType& operator[](const KeyType& key) const {
        return *Find(key);
    }

    void Insert(const KeyType& key, const ElementType& value) {
        InsertInternal(key, value);
    }

    void Insert(KeyType&& key, ElementType&& value) {
        InsertInternal(std::move(key), std::move(value));
    }

    template<typename... Args>
    ElementType& Emplace(const KeyType& key, Args&&... args) {
        // New element gets moved to insert
        ElementType newElement = ElementType(std::forward<Args>(args)...);
        // TODO Consider if we should assert when returned element is null
        return *InsertInternal(key, newElement);
    }

    bool Remove(const KeyType& key) {

        uint32 hash = ComputeHash(key);
        uint32 index = hash % capacity;

        for (uint32 i = 0; i < capacity; ++i) {

            Bucket& bucket = *GetBucketAt(index);

            if (!bucket.occupied) {
                continue;
            }

            KeyValuePair& keyValuePair = *GetKeyValuePairAt(index);

            if (keyValuePair.hash == hash && keyValuePair.key == key) {

                if constexpr (!std::is_trivially_destructible_v<KeyType>) {
                    keyValuePair.key.~KeyType();
                }

                if constexpr (!std::is_trivially_destructible_v<ElementType>) {
                    keyValuePair.value.~ElementType();
                }

                bucket.occupied = false;
                --size;

                return true;
            }

            index = (index + 1) % capacity;
        }
        return false;
    }

    NO_DISCARD ElementType* Find(const KeyType& key) {
        return FindByHash(key, ComputeHash(key));
    }

    NO_DISCARD ElementType* Find(const KeyType& key) const {
        return FindByHash(key, ComputeHash(key));
    }

    NO_DISCARD bool Contains(const KeyType& key) const
    {
        // TODO This function is currently almost identical to Find,
        // So consider calling find instead and check for nullptr

        uint32 hash = ComputeHash(key);
        uint32 index = hash % capacity;

        for (uint32 i = 0; i < capacity; ++i) {

            const Bucket& bucket = *GetBucketAt(index);

            if (!bucket.occupied) {
                continue;
            }

            const KeyValuePair& keyValuePair = *GetKeyValuePairAt(index);

            if (keyValuePair.hash == hash && keyValuePair.key == key) {
                return true;
            }

            index = (index + 1) % capacity;
        }

        return false;
    }

private:
    NO_DISCARD FORCE_INLINE Bucket* GetBucketAt(uint32 index) {
        return reinterpret_cast<Bucket*>(data + index * stride + sizeof(KeyValuePair) + paddingAfterPair);
    }

    NO_DISCARD FORCE_INLINE Bucket* GetBucketAt(uint8* hashMapData, uint32 index) {
        return reinterpret_cast<Bucket*>(hashMapData + index * stride + sizeof(KeyValuePair) + paddingAfterPair);
    }

    NO_DISCARD FORCE_INLINE KeyValuePair* GetKeyValuePairAt(uint32 index) {
        return reinterpret_cast<KeyValuePair*>(data + index * stride);
    }

    NO_DISCARD FORCE_INLINE KeyValuePair* GetKeyValuePairAt(uint8* hashMapData, uint32 index) {
        return reinterpret_cast<KeyValuePair*>(hashMapData + index * stride);
    }

    NO_DISCARD FORCE_INLINE const Bucket* GetBucketAt(uint32 index) const {
        return reinterpret_cast<const Bucket*>(data + index * stride + sizeof(KeyValuePair) + paddingAfterPair);
    }

    NO_DISCARD FORCE_INLINE const Bucket* GetBucketAt(uint8* hashMapData, uint32 index) const {
        return reinterpret_cast<const Bucket*>(hashMapData + index * stride + sizeof(KeyValuePair) + paddingAfterPair);
    }

    NO_DISCARD FORCE_INLINE const KeyValuePair* GetKeyValuePairAt(uint32 index) const {
        return reinterpret_cast<const KeyValuePair*>(data + index * stride);
    }

    NO_DISCARD FORCE_INLINE const KeyValuePair* GetKeyValuePairAt(uint8* hashMapData, uint32 index) const {
        return reinterpret_cast<const KeyValuePair*>(hashMapData + index * stride);
    }

    NO_DISCARD ElementType* FindByHash(const KeyType& key, uint32 hash) {

        uint32 index = hash % capacity;

        for (uint32 i = 0; i < capacity; ++i) {

            Bucket& bucket = *GetBucketAt(index);

            if (!bucket.occupied) {
                continue;
            }

            KeyValuePair& keyValuePair = *GetKeyValuePairAt(index);

            if (keyValuePair.hash == hash && keyValuePair.key == key) {
                return &keyValuePair.value;
            }

            index = (index + 1) % capacity;
        }

        return nullptr;
    }

    NO_DISCARD FORCE_INLINE uint32 ProbeDistance(uint32 hash, uint32 index) const {
        const uint32 idealIndex = hash % capacity;
        return (index + capacity - idealIndex) % capacity;
    }

    template<typename K, typename E>
    ElementType* InsertInternal(K&& key, E&& value) {

        uint32 hash = ComputeHash(key);

        // Check for duplicate before growing
        if (FindByHash(key, hash) != nullptr) {
            ASSERT(false, "HashMap::Insert - Key already exists");
            return nullptr;
        }

        if (static_cast<float>(size + 1) / static_cast<float>(capacity) > MAX_LOAD_FACTOR) {
            Grow(capacity * 2);
        }

        return InsertInternalNoGrow(std::forward<K>(key), std::forward<E>(value), hash);
    }

    template<typename K, typename E>
    ElementType* InsertInternalNoGrow(K&& key, E&& value, uint32 hash) {

        uint32 index = hash % capacity;
        uint32 distance = 0;

        KeyValuePair newPair (std::forward<K>(key), std::forward<E>(value), hash);

        ElementType* insertedElement = nullptr;

        for (int32 i = 0; i < capacity; ++i) {

            Bucket& bucket = *GetBucketAt(index);

            // If bucket is free just place it here
            if (!bucket.occupied) {
                bucket.occupied = true;
                KeyValuePair* newKeyValuePair = new (GetKeyValuePairAt(index)) KeyValuePair(std::move(newPair));
                ++size;
                return insertedElement != nullptr ? insertedElement : &newKeyValuePair->value;
            }

            KeyValuePair& existingPair = *GetKeyValuePairAt(index);

            // Bucket is not free so compare probe distance
            uint32 existingDistance = ProbeDistance(existingPair.hash, index);
            if (existingDistance < distance) {
                std::swap(existingPair, newPair);
                distance = existingDistance;

                if (insertedElement == nullptr)
                {
                    insertedElement = &existingPair.value;
                }
            }

            ++distance;
            index = (index + 1) % capacity;
        }

        return insertedElement;
    }

    void Grow(uint32 newCapacity) {

        if (newCapacity <= capacity)
            return;

        // Save old data
        uint8* oldData = data;
        uint32 oldCapacity = capacity;

        // Allocate new memory
        data = allocator->Allocate(stride * newCapacity, alignof(PairBucket));
        memset(data, 0, stride * newCapacity);

        uint32 oldSize = size;
        size = 0;
        capacity = newCapacity;

        // Re-insert old elements into new table
        for (uint32 i = 0; i < oldCapacity; ++i) {

            Bucket* oldBucket = GetBucketAt(oldData, i);

            if (!oldBucket->occupied) continue;

            KeyValuePair* oldPair = GetKeyValuePairAt(oldData, i);

            InsertInternalNoGrow(std::move(oldPair->key), std::move(oldPair->value), oldPair->hash);

            // Explicitly destroy the old pair
            if constexpr (!std::is_trivially_destructible_v<KeyType>)
                oldPair->key.~KeyType();
            if constexpr (!std::is_trivially_destructible_v<ElementType>)
                oldPair->value.~ElementType();
        }

        // Free old memory
        allocator->Free(oldData);

        // Sanity check
        ASSERT(size == oldSize);
    }

public:
    struct Iterator {

        using iterator_category = std::forward_iterator_tag;
        using value_type        = KeyValuePair;
        using difference_type   = std::ptrdiff_t;
        using pointer           = KeyValuePair*;
        using reference         = KeyValuePair&;

        HashMap* map;
        uint32 index;

        Iterator() : map(nullptr), index(0) {}

        Iterator(HashMap& map, uint32 startIndex) : map(&map), index(startIndex) {
            SkipEmpty();
        }

        void SkipEmpty() {
            while (index < map->capacity && !map->GetBucketAt(index)->occupied) {
                ++index;
            }
        }

        Iterator& operator++() {
            ++index;
            SkipEmpty();
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        NO_DISCARD bool operator==(const Iterator& other) const { return index == other.index; }
        NO_DISCARD bool operator!=(const Iterator& other) const { return index != other.index; }

        NO_DISCARD reference operator*() const { return *map->GetKeyValuePairAt(index); }
        NO_DISCARD pointer operator->() const { return  map->GetKeyValuePairAt(index); }
    };

    struct ConstIterator {

        using iterator_category = std::forward_iterator_tag;
        using value_type        = KeyValuePair;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const KeyValuePair*;
        using reference         = const KeyValuePair&;

        const HashMap* map;
        uint32 index;

        ConstIterator() : map(nullptr), index(0) {}

        ConstIterator(const HashMap& map, uint32 startIndex) : map(&map), index(startIndex) {
            SkipEmpty();
        }

        ConstIterator(const Iterator& other) : map(other.map), index(other.index) {
            SkipEmpty();
        }

        void SkipEmpty() {
            while (map != nullptr && index < map->capacity && !map->GetBucketAt(index)->occupied) {
                ++index;
            }
        }

        ConstIterator& operator++() {
            ++index;
            SkipEmpty();
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        NO_DISCARD bool operator==(const ConstIterator& other) const { return index == other.index; }
        NO_DISCARD bool operator!=(const ConstIterator& other) const { return index != other.index; }

        NO_DISCARD reference operator*() const { return *map->GetKeyValuePairAt(index); }
        NO_DISCARD pointer operator->() const { return  map->GetKeyValuePairAt(index); }
    };

    NO_DISCARD Iterator begin() { return Iterator(*this, 0); }
    NO_DISCARD Iterator end() { return Iterator(*this, capacity); }

    NO_DISCARD ConstIterator begin() const { return ConstIterator(*this, 0); }
    NO_DISCARD ConstIterator end() const { return ConstIterator(*this, capacity); }

    NO_DISCARD ConstIterator cbegin() const { return ConstIterator(*this, 0); }
    NO_DISCARD ConstIterator cend() const { return ConstIterator(*this, capacity); }
};

/* TODO
template<uint32 ElementSize, size_t Alignment, uint32 Size>
using InlineHashMapAllocator = InlineLinearAllocator<ElementSize * Size, Alignment>;

template<typename KeyType, typename ElementType, uint32 Size>
class InlineHashMap :
    private InlineHashMapAllocator<HashMap<KeyType, ElementType>::Stride(), Size>,
    public HashMap<ElementType, InlineArrayAllocator<ElementType, Size>> {

public:
    InlineArray() : Array<ElementType, InlineArrayAllocator<ElementType, Size>>(Size, *this) {}

    template<typename... Args>
    explicit InlineArray(Args&&... args) : Array<ElementType, InlineArrayAllocator<ElementType, Size>>(Size, *this, std::forward<Args>(args)...) {}
};*/
