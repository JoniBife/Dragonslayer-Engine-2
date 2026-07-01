#include "Editor/Systems/TransformGizmosSystem.hpp"

#include <ImGuizmo.h>
#include <imgui.h>

#include "Core/EngineGlobals.hpp"
#include "Editor/Components/HierarchySelection.hpp"
#include "Editor/EditorGlobals.hpp"
#include "Math/MathAux.hpp"
#include "Physics/Components/DynamicRigidBody.hpp"
#include "Renderer/Camera.hpp"
#include "Runtime/Components/PhysicsNPC.hpp"
#include "Runtime/Components/Transform.hpp"
#include "Runtime/Input.hpp"

void TransformGizmosSystem::Start(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }
    ImGuizmo::Style &style = ImGuizmo::GetStyle();

    // default values
    style.TranslationLineThickness = 6.0f;
    style.TranslationLineArrowSize = 12.0f;
    style.RotationLineThickness = 8.0f;
    style.RotationOuterLineThickness = 8.0f;
    style.ScaleLineThickness = 6.0f;
    style.ScaleLineCircleSize = 12.0f;
    style.HatchedAxisLineThickness = 0.0f;
    style.CenterCircleSize = 12.0f;

    constexpr ImVec4 xColor = ImVec4(254.f / 255.f, 70.f / 255.f, 36.f / 255.f, 1.f);
    constexpr ImVec4 yColor = ImVec4(131.f / 255.f, 191.f / 255.f, 87.f / 255.f, 1.f);
    constexpr ImVec4 zColor = ImVec4(61.f / 255.f, 123.f / 255.f, 242.f / 255.f, 1.f);
    constexpr ImVec4 selection = ImVec4(255.f / 255.f, 236.f / 255.f, 32.f / 255.f, 1.f);

    // initialize default colors
    style.Colors[ImGuizmo::DIRECTION_X] = xColor;
    style.Colors[ImGuizmo::DIRECTION_Y] = yColor;
    style.Colors[ImGuizmo::DIRECTION_Z] = zColor;
    style.Colors[ImGuizmo::PLANE_X] = xColor;
    style.Colors[ImGuizmo::PLANE_Y] = yColor;
    style.Colors[ImGuizmo::PLANE_Z] = zColor;
    style.Colors[ImGuizmo::SELECTION] = selection;
    style.Colors[ImGuizmo::INACTIVE] = ImVec4(0.600f, 0.600f, 0.600f, 0.600f);
    style.Colors[ImGuizmo::TRANSLATION_LINE] = selection;
    style.Colors[ImGuizmo::SCALE_LINE] = selection;
    style.Colors[ImGuizmo::ROTATION_USING_BORDER] = selection;
    style.Colors[ImGuizmo::ROTATION_USING_FILL] = selection;
    style.Colors[ImGuizmo::HATCHED_AXIS_LINES] = ImVec4(0.000f, 0.000f, 0.000f, 0.500f);
    style.Colors[ImGuizmo::TEXT] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    style.Colors[ImGuizmo::TEXT_SHADOW] = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
}

void TransformGizmosSystem::Update(ThreadContext& threadContext, Vault& vault) {

    if (!threadContext.IsMainThread()) {
        return;
    }

    if (!gDetachedFromGame) {
        return;
    }

    if (!vault.IsEntityValid(gHierarchySelection.selectedEntity)) {
        return;
    }

    Transform* selectedTransformPtr = nullptr;
    if (Transform* transform = vault.TryGetComponent<Transform>(gHierarchySelection.selectedEntity)) {
        selectedTransformPtr = transform;
    } else if (PhysicsNPC* physicsNpc = vault.TryGetComponent<PhysicsNPC>(gHierarchySelection.selectedEntity)) {
        selectedTransformPtr = &physicsNpc->transform;
    }

    // If selected entity does not have a transform we cannot show a gizmo so return
    if (!selectedTransformPtr) {
        return;
    }

    // We have a entity which is selected and has a transform so lets show the gizmo
    ImGuizmo::BeginFrame();
    Transform& selectedTransform = *selectedTransformPtr;

    EditorCamera& editorCam = *gEditorCamera;

    // Entity was double-clicked so focus on it
    if (gHierarchySelection.doubleClickedEntity) {
        constexpr float FOCUS_OFFSET = 10.f;
        editorCam.SetPosition(selectedTransform.location + Vec3::UP * FOCUS_OFFSET - Vec3::FORWARD * FOCUS_OFFSET);
        editorCam.SetTarget(selectedTransform.location);
    }

    static ImGuizmo::OPERATION selectedOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE selectedCoordinateSystem(ImGuizmo::WORLD);

    if (Input::IsKeyPressed(KeyCode::W)) {
        selectedOperation = ImGuizmo::OPERATION::TRANSLATE;
    } else if (Input::IsKeyPressed(KeyCode::E)) {
        selectedOperation = ImGuizmo::OPERATION::SCALE;
    } else if (Input::IsKeyPressed(KeyCode::R)) {
        selectedOperation = ImGuizmo::OPERATION::ROTATE;
    }

    if (Input::IsKeyPressed(KeyCode::M)) {
        if (selectedCoordinateSystem == ImGuizmo::WORLD) {
            selectedCoordinateSystem = ImGuizmo::LOCAL;
        } else {
            selectedCoordinateSystem = ImGuizmo::WORLD;
        }
    }

    ImGuiIO &io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    float viewMatrix[16];
    editorCam.camera.GetView().ToOpenGLFormat(viewMatrix);

    float projectionMatrix[16];
    editorCam.camera.GetProjection().ToOpenGLFormat(projectionMatrix);

    float rotationMatrix[16];
    selectedTransform.ToMat4().ToOpenGLFormat(rotationMatrix);

    if (gEditorCamera->IsFreeModeEnabled() || gEditorCamera->IsDragModeEnabled())
    {
        io.SetAppAcceptingEvents(false);
    }

    if (ImGuizmo::Manipulate(viewMatrix, projectionMatrix, selectedOperation, selectedCoordinateSystem, rotationMatrix, nullptr)) {
        float matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents(rotationMatrix, matrixTranslation, matrixRotation, matrixScale);

        const Vec3 newLocation = {matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]};
        const Quat newRotation =
                Quat(DegreesToRadians(matrixRotation[2]), Vec3::Z) *
                Quat(DegreesToRadians(matrixRotation[1]), Vec3::Y) *
                Quat(DegreesToRadians(matrixRotation[0]), Vec3::X);
        const Vec3 newScale = {matrixScale[0], matrixScale[1], matrixScale[2]};

        selectedTransform.location = newLocation;
        selectedTransform.rotation = newRotation;
        selectedTransform.scale = newScale;

        if (DynamicRigidBody* rigidBody = vault.TryGetComponent<DynamicRigidBody>(gHierarchySelection.selectedEntity)) {
            rigidBody->positionLastPhysicsUpdate = newLocation;
            rigidBody->rotationLastPhysicsUpdate = newRotation;
        }
    }

    if (gEditorCamera->IsFreeModeEnabled() || gEditorCamera->IsDragModeEnabled())
    {
        io.SetAppAcceptingEvents(true);
    }
}
