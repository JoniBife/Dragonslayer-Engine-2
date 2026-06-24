#pragma once

#include "Platform.hpp"

#if HOT_RELOAD_ON
    #if DS_PLATFORM_WINDOWS
        #if ENGINE_EXPORTS
            #define ENGINE_API __declspec(dllexport)
        #else
            #define ENGINE_API __declspec(dllimport)
        #endif
    #elif DS_PLATFORM_LINUX
        #if ENGINE_EXPORTS
            #define ENGINE_API __attribute__ ((visibility ("default")))
            #if DS_COMPILER_GCC
                // Prevents an issue with GCC attribute parsing (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=69585)
                // Taken directly from Jolt
                // TODO Reconsider if this is really necessary since its fixed in recent versions of GCC
                #define ENGINE_API_GCC_BUG_WORKAROUND [[gnu::visibility("default")]]
            #endif
        #endif
    #else
        #define ENGINE_API
    #endif
#else
    #define ENGINE_API
#endif
