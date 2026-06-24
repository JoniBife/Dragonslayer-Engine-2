#pragma once

#include <cstddef>
#include <utility>

#include "Core/Assert.hpp"
#include "Core/Export.hpp"
#include "Core/Macros.hpp"
#include "Core/Memory.hpp"
#include "Core/NotCopyable.hpp"
#include "Core/Types.hpp"
#include "ParentAllocator.hpp"

enum class FreeListStrategy : uint8  {
    FirstFit = 0,
    BestFit = 1,
};

/*
 * FreeList allocator
 * - Memory DOES not get freed on destruction (Free needs to be called explicitly)
 * - Currently uses a very simple linked list implementation
 * - Depending on alignment there may be padding between the head node and the baseAddress (that is ok!)
 * TODO Add boundaries checks (e.g. if aligning is going being the allocator's memory limits)
 * TODO Consider adding optional threadsafety
 * TODO Should it be copyable?
 */
class ENGINE_API FreeListAllocator final : public NotCopyable {

private:
    ParentAllocator parentAllocator;

    struct Node {
        Node* next;
        size_t size;
        bool isFree;
    };

    Node* head;

    uint8* baseAddress;
    size_t totalMemory;

    FreeListStrategy allocationStrategy;

public:
    explicit FreeListAllocator(size_t size, size_t alignment = alignof(std::max_align_t), FreeListStrategy allocationStrategy = FreeListStrategy::BestFit);

    template<typename ParentAllocator>
    FreeListAllocator(ParentAllocator& parentAllocator, size_t size, size_t alignment = alignof(std::max_align_t), FreeListStrategy allocationStrategy = FreeListStrategy::BestFit) :
        parentAllocator(parentAllocator),
        baseAddress(parentAllocator.Allocate(size, alignment)),
        totalMemory(size),
        allocationStrategy(allocationStrategy) {

        // We need at least one byte of space to store something :)
        ASSERT(size > sizeof(Node) + 1);

        head = reinterpret_cast<Node*>(AlignUp(reinterpret_cast<uint64>(baseAddress), alignof(Node)));

        const size_t paddingFromAlignment = reinterpret_cast<uint8*>(head) - baseAddress;

        *head = { nullptr, size - (sizeof(Node) + paddingFromAlignment), true };
    }

    // TODO Decide if we add a destructor
    //~FreeListAllocator() = delete;

    NO_DISCARD uint8* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    template<typename Type, typename... Args>
    Type* Allocate(Args&&... args) {
        Type* objectMemory = reinterpret_cast<Type*>(Allocate(sizeof(Type), alignof(Type)));
        new (objectMemory) Type(std::forward<Args>(args)...);
        return objectMemory;
    }

    NO_DISCARD uint8* Reallocate(uint8* blockAddress, size_t newSize);

    NO_DISCARD bool TryExtend(uint8* blockAddress, size_t newSize);

    void Free();

    template<typename Type>
    void Free(Type* address) {
        if (address) {
            address->~Type();
            Free(reinterpret_cast<uint8*>(address));
        }
    }

    void Free(uint8* blockAddress);

    void Clear();

    NO_DISCARD size_t GetAllocatedMemory() const;

    NO_DISCARD size_t GetNumOfNodes() const;

    NO_DISCARD size_t GetHeaderMemory() const;

    NO_DISCARD FORCE_INLINE size_t GetUsedMemory() const { return GetAllocatedMemory() + GetHeaderMemory(); }
    NO_DISCARD FORCE_INLINE size_t GetFreeMemory() const { return totalMemory - (GetAllocatedMemory() + GetHeaderMemory()); }
    NO_DISCARD FORCE_INLINE size_t GetTotalMemory() const { return totalMemory; }

private:
    static bool TrySplitNode(Node* node, size_t newSize);

    static bool TryExtendNode(Node* node, size_t newSize);

};


