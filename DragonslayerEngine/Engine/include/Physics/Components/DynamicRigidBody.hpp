#pragma once

#include <PxPhysics.h>
#include <PxRigidDynamic.h>
#include <PxScene.h>

#include "Core/EngineGlobals.hpp"
#include "Core/Time.hpp"
#include "ECS/Component.hpp"
#include "Math/MathAux.hpp"
#include "Math/Quat.hpp"
#include "Math/Vec3.hpp"
#include "Physics/PhysicsCommon.hpp"
#include "Core/Containers/ArrayHelper.hpp"

using namespace physx;

ENGINE_DSTRUCT(DynamicRigidBody, .isSerializable = true, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(DynamicRigidBody)

    /* The position of the dynamic rigid body during the last physics update */
    Vec3 positionLastPhysicsUpdate = Vec3::ZERO;

    /* The rotation of the dynamic rigid body during the last physics update */
    Quat rotationLastPhysicsUpdate = Quat::IDENTITY;

    PxRigidDynamic* body = nullptr;

    bool addManually = false;

    bool receiveCollisionEvents = false;

    bool receiveTriggerEvents = false;

    explicit DynamicRigidBody(const PxTransform& transform = PxTransform(PxIdentity), const bool addManually = false)
    : addManually(addManually) {
        // We create the body immediately, however it's not yet added to the physics system
        // this is done later in the physics update in batches, unless addManually is true.
        body = gPhysics->createRigidDynamic(transform);
        ASSERT(body != nullptr, "Failed to create dynamic rigid body!");
    }

    explicit DynamicRigidBody(const Transform& transform, const bool addManually = false)
    : DynamicRigidBody(ToPX(transform), addManually) {
    }

#pragma region API
    // NOLINTBEGIN - Disables "Member function can be made const"

    NO_DISCARD FORCE_INLINE Entity GetEntity() {
        return ToEntity(*body);
    }

    NO_DISCARD FORCE_INLINE bool ExistsInScene() const {
        return body->getScene() != nullptr;
    }
    
    FORCE_INLINE void SetLinearVelocity(const Vec3& velocity, bool autowake = true) {
        body->setLinearVelocity(ToPX(velocity), autowake);
    }

    NO_DISCARD FORCE_INLINE Vec3 GetLinearVelocity() const {
        return ToDS(body->getLinearVelocity());
    }

    FORCE_INLINE void SetAngularVelocity(const Vec3& velocity, bool autowake = true) {
        body->setAngularVelocity(ToPX(velocity), autowake);
    }

    NO_DISCARD FORCE_INLINE Vec3 GetAngularVelocity() const {
        return ToDS(body->getAngularVelocity());
    }

    FORCE_INLINE void AddForce(const Vec3& force, PxForceMode::Enum mode = PxForceMode::eFORCE, bool autowake = true) {
        body->addForce(ToPX(force), mode, autowake);
    }

    FORCE_INLINE void AddTorque(const Vec3& torque, PxForceMode::Enum mode = PxForceMode::eFORCE, bool autowake = true) {
        body->addTorque(ToPX(torque), mode, autowake);
    }

    FORCE_INLINE void ClearForce(PxForceMode::Enum mode = PxForceMode::eFORCE) {
        body->clearForce(mode);
    }

    FORCE_INLINE void ClearTorque(PxForceMode::Enum mode = PxForceMode::eFORCE) {
        body->clearTorque(mode);
    }

    FORCE_INLINE void ConstrainMovement(PxRigidDynamicLockFlag::Enum flag, bool constrain) {
        body->setRigidDynamicLockFlag(flag, constrain);
    }

    // Not inline because relies in extension
    void SetMass(float mass, bool updateInertia = false);

    NO_DISCARD FORCE_INLINE float GetMass() const {
        return body->getMass();
    }

    FORCE_INLINE void MoveKinematic(const PxTransform& targetTransform) {
        body->setKinematicTarget(targetTransform);
    }

    FORCE_INLINE void SetTransform(const PxTransform& transform, bool autowake = true) {
        body->setGlobalPose(transform, autowake);
    }

    NO_DISCARD FORCE_INLINE PxTransform GetTransform() const {
        return body->getGlobalPose();
    }

    FORCE_INLINE void SetGravity(bool active) {
        body->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !active);
    }

    FORCE_INLINE void SetIsKinematic(bool isKinematic) {
        body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, isKinematic);
    }

    FORCE_INLINE bool IsKinematic() {
        return body->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC);
    }

    FORCE_INLINE bool SetIsTrigger(bool isTrigger) {
        const uint32 numShapes = body->getNbShapes();
        TempArray<PxShape*> shapes(numShapes, nullptr);
        if (body->getShapes(shapes.GetData(), numShapes) > 0) {
            for (PxShape* shape : shapes) {
                shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !isTrigger);
                shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, isTrigger);
            }
            return true;
        }
        return false;
    }

    FORCE_INLINE void WakeUp() {
        body->wakeUp();
    }

    FORCE_INLINE void Sleep() {
        body->putToSleep();
    }

    NO_DISCARD FORCE_INLINE bool IsSleeping() const {
        return body->isSleeping();
    }

    FORCE_INLINE void SetMaxAngularVelocity(float maxAngularVelocity) {
        body->setMaxAngularVelocity(maxAngularVelocity);
    }

    NO_DISCARD FORCE_INLINE float GetMaxAngularVelocity() const {
        return body->getMaxAngularVelocity();
    }

    FORCE_INLINE void SetMaxLinearVelocity(float maxLinearVelocity) {
        body->setMaxLinearVelocity(maxLinearVelocity);
    }

    NO_DISCARD FORCE_INLINE float GetMaxLinearVelocity() const {
        return body->getMaxLinearVelocity();
    }

    // Adds shape owned exclusively by this rigidbody
    FORCE_INLINE PxShape* AddExclusiveShape(
        const PxGeometry& geometry,
        const PxMaterial& material = *gDefaultPhysicsMaterial,
        PxShapeFlags shapeFlags = PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE
        ) {
#if WITH_EDITOR
        shapeFlags |= PxShapeFlag::eVISUALIZATION;
#endif

        PxShape* shape = gPhysics->createShape(geometry, material, true, shapeFlags);
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
    }

    // Adds shared shape
    FORCE_INLINE bool AddShape(PxShape& shape, const PxTransform& localPose = PxTransform(PxIdentity)) {
        shape.setLocalPose(localPose);
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

    NO_DISCARD FORCE_INLINE Vec3 GetInterpolatedPosition(const Vec3& positionCurrentPhysicsUpdate) const
    {
        return Vec3::Lerp(positionLastPhysicsUpdate, positionCurrentPhysicsUpdate, gTime.physicsUpdateInterpolationFactor);
    }

    NO_DISCARD FORCE_INLINE Quat GetInterpolatedRotation(const Quat& rotationCurrentPhysicsUpdate) const
    {
        return Quat::Slerp(rotationLastPhysicsUpdate,rotationCurrentPhysicsUpdate, gTime.physicsUpdateInterpolationFactor);
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

        const PxVec3 linearVelocity = body->getLinearVelocity();
        const PxVec3 angularVelocity = body->getAngularVelocity();
        file.Write(linearVelocity.x);
        file.Write(linearVelocity.y);
        file.Write(linearVelocity.z);
        file.Write(angularVelocity.x);
        file.Write(angularVelocity.y);
        file.Write(angularVelocity.z);

        const float mass = body->getMass();
        const float maxLinearVelocity = body->getMaxLinearVelocity();
        const float maxAngularVelocity = body->getMaxAngularVelocity();
        file.Write(mass);
        file.Write(maxLinearVelocity);
        file.Write(maxAngularVelocity);

        const PxRigidBodyFlags rigidBodyFlags = body->getRigidBodyFlags();
        const PxActorFlags actorFlags = body->getActorFlags();
        const bool isKinematic = rigidBodyFlags.isSet(PxRigidBodyFlag::eKINEMATIC);
        const bool gravityDisabled = actorFlags.isSet(PxActorFlag::eDISABLE_GRAVITY);
        file.Write(isKinematic);
        file.Write(gravityDisabled);

        const uint8 lockFlags = static_cast<uint8>(static_cast<PxU32>(body->getRigidDynamicLockFlags()));
        file.Write(lockFlags);
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

        new (obj) DynamicRigidBody(pose, addManually);
        DynamicRigidBody& instance = *static_cast<DynamicRigidBody*>(obj);
        instance.receiveCollisionEvents = receiveCollisionEvents;
        instance.receiveTriggerEvents = receiveTriggerEvents;

        uint32 numShapes = 0;
        file.Read(numShapes);
        for (uint32 i = 0; i < numShapes; ++i) {
            Physics::DeserializeAndAttachShape(file, *instance.body);
        }

        PxVec3 linearVelocity(0.f);
        PxVec3 angularVelocity(0.f);
        file.Read(linearVelocity.x);
        file.Read(linearVelocity.y);
        file.Read(linearVelocity.z);
        file.Read(angularVelocity.x);
        file.Read(angularVelocity.y);
        file.Read(angularVelocity.z);

        float mass = 0.f;
        float maxLinearVelocity = 0.f;
        float maxAngularVelocity = 0.f;
        file.Read(mass);
        file.Read(maxLinearVelocity);
        file.Read(maxAngularVelocity);

        bool isKinematic = false;
        bool gravityDisabled = false;
        file.Read(isKinematic);
        file.Read(gravityDisabled);

        uint8 lockFlags = 0;
        file.Read(lockFlags);

        // Kinematic flag must be set before velocities — kinematic bodies don't accept linear/angular velocity.
        instance.SetIsKinematic(isKinematic);
        instance.body->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, gravityDisabled);
        instance.SetMass(mass);
        instance.SetMaxLinearVelocity(maxLinearVelocity);
        instance.SetMaxAngularVelocity(maxAngularVelocity);

        if (!isKinematic) {
            instance.body->setLinearVelocity(linearVelocity, false);
            instance.body->setAngularVelocity(angularVelocity, false);
        }

        const PxRigidDynamicLockFlag::Enum allLockFlags[] = {
            PxRigidDynamicLockFlag::eLOCK_LINEAR_X,
            PxRigidDynamicLockFlag::eLOCK_LINEAR_Y,
            PxRigidDynamicLockFlag::eLOCK_LINEAR_Z,
            PxRigidDynamicLockFlag::eLOCK_ANGULAR_X,
            PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y,
            PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z,
        };
        for (PxRigidDynamicLockFlag::Enum flag : allLockFlags) {
            instance.ConstrainMovement(flag, (lockFlags & static_cast<uint8>(flag)) != 0);
        }
    }

#if WITH_EDITOR
    void OnHierarchy();
#endif

}; END_DSTRUCT(DynamicRigidBody)
