#include "Core/Allocators/FreeListAllocator.hpp"

#include <algorithm>
#include <cstring>

#include "Core/Assert.hpp"
#include "Core/Memory.hpp"

FreeListAllocator::FreeListAllocator(size_t size, size_t alignment, FreeListStrategy allocationStrategy)
    : totalMemory(size), allocationStrategy(allocationStrategy) {

    ASSERT(alignment > 0);
    // We need at least one byte of space to store something :)
    ASSERT(size > sizeof(Node) + 1);

    // The free list needs to also store the nodes therefore its alignment needs
    // to be able to support at least that
    const size_t baseAddressAlignment = std::max(alignment, alignof(Node));
    baseAddress = static_cast<uint8*>(AlignedMalloc(size, baseAddressAlignment));

    ASSERT(baseAddress != nullptr);

    head = reinterpret_cast<Node*>(baseAddress);

    *head = { nullptr, size - sizeof(Node), true };
}

uint8* FreeListAllocator::Allocate(size_t size, size_t alignment) {

    ASSERT(size > 0 && alignment > 0);
    ASSERT(IsPowerOf2(alignment));
    // ASSERT(size % alignment == 0); Sadly cannot have this because GLFW does not follow this rule!

    Node* targetNode = nullptr;
    size_t targetPaddingFromAlignment = 0;
    uint8* targetAddress = nullptr;

    if (allocationStrategy == FreeListStrategy::BestFit) {

        Node* currNode = head;
        while (currNode != nullptr) {

            if (!currNode->isFree || currNode->size < size) {
                currNode = currNode->next;
                continue;
            }

            const uint8* addressAfterNode = reinterpret_cast<uint8*>(currNode + 1);
            uint8* allocationBaseAddress =
                    reinterpret_cast<uint8*>(AlignUp(reinterpret_cast<uint64>(addressAfterNode), alignment));

            // After aligning the address we need to check if there is still enough space to fit the requested
            // allocation because the addition of the padding may have resulted in not having enough space for it
            const size_t paddingFromAlignment = allocationBaseAddress - addressAfterNode;

            if (size + paddingFromAlignment > currNode->size) {
                currNode = currNode->next;
                continue;
            }

            if (targetNode == nullptr || currNode->size < targetNode->size) {
                targetNode = currNode;
                targetAddress = allocationBaseAddress;
                targetPaddingFromAlignment = paddingFromAlignment;

                // Perfect fit, thus break immediately!
                if (currNode->size == size + paddingFromAlignment) {
                    break;
                }
            }

            currNode = currNode->next;
        }

    } else {

        Node* currNode = head;
        while (currNode != nullptr) {

            if (!currNode->isFree || currNode->size < size) {
                currNode = currNode->next;
                continue;
            }

            const uint8* addressAfterNode = reinterpret_cast<uint8*>(currNode + 1);
            uint8* allocationBaseAddress =
                    reinterpret_cast<uint8*>(AlignUp(reinterpret_cast<uint64>(addressAfterNode), alignment));

            // After aligning the address we need to check if there is still enough space to fit the requested
            // allocation because the addition of the padding may have resulted in not having enough space for it
            const size_t paddingFromAlignment = allocationBaseAddress - addressAfterNode;

            if (size + paddingFromAlignment > currNode->size) {
                currNode = currNode->next;
                continue;
            }

            targetNode = currNode;
            targetAddress = allocationBaseAddress;
            targetPaddingFromAlignment = paddingFromAlignment;
            break;
        }
    }

    if (targetNode == nullptr) {
        FAIL("Failed to find Node with enough space for the allocation");
        return nullptr;
    }

    targetNode->isFree = false;

    // After finding that target node, there may still be enough space to add a new node after the allocation
    if (size + targetPaddingFromAlignment < targetNode->size) {
        TrySplitNode(targetNode, size + targetPaddingFromAlignment);
    }

    return targetAddress;
}

uint8* FreeListAllocator::Reallocate(uint8* blockAddress, size_t newSize) {

    Node* currNode = head;
    while (currNode != nullptr) {

        uint8* addressAfterNode = reinterpret_cast<uint8*>(currNode + 1);
        if (blockAddress >= addressAfterNode && blockAddress < addressAfterNode + currNode->size) {

            const size_t paddingBeforeBlock = blockAddress - addressAfterNode;
            const size_t adjustedNewSize = newSize + paddingBeforeBlock;

            // Shrink to fit
            if (adjustedNewSize < currNode->size) {
                TrySplitNode(currNode, adjustedNewSize);
                return blockAddress;
            }

            // Try to extend or allocate
            if (adjustedNewSize > currNode->size) {

                if (TryExtendNode(currNode, adjustedNewSize)) {
                    return blockAddress;
                }

                // TODO Using this alignmentOf assumes the original allocation address is the blockAddress
                // however if the programmer decides to call reallocate with allocAddress + something
                // The alignment will not be the same and this will incorrectly allocate a new block
                // So, solution should be storing the original allocation alignment in the node!
                // All well known memory allocators follow this convention, so it should not be an issue!
                const size_t alignmentOfBlock = AlignmentOf(blockAddress);

                uint8* newBlockAddress = Allocate(newSize, alignmentOfBlock);
                std::memmove(newBlockAddress, blockAddress, currNode->size - paddingBeforeBlock);
                Free(blockAddress);

                return newBlockAddress;
            }

            // Same size, so do nothing!
            return blockAddress;
        }

        currNode = currNode->next;
    }

    return nullptr;
}

