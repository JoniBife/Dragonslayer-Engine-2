#pragma once

#include "ECS/System.hpp"

#include <PxPhysicsAPI.h>
#include <characterkinematic/PxControllerManager.h>

#include "Core/Allocators/FreeListAllocator.hpp"
#include "Core/Containers/ArrayHelper.hpp"
#include "Core/Lock.hpp"
#include "Core/Log.hpp"
#include "Physics/Components/CollisionEvent.hpp"
#include "Physics/Components/TriggerEvent.hpp"
#include "Physics/PhysicsCommon.hpp"

using namespace physx;

// TODO - Limit coordinates ( PxSceneDesc::sanityBounds)
//
class PhysXAllocator final : public PxAllocatorCallback
{
private:
    FreeListAllocator& allocator;
    SpinLock lock;

public:
    explicit PhysXAllocator(FreeListAllocator& allocator) : allocator(allocator) {}

    void* allocate(size_t size, const char*, const char*, int) override
    {
        ScopeLock scopeLock(lock);
        void* ptr = allocator.Allocate(size, 16 /* PhysX required alignment */);
        ASSERT((reinterpret_cast<size_t>(ptr) & 15) == 0);
        return ptr;
    }

    void deallocate(void* ptr) override
    {
        ScopeLock scopeLock(lock);
        allocator.Free(static_cast<uint8*>(ptr));
    }
};

class PhysXErrorCallback : public PxErrorCallback
{
public:
    void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
        const char* errorCode = nullptr;
        LogType logType = LogType::Info;

        switch (code)
        {
            case PxErrorCode::eNO_ERROR:
                errorCode = "no error";
                break;
            case PxErrorCode::eINVALID_PARAMETER:
                logType = LogType::Error;
                errorCode = "invalid parameter";
                break;
            case PxErrorCode::eINVALID_OPERATION:
                logType = LogType::Error;
                errorCode = "invalid operation";
                break;
            case PxErrorCode::eOUT_OF_MEMORY:
                logType = LogType::Error;
                errorCode = "out of memory";
                break;
            case PxErrorCode::eDEBUG_INFO:
                errorCode = "info";
                break;
            case PxErrorCode::eDEBUG_WARNING:
                logType = LogType::Warning;
                errorCode = "warning";
                break;
            case PxErrorCode::ePERF_WARNING:
                logType = LogType::Error;
                errorCode = "performance warning";
                break;
            case PxErrorCode::eABORT:
                logType = LogType::Error;
                errorCode = "abort";
                break;
            case PxErrorCode::eINTERNAL_ERROR:
                logType = LogType::Error;
                errorCode = "internal error";
                break;
            case PxErrorCode::eMASK_ALL:
                logType = LogType::Error;
                errorCode = "unknown error";
                break;
        }

        if(errorCode)
        {
            char logString[MAX_LOG_LENGTH]; // TODO This was 1024 by default
            sprintf(logString, "[PHYSX] %s (%d) : %s : %s\n", file, line, errorCode, message);

            Log::Any(logType, logString);

            ASSERT(code != PxErrorCode::eABORT);
        }
    }
};

static FORCE_INLINE PxFilterFlags PhysXFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // Check if shape A's group is in shape B's mask, AND shape B's group is in shape A's mask
    if (!(filterData0.word0 & filterData1.word1) || !(filterData1.word0 & filterData0.word1))
    {
        // They don't collide. Suppress the collision entirely.
        return PxFilterFlag::eSUPPRESS;
    }

    // Let triggers through
    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
    {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }

    // Generate standard physical contacts
    pairFlags = PxPairFlag::eCONTACT_DEFAULT;

    // If either object has the FLAG_TRIGGER_CONTACT bit set in word2, ask PhysX to notify us
    if (filterData0.word2 & FLAG_TRIGGER_CONTACT || filterData1.word2 & FLAG_TRIGGER_CONTACT)
    {
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
    }

    return PxFilterFlag::eDEFAULT;
}

