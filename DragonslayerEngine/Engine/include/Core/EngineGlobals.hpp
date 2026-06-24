#pragma once

#include "Core/Export.hpp"
#include "Core/Types.hpp"

// The engine!
extern ENGINE_API class Engine* gEngine;

// Global time, gets updated by the time system
extern ENGINE_API struct Time gTime;

// The main allocator from where all the memory comes from
extern ENGINE_API class FreeListAllocator* gGlobalAllocator;

// The allocator dedicated to the active game
extern ENGINE_API FreeListAllocator* gGameAllocator;

// Per-thread temp allocator, this allocator gets cleared every frame
extern thread_local class StackAllocator* gThreadTempAllocator;

// The context of the current thread
extern thread_local struct ThreadContext* gThreadContext;

// Total of threads explicitly created by the engine
extern ENGINE_API uint32 gNumThreads;

// Global window
extern ENGINE_API class Window* gWindow;

// Main camera, if set to null will default to default camera defined in the renderer
extern ENGINE_API class Camera* gCamera;

namespace physx {
    class PxPhysics;
    class PxMaterial;
    class PxScene;
    class PxControllerManager;
}
extern ENGINE_API physx::PxPhysics* gPhysics;

extern ENGINE_API physx::PxMaterial* gDefaultPhysicsMaterial;

extern ENGINE_API physx::PxScene* gPhysicsScene;

extern ENGINE_API physx::PxControllerManager* gPhysicsControllerManager;
