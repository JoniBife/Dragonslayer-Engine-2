#pragma once

#include "Core/EngineGlobals.hpp"
#include "HashMap.hpp"

template<typename KeyType, typename ElementType, bool KeyIsHash = false>
class TempHashMap : public HashMap<KeyType, ElementType, StackAllocator, KeyIsHash> {

public:
    explicit TempHashMap(uint32 initialCapacity) : HashMap<KeyType, ElementType, StackAllocator, KeyIsHash>(initialCapacity, GetThreadTempAllocator()) {}

    // Intentionally not declaring destructor nor marking parent destructor virtual since there is nothing to destroy here
};
