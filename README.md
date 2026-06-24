# Dragonslayer Engine 2

A C++ game engine, currently in development. The spiritual successor to [Dragonslayer Engine](https://github.com/JoniBife/DragonslayerEngine). The previous iteration was mostly focused on building a renderer with lots of features, not really meant to build games.
This one aims to be a fully fledged game engine, with a focus on performance and ease of use. Here are some of its current features:

- Multi-threaded by default
- Custom Hierarchical Memory Manager with lots of custom allocators (mainly Linear, Freelist and Stack)
- Hot-reloading
- An ECS
- A GPU driven renderer
- PhysX integration
- A simple editor
- Minimal STD use (only features that require compiler support)

It can be currently built on Windows (MSVC) and Linux (both GCC and Clang)

## Building the Engine

1. Clone the repository and **its submodules**
2. (On Linux) Enable either X11 or Wayland support in CMake options
3. Generate the project using CMake
4. Copy the [assets](DragonslayerEngine/Engine/assets) folder to the build directory (Termporary measure until I figure out the ideal way to handle assets)
5. Build the project

## CMake Options

`DS_ENABLE_HOT_RELOADING` - Enables hot reloading

`DS_WITH_EDITOR` - Enables the editor

`DS_WITH_X11` - (Linux only) Enables X11 support

`DS_WITH_WAYLAND` - (Linux only) Enables wayland support

(There are currently a lot more options, I will add them here at some point)

## Launch arguments

`VK` - Changes the renderer's graphics API to Vulkan (default is DirectX 12) 

## License

This project is licensed under the MIT License