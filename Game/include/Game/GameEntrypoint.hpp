#pragma once

void RegisterSystems(class Engine& engine);

#if WITH_EDITOR
void SetImGuiContext(Engine& engine);
#endif