class PhysXSimulationCallback : public PxSimulationEventCallback {

public:
    // TODO Replace by a better thread-safe data structure, perhaps we can simply use thread local data since this
    // is a MPSC (Multiple Producer Single Consumer) problem
    SpinLock collisionEventsLock;
    SpinLock triggerEventsLock;
    Array<CollisionEvent, FreeListAllocator> collisionEvents;
    Array<TriggerEvent, FreeListAllocator> triggerEvents;

    explicit PhysXSimulationCallback(FreeListAllocator& allocator) :
        collisionEvents(128, &allocator),
        triggerEvents(128, &allocator) {}

    void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override {
        //Log::Info("OnConstraintBreak");
    }

    void onWake(PxActor** actors, PxU32 count) override {
        //Log::Info("OnWake");
    }

    void onSleep(PxActor** actors, PxU32 count) override {
        //Log::Info("OnSleep");
    }

    void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override {
        ScopeLock scopeLock(collisionEventsLock);
        collisionEvents.Emplace(pairHeader.actors[0], pairHeader.actors[1]);
    }

    void onTrigger(PxTriggerPair* pairs, PxU32 count) override {
        ScopeLock scopeLock(triggerEventsLock);
        for (uint32 i = 0; i < count; i++) {
            PxTriggerPair& triggerPair = pairs[i];
            const bool enteredTrigger = PxPairFlags(triggerPair.status).isSet(PxPairFlag::eNOTIFY_TOUCH_FOUND);
            triggerEvents.Emplace(triggerPair.triggerActor, triggerPair.otherActor, enteredTrigger);
        }
    }

    void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, PxU32 count) override {
        //Log::Info("OnAdvance");
    }

    void ResetEvents() {
        collisionEvents.Reset();
        triggerEvents.Reset();
    }
};

struct PhysicsSettings {
    size_t usedMemory;
    size_t scratchMemory; // Needs to be a multiple of 16
    bool enablePhysicsVisualDebugger;
};

/*
 * TODO Memory is not being 100% reset on stop (i.e. Play -> Stop)
 */
class PhysicsSystem final {

    GENERATE_SYSTEM_BODY(PhysicsSystem)

private:
    PhysicsSettings settings;

    FreeListAllocator allocator;
    uint8* scratchBuffer;
    PhysXAllocator physXAllocator; // TODO Consider switching to a better allocator, perhaps Stack
    //PxDefaultAllocator defaultAllocator;

    PxPhysics* physics = nullptr;
    PxScene* scene = nullptr;
    PxFoundation* foundation = nullptr;
    PxDefaultCpuDispatcher*	dispatcher = nullptr;
    PxControllerManager* controllerManager = nullptr;

#if WITH_EDITOR
    PxPvd* pvd = nullptr;
#endif

    PhysXSimulationCallback simulationCallback;
    PhysXErrorCallback errorCallback;

    PxMaterial* defaultMaterial = nullptr;
    HashMap<NameString, PxMaterial*, FreeListAllocator> materials;

public:
    explicit PhysicsSystem(const PhysicsSettings& settings);
    ~PhysicsSystem();

    void Start(ThreadContext& threadContext, Vault& vault);
    void Update(ThreadContext& threadContext, Vault& vault);
    void PostPhysicsUpdate(ThreadContext& threadContext, Vault& vault);
    void LateUpdate(ThreadContext& threadContext, Vault& vault);
    void End(ThreadContext& threadContext, Vault& vault);

    void AddNewBodiesToPhysicsScene(Vault& vault);

    void CreatePhysicsScene();
    void DestroyPhysicsScene();

    const PxMaterial& CreatePhysicsMaterial(
        const NameString& name,
        float staticFriction,
        float dynamicFriction,
        float restitution);

    const PxMaterial& GetExistingMaterial(const NameString& name);

#if WITH_EDITOR
    void ShowStats() const;
#endif

};
