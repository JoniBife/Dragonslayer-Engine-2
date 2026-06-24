#include "Runtime/Systems/LogSystem.hpp"

#include "Core/Log.hpp"

void LogSystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }
    Log::PrintLogsCurrentFrame();
    Log::ClearLogsCurrentFrame();
}

void LogSystem::End(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }
    // We may exist the engine before all logs are output, thus we print any remaining ones here
    Log::PrintLogsCurrentFrame();
    Log::ClearLogsCurrentFrame();
}
