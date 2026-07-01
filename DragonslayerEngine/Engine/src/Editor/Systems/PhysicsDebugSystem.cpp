#include "Editor/Systems/PhysicsDebugSystem.hpp"
#include "Editor/ImGuiExtensions.hpp"

#include <PxScene.h>
#include <imgui.h>

#include "Core/EngineGlobals.hpp"
#include "Editor/EditorGlobals.hpp"
#include "Runtime/Input.hpp"
#include "Runtime/Engine.hpp"

using namespace physx;

static bool showPhysicsData = false;

static uint32 numStatic = 0;
static uint32 numDynamic = 0;
static uint32 numConstraints = 0;
static PxSimulationStatistics stats;

void PhysicsDebugSystem::PostPhysicsUpdate(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    if (showPhysicsData) {
        numStatic  = gPhysicsScene->getNbActors(PxActorTypeFlag::eRIGID_STATIC);
        numDynamic = gPhysicsScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
        numConstraints = gPhysicsScene->getNbConstraints();
        gPhysicsScene->getSimulationStatistics(stats);
    }
}

void PhysicsDebugSystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    if (!gIsPlayingGame)
    {
        return;
    }

    if (Input::IsKeyHeld(KeyCode::LControl) && Input::IsKeyPressed(KeyCode::Four)) {
        showPhysicsData = !showPhysicsData;
    }

    if (ImGui::MainMenuBarItem("Stats", "Physics", "CTRL+4", &showPhysicsData)) {

        ImGui::Begin("Physics Data", &showPhysicsData);

        ImGui::SeparatorText("Actors");
        ImGui::Text("Static Actors: %u", numStatic);
        ImGui::Text("Dynamic Actors: %u", numDynamic);
        ImGui::Text("Active Dynamic Bodies: %u", stats.nbActiveDynamicBodies);
        ImGui::Text("Active Kinematic Bodies: %u", stats.nbActiveKinematicBodies);
        ImGui::Text("Total Actors: %u", numStatic + numDynamic);

        ImGui::SeparatorText("Collision");
        ImGui::Text("Discrete Contact Pairs: %u", stats.nbDiscreteContactPairsTotal);
        ImGui::Text("New Pairs: %u", stats.nbNewPairs);
        ImGui::Text("Lost Pairs: %u", stats.nbLostPairs);
        ImGui::Text("Trigger Pairs: %u", stats.nbTriggerPairs);

        ImGui::SeparatorText("Shapes");
        const char* typeNames[] = {"Sphere", "Plane", "Capsule", "Box", "ConvexCore", "Mesh",  "ParticleSystem",  "Tetrahedron", "HeightField", "TriangleMesh", "Custom"};
        for (PxU32 i = 0; i < PxGeometryType::eGEOMETRY_COUNT; ++i) {
            ImGui::Text("%s: %u", typeNames[i], stats.nbShapes[i]);
        }

        ImGui::SeparatorText("Scene");
        ImGui::Text("Constraints: %u", gPhysicsScene->getNbConstraints());
        ImGui::Text("Broad Phase Adds: %u", stats.nbBroadPhaseAdds);
        ImGui::Text("Broad Phase Removes: %u", stats.nbBroadPhaseRemoves);

        ImGui::End();
    }

    gEngine->GetEngineSystem<PhysicsSystem>().ShowStats();
}
