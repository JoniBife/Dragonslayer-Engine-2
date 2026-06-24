#pragma once

#include <Core/EngineMinimal.hpp>
#include <Math/MathAux.hpp>
#include <Renderer/Components/PrimitiveRenderer.hpp>
#include <Runtime/Components/Transform.hpp>

#include "StaticGameSettings.hpp"

enum class ModuleType : uint8 {
    Air,
    Shoot,
    Rotator,
    Chain,
    Piston,
    Shape,
    Last = Shape
};
static constexpr const char* MODULE_NAMES[] = { "Air", "Shoot", "Rotator", "Chain", "Piston", "Shape" };

enum class HandleType : uint8 {
    Sword,
    ShootV1,
    ShootV2,
    ShootV3,
    ShootV4,
    Last = ShootV4
};
static constexpr const char* HANDLE_NAMES[] = { "Sword", "ShootV1", "ShootV2", "ShootV3", "ShootV4" };

static const Vec3 DEFAULT_HANDLE_OFFSET = Vec3(-.7f, 0.f, 0.f);

struct ModuleVisual {
    PrimitiveType primitiveType;
    Vec3 color;
};

struct ModuleSlot {
    ModuleType type = ModuleType::Shape;
    Transform transform;
    ModuleSlot* parentModule = nullptr;
    InlineArray<ModuleSlot*, MAX_MODULES> childModuleNodes;
};

static FORCE_INLINE ModuleVisual GetModuleVisuals(ModuleType type) {
    switch (type) {
        case ModuleType::Air: return { PrimitiveType::Cylinder, Vec3(0.2f, 0.7f, 0.2f)};
        case ModuleType::Shoot: return { PrimitiveType::Cylinder, Vec3(0.f, 0.5f, 0.5f)};
        case ModuleType::Rotator: return { PrimitiveType::Cube, Vec3(1.f, 0.f, 0.f)};
        case ModuleType::Chain: return { PrimitiveType::Sphere, Vec3(0.5f, 0.5f, 0.f)};
        case ModuleType::Piston: return { PrimitiveType::Cylinder, Vec3(0.5f, 0.f, 0.5f)};
        case ModuleType::Shape: return { PrimitiveType::Cube, Vec3(0.1f, 0.1f, 0.1f)};
        default: return { PrimitiveType::Cube, Vec3(0.5f, 0.5f, 0.5f)};
    }
}

static FORCE_INLINE Vec3 GetModuleScale(ModuleType type) {
    switch (type) {
        case ModuleType::Air: return Vec3(.1);
        case ModuleType::Shoot: return Vec3(.1);
        case ModuleType::Rotator: return Vec3(.3);
        case ModuleType::Chain: return Vec3(.3);
        case ModuleType::Piston: return Vec3(.3);
        case ModuleType::Shape: return Vec3(.3);
        default: return Vec3(.3);
    }
}

static FORCE_INLINE const char* GetModuleName(ModuleType type) {
    return MODULE_NAMES[static_cast<uint32>(type)];
}

struct HandleVisuals {
    PrimitiveType primitiveType;
    Vec3 color;
};

static FORCE_INLINE HandleVisuals GetHandleVisuals(HandleType type) {
    switch (type) {
        case HandleType::Sword: return { PrimitiveType::Cube, Vec3(.25f, .28f, .20f)};
        case HandleType::ShootV1: return { PrimitiveType::Cube, Vec3(.33f, .20f, .15f)};
        case HandleType::ShootV2: return { PrimitiveType::Cube, Vec3(.44f, .45f, .46f)};
        case HandleType::ShootV3: return { PrimitiveType::Cube, Vec3(.10f, .11f, .13f)};
        case HandleType::ShootV4: return { PrimitiveType::Cube, Vec3(.54f, .03f, .03f)};
        default: return { PrimitiveType::Cube, Vec3(0.3f, 0.3f, 0.3f)};
    }
}

static FORCE_INLINE Vec3 GetHandleScale(HandleType type) {
    switch (type) {
        case HandleType::Sword: return Vec3(0.1f, 0.5f, 0.1f);
        case HandleType::ShootV1: return Vec3(0.1f, 0.2f, 0.1f);
        case HandleType::ShootV2: return Vec3(0.1f, 0.2f, 0.1f);
        case HandleType::ShootV3: return Vec3(0.1f, 0.2f, 0.1f);
        case HandleType::ShootV4: return Vec3(0.1f, 0.2f, 0.1f);
        default: return Vec3(0.1f, 0.2f, 0.1f);
    }
}

static FORCE_INLINE Vec3 GetHandleOffset(HandleType type) {
    return DEFAULT_HANDLE_OFFSET - Vec3(.2f, 0.f, 0.f);
    /*switch (type) {
        case HandleType::Sword: return Vec3(0.1f, 0.2f, 0.1f);
        case HandleType::ShootV1: return Vec3(0.1f, 0.2f, 0.1f);
        case HandleType::ShootV2: return Vec3(0.1f, 0.2f, 0.1f);
        case HandleType::ShootV3: return Vec3(0.1f, 0.2f, 0.1f);
        case HandleType::ShootV4: return Vec3(0.1f, 0.2f, 0.1f);
        default:
    }*/
}

static FORCE_INLINE Quat GetHandleRotation(HandleType type) {
    switch (type) {
        case HandleType::Sword: return Quat(PI / 2.f, Vec3(0.f, 0.f, 1.f));
        case HandleType::ShootV1: return Quat(PI / 2.f, Vec3(1.f, 0.f, 0.f));
        case HandleType::ShootV2: return Quat(PI / 2.f, Vec3(0.f, 0.f, 1.f));
        case HandleType::ShootV3: return Quat::IDENTITY;
        case HandleType::ShootV4: return Quat(PI / 2.f, Vec3(1.f, 0.f, 0.f));
        default: return Quat(PI / 2.f, Vec3(1.f, 0.f, 0.f));
    }
}

static FORCE_INLINE Quat GetHandlePreviewRotation(HandleType type) {
    switch (type) {
        case HandleType::Sword: return Quat::IDENTITY;
        case HandleType::ShootV1: return Quat(PI / 2.f, Vec3(1.f, 0.f, 0.f));
        case HandleType::ShootV2: return Quat::IDENTITY;
        case HandleType::ShootV3: return Quat(PI / 2.f, Vec3(1.f, 0.f, 0.f));
        case HandleType::ShootV4: return Quat(PI / 2.f, Vec3(1.f, 0.f, 0.f));
        default: return Quat(PI / 2.f, Vec3(1.f, 0.f, 0.f));
    }
}

static FORCE_INLINE const char* GetHandleName(HandleType type) {
    return HANDLE_NAMES[static_cast<uint32>(type)];
}



