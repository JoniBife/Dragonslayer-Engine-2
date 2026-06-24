#pragma once

#include "Components/HierarchySelection.hpp"
#include "Core/Export.hpp"
#include "EditorCamera.hpp"

// The currently selected element in the hierarchy
extern ENGINE_API HierarchySelection gHierarchySelection;

// The editor camera
extern ENGINE_API EditorCamera* gEditorCamera;

// Whether we are playing the game or just editing
extern ENGINE_API bool gIsPlayingGame;

// Whether we are detached from game
extern ENGINE_API bool gDetachedFromGame;
