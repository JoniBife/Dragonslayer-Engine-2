cmake_minimum_required(VERSION 3.31)

project(Tests)

set(FRAMEWORK_FILES
    "include/Framework/TestFramework.hpp"

    "src/Framework/TestFramework.cpp"
    "src/Main.cpp"
)

set(TEST_FILES
    "src/Tests/Misc/TestInlineString.cpp"
    "src/Tests/Containers/TestInlineArray.cpp"
    "src/Tests/Containers/TestArray.cpp"
    "src/Tests/Containers/TestHashMap.cpp"
    "src/Tests/Allocators/TestLinearAllocator.cpp"
    "src/Tests/Allocators/TestFreeListAllocator.cpp"
    "src/Tests/Allocators/TestInlineLinearAllocator.cpp"
    "src/Tests/Math/TestVec2.cpp"
    "src/Tests/Math/TestVec3.cpp"
    "src/Tests/Math/TestVec4.cpp"
    "src/Tests/Math/TestMat2.cpp"
    "src/Tests/Math/TestMat3.cpp"
    "src/Tests/Math/TestMat4.cpp"
    "src/Tests/Math/TestQuat.cpp"
    "src/Tests/ECS/TestVault.cpp"
)

add_executable(Tests
    ${FRAMEWORK_FILES}
    ${TEST_FILES}
)

target_include_directories(Tests PRIVATE "include")

target_link_libraries(Tests PRIVATE Engine)
