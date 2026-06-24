#pragma once

#include "ECS/System.hpp"
#include "Editor/EditorCamera.hpp"

class EditorCameraSystem final {

    GENERATE_SYSTEM_BODY(EditorCameraSystem)

private:
    EditorCamera editorCamera;

public:
    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
};
