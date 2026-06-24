#include "Physics/PhysicsCommon.hpp"

#include <PxPhysics.h>
#include <PxRigidActor.h>
#include <PxScene.h>
#include <PxShape.h>
#include <geometry/PxBoxGeometry.h>
#include <geometry/PxCapsuleGeometry.h>
#include <geometry/PxPlaneGeometry.h>
#include <geometry/PxSphereGeometry.h>

#include "Runtime/Engine.hpp"
#include "Core/EngineGlobals.hpp"


const PxMaterial& Physics::CreatePhysicsMaterial(
    const NameString& name,
    float staticFriction,
    float dynamicFriction,
    float restitution) {
    ASSERT(gEngine);
    return gEngine->GetEngineSystem<PhysicsSystem>().CreatePhysicsMaterial(name, staticFriction, dynamicFriction, restitution);
}

const PxMaterial& Physics::GetExistingPhysicsMaterial(const NameString& name) {
    ASSERT(gEngine);
    return gEngine->GetEngineSystem<PhysicsSystem>().GetExistingMaterial(name);
}

bool Physics::RayCast(const Vec3& origin, const Vec3& unitDirection, float maxDistance, RayCastHit& outHit, uint32 collisionGroup) {
    PxRaycastBuffer hit;
    PxQueryFilterData queryFilterData;
    queryFilterData.data.word0 = collisionGroup;
    if (gPhysicsScene->raycast(ToPX(origin), ToPX(unitDirection), maxDistance, hit, PxHitFlag::eDEFAULT, queryFilterData)) {
        const PxRaycastHit& closest = hit.block;
        outHit.position = ToDS(closest.position);
        outHit.normal = ToDS(closest.normal);
        outHit.distance = closest.distance;
        if (closest.actor) {
            outHit.entity = ToEntity(*closest.actor);
            outHit.actor = closest.actor;
        }
        if (closest.shape) {
            outHit.shape = closest.shape;
        }
        return true;
    }
    return false;
}

bool Physics::RayCast(const Vec3& origin, const Vec3& unitDirection, float maxDistance, uint32 collisionGroup) {
    PxRaycastBuffer hit;
    PxQueryFilterData queryFilterData;
    queryFilterData.flags = PxQueryFlag::eANY_HIT;
    queryFilterData.data.word0 = collisionGroup;
    return gPhysicsScene->raycast(ToPX(origin), ToPX(unitDirection), maxDistance, hit, PxHitFlag::eDEFAULT, queryFilterData);
}

void Physics::SerializeShape(VaultFile& file, const PxShape& shape) {
    const PxGeometry& geometry = shape.getGeometry();
    const uint8 geomType = static_cast<uint8>(geometry.getType());
    file.Write(geomType);

    switch (geometry.getType()) {
        case PxGeometryType::eSPHERE: {
            const PxSphereGeometry& sphere = static_cast<const PxSphereGeometry&>(geometry);
            file.Write(sphere.radius);
            break;
        }
        case PxGeometryType::eBOX: {
            const PxBoxGeometry& box = static_cast<const PxBoxGeometry&>(geometry);
            file.Write(box.halfExtents.x);
            file.Write(box.halfExtents.y);
            file.Write(box.halfExtents.z);
            break;
        }
        case PxGeometryType::eCAPSULE: {
            const PxCapsuleGeometry& capsule = static_cast<const PxCapsuleGeometry&>(geometry);
            file.Write(capsule.radius);
            file.Write(capsule.halfHeight);
            break;
        }
        case PxGeometryType::ePLANE: {
            break;
        }
        default:
            FAIL("SerializeShape: unsupported geometry type %d", static_cast<int>(geometry.getType()));
            break;
    }

    const PxTransform localPose = shape.getLocalPose();
    file.Write(localPose.p.x);
    file.Write(localPose.p.y);
    file.Write(localPose.p.z);
    file.Write(localPose.q.x);
    file.Write(localPose.q.y);
    file.Write(localPose.q.z);
    file.Write(localPose.q.w);

    const PxFilterData simData = shape.getSimulationFilterData();
    file.Write(simData.word0);
    file.Write(simData.word1);
    file.Write(simData.word2);
    file.Write(simData.word3);

    const PxFilterData queryData = shape.getQueryFilterData();
    file.Write(queryData.word0);
    file.Write(queryData.word1);
    file.Write(queryData.word2);
    file.Write(queryData.word3);

    const uint16 rawFlags = static_cast<uint16>(static_cast<PxU32>(shape.getFlags()));
    file.Write(rawFlags);
}

