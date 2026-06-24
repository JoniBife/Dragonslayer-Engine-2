#pragma once

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <PxRigidStatic.h>

#include "Core/Containers/ArrayHelper.hpp"
#include "Core/EngineGlobals.hpp"
#include "ECS/Component.hpp"
#include "Math/MathAux.hpp"
#include "Physics/PhysicsCommon.hpp"

using namespace physx;

ENGINE_DSTRUCT(StaticRigidBody, .isSerializable = true, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(StaticRigidBody)

    PxRigidStatic* body = nullptr;

    bool addManually = false;

    bool receiveCollisionEvents = false;

    bool receiveTriggerEvents = false;

    explicit StaticRigidBody(const PxTransform& transform = PxTransform(PxIdentity), const bool addManually = false)
    : addManually(addManually) {
        // We create the body immediately, however it's not yet added to the physics system
        // this is done later in the physics update in batches, unless addManually is true.
        body = gPhysics->createRigidStatic(transform);
        ASSERT(body != nullptr, "Failed to create static rigid body!");
    }

#pragma region API
    // NOLINTBEGIN - Disables "Member function can be made const"

    NO_DISCARD FORCE_INLINE bool ExistsInScene() const {
        return body->getScene() != nullptr;
    }

    // NOTE: Can be very expensive as it might cause internal data structures to be recomputed
    FORCE_INLINE void SetTransform(const PxTransform& transform, bool autowake = true) {
        body->setGlobalPose(transform, autowake);
    }

    NO_DISCARD FORCE_INLINE PxTransform GetTransform() const {
        return body->getGlobalPose();
    }

    // Adds shape owned exclusively by this rigidbody
    FORCE_INLINE PxShape* AddExclusiveShape(
        const PxGeometry& geometry,
        const PxMaterial& material = *gDefaultPhysicsMaterial,
        PxShapeFlags shapeFlags = PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE) {

#if WITH_EDITOR
        shapeFlags |= PxShapeFlag::eVISUALIZATION;
#endif
        
        PxShape* shape = gPhysics->createShape(geometry, material, true);
        PxFilterData filterData;
        filterData.word0 = 0xFFFFFFFF;
        filterData.word1 = 0xFFFFFFFF;
        shape->setQueryFilterData(filterData);
        shape->setSimulationFilterData(filterData);

        if (shape) {

            // attachShape increments the reference count
            bool success = body->attachShape(*shape);

            // PhysX shapes are X forward so we apply a local rotation
            if (geometry.getType() == PxGeometryType::eCAPSULE || geometry.getType() == PxGeometryType::ePLANE) {
                shape->setLocalPose(PxTransform(PxQuat(PI/2.f, PxVec3(0.f,0.f,1.f))));
            }

            // Release local reference so the actor owns the lifetime
            // Additionally if attach fails, we hold the only counted reference, and so this cleans up properly
            shape->release();

            if (success) {
                return shape;
            }
        }

        return nullptr;

        // Optional mass update (expensive if done for every shape in a compound)
        //if (updateMass) {
        //    PxRigidBodyExt::updateMassAndInertia(*body, 1.0f);
        //}
    }

    // Adds shared shape
    FORCE_INLINE bool AddShape(PxShape& shape) {
        return body->attachShape(shape);
    }

    FORCE_INLINE bool SetCollisionGroup(uint32 collisionGroup, uint32 collidesWith, bool triggerOnCollision = false) {
        const uint32 numShapes = body->getNbShapes();
        TempArray<PxShape*> shapes(numShapes, nullptr);
        if (body->getShapes(shapes.GetData(), numShapes) > 0) {
            PxFilterData filterData(collisionGroup, collidesWith, triggerOnCollision ? FilterFlags::FLAG_TRIGGER_CONTACT : FilterFlags::FLAG_NONE, 0);
            for (PxShape* shape : shapes) {
                shape->setSimulationFilterData(filterData);
                shape->setQueryFilterData(filterData);
            }
            return true;
        }
        return false;
    }

    FORCE_INLINE bool SetIsTrigger(bool isTrigger) {
        const uint32 numShapes = body->getNbShapes();
        TempArray<PxShape*> shapes(numShapes, nullptr);
        if (body->getShapes(shapes.GetData(), numShapes) > 0) {
            for (PxShape* shape : shapes) {
                shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
            }
            return true;
        }
        return false;
    }

    // NOLINTEND - End disables "Member function can be made const"
#pragma endregion

    void Serialize(VaultFile& file) const {
        file.Write(addManually);
        file.Write(receiveCollisionEvents);
        file.Write(receiveTriggerEvents);

        const PxTransform pose = body->getGlobalPose();
        file.Write(pose.p.x);
        file.Write(pose.p.y);
        file.Write(pose.p.z);
        file.Write(pose.q.x);
        file.Write(pose.q.y);
        file.Write(pose.q.z);
        file.Write(pose.q.w);

        const uint32 numShapes = body->getNbShapes();
        file.Write(numShapes);

        TempArray<PxShape*> shapes(numShapes, nullptr);
        body->getShapes(shapes.GetData(), numShapes);
        for (PxShape* shape : shapes) {
            Physics::SerializeShape(file, *shape);
        }
    }

    static void Deserialize(void* obj, VaultFile& file) {
        bool addManually = false;
        bool receiveCollisionEvents = false;
        bool receiveTriggerEvents = false;
        file.Read(addManually);
        file.Read(receiveCollisionEvents);
        file.Read(receiveTriggerEvents);

        PxTransform pose(PxIdentity);
        file.Read(pose.p.x);
        file.Read(pose.p.y);
        file.Read(pose.p.z);
        file.Read(pose.q.x);
        file.Read(pose.q.y);
        file.Read(pose.q.z);
        file.Read(pose.q.w);

        new (obj) StaticRigidBody(pose, addManually);
        StaticRigidBody& instance = *static_cast<StaticRigidBody*>(obj);
        instance.receiveCollisionEvents = receiveCollisionEvents;
        instance.receiveTriggerEvents = receiveTriggerEvents;

        uint32 numShapes = 0;
        file.Read(numShapes);
        for (uint32 i = 0; i < numShapes; ++i) {
            Physics::DeserializeAndAttachShape(file, *instance.body);
        }
    }

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::BeginDisabled();
        ImGui::Checkbox("Add To Scene Manually", &addManually);
        ImGui::EndDisabled();
    }
#endif

}; END_DSTRUCT(StaticRigidBody)
