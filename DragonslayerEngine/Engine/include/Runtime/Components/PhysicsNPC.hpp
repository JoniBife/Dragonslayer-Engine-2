#pragma once

#include "ECS/Component.hpp"

#include "Physics/Components/DynamicRigidBody.hpp"
#include "Renderer/Components/PrimitiveRenderer.hpp"
#include "Transform.hpp"

// Represents an NPC that is moved by physics
// Useful for when we know that the entity needs to have these 3 components
// Its much faster to iterate on very large entities data sets when they are contiguous
DSTRUCT(PhysicsNPC, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(PhysicsNPC)

    Transform transform;
    DynamicRigidBody dynamicRigidBody;
    PrimitiveRenderer primitiveRenderer;

    explicit PhysicsNPC(const Transform& transform = Transform(), const bool addManuallyToPhysicsScene = false) :
        transform(transform),
        dynamicRigidBody(transform, addManuallyToPhysicsScene) {
    }

    void Serialize(VaultFile& file) const {
        transform.Serialize(file);
        dynamicRigidBody.Serialize(file);
        primitiveRenderer.Serialize(file);
    }

    static void Deserialize(void* obj, VaultFile& file) {
        // Construct each sub-component in-place via its own Deserialize so we don't run
        // PhysicsNPC's constructor (which would allocate a throwaway DynamicRigidBody body
        // that the subsequent placement-new would leak).
        PhysicsNPC* physicsNPC = static_cast<PhysicsNPC*>(obj);
        Transform::Deserialize(&physicsNPC->transform, file);
        DynamicRigidBody::Deserialize(&physicsNPC->dynamicRigidBody, file);
        PrimitiveRenderer::Deserialize(&physicsNPC->primitiveRenderer, file);
    }

#if WITH_EDITOR
    void OnHierarchy() {
        {
            static bool isOpen = true;
            ImGui::SetNextItemOpen(isOpen);
            if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_Framed)) {
                isOpen = true;
                transform.OnHierarchy();
                ImGui::TreePop();
            } else {
                isOpen = false;
            }
        }
        {
            static bool isOpen = true;
            ImGui::SetNextItemOpen(isOpen);
            if (ImGui::TreeNodeEx("DynamicRigidBody", ImGuiTreeNodeFlags_Framed)) {
                isOpen = true;
                dynamicRigidBody.OnHierarchy();
                ImGui::TreePop();
            } else {
                isOpen = false;
            }
        }
        {
            static bool isOpen = true;
            ImGui::SetNextItemOpen(isOpen);
            if (ImGui::TreeNodeEx("PrimitiveRenderer", ImGuiTreeNodeFlags_Framed)) {
                isOpen = true;
                primitiveRenderer.OnHierarchy();
                ImGui::TreePop();
            } else {
                isOpen = false;
            }
        }
    }
#endif

}; END_DSTRUCT(PhysicsNPC)
