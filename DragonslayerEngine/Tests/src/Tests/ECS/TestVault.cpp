#include "Framework/TestFramework.hpp"
#include <Core/Memory.hpp>
#include <Core/Allocators/LinearAllocator.hpp>
#include <ECS/Vault.hpp>

// ---------------------------------------------------------------------------
// Test components (local to this file)
// ---------------------------------------------------------------------------

DSTRUCT(Position, .numComponents = 32) final {

    GENERATE_DSTRUCT_BODY(Position)

    float x = 0.0f;
    float y = 0.0f;
}; END_DSTRUCT(Position)

DSTRUCT(Velocity, .numComponents = 32) final {

    GENERATE_DSTRUCT_BODY(Velocity)

    float vx = 0.0f;
    float vy = 0.0f;

}; END_DSTRUCT(Velocity)


DSTRUCT(Tracker, .numComponents = 32) final  {

    GENERATE_DSTRUCT_BODY(Tracker)

    uint32* aliveCount;

    Tracker() : aliveCount(nullptr) {}
    explicit Tracker(uint32* counter) : aliveCount(counter) { if (aliveCount) ++(*aliveCount); }
    Tracker(Tracker&& other) noexcept : aliveCount(other.aliveCount) { other.aliveCount = nullptr; }
    ~Tracker() { if (aliveCount) --(*aliveCount); }

    Tracker(const Tracker&) = delete;
    Tracker& operator=(const Tracker&) = delete;

}; END_DSTRUCT(Tracker)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr size_t BUFFER_SIZE = Kb(128);

struct TestVault {
    alignas(16) uint8 buffer[BUFFER_SIZE];
    LinearAllocator alloc;
    Vault vault;

    TestVault()
        : alloc(buffer, BUFFER_SIZE)
        , vault(VaultSettings{ .maxMemory = Kb(64), .maxEntities = 64, .maxComponentTypes = 8 }, alloc)
    {}
};

// ---------------------------------------------------------------------------
// Entity lifecycle
// ---------------------------------------------------------------------------

TEST(Vault, CreateEntity) {
    TestVault tv;
    Entity e = tv.vault.CreateEntity();
    TEST_CHECK_EQUAL(e, ECS::FirstEntity);
    TEST_CHECK(tv.vault.IsEntityValid(e));
}

TEST(Vault, CreateMultipleEntities) {
    TestVault tv;
    Entity e1 = tv.vault.CreateEntity();
    Entity e2 = tv.vault.CreateEntity();
    Entity e3 = tv.vault.CreateEntity();
    TEST_CHECK_EQUAL(e1, ECS::FirstEntity);
    TEST_CHECK_EQUAL(e2, ECS::FirstEntity + 1);
    TEST_CHECK_EQUAL(e3, ECS::FirstEntity + 2);
}

TEST(Vault, DestroyEntity) {
    TestVault tv;
    Entity e = tv.vault.CreateEntity();
    TEST_CHECK(tv.vault.IsEntityValid(e));
    tv.vault.DestroyEntity(e);
    TEST_CHECK_FALSE(tv.vault.IsEntityValid(e));
}

TEST(Vault, ReuseDestroyedEntity) {
    TestVault tv;
    Entity e1 = tv.vault.CreateEntity();
    tv.vault.DestroyEntity(e1);
    Entity e2 = tv.vault.CreateEntity();
    TEST_CHECK_EQUAL(e1, e2);
    TEST_CHECK(tv.vault.IsEntityValid(e2));
}

TEST(Vault, IsEntityValidOnInvalid) {
    TestVault tv;
    TEST_CHECK_FALSE(tv.vault.IsEntityValid(ECS::InvalidEntity));
}

TEST(Vault, EntityIterator) {
    TestVault tv;
    Entity e1 = tv.vault.CreateEntity();
    Entity e2 = tv.vault.CreateEntity();
    Entity e3 = tv.vault.CreateEntity();
    tv.vault.DestroyEntity(e2);

    uint32 count = 0;
    bool sawE1 = false, sawE2 = false, sawE3 = false;
    for (Entity e : tv.vault.GetAllEntities()) {
        if (e == e1) sawE1 = true;
        if (e == e2) sawE2 = true;
        if (e == e3) sawE3 = true;
        ++count;
    }
    TEST_CHECK_EQUAL(count, 2u);
    TEST_CHECK(sawE1);
    TEST_CHECK_FALSE(sawE2);
    TEST_CHECK(sawE3);
}

// ---------------------------------------------------------------------------
// Component basics
// ---------------------------------------------------------------------------

TEST(Vault, EmplaceAndGetComponent) {
    TestVault tv;
    Entity e = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Position>(e, 1.0f, 2.0f);

    Position& pos = tv.vault.GetComponent<Position>(e);
    TEST_CHECK_EQUAL(pos.x, 1.0f);
    TEST_CHECK_EQUAL(pos.y, 2.0f);
}

