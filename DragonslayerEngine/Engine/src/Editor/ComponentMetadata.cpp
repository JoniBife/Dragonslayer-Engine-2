#include "Editor/ComponentMetadata.hpp"

InlineArray<ComponentMetadata, 1024>& GetComponentsMetadata() {
    static InlineArray<ComponentMetadata, 1024> gComponentsMetadata = {};
    return gComponentsMetadata;
}