void Physics::DeserializeAndAttachShape(VaultFile& file, PxRigidActor& actor) {
    uint8 geomType = 0;
    file.Read(geomType);

    PxShape* shape = nullptr;
    PxShapeFlags shapeFlagsTemp = PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE;

    switch (static_cast<PxGeometryType::Enum>(geomType)) {
        case PxGeometryType::eSPHERE: {
            float radius = 0.f;
            file.Read(radius);
            shape = gPhysics->createShape(PxSphereGeometry(radius), *gDefaultPhysicsMaterial, true, shapeFlagsTemp);
            break;
        }
        case PxGeometryType::eBOX: {
            PxVec3 halfExtents(0.f);
            file.Read(halfExtents.x);
            file.Read(halfExtents.y);
            file.Read(halfExtents.z);
            shape = gPhysics->createShape(PxBoxGeometry(halfExtents), *gDefaultPhysicsMaterial, true, shapeFlagsTemp);
            break;
        }
        case PxGeometryType::eCAPSULE: {
            float radius = 0.f;
            float halfHeight = 0.f;
            file.Read(radius);
            file.Read(halfHeight);
            shape = gPhysics->createShape(PxCapsuleGeometry(radius, halfHeight), *gDefaultPhysicsMaterial, true, shapeFlagsTemp);
            break;
        }
        case PxGeometryType::ePLANE: {
            shape = gPhysics->createShape(PxPlaneGeometry(), *gDefaultPhysicsMaterial, true, shapeFlagsTemp);
            break;
        }
        default:
            FAIL("DeserializeAndAttachShape: unsupported geometry type %d", static_cast<int>(geomType));
            return;
    }

    ASSERT(shape != nullptr, "Failed to create shape during deserialization!");

    PxTransform localPose(PxIdentity);
    file.Read(localPose.p.x);
    file.Read(localPose.p.y);
    file.Read(localPose.p.z);
    file.Read(localPose.q.x);
    file.Read(localPose.q.y);
    file.Read(localPose.q.z);
    file.Read(localPose.q.w);

    PxFilterData simData;
    file.Read(simData.word0);
    file.Read(simData.word1);
    file.Read(simData.word2);
    file.Read(simData.word3);

    PxFilterData queryData;
    file.Read(queryData.word0);
    file.Read(queryData.word1);
    file.Read(queryData.word2);
    file.Read(queryData.word3);

    uint16 rawFlags = 0;
    file.Read(rawFlags);

    shape->setLocalPose(localPose);
    shape->setSimulationFilterData(simData);
    shape->setQueryFilterData(queryData);
    shape->setFlags(PxShapeFlags(static_cast<PxU8>(rawFlags)));

    actor.attachShape(*shape);
    shape->release();
}

bool Physics::RayCastSingleTarget(const Vec3& origin, const Vec3& unitDirection, float maxDistance, PxActor* targetActor, RayCastHit& outHit) {

    class QueryFilterSingleActorCallback : public PxQueryFilterCallback {

    public:
        PxActor* onlyValidActor = nullptr;

        PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape,
                                       const PxRigidActor* actor, PxHitFlags& queryFlags) override {
            if (onlyValidActor == actor) {
                return PxQueryHitType::eBLOCK;
            }
            return PxQueryHitType::eNONE;
        }

        PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit,
                                        const PxShape* shape, const PxRigidActor* actor) override {
            return PxQueryHitType::eNONE;
        }
    };

    QueryFilterSingleActorCallback queryFilterSingleActorCallback;
    queryFilterSingleActorCallback.onlyValidActor = targetActor;

    PxQueryFilterData queryFilterData;
    queryFilterData.flags |= PxQueryFlag::ePREFILTER;

    PxRaycastBuffer hit;
    if (gPhysicsScene->raycast(ToPX(origin), ToPX(unitDirection), maxDistance, hit, PxHitFlag::ePOSITION | PxHitFlag::eNORMAL, queryFilterData, &queryFilterSingleActorCallback)) {
        const PxRaycastHit& closest = hit.block;
        outHit.position = ToDS(closest.position);
        outHit.normal = ToDS(closest.normal);
        outHit.distance = closest.distance;
        if (closest.actor) {
            outHit.entity = ToEntity(*closest.actor);
            outHit.actor = closest.actor;
        }
        if (closest.shape) {
            outHit.shape = closest.shape;
        }
        return true;
    }
    return false;
}