bool FreeListAllocator::TryExtend(uint8* blockAddress, size_t newSize) {

    Node* currNode = head;
    while (currNode != nullptr) {

        uint8* addressAfterNode = reinterpret_cast<uint8*>(currNode + 1);
        if (blockAddress >= addressAfterNode && blockAddress < addressAfterNode + currNode->size) {

            if (newSize < currNode->size) {
                FAIL("New allocation size is smaller than original!");
                return false;
            }

            if (newSize == currNode->size) {
                return true; // No extension needed
            }

            return TryExtendNode(currNode, newSize);
        }

        currNode = currNode->next;
    }

    FAIL("Address does not belong to this allocator!");
    return false;
}

void FreeListAllocator::Free() {
    // TODO This completely destroys the allocator, it should probably be renamed destroy
    if (parentAllocator.Exists()) {
        parentAllocator.Free(baseAddress);
    } else {
        AlignedFree(baseAddress);
    }
}

void FreeListAllocator::Free(uint8* blockAddress) {

    Node* prev = nullptr;
    Node* currNode = head;
    while (currNode != nullptr) {

        uint8* addressAfterNode = reinterpret_cast<uint8*>(currNode + 1);
        if (blockAddress >= addressAfterNode && blockAddress < addressAfterNode + currNode->size) {

            if (currNode->isFree) {
                //TODO Consider if this is really necessary
                FAIL("Double free!");
                return;
            }

            currNode->isFree = true;

            // If possible let prev absorb current
            if (prev != nullptr && prev->isFree) {
                prev->size += sizeof(Node) + currNode->size;
                prev->next = currNode->next;
                currNode = prev;
            }

            Node* nextNode = currNode->next;

            // If possible let current (or prev) absorb the next
            if (nextNode != nullptr && nextNode->isFree) {
                currNode->size += sizeof(Node) + nextNode->size;
                currNode->next = nextNode->next;
            }

            return;
        }

        prev = currNode;
        currNode = currNode->next;
    }

    FAIL("Address does not belong to this allocator!");
}

void FreeListAllocator::Clear() {

    // TODO Reassigning head is likely unnecessary, since we will never remove it
    head = reinterpret_cast<Node*>(AlignUp(reinterpret_cast<uint64>(baseAddress), alignof(Node)));

    const size_t padding = reinterpret_cast<uint8*>(head) - baseAddress;

    *head = {nullptr, totalMemory - (sizeof(Node) + padding), true};
}

size_t FreeListAllocator::GetAllocatedMemory() const {

    size_t totalAllocated = 0;

    Node* currNode = head;
    while (currNode != nullptr) {
        if (!currNode->isFree)
        {
            totalAllocated += currNode->size;
        }
        currNode = currNode->next;
    }

    return totalAllocated;
}

size_t FreeListAllocator::GetNumOfNodes() const
{
    size_t numNodes = 0;

    Node* currNode = head;
    while (currNode != nullptr) {
        ++numNodes;
        currNode = currNode->next;
    }

    return numNodes;
}

size_t FreeListAllocator::GetHeaderMemory() const {

    size_t totalHeader = 0;

    Node* currNode = head;
    while (currNode != nullptr) {

        totalHeader += sizeof(Node);
        currNode = currNode->next;
    }

    return totalHeader;
}

bool FreeListAllocator::TrySplitNode(Node* node, size_t newSize) {

    ASSERT(newSize > 0);
    ASSERT(newSize < node->size, "Tried splitting a node with a larger new size");

    const size_t freeMemoryAfterAllocation = node->size - newSize;

    uint8* addressAfterAllocation = reinterpret_cast<uint8*>(node + 1) + newSize;
    uint8* nextNodeAddress =
            reinterpret_cast<uint8*>(AlignUp(reinterpret_cast<uint64>(addressAfterAllocation), alignof(Node)));
    const size_t nodePaddingFromAlignment = nextNodeAddress - addressAfterAllocation;

    const size_t minimumRequiredMemoryForNextNode = nodePaddingFromAlignment + sizeof(Node) + 1;

    // After the allocation there may still be enough space for another node
    // But that needs to include potential padding from node address + the node itself + at least 1 byte for storage
    // (otherwise it's pointless)
    if (minimumRequiredMemoryForNextNode <= freeMemoryAfterAllocation) {

        Node* newNode = reinterpret_cast<Node*>(nextNodeAddress);
        newNode->isFree = true;
        newNode->size = freeMemoryAfterAllocation - (sizeof(Node) + nodePaddingFromAlignment);
        newNode->next = node->next;

        node->next = newNode;
        // The padding added before the new node is added to the previous to prevent gaps in the linked list
        // The alternative would require more complicated logic on Free to ensure that padding is still used
        node->size = newSize + nodePaddingFromAlignment;

        return true;
    }

    return false;
}

bool FreeListAllocator::TryExtendNode(Node* node, size_t newSize) {

    ASSERT(newSize > 0);
    ASSERT(newSize > node->size, "Tried extending node with a smaller new size");

    size_t availableSize = node->size;

    // Iteratively absorb consecutive free nodes until we have enough space
    Node* lastMerged = node;
    while (availableSize < newSize) {

        Node* nextNode = lastMerged->next;
        if (nextNode == nullptr || !nextNode->isFree) {
            return false; // No more free adjacent nodes
        }

        availableSize += sizeof(Node) + nextNode->size;
        lastMerged = nextNode;
    }

    node->size = availableSize;
    node->next = lastMerged->next;

    if (newSize < node->size) {
        TrySplitNode(node, newSize);
    }

    return true;
}


