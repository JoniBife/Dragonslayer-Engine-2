#pragma once

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <type_traits>

#include "ContainerView.hpp"
#include "Core/Allocators/InlineLinearAllocator.hpp"
#include "Core/Allocators/LinearAllocator.hpp"
#include "Core/Assert.hpp"
#include "Core/IO/File.hpp"
#include "Core/Macros.hpp"
#include "Core/Types.hpp"

/**
 * Array container (similar to std::vector)
 * TODO Decide the growth strategy (exponential, linear, etc..)
 * TODO Revisit how growth works, currently we just allocate again, move and free previous
 * we could have a smart reallocate that checks if the allocator has enough space ahead and this way
 * we don't have to move the elements just extend the allocation
 * - Only supports types that have a move constructor or are trivially movable
 * TODO Consider add constexpr initialization list for use with inline array
 */
template<typename ElementType, typename Allocator>
class Array {

protected:
    ElementType* elements;
    Allocator* allocator;
    uint32 size;
    uint32 capacity;

public:
    Array(uint32 initialCapacity, Allocator* allocator) : allocator(allocator), size(0), capacity(initialCapacity)
    {
        // TODO We should probably round the capacity to a value that is more appropriate
        ASSERT(initialCapacity > 0);
        ASSERT(allocator != nullptr);
        elements = reinterpret_cast<ElementType*>(allocator->Allocate(sizeof(ElementType) * capacity, alignof(ElementType)));
    }

    template<typename... Args>
    Array(uint32 size, Allocator* allocator, Args&&... args) : allocator(allocator), size(size), capacity(size)
    {
        // TODO We should probably round the capacity to a value that is more appropriate

        ASSERT(size > 0);
        ASSERT(allocator != nullptr);

        elements = reinterpret_cast<ElementType*>(allocator->Allocate(sizeof(ElementType) * capacity, alignof(ElementType)));

        for (uint32 i = 0; i < size; i++) {
            new (&elements[i]) ElementType(std::forward<Args>(args)...);
        }
    }

    Array(std::initializer_list<ElementType> initializerList, Allocator* allocator) :
        allocator(allocator),
        size(static_cast<uint32>(initializerList.size())),
        capacity(static_cast<uint32>(initializerList.size()))
    {

        ASSERT(size > 0);

        elements = reinterpret_cast<ElementType*>(allocator->Allocate(sizeof(ElementType) * capacity, alignof(ElementType)));

        uint32 i = 0;
        for (const ElementType& element : initializerList) {
            new (&elements[i]) ElementType(element);
            i++;
        }
    }

    // Copy constructor
    Array(const Array& other) : allocator(other.allocator), size(other.size), capacity(other.capacity)
    {
        elements = reinterpret_cast<ElementType*>(allocator->Allocate(sizeof(ElementType) * capacity, alignof(ElementType)));

        if constexpr (std::is_trivially_copyable_v<ElementType>) {
            memcpy(elements, other.elements, sizeof(ElementType) * size);
        } else {
            for (uint32 i = 0; i < size; ++i) {
                new (&elements[i]) ElementType(other.elements[i]);
            }
        }

    }

    // Move constructor
    Array(Array&& other) noexcept : elements(other.elements), allocator(other.allocator), size(other.size), capacity(other.capacity)
    {
        other.elements = nullptr;
        other.size = 0;
        other.capacity = 0;
        other.allocator = nullptr;
    }

    // Copy assignment operator
    Array& operator=(const Array& other)
    {
        if (this == &other) {
            return *this;
        }

        // Destroy current contents
        Reset();

        if (other.IsEmpty()) {
            return *this;
        }

        // No need to copy capacity, only do it
        // when current is not large enough to fit
        if (capacity < other.size) {
            Grow(other.size);
        }

        size = other.size;

        // Never propagate allocator!
        // Copy assignment is about copying the elements not the rest!
        //allocator = other.allocator;

        if constexpr (std::is_trivially_copyable_v<ElementType>) {
            memcpy(elements, other.elements, sizeof(ElementType) * size);
        } else {
            for (uint32 i = 0; i < size; ++i) {
                new (&elements[i]) ElementType(other.elements[i]);
            }
        }

        return *this;
    }

    // Move assignment operator
    Array& operator=(Array&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        // Destroy current contents
        Reset();
        if (elements) {
            allocator->Free(reinterpret_cast<uint8*>(elements));
        }

        elements = other.elements;
        allocator = other.allocator;
        size = other.size;
        capacity = other.capacity;

        other.elements = nullptr;
        other.size = 0;
        other.capacity = 0;
        other.allocator = nullptr;

        return *this;
    }

    ~Array() {
        Reset(); // Calling reset to call the destructors
        if (elements) {
            allocator->Free(reinterpret_cast<uint8*>(elements));
            elements = nullptr;
        }
    }