TEST(Vault, TryGetComponentExists) {
    TestVault tv;
    Entity e = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Position>(e, 5.0f, 6.0f);

    Position* pos = tv.vault.TryGetComponent<Position>(e);
    TEST_REQUIRE(pos != nullptr);
    TEST_CHECK_EQUAL(pos->x, 5.0f);
}

TEST(Vault, TryGetComponentMissing) {
    TestVault tv;
    Entity e = tv.vault.CreateEntity();

    Position* pos = tv.vault.TryGetComponent<Position>(e);
    TEST_CHECK(pos == nullptr);
}

TEST(Vault, HasComponent) {
    TestVault tv;
    Entity e = tv.vault.CreateEntity();

    TEST_CHECK_FALSE(tv.vault.HasComponent<Position>(e));
    tv.vault.EmplaceComponent<Position>(e, 0.0f, 0.0f);
    TEST_CHECK(tv.vault.HasComponent<Position>(e));
}

TEST(Vault, RemoveComponent) {
    TestVault tv;
    Entity e = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Position>(e, 1.0f, 2.0f);

    TEST_CHECK(tv.vault.HasComponent<Position>(e));
    tv.vault.RemoveAndSwapComponent<Position>(e);
    TEST_CHECK_FALSE(tv.vault.HasComponent<Position>(e));
}

TEST(Vault, GetNumberOfComponents) {
    TestVault tv;
    Entity e = tv.vault.CreateEntity();

    TEST_CHECK_EQUAL(tv.vault.GetNumberOfComponents(e), 0u);
    tv.vault.EmplaceComponent<Position>(e, 0.0f, 0.0f);
    TEST_CHECK_EQUAL(tv.vault.GetNumberOfComponents(e), 1u);
    tv.vault.EmplaceComponent<Velocity>(e, 0.0f, 0.0f);
    TEST_CHECK_EQUAL(tv.vault.GetNumberOfComponents(e), 2u);
    tv.vault.RemoveAndSwapComponent<Position>(e);
    TEST_CHECK_EQUAL(tv.vault.GetNumberOfComponents(e), 1u);
}

TEST(Vault, GetOrEmplaceComponentNew) {
    TestVault tv;
    Entity e = tv.vault.CreateEntity();

    Position& pos = tv.vault.GetOrEmplaceComponent<Position>(e, 3.0f, 4.0f);
    TEST_CHECK_EQUAL(pos.x, 3.0f);
    TEST_CHECK_EQUAL(pos.y, 4.0f);
    TEST_CHECK(tv.vault.HasComponent<Position>(e));
}

TEST(Vault, GetOrEmplaceComponentExisting) {
    TestVault tv;
    Entity e = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Position>(e, 1.0f, 2.0f);

    Position& pos = tv.vault.GetOrEmplaceComponent<Position>(e, 99.0f, 99.0f);
    TEST_CHECK_EQUAL(pos.x, 1.0f);
    TEST_CHECK_EQUAL(pos.y, 2.0f);
    TEST_CHECK_EQUAL(tv.vault.GetNumberOfComponents(e), 1u);
}

// ---------------------------------------------------------------------------
// Component iteration
// ---------------------------------------------------------------------------

TEST(Vault, GetComponents) {
    TestVault tv;

    Entity e1 = tv.vault.CreateEntity();
    Entity e2 = tv.vault.CreateEntity();
    Entity e3 = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Position>(e1, 1.0f, 0.0f);
    tv.vault.EmplaceComponent<Position>(e2, 2.0f, 0.0f);
    tv.vault.EmplaceComponent<Position>(e3, 3.0f, 0.0f);

    auto components = tv.vault.GetComponents<Position>();
    uint32 count = 0;
    float sum = 0.0f;
    for (auto& pair : components) {
        sum += pair.component.x;
        ++count;
    }
    TEST_CHECK_EQUAL(count, 3u);
    TEST_CHECK_EQUAL(sum, 6.0f);
}

// ---------------------------------------------------------------------------
// Destroy entity cleans components
// ---------------------------------------------------------------------------

TEST(Vault, DestroyEntityCleansComponents) {
    TestVault tv;

    Entity e1 = tv.vault.CreateEntity();
    Entity e2 = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Position>(e1, 1.0f, 0.0f);
    tv.vault.EmplaceComponent<Position>(e2, 2.0f, 0.0f);

    auto before = tv.vault.GetComponents<Position>();
    TEST_CHECK_EQUAL(before.GetSize(), 2u);

    tv.vault.DestroyEntity(e1);

    auto after = tv.vault.GetComponents<Position>();
    TEST_CHECK_EQUAL(after.GetSize(), 1u);
}

// ---------------------------------------------------------------------------
// Destructor correctness
// ---------------------------------------------------------------------------

