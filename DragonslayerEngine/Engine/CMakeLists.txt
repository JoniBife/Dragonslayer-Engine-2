cmake_minimum_required(VERSION 3.31)

project(Engine)

# TODO Missing cmake things
# - Remove ImGui in non-editor builds!
# - Proper explicit compiler options
# - Disabling any editor-only option from release buildings!

# Window system
option(DS_USE_WAYLAND "Enabled Wayland support for Dragonslayer Engine" OFF)
option(DS_USE_X11 "Enabled X11 support for Dragonslayer Engine" OFF)

# ----------------------------------------
# Compiler Setup
# ----------------------------------------

# Compile definitions
set(COMPILE_DEFINITIONS
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    WIN32_LEAN_AND_MEAN # Exclude a lot of the default included winapi to speed up builds
    NOMINMAX # Prevent windows headers from defining their own min max to avoid conflicts
    # _CRT_SECURE_NO_WARNINGS
)

# Compile options
# if(NOT IS_ARM64)
#     set(SIMD -mssse3)
# endif()
# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     set(COMPILE_OPTIONS ${SIMD} -Wextra)
# elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#     set(COMPILE_OPTIONS ${SIMD} -Wextra)
# elseif(MSVC)
#     set(COMPILE_OPTIONS
#             # /W4
#             /WX
#             /wd4324 # padding was added at the end of a structure because you specified an alignment specifier
#     )
# else()
#     message(WARNING "Unknown compiler!")
# endif()

# ----------------------------------------
# Imgui Setup as static lib
# ----------------------------------------

add_library (Imgui STATIC
        "libs/imgui/imgui.cpp"
        "libs/imgui/imgui_draw.cpp"
        "libs/imgui/imgui_tables.cpp"
        "libs/imgui/imgui_widgets.cpp"
        "libs/imgui/imgui_demo.cpp"
)
target_compile_definitions(Imgui PRIVATE ${COMPILE_DEFINITIONS})
target_include_directories (Imgui PUBLIC "libs/imgui" "libs/imgui/backends")
target_link_libraries (Imgui glfw)

# ----------------------------------------
# Imguizmo Setup as static lib
# ----------------------------------------

add_library (Imguizmo STATIC
        "libs/imguizmo/src/ImGuizmo.cpp"
)
target_compile_definitions(Imgui PRIVATE ${COMPILE_DEFINITIONS})
target_include_directories (Imguizmo PUBLIC "libs/imguizmo/src")
target_link_libraries (Imguizmo Imgui)

# ----------------------------------------
# Detex Setup as static lib
# ----------------------------------------

add_subdirectory ("libs/detex")

# ----------------------------------------
# GLFW Setup as static lib (the default)
# ----------------------------------------
if(DS_USE_WAYLAND)
    set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
elseif(DS_USE_X11)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
else()
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory ("libs/glfw")

# ----------------------------------------
# NRI Setup as shared lib
# ----------------------------------------

