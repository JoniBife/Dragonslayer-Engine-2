cmake_minimum_required(VERSION 3.31)

project(Game)

set(GAME_FILES
    "include/Game/GameExports.hpp"
    "include/Game/GameEntrypoint.hpp"
    "include/Game/StaticGameSettings.hpp"
    "include/Game/Components/GameState.hpp"
    "include/Game/Components/Player.hpp"
    "include/Game/Components/PlayerCamera.hpp"
    "include/Game/Components/WeaponEditor.hpp"
    "include/Game/Components/WeaponEditorCamera.hpp"
    "include/Game/Components/FollowEntity.hpp"
    "include/Game/Components/Projectile.hpp"
    "include/Game/Components/WeaponBlueprint.hpp"
    "include/Game/Components/WeaponRuntime.hpp"
    "include/Game/Components/Enemies/BigMouth.hpp"
    "include/Game/Components/Enemies/Enemy.hpp"
    "include/Game/Components/Enemies/Hover.hpp"
    "include/Game/Components/Modules/AirModule.hpp"
    "include/Game/Components/Modules/ChainModule.hpp"
    "include/Game/Components/Modules/PistonModule.hpp"
    "include/Game/Components/Modules/RotatorModule.hpp"
    "include/Game/Components/Modules/ShootModule.hpp"
    "include/Game/Components/Modules/ShapeModule.hpp"
    "include/Game/Components/Handles/SwordHandle.hpp"
    "include/Game/Components/Handles/ShootV1Handle.hpp"
    "include/Game/Components/Handles/ShootV2Handle.hpp"
    "include/Game/Components/Handles/ShootV3Handle.hpp"
    "include/Game/Components/Handles/ShootV4Handle.hpp"
    "include/Game/Systems/GameStateSystem.hpp"
    "include/Game/Systems/PlayerCameraSystem.hpp"
    "include/Game/Systems/PlayerSystem.hpp"
    "include/Game/Systems/MapSystem.hpp"
    "include/Game/Systems/WeaponEditorSystem.hpp"
    "include/Game/Systems/WeaponEditorCameraSystem.hpp"
    "include/Game/Systems/WeaponMotionSystem.hpp"
    "include/Game/Systems/EnemyAISystem.hpp"

    "src/GameEntrypoint.cpp"
    "src/Systems/GameStateSystem.cpp"
    "src/Systems/PlayerCameraSystem.cpp"
    "src/Systems/PlayerSystem.cpp"
    "src/Systems/MapSystem.cpp"
    "src/Systems/WeaponEditorSystem.cpp"
    "src/Systems/WeaponEditorCameraSystem.cpp"
    "src/Systems/WeaponMotionSystem.cpp"
    "src/Systems/EnemyAISystem.cpp"
)

if (DS_ENABLE_HOT_RELOADING)
    add_library(Game SHARED ${GAME_FILES})
    target_include_directories(Game PUBLIC "include")
    target_compile_definitions(Game PRIVATE GAME_EXPORTS)
    movePDB(Game)
else ()
    add_library(Game STATIC ${GAME_FILES})
    target_include_directories(Game PUBLIC "include")
endif ()

target_link_libraries(Game PUBLIC Engine)