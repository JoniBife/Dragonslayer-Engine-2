#include "Physics/Components/Joints/BallSocketJoint.hpp"
#include "Physics/Components/Joints/DistanceJoint.hpp"
#include "Physics/Components/Joints/FixedJoint.hpp"
#include "Physics/Components/Joints/GearJoint.hpp"
#include "Physics/Components/Joints/HingeJoint.hpp"
#include "Physics/Components/Joints/RackAndPinionJoint.hpp"
#include "Physics/Components/Joints/SixDOFJoint.hpp"
#include "Physics/Components/Joints/SliderJoint.hpp"

// Joint constructors are defined here (in Runtime) rather than inline in headers
// so that PhysX extension functions (PxD6JointCreate, etc.) resolve to Runtime.dll's
// copy of the static PhysX extensions library. This is critical for hot-reload:
// if these were inlined in headers compiled by Game.dll, the PhysX constraint shaders
// and vtables would point into Game.dll's code segment and dangle after FreeLibrary.

BallSocketJoint::BallSocketJoint(
    PxRigidActor* bodyA, PxRigidActor* bodyB,
    const PxTransform& localFrameA, const PxTransform& localFrameB
) {
    PxTransform globalPoseA = bodyA->getGlobalPose();
    PxTransform globalPoseB = bodyB->getGlobalPose();
    PxTransform globalJointPose = globalPoseA.transform(localFrameA);
    PxTransform computedLocalFrameB = globalPoseB.transformInv(globalJointPose);
    joint = PxSphericalJointCreate(*gPhysics, bodyA, localFrameA, bodyB, computedLocalFrameB);
}

DistanceJoint::DistanceJoint(
    PxRigidActor* bodyA, PxRigidActor* bodyB,
    const PxTransform& localFrameA, const PxTransform& localFrameB
) {
    PxTransform globalPoseA = bodyA->getGlobalPose();
    PxTransform globalPoseB = bodyB->getGlobalPose();
    PxTransform globalJointPose = globalPoseA.transform(localFrameA);
    PxTransform computedLocalFrameB = globalPoseB.transformInv(globalJointPose);
    joint = PxDistanceJointCreate(*gPhysics, bodyA, localFrameA, bodyB, computedLocalFrameB);
}

FixedJoint::FixedJoint(
    PxRigidActor* bodyA, PxRigidActor* bodyB,
    const PxTransform& localFrameA, const PxTransform& localFrameB
) {
    PxTransform globalPoseA = bodyA->getGlobalPose();
    PxTransform globalPoseB = bodyB->getGlobalPose();
    PxTransform globalJointPose = globalPoseA.transform(localFrameA);
    PxTransform computedLocalFrameB = globalPoseB.transformInv(globalJointPose);
    joint = PxFixedJointCreate(*gPhysics, bodyA, localFrameA, bodyB, computedLocalFrameB);
}

GearJoint::GearJoint(
    PxRigidActor* bodyA, PxRigidActor* bodyB,
    const PxTransform& localFrameA, const PxTransform& localFrameB
) {
    PxTransform globalPoseA = bodyA->getGlobalPose();
    PxTransform globalPoseB = bodyB->getGlobalPose();
    PxTransform globalJointPose = globalPoseA.transform(localFrameA);
    PxTransform computedLocalFrameB = globalPoseB.transformInv(globalJointPose);
    joint = PxGearJointCreate(*gPhysics, bodyA, localFrameA, bodyB, computedLocalFrameB);
}

HingeJoint::HingeJoint(
    PxRigidActor* bodyA, PxRigidActor* bodyB,
    const PxTransform& localFrameA, const PxTransform& localFrameB
) {
    PxTransform globalPoseA = bodyA->getGlobalPose();
    PxTransform globalPoseB = bodyB->getGlobalPose();
    PxTransform globalJointPose = globalPoseA.transform(localFrameA);
    PxTransform computedLocalFrameB = globalPoseB.transformInv(globalJointPose);
    joint = PxRevoluteJointCreate(*gPhysics, bodyA, localFrameA, bodyB, computedLocalFrameB);
}

RackAndPinionJoint::RackAndPinionJoint(
    PxRigidActor* bodyA, PxRigidActor* bodyB,
    const PxTransform& localFrameA, const PxTransform& localFrameB
) {
    PxTransform globalPoseA = bodyA->getGlobalPose();
    PxTransform globalPoseB = bodyB->getGlobalPose();
    PxTransform globalJointPose = globalPoseA.transform(localFrameA);
    PxTransform computedLocalFrameB = globalPoseB.transformInv(globalJointPose);
    joint = PxRackAndPinionJointCreate(*gPhysics, bodyA, localFrameA, bodyB, computedLocalFrameB);
}

SixDOFJoint::SixDOFJoint(
    PxRigidActor* bodyA, PxRigidActor* bodyB,
    const PxTransform& localFrameA, const PxTransform& localFrameB
) {
    PxTransform globalPoseA = bodyA->getGlobalPose();
    PxTransform globalPoseB = bodyB->getGlobalPose();
    PxTransform globalJointPose = globalPoseA.transform(localFrameA);
    PxTransform computedLocalFrameB = globalPoseB.transformInv(globalJointPose);
    joint = PxD6JointCreate(*gPhysics, bodyA, localFrameA, bodyB, computedLocalFrameB);
}

SliderJoint::SliderJoint(
    PxRigidActor* bodyA, PxRigidActor* bodyB,
    const PxTransform& localFrameA, const PxTransform& localFrameB
) {
    PxTransform globalPoseA = bodyA->getGlobalPose();
    PxTransform globalPoseB = bodyB->getGlobalPose();
    PxTransform globalJointPose = globalPoseA.transform(localFrameA);
    PxTransform computedLocalFrameB = globalPoseB.transformInv(globalJointPose);
    joint = PxPrismaticJointCreate(*gPhysics, bodyA, localFrameA, bodyB, computedLocalFrameB);
}
