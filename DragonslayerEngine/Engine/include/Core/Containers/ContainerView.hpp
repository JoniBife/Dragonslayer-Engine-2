#pragma once

#include "Core/Macros.hpp"

/*
 * Lightweight General purpose container view
 * It provides a view over containers (e.g. arrays, vectors, queues etc..)
 * Does NOT validate data
 * Does NOT copy the elements
 * Supports range-based for loops
 * Guarantees contiguous memory access on for loops
 * TODO CLEANUP!
 * TODO Instead of numElements store a start and end to be able to create views for segments of a container
 * TODO Instead of storing the elements and numElement store a ref to the actual container and then call its functions
 */
template <typename ElementType>
class ContainerView {
private:
    ElementType* elements;
    size_t numElements;

public:
    ContainerView(ElementType* elements, size_t numElements) :
        elements(elements), numElements(numElements) {}

    ContainerView(ElementType* elements, size_t rangeStart, size_t rangeEnd) :
        elements(elements + rangeStart), numElements(rangeEnd - rangeStart) {}

    // --- Modern Random Access Iterator ---
    struct Iterator {
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = ElementType;
        using difference_type   = std::ptrdiff_t;
        using pointer           = ElementType*;
        using reference         = ElementType&;

        pointer ptr;

        Iterator() : ptr(nullptr) {}
        explicit Iterator(pointer p) : ptr(p) {}

        NO_DISCARD reference operator*() const { return *ptr; }
        pointer operator->() const { return ptr; }

        Iterator& operator++() { ++ptr; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; ++ptr; return tmp; }
        Iterator& operator--() { --ptr; return *this; }
        Iterator operator--(int) { Iterator tmp = *this; --ptr; return tmp; }

        Iterator& operator+=(difference_type n) { ptr += n; return *this; }
        Iterator& operator-=(difference_type n) { ptr -= n; return *this; }

        friend Iterator operator+(Iterator it, difference_type n) { return Iterator(it.ptr + n); }
        friend Iterator operator+(difference_type n, Iterator it) { return Iterator(it.ptr + n); }
        friend Iterator operator-(Iterator it, difference_type n) { return Iterator(it.ptr - n); }
        friend difference_type operator-(Iterator a, Iterator b) { return a.ptr - b.ptr; }

        NO_DISCARD bool operator==(const Iterator& other) const { return ptr == other.ptr; }
        NO_DISCARD bool operator!=(const Iterator& other) const { return ptr != other.ptr; }
        NO_DISCARD bool operator<(const Iterator& other) const { return ptr < other.ptr; }
    };

    // --- Modern Const Iterator ---
    struct ConstIterator {
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = ElementType;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const ElementType*;
        using reference         = const ElementType&;

        pointer ptr;

        ConstIterator() : ptr(nullptr) {}
        explicit ConstIterator(pointer p) : ptr(p) {}
        // Allow conversion from Iterator to ConstIterator
        ConstIterator(const Iterator& other) : ptr(other.ptr) {}

        NO_DISCARD reference operator*() const { return *ptr; }
        pointer operator->() const { return ptr; }

        ConstIterator& operator++() { ++ptr; return *this; }
        ConstIterator operator++(int) { ConstIterator tmp = *this; ++ptr; return tmp; }

        // ... (Include similar arithmetic/comparison operators as Iterator) ...

        NO_DISCARD bool operator!=(const ConstIterator& other) const { return ptr != other.ptr; }
        NO_DISCARD bool operator==(const ConstIterator& other) const { return ptr == other.ptr; }
    };

    // --- Container Interface ---
    NO_DISCARD Iterator begin() { return Iterator(elements); }
    NO_DISCARD Iterator end()   { return Iterator(elements + numElements); }

    NO_DISCARD ConstIterator begin() const { return ConstIterator(elements); }
    NO_DISCARD ConstIterator end()   const { return ConstIterator(elements + numElements); }

    NO_DISCARD ElementType& operator[](size_t index) { return elements[index]; }
    NO_DISCARD const ElementType& operator[](size_t index) const { return elements[index]; }

    NO_DISCARD size_t GetSize() const { return numElements; }
};
