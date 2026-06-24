#include "Physics/Components/DynamicRigidBody.hpp"

#include <extensions/PxRigidBodyExt.h>

#if WITH_EDITOR
#include <imgui.h>
#include "Editor/ImGuiExtensions.hpp"
#endif

using namespace physx;

void DynamicRigidBody::SetMass(float mass, bool updateInertia) {
    if (updateInertia) {
        PxRigidBodyExt::setMassAndUpdateInertia(*body, mass);
    } else {
        body->setMass(mass);
    }
}

#if WITH_EDITOR
void DynamicRigidBody::OnHierarchy() {
    PxRigidDynamic& body = *this->body;

    ImGui::BeginDisabled();

    //Vec3 linearVelocity = ToDS(body.getLinearVelocity());
    //Vec3 angularVelocity = ToDS(body.getAngularVelocity());
    //ImGui::InputVec3("Linear Velocity", linearVelocity);
    //ImGui::InputVec3("Angular Velocity", angularVelocity);

    //bool isSleeping = body.isSleeping();
    //ImGui::Checkbox("Is Sleeping", &isSleeping);

    ImGui::Checkbox("Add To Scene Manually", &addManually);

    ImGui::EndDisabled();

    ImGui::PushItemWidth(100.f);
    float mass = body.getMass();
    if (ImGui::InputFloat("Mass", &mass, 0, 0, "%.10f")) {
        mass = mass <= 0.f ? .00001f : mass;
        PxRigidBodyExt::setMassAndUpdateInertia(body, mass);
    }

    uint32 unsignedPositionIterations;
    uint32 unsignedVelocityIterations;
    body.getSolverIterationCounts(unsignedPositionIterations, unsignedVelocityIterations);

    int positionIterations = unsignedPositionIterations;
    int velocityIterations = unsignedVelocityIterations;
    const bool modifiedPositionIterations = ImGui::InputInt("Min Position Iterations", &positionIterations);
    const bool modifiedVelocityIterations = ImGui::InputInt("Min Velocity Iterations", &velocityIterations);

    if (modifiedPositionIterations || modifiedVelocityIterations) {
        positionIterations = std::clamp(positionIterations, 1, 255);
        velocityIterations = std::clamp(velocityIterations, 1, 255);
        body.setSolverIterationCounts(positionIterations, velocityIterations);
    }
    ImGui::PopItemWidth();

    const PxRigidBodyFlags rigidBodyFlags = body.getRigidBodyFlags();
    bool isKinematic = rigidBodyFlags.isSet(PxRigidBodyFlag::eKINEMATIC);
    if (ImGui::Checkbox("Is Kinematic", &isKinematic)) {
        body.setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, isKinematic);
    }

    const PxActorFlags actorFlags = body.getActorFlags();
    bool withGravity = !actorFlags.isSet(PxActorFlag::eDISABLE_GRAVITY);
    if (ImGui::Checkbox("With Gravity", &withGravity)) {
        body.setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !withGravity);
    }

    const PxRigidDynamicLockFlags rigidDynamicLockFlags = body.getRigidDynamicLockFlags();
    bool lockLinearX = rigidDynamicLockFlags.isSet(PxRigidDynamicLockFlag::eLOCK_LINEAR_X);
    bool lockLinearY = rigidDynamicLockFlags.isSet(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y);
    bool lockLinearZ = rigidDynamicLockFlags.isSet(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z);
    bool lockAngularX = rigidDynamicLockFlags.isSet(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X);
    bool lockAngularY = rigidDynamicLockFlags.isSet(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y);
    bool lockAngularZ = rigidDynamicLockFlags.isSet(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z);

    ImGui::Text("Constrain Translation");
    ImGui::PushID("Linear");

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
    ImGui::SameLine();
    if (ImGui::Checkbox("##X", &lockLinearX)) {
        body.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X, lockLinearX);
    }

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y");
    ImGui::SameLine();
    if (ImGui::Checkbox("##Y", &lockLinearY)) {
        body.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, lockLinearY);
    }

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z");
    ImGui::SameLine();
    if (ImGui::Checkbox("##Z", &lockLinearZ)) {
        body.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, lockLinearZ);
    }

    ImGui::PopID();

    ImGui::Text("Constrain Rotation");
    ImGui::PushID("Angular");

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
    ImGui::SameLine();
    if (ImGui::Checkbox("##X", &lockAngularX)) {
        body.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, lockAngularX);
    }

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y");
    ImGui::SameLine();
    if (ImGui::Checkbox("##Y", &lockAngularY)) {
        body.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, lockAngularY);
    }

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z");
    ImGui::SameLine();
    if (ImGui::Checkbox("##Z", &lockAngularZ)) {
        body.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, lockAngularZ);
    }

    ImGui::PopID();
}
#endif
