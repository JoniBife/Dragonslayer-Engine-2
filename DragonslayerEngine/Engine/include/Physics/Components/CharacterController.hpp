#pragma once

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <characterkinematic/PxController.h>
#include <characterkinematic/PxCapsuleController.h>
#include <characterkinematic/PxControllerManager.h>

#include "Core/Containers/ArrayHelper.hpp"
#include "Core/EngineGlobals.hpp"
#include "Core/Time.hpp"
#include "ECS/Component.hpp"
#include "Math/MathAux.hpp"
#include "Math/Quat.hpp"
#include "Math/Vec3.hpp"
#include "Physics/PhysicsCommon.hpp"

using namespace physx;

ENGINE_DSTRUCT(CharacterController, .numComponents = 4, .isSerializable = true, .hasMetadata = true, .isDisplayable = false) final {

    GENERATE_DSTRUCT_BODY(CharacterController)

    /* The position of the controller during the last physics update */
    Vec3 positionLastPhysicsUpdate = Vec3::ZERO;

    /* The rotation of the controller's entity during the last physics update (controller itself has no rotation) */
    Quat rotationLastPhysicsUpdate = Quat::IDENTITY;

    PxController* controller = nullptr;

    bool interpolateTransform = true;

    bool receiveCollisionEvents = false;

    bool receiveTriggerEvents = false;

    CharacterController(
        const Vec3& position,
        float height,
        float radius,
        const PxMaterial& material = *gDefaultPhysicsMaterial,
        float stepOffset = 0.3f,
        float slopeLimitRadians = 0.785398f /* 45 degrees */,
        float contactOffset = 0.1f) {

        PxCapsuleControllerDesc desc = {};
        desc.height = height;
        desc.radius = radius;
        // Necessary const_cast because unfortunately we cannot use a const designated initializer for CapsuleControllerDesc
        // and ultimately this desc is const (check createController).
        desc.material = &const_cast<PxMaterial&>(material);
        desc.position = PxExtendedVec3(position.x, position.y, position.z);
        desc.upDirection = PxVec3(0.f, 1.f, 0.f);
        desc.stepOffset = stepOffset;
        desc.slopeLimit = std::cosf(slopeLimitRadians);
        desc.contactOffset = contactOffset;
        desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
        ASSERT(desc.isValid(), "Invalid PxCapsuleControllerDesc!");

        controller = gPhysicsControllerManager->createController(desc);
        ASSERT(controller != nullptr, "Failed to create character controller!");
    }

#pragma region API
    // NOLINTBEGIN - Disables "Member function can be made const"

    NO_DISCARD FORCE_INLINE PxRigidDynamic* GetActor() const {
        return controller->getActor();
    }

    NO_DISCARD FORCE_INLINE Entity GetEntity() const {
        return ToEntity(*controller->getActor());
    }

    NO_DISCARD FORCE_INLINE bool ExistsInScene() const {
        return controller->getActor()->getScene() != nullptr;
    }

    // Main movement entry. Returns flags indicating which sides of the capsule are touching.
    FORCE_INLINE PxControllerCollisionFlags Move(
        const Vec3& displacement,
        float deltaTime,
        float minDistance = 0.001f,
        const PxControllerFilters& filters = PxControllerFilters()) {
        return controller->move(ToPX(displacement), minDistance, deltaTime, filters);
    }

    FORCE_INLINE void SetPosition(const Vec3& position) {
        controller->setPosition(PxExtendedVec3(position.x, position.y, position.z));
    }

    NO_DISCARD FORCE_INLINE Vec3 GetPosition() const {
        const PxExtendedVec3& p = controller->getPosition();
        return Vec3(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z));
    }

    // Foot position = bottom of capsule. Convenient for spawning on ground.
    FORCE_INLINE void SetFootPosition(const Vec3& footPosition) {
        controller->setFootPosition(PxExtendedVec3(footPosition.x, footPosition.y, footPosition.z));
    }

    NO_DISCARD FORCE_INLINE Vec3 GetFootPosition() const {
        const PxExtendedVec3& p = controller->getFootPosition();
        return Vec3(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z));
    }

    FORCE_INLINE void Resize(float height) {
        controller->resize(height);
    }

    FORCE_INLINE bool SetCollisionGroup(uint32 collisionGroup, uint32 collidesWith, bool triggerOnCollision = false) {
        PxRigidDynamic* actor = controller->getActor();
        const uint32 numShapes = actor->getNbShapes();
        TempArray<PxShape*> shapes(numShapes, nullptr);
        if (actor->getShapes(shapes.GetData(), numShapes) > 0) {
            PxFilterData filterData(collisionGroup, collidesWith,
                triggerOnCollision ? FilterFlags::FLAG_TRIGGER_CONTACT : FilterFlags::FLAG_NONE, 0);
            for (PxShape* shape : shapes) {
                shape->setSimulationFilterData(filterData);
                shape->setQueryFilterData(filterData);
            }
            return true;
        }
        return false;
    }

    NO_DISCARD FORCE_INLINE Vec3 GetInterpolatedPosition(const Vec3& positionCurrentPhysicsUpdate) const {
        return Vec3::Lerp(positionLastPhysicsUpdate, positionCurrentPhysicsUpdate, gTime.physicsUpdateInterpolationFactor);
    }

    NO_DISCARD FORCE_INLINE Quat GetInterpolatedRotation(const Quat& rotationCurrentPhysicsUpdate) const
    {
        return Quat::Slerp(rotationLastPhysicsUpdate,rotationCurrentPhysicsUpdate, gTime.physicsUpdateInterpolationFactor);
    }

    // NOLINTEND - End disables "Member function can be made const"
