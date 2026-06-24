#pragma once

#include "ECS/Vault.hpp"
#include "Math/Mat2.hpp"
#include "Math/Quat.hpp"
#include "Math/Vec2.hpp"
#include "Math/Vec3.hpp"
#include "Math/Vec4.hpp"

namespace ImGui {

    using LVec2 = Vec2;
    using LVec3 = Vec3;
    using LVec4 = Vec4;
    using LMat2 = Mat2;
    using LMat3 = Mat3;
    using LMat4 = Mat4;
    using LQuat = Quat;

    extern ENGINE_API void Vec2(const char* label, const LVec2& v);

	extern ENGINE_API void Vec3(const char* label, const LVec3& v);

	extern ENGINE_API void Vec4(const char* label, const LVec4& v);

	extern ENGINE_API void Quat(const char* label, const LQuat& q, bool asEulerAngles = true);

	extern ENGINE_API bool InputVec2(const char* label, LVec2& v);

	extern ENGINE_API bool InputVec3(const char* label, LVec3& v);

	extern ENGINE_API bool InputVec4(const char* label, LVec4& v);

	extern ENGINE_API bool InputQuat(const char* label, LQuat& q, bool asEulerAngles = true);

	extern ENGINE_API bool IsInsidePreviousItem(const LVec2& position);

	extern ENGINE_API bool IsInsideWindow(const LVec2& position);

    extern ENGINE_API bool IsItemDisabled();

    extern ENGINE_API bool MainMenuBarItem(const char* menuName, const char* name, const char* shortcut, bool* isSelected);
}