    NO_DISCARD FORCE_INLINE ElementType& operator[](uint32 index) {
        ASSERT(index < size);
        return elements[index];
    }

    NO_DISCARD FORCE_INLINE const ElementType& operator[](uint32 index) const {
        ASSERT(index < size);
        return elements[index];
    }

    NO_DISCARD FORCE_INLINE ElementType& GetLast() {
        ASSERT(size > 0);
        return elements[size - 1];
    }

    NO_DISCARD FORCE_INLINE const ElementType& GetLast() const {
        ASSERT(size > 0);
        return elements[size - 1];
    }

    uint32 Add(const ElementType& element) {
        if (size == capacity) {
            Grow(size * 2);
        }
        new (&elements[size]) ElementType(element);
        ++size;
        return size - 1;
    }

    uint32 Add(ElementType&& element) {
        if (size == capacity) {
            Grow(size * 2);
        }
        new (&elements[size]) ElementType(std::move(element));
        ++size;
        return size - 1;
    }

    template<typename... Args>
    ElementType& Emplace(Args&&... args) {
        if (size == capacity) {
            Grow(size * 2);
        }
        new (&elements[size]) ElementType(std::forward<Args>(args)...);
        ++size;
        return elements[size - 1];
    }

    bool RemoveAt(uint32 index) {

        // TODO Should we assert here?

        if (index >= size) {
            return false;
        }

        elements[index].~ElementType();

        --size;

        // Shift elements down to fill the gap
        if constexpr (std::is_trivially_copyable_v<ElementType>) {
            if (index < size) {
                memmove(&elements[index], &elements[index + 1], sizeof(ElementType) * (size - index));
                // TODO Should we maybe memset to 0 here?
            }
        } else {
            for (uint32 i = index; i < size; ++i) {
                // Call move operator and destructor on all elements
                new (&elements[i]) ElementType(std::move(elements[i + 1]));
                elements[i + 1].~ElementType();
            }
        }

        return true;
    }

    bool Remove(const ElementType& element) {

        for (uint32 i = 0; i < size; ++i) {
            if (elements[i] == element) {
                return RemoveAt(i);
            }
        }

        return false;
    }

    bool RemoveAndSwapAt(uint32 index) {

        // TODO Should we assert here?

        if (index >= size) {
            return false;
        }

        elements[index].~ElementType();

        --size;

        if (index < size) {
            if constexpr (std::is_trivially_copyable_v<ElementType>) {
                memmove(&elements[index], &elements[size], sizeof(ElementType));
            } else {
                // Call move operator and destructor on swapped element
                new (&elements[index]) ElementType(std::move(elements[size]));
                elements[size].~ElementType();
            }
        }

        return true;
    }

    bool RemoveAndSwap(const ElementType& element) {

        for (uint32 i = 0; i < size; ++i) {
            if (elements[i] == element) {
                return RemoveAndSwapAt(i);
            }
        }

        return false;
    }

    FORCE_INLINE bool RemoveLast() {
        return RemoveAndSwapAt(size - 1);
    }

    void Reset() {
        if constexpr (!std::is_trivially_destructible_v<ElementType>) {
            for (uint32 i = 0; i < size; ++i) {
                elements[i].~ElementType();
            }
        }
        size = 0;
    }

    template<typename... Args>
    ElementType* EmplaceMany(uint32 numOfElements, Args&&... args) {

        if (numOfElements == 0) {
            return nullptr;
        }

        const uint32 newSize = size + numOfElements;

        Grow(newSize);

        for (uint32 i = size; i < newSize; i++) {
            new (&elements[i]) ElementType(std::forward<Args>(args)...);
        }

        size = newSize;

        return &elements[size - numOfElements];
    }

    void Grow(uint32 newCapacity) {

        if (newCapacity <= capacity) {
            return;
        }

        if (allocator->TryExtend(reinterpret_cast<uint8*>(elements), sizeof(ElementType) * newCapacity)) {
            capacity = newCapacity;
            return;
        }

        ElementType* newElements = reinterpret_cast<ElementType*>(allocator->Allocate(sizeof(ElementType) * newCapacity, alignof(ElementType)));

        if (!IsEmpty()) {

            // Move or copy existing elements to new storage
            if constexpr (std::is_trivially_copyable_v<ElementType>) {
                memmove(&newElements[0], &elements[0], sizeof(ElementType) * size);
            } else {
                for (uint32 i = 0; i < size; ++i) {
                    new (&newElements[i]) ElementType(std::move(elements[i]));
                    elements[i].~ElementType();
                }
            }
        }

        if (elements) {
            allocator->Free(reinterpret_cast<uint8*>(elements));
        }

        elements = newElements;
        capacity = newCapacity;
    }

