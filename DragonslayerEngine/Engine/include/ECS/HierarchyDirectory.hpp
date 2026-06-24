#pragma once

#include "Component.hpp"

constexpr uint32 MAX_DIRECTORY_DEPTH = 4;

DSTRUCT(HierarchyDirectory) final {
    GENERATE_DSTRUCT_BODY(HierarchyDirectory)

    // Represents the folder structure { "NPCs", "Enemies", "Robots" } -> NPSs/Enemies/Robots
    InlineArray<NameString, MAX_DIRECTORY_DEPTH> Folders;

}; END_DSTRUCT(HierarchyDirectory)