#pragma endregion

    void Serialize(VaultFile& file) const {
        file.Write(interpolateTransform);
        file.Write(receiveCollisionEvents);
        file.Write(receiveTriggerEvents);

        const PxExtendedVec3& pos = controller->getPosition();
        file.Write(pos.x);
        file.Write(pos.y);
        file.Write(pos.z);

        const PxCapsuleController* capsule = static_cast<const PxCapsuleController*>(controller);
        const float radius = capsule->getRadius();
        const float height = capsule->getHeight();
        const float stepOffset = controller->getStepOffset();
        const float slopeLimitCos = controller->getSlopeLimit();
        const float contactOffset = controller->getContactOffset();
        file.Write(radius);
        file.Write(height);
        file.Write(stepOffset);
        file.Write(slopeLimitCos);
        file.Write(contactOffset);

        // Capsule controllers always have exactly one shape on their actor.
        PxRigidDynamic* actor = controller->getActor();
        const uint32 numShapes = actor->getNbShapes();
        file.Write(numShapes);
        TempArray<PxShape*> shapes(numShapes, nullptr);
        actor->getShapes(shapes.GetData(), numShapes);
        for (PxShape* shape : shapes) {
            const PxFilterData simData = shape->getSimulationFilterData();
            file.Write(simData.word0);
            file.Write(simData.word1);
            file.Write(simData.word2);
            file.Write(simData.word3);

            const PxFilterData queryData = shape->getQueryFilterData();
            file.Write(queryData.word0);
            file.Write(queryData.word1);
            file.Write(queryData.word2);
            file.Write(queryData.word3);
        }
    }

    static void Deserialize(void* obj, VaultFile& file) {
        bool interpolateTransform = true;
        bool receiveCollisionEvents = false;
        bool receiveTriggerEvents = false;
        file.Read(interpolateTransform);
        file.Read(receiveCollisionEvents);
        file.Read(receiveTriggerEvents);

        double posX = 0.0;
        double posY = 0.0;
        double posZ = 0.0;
        file.Read(posX);
        file.Read(posY);
        file.Read(posZ);

        float radius = 0.f;
        float height = 0.f;
        float stepOffset = 0.f;
        float slopeLimitCos = 0.f;
        float contactOffset = 0.f;
        file.Read(radius);
        file.Read(height);
        file.Read(stepOffset);
        file.Read(slopeLimitCos);
        file.Read(contactOffset);

        const Vec3 position{ static_cast<float>(posX), static_cast<float>(posY), static_cast<float>(posZ) };
        const float slopeLimitRadians = acosf(slopeLimitCos);

        new (obj) CharacterController(position, height, radius, *gDefaultPhysicsMaterial, stepOffset, slopeLimitRadians, contactOffset);
        CharacterController& instance = *static_cast<CharacterController*>(obj);
        instance.interpolateTransform = interpolateTransform;
        instance.receiveCollisionEvents = receiveCollisionEvents;
        instance.receiveTriggerEvents = receiveTriggerEvents;

        // Restore position with full double precision (the controller ctor truncates to float).
        instance.controller->setPosition(PxExtendedVec3(posX, posY, posZ));

        uint32 numShapes = 0;
        file.Read(numShapes);
        PxRigidDynamic* actor = instance.controller->getActor();
        TempArray<PxShape*> shapes(numShapes, nullptr);
        actor->getShapes(shapes.GetData(), numShapes);
        for (uint32 i = 0; i < numShapes; ++i) {
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

            if (i < shapes.GetSize() && shapes[i] != nullptr) {
                shapes[i]->setSimulationFilterData(simData);
                shapes[i]->setQueryFilterData(queryData);
            }
        }
    }

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::Checkbox("Interpolate Transform", &interpolateTransform);
        if (controller) {
            PxCapsuleController* capsule = static_cast<PxCapsuleController*>(controller);
            ImGui::Text("Radius: %.2f", capsule->getRadius());
            ImGui::Text("Height: %.2f", capsule->getHeight());
            const PxExtendedVec3& p = controller->getPosition();
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", p.x, p.y, p.z);
        }
    }
#endif

}; END_DSTRUCT(CharacterController)
