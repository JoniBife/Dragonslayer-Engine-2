#include "Core/EngineGlobals.hpp"

#include "Core/Time.hpp"

#include <Core/Assert.hpp>
#include <Core/ThreadContext.hpp>

Engine * gEngine = nullptr;

Time gTime;

FreeListAllocator* gGlobalAllocator = nullptr;

FreeListAllocator* gGameAllocator = nullptr;

// The context of the current thread
thread_local ThreadContext* gThreadContext = nullptr;

StackAllocator& GetThreadTempAllocator() { return GetThreadContext().tempAllocator; }

void SetThreadContext(ThreadContext* context) { gThreadContext = context; }
ThreadContext& GetThreadContext() {
  ASSERT(gThreadContext, "Failed to grab thread context, thread is not tracked!");
  return *gThreadContext;
}
bool HasThreadContext() { return gThreadContext != nullptr; }

uint32 gNumThreads = 0;

Window* gWindow = nullptr;

Camera* gCamera = nullptr;

physx::PxPhysics* gPhysics = nullptr;

physx::PxMaterial* gDefaultPhysicsMaterial = nullptr;

physx::PxScene* gPhysicsScene = nullptr;

physx::PxControllerManager* gPhysicsControllerManager = nullptr;