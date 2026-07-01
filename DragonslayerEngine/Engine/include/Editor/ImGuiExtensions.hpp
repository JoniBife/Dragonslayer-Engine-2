#pragma once

#include "ECS/Vault.hpp"
#include "Math/Vec2.hpp"

namespace ImGui {

    ENGINE_API void Vec2(const char* label, const Vec2& v);

	ENGINE_API void Vec3(const char* label, const Vec3& v);

	ENGINE_API void Vec4(const char* label, const Vec4& v);

	ENGINE_API void Quat(const char* label, const Quat& q, bool asEulerAngles = true);

	ENGINE_API bool InputVec2(const char* label, struct Vec2& v);

	ENGINE_API bool InputVec3(const char* label, struct Vec3& v);

	ENGINE_API bool InputVec4(const char* label, struct Vec4& v);

	ENGINE_API bool InputQuat(const char* label, struct Quat& q, bool asEulerAngles = true);

	ENGINE_API bool IsInsidePreviousItem(const struct Vec2& position);

	ENGINE_API bool IsInsideWindow(const struct Vec2& position);

    ENGINE_API bool IsItemDisabled();

    ENGINE_API bool MainMenuBarItem(const char* menuName, const char* name, const char* shortcut, bool* isSelected);
}