# Build / Setup
set(NRI_STATIC_LIBRARY OFF CACHE BOOL "Build static library" FORCE)
set(NRI_SHADERS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/_Shaders" CACHE STRING "Shader output path override" FORCE)
set(NRI_NVAPI_CUSTOM_PATH "" CACHE STRING "Path to a custom NVAPI library directory" FORCE)
set(NRI_STREAMER_THREAD_SAFE ON CACHE BOOL "'NRIStreamer' thread safety (OFF is faster)" FORCE)

# Debug & Profiling options
set(NRI_ENABLE_NVTX_SUPPORT ON CACHE BOOL "Annotations for NVIDIA Nsight Systems" FORCE)
set(NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS ON CACHE BOOL "Enable debug names, host and device annotations" FORCE)
set(NRI_ENABLE_VALIDATION_SUPPORT ON CACHE BOOL "Enable Validation backend (otherwise 'enableNRIValidation' is ignored)" FORCE)

# Backend support
set(NRI_ENABLE_NONE_SUPPORT ON CACHE BOOL "Enable NONE backend" FORCE)
set(NRI_ENABLE_VK_SUPPORT ON CACHE BOOL "Enable Vulkan backend" FORCE)
set(NRI_ENABLE_D3D11_SUPPORT OFF CACHE BOOL "Enable D3D11 backend" FORCE)
set(NRI_ENABLE_D3D12_SUPPORT ON CACHE BOOL "Enable D3D12 backend" FORCE)

# Agility SDK
set(NRI_ENABLE_AGILITY_SDK_SUPPORT ON CACHE BOOL "Enable Agility SDK support to unlock access to latest D3D12 features" FORCE)
set(NRI_AGILITY_SDK_DIR "AgilitySDK" CACHE STRING "Directory where Agility SDK will be copied relative to the directory with binaries" FORCE)
set(NRI_AGILITY_SDK_VERSION_MAJOR "619" CACHE STRING "Agility SDK major version" FORCE) # 717+ defines D3D12_ERROR_INVALID_REDIST
set(NRI_AGILITY_SDK_VERSION_MINOR "1" CACHE STRING "Agility SDK minor version" FORCE)   # 1-preview

# Imgui
set(NRI_ENABLE_IMGUI_EXTENSION ON CACHE BOOL "Enable 'NRIImgui' extension" FORCE)

# Linux Windowing Systems
if(DS_USE_WAYLAND)
    set(NRI_ENABLE_WAYLAND_SUPPORT ON CACHE BOOL "Enable Wayland support" FORCE)
    set(NRI_ENABLE_XLIB_SUPPORT OFF CACHE BOOL "Enable X11 support" FORCE)
elseif(DS_USE_X11)
    set(NRI_ENABLE_XLIB_SUPPORT ON CACHE BOOL "Enable X11 support" FORCE)
    set(NRI_ENABLE_WAYLAND_SUPPORT OFF CACHE BOOL "Enable Wayland support" FORCE)
else()
    set(NRI_ENABLE_XLIB_SUPPORT OFF CACHE BOOL "Enable X11 support" FORCE)
    set(NRI_ENABLE_WAYLAND_SUPPORT OFF CACHE BOOL "Enable Wayland support" FORCE)
endif()

# AI Upscaling SDKs
set(NRI_ENABLE_NGX_SDK OFF CACHE BOOL "Enable NVIDIA NGX (DLSS) SDK" FORCE)
set(NRI_ENABLE_FFX_SDK OFF CACHE BOOL "Enable AMD FidelityFX SDK" FORCE)
set(NRI_ENABLE_XESS_SDK OFF CACHE BOOL "Enable INTEL XeSS SDK" FORCE)

# Vendor / Platform Integrations
set(NRI_ENABLE_NIS_SDK OFF CACHE BOOL "Enable NVIDIA Image Sharpening SDK" FORCE)
set(NRI_ENABLE_AMDAGS ON CACHE BOOL "Enable AMD AGS library for D3D" FORCE)
set(NRI_ENABLE_NVAPI ON CACHE BOOL "Enable NVAPI library for D3D" FORCE)

add_subdirectory("libs/NRI")

# ----------------------------------------
# PhysX Setup as shared lib
# ----------------------------------------

# PhysX build flavor (matches the PhysX docs: debug / checked / profile / release).
# Default checked: optimized + asserts on, recommended for development.
set(PHYSX_BUILD_TYPE "checked" CACHE STRING "PhysX build type")
set_property(CACHE PHYSX_BUILD_TYPE PROPERTY STRINGS debug checked profile release)

if(DS_WINDOWS)
    set(PHYSX_PLATFORM_DIR "win")
    set(PHYSX_STATIC_PREFIX "")
    set(PHYSX_STATIC_SUFFIX ".lib")
elseif(DS_LINUX)
    if(DS_CLANG)
        set(PHYSX_PLATFORM_DIR "linux-clang")
    elseif(DS_GCC)
        set(PHYSX_PLATFORM_DIR "linux-gcc")
    endif()
    set(PHYSX_STATIC_PREFIX "lib")
    set(PHYSX_STATIC_SUFFIX ".a")
endif()

set(PHYSX_LIB_DIR "${PROJECT_SOURCE_DIR}/libs/physx/binDynamic/${PHYSX_BUILD_TYPE}/${PHYSX_PLATFORM_DIR}")

# PhysX static libs are linked PRIVATE to Runtime so they don't propagate to Game.dll.
# This is critical for hot-reload: if Game.dll gets its own copy of PhysX extensions,
# then PhysX constraint shaders and vtables point into Game.dll's code segment and
# dangle after FreeLibrary, crashing on the next simulate() call.
set(PhysXStaticLibs
    "${PHYSX_LIB_DIR}/${PHYSX_STATIC_PREFIX}PhysXCharacterKinematic_static_64${PHYSX_STATIC_SUFFIX}"
    "${PHYSX_LIB_DIR}/${PHYSX_STATIC_PREFIX}PhysXExtensions_static_64${PHYSX_STATIC_SUFFIX}"
    "${PHYSX_LIB_DIR}/${PHYSX_STATIC_PREFIX}PhysXPvdSDK_static_64${PHYSX_STATIC_SUFFIX}"
    # "${PHYSX_LIB_DIR}/${PHYSX_STATIC_PREFIX}PhysXVehicle2_static_64${PHYSX_STATIC_SUFFIX}"
)

set(PhysXSharedLibs)
set(PhysXLibs "PhysX_64" "PhysXCommon_64" "PhysXCooking_64" "PhysXFoundation_64")

foreach(LIB_NAME ${PhysXLibs})
    # Create an imported target for each
    add_library(${LIB_NAME} SHARED IMPORTED)

    if(DS_WINDOWS)
        set_target_properties(${LIB_NAME} PROPERTIES
            IMPORTED_IMPLIB "${PHYSX_LIB_DIR}/${LIB_NAME}.lib"
            IMPORTED_LOCATION "${PHYSX_LIB_DIR}/${LIB_NAME}.dll"
            INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/libs/physx/include"
        )
    else()
        # IMPORTED_NO_SONAME so the linker records the bare filename and the
        # .so copied next to the binary resolves at runtime.
        set_target_properties(${LIB_NAME} PROPERTIES
            IMPORTED_LOCATION "${PHYSX_LIB_DIR}/lib${LIB_NAME}.so"
            IMPORTED_NO_SONAME FALSE
            INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/libs/physx/include"
        )
    endif()

    list(APPEND PhysXSharedLibs ${LIB_NAME})
endforeach()

# ----------------------------------------
# ShaderMake Setup
# ----------------------------------------

add_subdirectory("libs/ShaderMake")

# Shaders

set(SHADERS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/Renderer/Shaders")
set(SHADERS_OUTPUT_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shaders")

file(GLOB_RECURSE SHADERS "${SHADERS_PATH}/*.hlsl" "${SHADERS_PATH}/*.hlsli")
set_source_files_properties(${SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")

get_target_property(NRI_SOURCE_DIR NRI SOURCE_DIR)

set(SHADERMAKE_GENERAL_ARGS
    --project "DragonslayerEngine"
    --shaderModel 6_6
    --compactProgress
    --binary
    --flatten
    --stripReflection
    --WX
    --sRegShift 0 --bRegShift 32 --uRegShift 64 --tRegShift 128 # TODO: must be in sync with "VK_BINDING_OFFSETS"
    --sourceDir "${SHADERS_PATH}"
    --ignoreConfigDir
    -c "${SHADERS_PATH}/Shaders.cfg"
    -o "${SHADERS_OUTPUT_PATH}"
    -I "${SHADERS_PATH}"
    -I "${NRI_SOURCE_DIR}/Include"
)

set(SHADERMAKE_COMMANDS "")

if(NRI_ENABLE_D3D11_SUPPORT)
    set(SHADERMAKE_COMMANDS ${SHADERMAKE_COMMANDS} COMMAND ${SHADERMAKE_PATH} -p DXBC --compiler "${SHADERMAKE_FXC_PATH}" ${SHADERMAKE_GENERAL_ARGS})
endif()

if(NRI_ENABLE_D3D12_SUPPORT)
    set(SHADERMAKE_COMMANDS ${SHADERMAKE_COMMANDS} COMMAND ${SHADERMAKE_PATH} -p DXIL --compiler "${SHADERMAKE_DXC_PATH}" ${SHADERMAKE_GENERAL_ARGS})
endif()

if(NRI_ENABLE_VK_SUPPORT)
    set(SHADERMAKE_COMMANDS ${SHADERMAKE_COMMANDS} COMMAND ${SHADERMAKE_PATH} -p SPIRV --compiler "${SHADERMAKE_DXC_VK_PATH}" ${SHADERMAKE_GENERAL_ARGS})
endif()

add_custom_target(Shaders ALL
    ${SHADERMAKE_COMMANDS}
    DEPENDS ShaderMake
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    SOURCES ${SHADERS}
    VERBATIM
)

# TODO Remove this hack!
# Check if the compiler is Clang
# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     target_compile_options(NRI PRIVATE -Wno-error)
#     target_compile_options(NRI_Shared PRIVATE -Wno-error)
#     target_compile_options(NRI_Validation PRIVATE -Wno-error)
#     # target_compile_options(NRI_Shaders PRIVATE -Wno-error)
#     # target_compile_options(NRI_D3D11 PRIVATE -Wno-error)
#     # target_compile_options(NRI_D3D12 PRIVATE -Wno-error)
#     target_compile_options(NRI_VK PRIVATE -Wno-error)
#     target_compile_options(NRI_NONE PRIVATE -Wno-error)
# endif()

# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     foreach(tgt IN ITEMS NRI NRI_Shared NRI_Validation NRI_VK NRI_NONE)
#         if(TARGET ${tgt})
#             get_target_property(flags ${tgt} COMPILE_OPTIONS)
#             if(flags)
#                 list(REMOVE_ITEM flags -Werror)
#                 set_target_properties(${tgt} PROPERTIES COMPILE_OPTIONS "${flags}")
#             endif()
#             # Add back specific suppressions so Clang doesn’t choke
#             target_compile_options(${tgt} PRIVATE
#                     -Wno-return-type-c-linkage
#                     -Wno-null-pointer-subtraction
#             )
#         endif()
#     endforeach()
# endif()

# ----------------------------------------
# SETUP RUNTIME
# ----------------------------------------

option(DS_WITH_EDITOR "Compile Dragonslayer Engine with editor" ON)

set(CORE_FILES
    "include/Core/Platform.hpp"
    "include/Core/Assert.hpp"
    "include/Core/Macros.hpp"
    "include/Core/Export.hpp"
    "include/Core/Memory.hpp"
    "include/Core/Lock.hpp"
    "include/Core/Thread.hpp"
    "include/Core/ThreadContext.hpp"
    "include/Core/Allocators/ParentAllocator.hpp"
    "include/Core/Allocators/LinearAllocator.hpp"
    "include/Core/Allocators/InlineLinearAllocator.hpp"
    "include/Core/Allocators/StackAllocator.hpp"
    "include/Core/Allocators/FreeListAllocator.hpp"
    "include/Core/Containers/ContainerView.hpp"
    "include/Core/Containers/Array.hpp"
    "include/Core/Containers/ArrayHelper.hpp"
    "include/Core/Containers/HashMap.hpp"
    "include/Core/Containers/HashMapHelper.hpp"
    "include/Core/IO/File.hpp"
    "include/Core/IO/BufferedFile.hpp"
    "include/Core/NotCopyable.hpp"
    "include/Core/Types.hpp"
    "include/Core/TypeInfo.hpp"
    "include/Core/String.hpp"
    "include/Core/Hash.hpp"
    "include/Core/Time.hpp"
    "include/Core/Window.hpp"
    "include/Core/Log.hpp"
    "include/Core/EngineGlobals.hpp"
    "include/Core/EngineMinimal.hpp"

    "src/Core/Allocators/LinearAllocator.cpp"
    "src/Core/Allocators/StackAllocator.cpp"
    "src/Core/Allocators/FreeListAllocator.cpp"
    "src/Core/EngineGlobals.cpp"
    "src/Core/Window.cpp"
    "src/Core/Log.cpp"
)

set(ECS_FILES
    "include/ECS/Entity.hpp"
    "include/ECS/EntityName.hpp"
    "include/ECS/HierarchyDirectory.hpp"
    "include/ECS/Component.hpp"
    "include/ECS/System.hpp"
    "include/ECS/Vault.hpp"

    "src/ECS/Vault.cpp"
)

set(MATH_FILES
    "include/Math/Mat2.hpp"
    "include/Math/Mat3.hpp"
    "include/Math/Mat4.hpp"
    "include/Math/MathAux.hpp"
    "include/Math/Quat.hpp"
    "include/Math/Vec2.hpp"
    "include/Math/Vec3.hpp"
    "include/Math/Vec4.hpp"

    "src/Math/Mat2.cpp"
    "src/Math/Mat3.cpp"
    "src/Math/Mat4.cpp"
    "src/Math/MathAux.cpp"
    "src/Math/Quat.cpp"
    "src/Math/Vec2.cpp"
    "src/Math/Vec3.cpp"
    "src/Math/Vec4.cpp"
)

set(RENDERER_FILES
    "include/Renderer/RenderingSystem.hpp"
    "include/Renderer/RendererCore.hpp"
    "include/Renderer/Shader.hpp"
    "include/Renderer/Texture.hpp"
    "include/Renderer/Camera.hpp"
    "include/Renderer/BasicPrimitives.hpp"
    "include/Renderer/NRIWindowCreation.hpp"
    "include/Renderer/Components/PrimitiveRenderer.hpp"
    "include/Renderer/FrameGraph/Handles.hpp"
    "include/Renderer/FrameGraph/FrameGraph.hpp"

    "src/Renderer/RenderingSystem.cpp"
    "src/Renderer/Camera.cpp"
    "src/Renderer/FrameGraph/FrameGraph.cpp"
    "src/Renderer/NRIWindowCreation.cpp"
)

set(PHYSICS_FILES
    "include/Physics/PhysicsCommon.hpp"
    "include/Physics/Systems/PhysicsSystem.hpp"
    "include/Physics/Components/StaticRigidBody.hpp"
    "include/Physics/Components/DynamicRigidBody.hpp"
    "include/Physics/Components/CharacterController.hpp"
    "include/Physics/Components/CollisionEvent.hpp"
    "include/Physics/Components/TriggerEvent.hpp"
    "include/Physics/Components/Joints/BallSocketJoint.hpp"
    "include/Physics/Components/Joints/DistanceJoint.hpp"
    "include/Physics/Components/Joints/FixedJoint.hpp"
    "include/Physics/Components/Joints/GearJoint.hpp"
    "include/Physics/Components/Joints/HingeJoint.hpp"
    "include/Physics/Components/Joints/RackAndPinionJoint.hpp"
    "include/Physics/Components/Joints/SixDOFJoint.hpp"
    "include/Physics/Components/Joints/SliderJoint.hpp"

    "src/Physics/DynamicRigidBody.cpp"
    "src/Physics/Joints.cpp"
    "src/Physics/PhysicsCommon.cpp"
    "src/Physics/Systems/PhysicsSystem.cpp"
)

set(RUNTIME_FILES
    "include/Runtime/Engine.hpp"
    "include/Runtime/Input.hpp"
    "include/Runtime/Components/Transform.hpp"
    "include/Runtime/Components/PhysicsNPC.hpp"
    "include/Runtime/Systems/TimeSystem.hpp"
    "include/Runtime/Systems/LogSystem.hpp"

    "src/Runtime/Engine.cpp"
    "src/Runtime/Input.cpp"
    "src/Runtime/Systems/TimeSystem.cpp"
    "src/Runtime/Systems/LogSystem.cpp"
)

set(ENGINE_FILES
    ${CORE_FILES}
    ${ECS_FILES}
    ${MATH_FILES}
    ${RENDERER_FILES}
    ${PHYSICS_FILES}
    ${RUNTIME_FILES}
)

if(DS_WITH_EDITOR)

    set(EDITOR_FILES
        "include/Editor/ComponentMetadata.hpp"
        "include/Editor/EditorGlobals.hpp"
        "include/Editor/EditorCamera.hpp"
        "include/Editor/ImGuiExtensions.hpp"
        "include/Editor/ImGuiSetup.hpp"
        "include/Editor/ImGuiState.hpp"
        "include/Editor/TimeCapturesManager.hpp"
        "include/Editor/TimeScope.hpp"
        "include/Editor/Systems/EntityHierarchySystem.hpp"
        "include/Editor/Systems/ECSDebugSystem.hpp"
        "include/Editor/Systems/EditorCameraSystem.hpp"
        "include/Editor/Systems/EditorStateSystem.hpp"
        "include/Editor/Systems/MemoryUsageSystem.hpp"
        "include/Editor/Systems/OutputLogSystem.hpp"
        "include/Editor/Systems/TransformGizmosSystem.hpp"
        "include/Editor/Systems/TimeCapturesSystem.hpp"
        "include/Editor/Systems/PhysicsDebugSystem.hpp"
        "include/Editor/Components/HierarchySelection.hpp"

        "src/Editor/ComponentMetadata.cpp"
        "src/Editor/EditorGlobals.cpp"
        "src/Editor/EditorCamera.cpp"
        "src/Editor/ImGuiExtensions.cpp"
        "src/Editor/ImGuiState.cpp"
        "src/Editor/TimeCapturesManager.cpp"
        "src/Editor/Systems/EntityHierarchySystem.cpp"
        "src/Editor/Systems/ECSDebugSystem.cpp"
        "src/Editor/Systems/EditorCameraSystem.cpp"
        "src/Editor/Systems/EditorStateSystem.cpp"
        "src/Editor/Systems/MemoryUsageSystem.cpp"
        "src/Editor/Systems/OutputLogSystem.cpp"
        "src/Editor/Systems/TransformGizmosSystem.cpp"
        "src/Editor/Systems/TimeCapturesSystem.cpp"
        "src/Editor/Systems/PhysicsDebugSystem.cpp"
    )

    list(APPEND ENGINE_FILES
        ${EDITOR_FILES}
    )

endif()

if (DS_ENABLE_HOT_RELOADING)

    add_library(Engine SHARED
        ${ENGINE_FILES}
    )

    target_compile_definitions(Engine PRIVATE ENGINE_EXPORTS)
    target_compile_definitions(Engine PUBLIC HOT_RELOAD_ON=1)

else ()

    add_library(Engine STATIC
        ${ENGINE_FILES}
    )

    target_compile_definitions(Engine PUBLIC HOT_RELOAD_ON=0)

endif ()

if(DS_WITH_EDITOR)
    target_compile_definitions(Engine PUBLIC WITH_EDITOR=1)
else()
    target_compile_definitions(Engine PUBLIC WITH_EDITOR=0)
endif()

target_include_directories(Engine PUBLIC
    "include"
    ${PHYSX_INCLUDE}
)

target_compile_options(Engine PRIVATE ${COMPILE_OPTIONS})

# Public to make sure the game inherits them too
target_compile_definitions(Engine PUBLIC ${COMPILE_DEFINITIONS})

target_link_libraries (Engine
    PUBLIC
        glfw
        NRI
        ${PhysXSharedLibs}
        Detex
        Imgui
        Imguizmo
        ShaderMakeBlob
    PRIVATE
        ${PhysXStaticLibs}
)

add_dependencies(Engine Shaders)

# Deploy the PhysX runtime DLLs (and their PDBs) next to the executable.
# These are tracked OUTPUTs rather than a POST_BUILD of Engine: Engine is a
# STATIC lib when hot-reload is off and rarely relinks, so a POST_BUILD step
# would not re-run when BUILD_OUTPUT_DIR is wiped. Modelling each copy as an
# OUTPUT lets Ninja re-copy whenever the destination file is missing.
# Shipping the PDB makes crashes inside PhysX resolve to source instead of
# dropping into raw disassembly.
set(PhysXDeployedFiles "")

foreach(LIB_NAME ${PhysXLibs})
    set(_src_dll "$<TARGET_FILE:${LIB_NAME}>")
    set(_dst_dll "${BUILD_OUTPUT_DIR}/${LIB_NAME}.dll")
    add_custom_command(
        OUTPUT "${_dst_dll}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src_dll}" "${BUILD_OUTPUT_DIR}"
        DEPENDS "${_src_dll}"
        VERBATIM
    )
    list(APPEND PhysXDeployedFiles "${_dst_dll}")

    # PDB sits beside the imported DLL in PHYSX_LIB_DIR; copy only if present
    # (release flavors may not ship one).
    if(DS_WINDOWS AND EXISTS "${PHYSX_LIB_DIR}/${LIB_NAME}.pdb")
        set(_src_pdb "${PHYSX_LIB_DIR}/${LIB_NAME}.pdb")
        set(_dst_pdb "${BUILD_OUTPUT_DIR}/${LIB_NAME}.pdb")
        add_custom_command(
            OUTPUT "${_dst_pdb}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src_pdb}" "${BUILD_OUTPUT_DIR}"
            DEPENDS "${_src_pdb}"
            VERBATIM
        )
        list(APPEND PhysXDeployedFiles "${_dst_pdb}")
    endif()
endforeach()

# ALL ties it into the default build; Engine depends on it so the DLLs are in
# place by the time anything linking Engine is run.
add_custom_target(DeployPhysX ALL DEPENDS ${PhysXDeployedFiles})
add_dependencies(Engine DeployPhysX)

# AgilitySDK requires specific symbols to be exported from the executable, which are not needed elsewhere.
# Any other method doesn't work. "PUBLIC" to make settings visible in the parent project (EXE).
# if(MSVC)
#     include(CheckLinkerFlag)
#     check_linker_flag(CXX /NOEXP HAS_NOEXP)
#     set(LINK_OPTIONS /NOIMPLIB)
#     if(HAS_NOEXP)
#         set(LINK_OPTIONS ${LINK_OPTIONS} /NOEXP)
#     endif()
#     target_link_options(Runtime PUBLIC ${LINK_OPTIONS})
# endif()