TEST(Vault, RemoveCallsDestructor) {
    TestVault tv;
    uint32 aliveCount = 0;

    Entity e = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Tracker>(e, &aliveCount);
    TEST_CHECK_EQUAL(aliveCount, 1u);

    tv.vault.RemoveAndSwapComponent<Tracker>(e);
    TEST_CHECK_EQUAL(aliveCount, 0u);
}

TEST(Vault, DestroyEntityCallsDestructor) {
    TestVault tv;
    uint32 aliveCount = 0;

    Entity e = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Tracker>(e, &aliveCount);
    TEST_CHECK_EQUAL(aliveCount, 1u);

    tv.vault.DestroyEntity(e);
    TEST_CHECK_EQUAL(aliveCount, 0u);
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

static Entity g_lastCreateEntity = ECS::InvalidEntity;
static float g_lastCreateX = 0.0f;

static void OnPositionCreate(Entity entity, Position& pos, Vault&) {
    g_lastCreateEntity = entity;
    g_lastCreateX = pos.x;
}

static Entity g_lastDestroyEntity = ECS::InvalidEntity;
static uint32 g_destroyCallCount = 0;

static void OnPositionDestroy(Entity entity, Position&, Vault&) {
    g_lastDestroyEntity = entity;
    ++g_destroyCallCount;
}

TEST(Vault, OnCreateCallback) {
    TestVault tv;
    tv.vault.RegisterOnCreate<Position, &OnPositionCreate>();

    g_lastCreateEntity = ECS::InvalidEntity;
    g_lastCreateX = 0.0f;

    Entity e = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Position>(e, 42.0f, 0.0f);

    TEST_CHECK_EQUAL(g_lastCreateEntity, e);
    TEST_CHECK_EQUAL(g_lastCreateX, 42.0f);
}

TEST(Vault, OnDestroyCallbackOnRemove) {
    TestVault tv;
    tv.vault.RegisterOnDestroy<Position, &OnPositionDestroy>();

    g_lastDestroyEntity = ECS::InvalidEntity;
    g_destroyCallCount = 0;

    Entity e = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Position>(e, 0.0f, 0.0f);
    tv.vault.RemoveAndSwapComponent<Position>(e);

    TEST_CHECK_EQUAL(g_lastDestroyEntity, e);
    TEST_CHECK_EQUAL(g_destroyCallCount, 1u);
}

TEST(Vault, OnDestroyCallbackOnEntityDestroy) {
    TestVault tv;
    tv.vault.RegisterOnDestroy<Position, &OnPositionDestroy>();

    g_lastDestroyEntity = ECS::InvalidEntity;
    g_destroyCallCount = 0;

    Entity e = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Position>(e, 0.0f, 0.0f);
    tv.vault.DestroyEntity(e);

    TEST_CHECK_EQUAL(g_lastDestroyEntity, e);
    TEST_CHECK_EQUAL(g_destroyCallCount, 1u);
}

// ---------------------------------------------------------------------------
// DestroyEntitiesWithComponent
// ---------------------------------------------------------------------------

TEST(Vault, DestroyEntitiesWithComponent) {
    TestVault tv;

    Entity e1 = tv.vault.CreateEntity();
    Entity e2 = tv.vault.CreateEntity();
    Entity e3 = tv.vault.CreateEntity();

    tv.vault.EmplaceComponent<Position>(e1, 0.0f, 0.0f);
    tv.vault.EmplaceComponent<Position>(e2, 0.0f, 0.0f);
    tv.vault.EmplaceComponent<Velocity>(e2, 0.0f, 0.0f);
    // e3 has no Position

    tv.vault.DestroyEntitiesWithComponent<Position>();

    TEST_CHECK_FALSE(tv.vault.IsEntityValid(e1));
    TEST_CHECK_FALSE(tv.vault.IsEntityValid(e2));
    TEST_CHECK(tv.vault.IsEntityValid(e3));

    // e2's Velocity should also be gone since the entity was destroyed
    auto velocities = tv.vault.GetComponents<Velocity>();
    TEST_CHECK_EQUAL(velocities.GetSize(), 0u);
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

TEST(Vault, ResetVault) {
    TestVault tv;

    Entity e1 = tv.vault.CreateEntity();
    Entity e2 = tv.vault.CreateEntity();
    tv.vault.EmplaceComponent<Position>(e1, 1.0f, 2.0f);
    tv.vault.EmplaceComponent<Position>(e2, 3.0f, 4.0f);

    tv.vault.Reset();

    TEST_CHECK_FALSE(tv.vault.IsEntityValid(e1));
    TEST_CHECK_FALSE(tv.vault.IsEntityValid(e2));

    auto components = tv.vault.GetComponents<Position>();
    TEST_CHECK_EQUAL(components.GetSize(), 0u);

    // Should be able to create entities again after reset
    Entity e3 = tv.vault.CreateEntity();
    TEST_CHECK(tv.vault.IsEntityValid(e3));
    TEST_CHECK_EQUAL(e3, ECS::FirstEntity);
}
