#pragma once

#include "ECS/Component.hpp"

#include <extensions/PxRevoluteJoint.h>

#include "Physics/Components/DynamicRigidBody.hpp"
#include "Physics/Components/StaticRigidBody.hpp"

ENGINE_DSTRUCT(HingeJoint, .numComponents = 1024, .hasMetadata = true, .isDisplayable = false) final {

    GENERATE_DSTRUCT_BODY(HingeJoint)
    
    PxRevoluteJoint* joint = nullptr;

    // NOLINTBEGIN - Disables "Member function can be made const" and "Can be made reference to const"

    HingeJoint(
        PxRigidActor* bodyA, PxRigidActor* bodyB,
        const PxTransform& localFrameA = PxTransform(PxIdentity),
        const PxTransform& localFrameB = PxTransform(PxIdentity)
    );

    HingeJoint(
        DynamicRigidBody& bodyA, DynamicRigidBody& bodyB,
        const PxTransform& localFrameA = PxTransform(PxIdentity),
        const PxTransform& localFrameB = PxTransform(PxIdentity)
    ) : HingeJoint(bodyA.body, bodyB.body, localFrameA, localFrameB) {}

    HingeJoint(
        DynamicRigidBody& bodyA, StaticRigidBody& bodyB,
        const PxTransform& localFrameA = PxTransform(PxIdentity),
        const PxTransform& localFrameB = PxTransform(PxIdentity)
    ) : HingeJoint(bodyA.body, bodyB.body, localFrameA, localFrameB) {}

    HingeJoint(
        StaticRigidBody& bodyA, DynamicRigidBody& bodyB,
        const PxTransform& localFrameA = PxTransform(PxIdentity),
        const PxTransform& localFrameB = PxTransform(PxIdentity)
    ) : HingeJoint(bodyA.body, bodyB.body, localFrameA, localFrameB) {}

#pragma region API

    NO_DISCARD FORCE_INLINE bool ExistsInScene() const {
        return joint->getScene() != nullptr;
    }

    FORCE_INLINE void SetActors(PxRigidActor* actor0, PxRigidActor* actor1) {
        joint->setActors(actor0, actor1);
    }

    FORCE_INLINE void GetActors(PxRigidActor* actor0, PxRigidActor* actor1) const {
        joint->getActors(actor0, actor1);
    }

    FORCE_INLINE void GetEntities(Entity& entity0, Entity& entity1) const {
        PxRigidActor* actor0;
        PxRigidActor* actor1;
        joint->getActors(actor0, actor1);
        if (actor0 == nullptr || actor1 == nullptr) {
            ASSERT("Failed to retrieve entities from actors!")
            return;
        }
        entity0 = ToEntity(*actor0);
        entity1 = ToEntity(*actor1);
    }

    FORCE_INLINE void SetLocalPose0(const PxTransform& t) {
        joint->setLocalPose(PxJointActorIndex::eACTOR0, t);
    }

    NO_DISCARD FORCE_INLINE PxTransform GetLocalPose0() const {
        return joint->getLocalPose(PxJointActorIndex::eACTOR0);
    }

    FORCE_INLINE void SetLocalPose1(const PxTransform& t) {
        joint->setLocalPose(PxJointActorIndex::eACTOR1, t);
    }

    NO_DISCARD FORCE_INLINE PxTransform GetLocalPose1() const {
        return joint->getLocalPose(PxJointActorIndex::eACTOR1);
    }

#pragma endregion

    // NOLINTEND

#if WITH_EDITOR
    void OnHierarchy() {

    }
#endif

}; END_DSTRUCT(HingeJoint)

