#include "Core/EngineGlobals.hpp"

#include "Core/Time.hpp"

Engine* gEngine = nullptr;

Time gTime;

FreeListAllocator* gGlobalAllocator = nullptr;

FreeListAllocator* gGameAllocator = nullptr;

thread_local StackAllocator* gThreadTempAllocator = nullptr;

thread_local ThreadContext* gThreadContext = nullptr;

uint32 gNumThreads = 0;

Window* gWindow = nullptr;

Camera* gCamera = nullptr;

physx::PxPhysics* gPhysics = nullptr;

physx::PxMaterial* gDefaultPhysicsMaterial = nullptr;

physx::PxScene* gPhysicsScene = nullptr;

physx::PxControllerManager* gPhysicsControllerManager = nullptr;