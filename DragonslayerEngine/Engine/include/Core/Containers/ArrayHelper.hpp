#pragma once

#include "Core/EngineGlobals.hpp"
#include "Array.hpp"
#include "Core/Allocators/StackAllocator.hpp"

template<typename ElementType>
class TempArray : public Array<ElementType, StackAllocator> {

public:
    explicit TempArray(uint32 initialCapacity) : Array<ElementType, StackAllocator>(initialCapacity, &GetThreadTempAllocator()) {}

    template<typename... Args>
    explicit TempArray(uint32 size, Args&&... args) : Array<ElementType, StackAllocator>(size, &GetThreadTempAllocator(), std::forward<Args>(args)...) {}

    TempArray(std::initializer_list<ElementType> initializerList) : Array<ElementType, StackAllocator>(initializerList, &GetThreadTempAllocator()) {}

    // Intentionally not declaring destructor nor marking parent destructor virtual since there is nothing to destroy here
};