    /**
     * Sorts the elements in the array using a custom predicate.
     * A callable that returns true if the first argument is less than the second.
     */
    template<typename Predicate>
    void Sort(Predicate predicate) {
        if (size > 1) {
            std::sort(elements, elements + size, predicate);
        }
    }

    /**
     * Sorts the elements in the array using the default less-than operator (operator<).
     */
    void Sort() {
        if (size > 1) {
            std::sort(elements, elements + size);
        }
    }

    FORCE_INLINE bool IsValidIndex(uint32 idx) const {
        return idx < size;
    }

    template<typename T = ElementType, typename FileType>
    FORCE_INLINE std::enable_if_t<std::is_trivially_copyable_v<T>, bool> BulkSerialize(FileInterface<FileType>& file) {
        bool success = file.Write(size);
        success &= file.Write(elements, size);
        return success;
    }

    template<typename T = ElementType, typename FileType>
    FORCE_INLINE std::enable_if_t<std::is_trivially_copyable_v<T>, bool> BulkDeserialize(FileInterface<FileType>& file) {
        //Reset(); Not necessary since type is trivially copyable thus trivially destructible as well
        uint32 newSize = 0;
        bool success = file.Read(newSize);
        Grow(newSize);
        size = newSize;
        success &= file.Read(elements, size);
        return success;
    }

    NO_DISCARD FORCE_INLINE ContainerView<ElementType> GetView() {
        return { elements, size };
    }

    NO_DISCARD FORCE_INLINE ElementType* GetData() const {
        return elements;
    }

    NO_DISCARD FORCE_INLINE uint32 GetSize() const {
        return size;
    }

    NO_DISCARD FORCE_INLINE uint32 GetCapacity() const {
        return capacity;
    }

    NO_DISCARD FORCE_INLINE bool IsEmpty() const {
        return size == 0;
    }

    NO_DISCARD FORCE_INLINE Allocator* GetAllocator() const {
        return allocator;
    }

    struct Iterator {

        using iterator_category = std::random_access_iterator_tag;
        using value_type        = ElementType;
        using difference_type   = std::ptrdiff_t;
        using pointer           = ElementType*;
        using reference         = ElementType&;

        Array* array;
        size_t currentIndex;

        Iterator() : array(nullptr), currentIndex(0) {}

        Iterator(Array& array, size_t startingIndex)
            : array(&array), currentIndex(startingIndex) {}

        NO_DISCARD reference operator*() const { return (*array)[static_cast<uint32>(currentIndex)]; }
        NO_DISCARD pointer operator->() const { return &(*array)[static_cast<uint32>(currentIndex)]; }
        NO_DISCARD reference operator[](difference_type n) const { return (*array)[static_cast<uint32>(currentIndex + n)]; }

        Iterator& operator++() { ++currentIndex; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; ++currentIndex; return tmp; }
        Iterator& operator--() { --currentIndex; return *this; }
        Iterator operator--(int) { Iterator tmp = *this; --currentIndex; return tmp; }

        Iterator& operator+=(difference_type n) { currentIndex += n; return *this; }
        Iterator& operator-=(difference_type n) { currentIndex -= n; return *this; }

        NO_DISCARD friend Iterator operator+(Iterator it, difference_type n) { it += n; return it; }
        NO_DISCARD friend Iterator operator+(difference_type n, Iterator it) { it += n; return it; }
        NO_DISCARD friend Iterator operator-(Iterator it, difference_type n) { it -= n; return it; }
        NO_DISCARD friend difference_type operator-(const Iterator& a, const Iterator& b) {
            return static_cast<difference_type>(a.currentIndex) - static_cast<difference_type>(b.currentIndex);
        }

        NO_DISCARD bool operator==(const Iterator& other) const { return currentIndex == other.currentIndex; }
        NO_DISCARD bool operator!=(const Iterator& other) const { return currentIndex != other.currentIndex; }
        NO_DISCARD bool operator<(const Iterator& other)  const { return currentIndex <  other.currentIndex; }
        NO_DISCARD bool operator<=(const Iterator& other) const { return currentIndex <= other.currentIndex; }
        NO_DISCARD bool operator>(const Iterator& other)  const { return currentIndex >  other.currentIndex; }
        NO_DISCARD bool operator>=(const Iterator& other) const { return currentIndex >= other.currentIndex; }
    };

    struct ConstIterator {

        using iterator_category = std::random_access_iterator_tag;
        using value_type        = ElementType;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const ElementType*;
        using reference         = const ElementType&;

        const Array* array;
        size_t currentIdx;

        ConstIterator() : array(nullptr), currentIdx(0) {}

        ConstIterator(const Array& array, size_t startingIdx)
            : array(&array), currentIdx(startingIdx) {}

        ConstIterator(const Iterator& other) : array(other.array), currentIdx(other.currentIndex) {}

