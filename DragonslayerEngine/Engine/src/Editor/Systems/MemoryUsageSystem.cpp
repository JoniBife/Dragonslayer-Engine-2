
#include "Core/Platform.hpp"

#if DS_PLATFORM_WINDOWS
#include <windows.h>
#include <psapi.h>
#endif

#include "Editor/Systems/MemoryUsageSystem.hpp"
#include "Core/Allocators/StackAllocator.hpp"

#include <imgui.h>

#include "Core/EngineGlobals.hpp"
#include "Runtime/Input.hpp"


void MemoryUsageSystem::Update(ThreadContext& threadContext, Vault& vault)
{
    // TODO Accumulate temp memory from all threads!
    if (!threadContext.IsMainThread()) {
        return;
    }

    static bool showMemoryData = false;

    if (Input::IsKeyHeld(KeyCode::LControl) && Input::IsKeyPressed(KeyCode::Five)) {
        showMemoryData = !showMemoryData;
    }
    
    if (ImGui::MainMenuBarItem("Stats", "Memory Usage", "CTRL+5", &showMemoryData)) {

        ImGui::Begin("Memory Usage", &showMemoryData);

        static bool inMB = false;

        const size_t totalMemory = gGlobalAllocator->GetTotalMemory();
        const size_t usedMemory = gGlobalAllocator->GetUsedMemory();
        const size_t freeMemory = gGlobalAllocator->GetFreeMemory();

        StackAllocator& threadTempAllocator = GetThreadTempAllocator();

        const size_t totalTempMemory = threadTempAllocator.GetTotalMemory();
        const size_t usedTempMemory = threadTempAllocator.GetUsedMemory();
        const size_t freeTempMemory = threadTempAllocator.GetFreeMemory();

        const size_t dividend = inMB ? 1024 * 1024 : 1024;

        const char* format = inMB ? "Mb" : "Kb";

        ImGui::Text("Global Allocator Total: %d %s", totalMemory / dividend, format);
        ImGui::Text("Global Allocator Used: %d %s", usedMemory / dividend, format);
        ImGui::Text("Global Allocator Free: %d %s", freeMemory / dividend, format);

        ImGui::Text("Temp Allocator Total: %d %s", totalTempMemory / dividend, format);
        ImGui::Text("Temp Allocator Used: %d %s", usedTempMemory / dividend, format);
        ImGui::Text("Temp Allocator Free: %d %s", freeTempMemory / dividend, format);

#if DS_PLATFORM_WINDOWS
        PROCESS_MEMORY_COUNTERS memInfo;
        GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo));
        ImGui::Text("WorkingSetSize: %d %s", memInfo.WorkingSetSize / dividend, format);
        ImGui::Text("PagefileUsage: %d %s", memInfo.PagefileUsage / dividend, format);
        ImGui::Text("PeakWorkingSetSize: %d %s", memInfo.PeakWorkingSetSize / dividend, format);
        ImGui::Text("PeakPagefileUsage: %d %s", memInfo.PeakPagefileUsage / dividend, format);
#endif

        ImGui::Checkbox("In Megabytes", &inMB);

        ImGui::End();
    }
}
