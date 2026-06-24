#include "Editor/EditorGlobals.hpp"


HierarchySelection gHierarchySelection = { ECS::InvalidEntity, false };

EditorCamera* gEditorCamera = nullptr;

bool gIsPlayingGame = false;

bool gDetachedFromGame = true;