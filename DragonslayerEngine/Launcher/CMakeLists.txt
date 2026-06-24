cmake_minimum_required (VERSION 3.31)

project(Launcher)

if (DS_ENABLE_HOT_RELOADING)

    add_executable(Launcher
            "MainHotReload.cpp"
    )
    target_link_libraries(Launcher PUBLIC Engine)
else ()

    add_executable(Launcher
            "Main.cpp"
    )
    target_link_libraries(Launcher PUBLIC Engine Game)

endif ()