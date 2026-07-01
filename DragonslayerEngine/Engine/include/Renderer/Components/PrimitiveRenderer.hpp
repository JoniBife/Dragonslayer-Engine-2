#pragma once

#include "ECS/Component.hpp"
#include "Renderer/RendererCore.hpp"

enum class PrimitiveType : uint8 {
    Plane = 0,
    Cube = 1,
    Sphere = 2,
    Cylinder = 3,
    Capsule = 4
};

static constexpr const char* primitiveTypes[] = {
    "Plane",
    "Cube",
    "Sphere",
    "Cylinder",
    "Capsule",
};

DSTRUCT(PrimitiveRenderer, .isSerializable = true, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(PrimitiveRenderer)

    PrimitiveType primitiveType = PrimitiveType::Cube;
    Vec3 color = { .5f, 0.5f, 0.5f};
    float alpha = 1.f;
    bool castShadow = true;
    bool isVisible = true;


    void Serialize(VaultFile& file) const {
        file.Write(primitiveType);
        file.Write(color.x);
        file.Write(color.y);
        file.Write(color.z);
        file.Write(alpha);
        file.Write(castShadow);
        file.Write(isVisible);
    }

    static void Deserialize(void* obj, VaultFile& file) {
        new (obj) PrimitiveRenderer();
        PrimitiveRenderer& primitiveRenderer = *static_cast<PrimitiveRenderer*>(obj);
        file.Read(primitiveRenderer.primitiveType);
        file.Read(primitiveRenderer.color.x);
        file.Read(primitiveRenderer.color.y);
        file.Read(primitiveRenderer.color.z);
        file.Read(primitiveRenderer.alpha);
        file.Read(primitiveRenderer.castShadow);
        file.Read(primitiveRenderer.isVisible);
    }

#if WITH_EDITOR
    void OnHierarchy() {
        int type = static_cast<int>(primitiveType);
        if (ImGui::Combo("PrimitiveType", &type, primitiveTypes, helper::GetCountOf(primitiveTypes))) {
            primitiveType = static_cast<PrimitiveType>(type);
        }
        ImGui::ColorPicker3("Color", &color.x);
        ImGui::SliderFloat("Alpha", &alpha, 0.f, 1.f);
        ImGui::Checkbox("Cast Shadow", &castShadow);
        ImGui::Checkbox("Is Visible", &isVisible);
    }
#endif

}; END_DSTRUCT(PrimitiveRenderer)