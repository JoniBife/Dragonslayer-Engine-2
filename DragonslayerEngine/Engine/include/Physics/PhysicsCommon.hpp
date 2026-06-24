#pragma once

#include <PxActor.h>
#include <PxShape.h>

#include "Core/Export.hpp"
#include "Core/Types.hpp"
#include "ECS/Component.hpp"
#include "ECS/Entity.hpp"
#include "Math/Quat.hpp"
#include "Math/Vec3.hpp"
#include "Runtime/Components/Transform.hpp"

namespace physx {
    class PxRigidActor;
}

using namespace physx;

enum FilterFlags : uint32
{
    FLAG_NONE                = 0,
    FLAG_TRIGGER_CONTACT     = 1 << 0, // Generate eNOTIFY_TOUCH_FOUND
};

// NOTE:
// The conversion from Dragonslayer's Math Types
// and PhysX can be done via a reinterpret_cast
// because the types share the exact same memory layout

NO_DISCARD FORCE_INLINE static PxVec3& ToPX(Vec3& vec3) {
    return reinterpret_cast<PxVec3&>(vec3);
}

NO_DISCARD FORCE_INLINE static Vec3& ToDS(PxVec3& vec3) {
    return reinterpret_cast<Vec3&>(vec3);
}

NO_DISCARD FORCE_INLINE static Quat& ToDS(PxQuat& quat) {
    return reinterpret_cast<Quat&>(quat);
}

NO_DISCARD FORCE_INLINE static PxQuat& ToPX(Quat& quat) {
    return reinterpret_cast<PxQuat&>(quat);
}

NO_DISCARD FORCE_INLINE static const PxVec3& ToPX(const Vec3& vec3) {
    return reinterpret_cast<const PxVec3&>(vec3);
}

NO_DISCARD FORCE_INLINE static const Vec3& ToDS(const PxVec3& vec3) {
    return reinterpret_cast<const Vec3&>(vec3);
}

NO_DISCARD FORCE_INLINE static const Quat& ToDS(const PxQuat& quat) {
    return reinterpret_cast<const Quat&>(quat);
}

NO_DISCARD FORCE_INLINE static const PxQuat& ToPX(const Quat& quat) {
    return reinterpret_cast<const PxQuat&>(quat);
}

NO_DISCARD FORCE_INLINE static PxTransform ToPX(const Transform& transform) {
    return { ToPX(transform.location), ToPX(transform.rotation) };
}

NO_DISCARD FORCE_INLINE static Transform ToDS(const PxTransform& transform) {
    return { ToDS(transform.p), Vec3::ONE, ToDS(transform.q) };
}

NO_DISCARD FORCE_INLINE static Entity ToEntity(PxActor& actor) {
    return static_cast<Entity>(reinterpret_cast<uintptr>(actor.userData));
}

struct ENGINE_API RayCastHit {
    Vec3 position = Vec3::ZERO;
    Vec3 normal = Vec3::ZERO;
    float distance = 0.f;
    Entity entity = ECS::InvalidEntity;
    PxActor* actor = nullptr;
    PxShape* shape = nullptr;
};

namespace Physics {
    ENGINE_API const PxMaterial& CreatePhysicsMaterial(
        const NameString& name,
        float staticFriction,
        float dynamicFriction,
        float restitution);

    NO_DISCARD ENGINE_API const PxMaterial& GetExistingPhysicsMaterial(const NameString& name);

    ENGINE_API bool RayCast(const Vec3& origin, const Vec3& unitDirection, float maxDistance, RayCastHit& outHit, uint32 collisionGroup = 0xFFFFFFFF);

    NO_DISCARD ENGINE_API bool RayCast(const Vec3& origin, const Vec3& unitDirection, float maxDistance = FLT_MAX, uint32 collisionGroup = 0xFFFFFFFF);

    ENGINE_API bool RayCastSingleTarget(const Vec3& origin, const Vec3& unitDirection, float maxDistance, PxActor* targetActor, RayCastHit& outHit);

    // Writes a single PxShape (geometry, localPose, sim+query filter data, raw shape flags).
    // Supports sphere/box/capsule/plane; asserts on unsupported geometry types.
    // Materials are NOT serialized — DeserializeAndAttachShape uses gDefaultPhysicsMaterial.
    ENGINE_API void SerializeShape(VaultFile& file, const PxShape& shape);

    // Reads one shape record and attaches it to `actor` as an exclusive shape.
    // Re-applies stored localPose and filter data after attach.
    ENGINE_API void DeserializeAndAttachShape(VaultFile& file, PxRigidActor& actor);
}