        NO_DISCARD reference operator*() const { return (*array)[static_cast<uint32>(currentIdx)]; }
        NO_DISCARD pointer operator->() const { return &(*array)[static_cast<uint32>(currentIdx)]; }
        NO_DISCARD reference operator[](difference_type n) const { return (*array)[static_cast<uint32>(currentIdx + n)]; }

        ConstIterator& operator++() { ++currentIdx; return *this; }
        ConstIterator operator++(int) { ConstIterator tmp = *this; ++currentIdx; return tmp; }
        ConstIterator& operator--() { --currentIdx; return *this; }
        ConstIterator operator--(int) { ConstIterator tmp = *this; --currentIdx; return tmp; }

        ConstIterator& operator+=(difference_type n) { currentIdx += n; return *this; }
        ConstIterator& operator-=(difference_type n) { currentIdx -= n; return *this; }

        NO_DISCARD friend ConstIterator operator+(ConstIterator it, difference_type n) { it += n; return it; }
        NO_DISCARD friend ConstIterator operator+(difference_type n, ConstIterator it) { it += n; return it; }
        NO_DISCARD friend ConstIterator operator-(ConstIterator it, difference_type n) { it -= n; return it; }
        NO_DISCARD friend difference_type operator-(const ConstIterator& a, const ConstIterator& b) {
            return static_cast<difference_type>(a.currentIdx) - static_cast<difference_type>(b.currentIdx);
        }

        NO_DISCARD bool operator==(const ConstIterator& other) const { return currentIdx == other.currentIdx; }
        NO_DISCARD bool operator!=(const ConstIterator& other) const { return currentIdx != other.currentIdx; }
        NO_DISCARD bool operator<(const ConstIterator& other)  const { return currentIdx <  other.currentIdx; }
        NO_DISCARD bool operator<=(const ConstIterator& other) const { return currentIdx <= other.currentIdx; }
        NO_DISCARD bool operator>(const ConstIterator& other)  const { return currentIdx >  other.currentIdx; }
        NO_DISCARD bool operator>=(const ConstIterator& other) const { return currentIdx >= other.currentIdx; }
    };

    NO_DISCARD Iterator begin() { return Iterator(*this, 0); }
    NO_DISCARD Iterator end() { return Iterator(*this, size); }

    NO_DISCARD ConstIterator begin() const { return ConstIterator(*this, 0); }
    NO_DISCARD ConstIterator end() const { return ConstIterator(*this, size); }

    NO_DISCARD ConstIterator cbegin() const { return ConstIterator(*this, 0); }
    NO_DISCARD ConstIterator cend() const { return ConstIterator(*this, size); }
};

template<typename ElementType, uint32 Size>
using InlineArrayAllocator = InlineLinearAllocator<sizeof(ElementType) * Size, alignof(ElementType)>;

/**
 * Inline Array container (similar to std::array)
 * - 100% Allocated on the stack
 * - The inheritance from InlineArrayAllocator is necessary to ensure that the inlineAllocator is
 * created (and valid) before the call to the Array's constructor
 * - NEGATIVE: This is definitely too templated and hard to read also multiple inheritance is not nice but
 * I could not fine a nicer way of doing it while providing the nice API it currently has
 */
template<typename ElementType, uint32 Size>
class InlineArray :
    InlineArrayAllocator<ElementType, Size>,
    public Array<ElementType, InlineArrayAllocator<ElementType, Size>> {

public:
    InlineArray() : Array<ElementType, InlineArrayAllocator<ElementType, Size>>(Size, this) {}

    template<typename... Args>
    explicit InlineArray(Args&&... args) : Array<ElementType, InlineArrayAllocator<ElementType, Size>>(Size, this, std::forward<Args>(args)...) {}

    InlineArray(std::initializer_list<ElementType> initializerList) : Array<ElementType, InlineArrayAllocator<ElementType, Size>>(initializerList, this) {}

    // Move constructor, cannot steal pointer since elements live in inline storage
    InlineArray(InlineArray&& other) noexcept
        : Array<ElementType, InlineArrayAllocator<ElementType, Size>>(Size, this)
    {
        this->size = other.size;
        for (uint32 i = 0; i < other.size; i++) {
            new (&this->elements[i]) ElementType(std::move(other.elements[i]));
        }
    }

    // Copy constructor, must copy into own inline storage
    InlineArray(const InlineArray& other)
        : Array<ElementType, InlineArrayAllocator<ElementType, Size>>(Size, this)
    {
        this->size = other.size;
        if constexpr (std::is_trivially_copyable_v<ElementType>) {
            memcpy(this->elements, other.elements, sizeof(ElementType) * other.size);
        } else {
            for (uint32 i = 0; i < other.size; i++) {
                new (&this->elements[i]) ElementType(other.elements[i]);
            }
        }
    }

    // Intentionally not declaring destructor nor marking parent destructor virtual since there is nothing to destroy here
};

