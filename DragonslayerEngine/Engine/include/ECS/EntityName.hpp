#pragma once

#include "Component.hpp"
#include "Core/String.hpp"

DSTRUCT(EntityName, .isSerializable = true) final {
    GENERATE_DSTRUCT_BODY(EntityName)

    NameString string;

    void Serialize(VaultFile& file) const {
        file.Write(string.Size());
        file.Write(string.CStr(), string.Size() + 1/*Null terminator*/);
    }

    static void Deserialize(void* obj, VaultFile& file) {
        new (obj) EntityName();
        EntityName& entityName = *static_cast<EntityName*>(obj);
        uint32 size;
        char string[NAME_STRING_LENGTH];
        file.Read(size);
        file.Read(string, size + 1/*Null terminator*/);
        entityName.string = NameString(string);
    }

}; END_DSTRUCT(EntityName)
