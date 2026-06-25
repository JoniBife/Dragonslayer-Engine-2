#include "Editor/Systems/EditorCameraSystem.hpp"

#include <Physics/Components/StaticRigidBody.hpp>
#include <Physics/PhysicsCommon.hpp>
#include <Renderer/Components/PrimitiveRenderer.hpp>
#include <Runtime/Components/Transform.hpp>

#include "Core/EngineGlobals.hpp"
#include "Core/Time.hpp"

#include "Editor/EditorCamera.hpp"
#include "Editor/EditorGlobals.hpp"

#include "Runtime/Input.hpp"

void EditorCameraSystem::Start(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }
    editorCamera.SetPosition(Vec3(0.0, 5.0, 10.0));
    editorCamera.camera.SetFarPlane(2000.0);

    gEditorCamera = &editorCamera;

    /*BufferedFile mapFile = BufferedFile<>::OpenOrCreate("Map.txt");
    vault.Deserialize(mapFile);
    mapFile.Close();*/
}

void EditorCameraSystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }
	if (gDetachedFromGame)
	{
	    editorCamera.Move(gTime.unscaledDeltaTime);
	}

    /*ImGui::Begin("SaveSystem");

    if (ImGui::Button("Save")) {
        BufferedFile mapFile = BufferedFile<>::OpenOrCreate("Map.txt");
        vault.Serialize(mapFile);
        mapFile.Close();
    }

    if (ImGui::Button("Load")) {
        BufferedFile mapFile = BufferedFile<>::OpenOrCreate("Map.txt");
        vault.Deserialize(mapFile);
        mapFile.Close();
    }

    ImGui::End(); */
}