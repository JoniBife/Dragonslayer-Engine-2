#include "Editor/ImGuiExtensions.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "Math/MathAux.hpp"

void ImGui::Vec2(const char *label, const LVec2 &v) {
    const float inputFieldsWidth = ImGui::GetContentRegionAvail().x * 0.25f;

    ImGui::Text(label);

    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    ImGui::Text("%f", v.x); ImGui::SameLine();

    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    ImGui::Text("%f", v.y); ImGui::SameLine();
}

void ImGui::Vec3(const char *label, const LVec3 &v) {
    const float inputFieldsWidth = ImGui::GetContentRegionAvail().x * 0.25f;

    ImGui::Text(label);

    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    ImGui::Text("%f", v.x); ImGui::SameLine();

    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    ImGui::Text("%f", v.y); ImGui::SameLine();

    ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    ImGui::Text("%f", v.z);
}

void ImGui::Vec4(const char *label, const LVec4 &v) {
    const float inputFieldsWidth = ImGui::GetContentRegionAvail().x * 0.25f;

    ImGui::Text(label);

    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    ImGui::Text("%f", v.x); ImGui::SameLine();

    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    ImGui::Text("%f", v.y); ImGui::SameLine();

    ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    ImGui::Text("%f", v.z);

    ImGui::TextColored(ImVec4(1, 1, 1, 1), "W"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    ImGui::Text("%f", v.w);
}

void ImGui::Quat(const char *label, const LQuat &q, bool asEulerAngles) {

    if (asEulerAngles) {
        LVec3 euler;
        q.ToEuler(euler.x, euler.y, euler.z);

        euler.x = RadiansToDegrees(euler.x);
        euler.y = RadiansToDegrees(euler.y);
        euler.z = RadiansToDegrees(euler.z);

        ImGui::Vec3(label, euler);
    } else {
        const float inputFieldsWidth = ImGui::GetContentRegionAvail().x * 0.25f;

        ImGui::Text(label);

        ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        ImGui::Text("%f", q.x); ImGui::SameLine();

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        ImGui::Text("%f", q.y); ImGui::SameLine();

        ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        ImGui::Text("%f", q.z);

        ImGui::TextColored(ImVec4(1, 1, 1, 1), "T"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        ImGui::Text("%f", q.t);
    }
}

bool ImGui::InputVec2(const char *label, LVec2 &v) {
    const float inputFieldsWidth = ImGui::GetContentRegionAvail().x * 0.25f;

    bool valueChanged = false;

    ImGui::Text(label);

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    valueChanged |= ImGui::DragFloat((NameString("##X") + label).CStr(), &(v.x) ,.025f); ImGui::SameLine();

    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    valueChanged |= ImGui::DragFloat((NameString("##Y") + label).CStr(), &(v.y), .025f); ImGui::SameLine();

    return valueChanged;
}

bool ImGui::InputVec3(const char *label, LVec3 &v) {
    const float inputFieldsWidth = ImGui::GetContentRegionAvail().x * 0.25f;

    bool valueChanged = false;

    ImGui::Text(label);

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    valueChanged |= ImGui::DragFloat((NameString("##X") + label).CStr(), &(v.x), .025f); ImGui::SameLine();

    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    valueChanged |= ImGui::DragFloat((NameString("##Y") + label).CStr(), &(v.y), .025f); ImGui::SameLine();

    ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    valueChanged |= ImGui::DragFloat((NameString("##Z") + label).CStr(), &(v.z), .025f);

    return valueChanged;
}

bool ImGui::InputVec4(const char *label, LVec4 &v) {
    const float inputFieldsWidth = ImGui::GetContentRegionAvail().x * 0.25f;

    bool valueChanged = false;

    ImGui::Text(label);

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    valueChanged |= ImGui::DragFloat((NameString("##X") + label).CStr(), &v.x, .025f); ImGui::SameLine();

    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    valueChanged |= ImGui::DragFloat((NameString("##Y") + label).CStr(), &v.y, .025f); ImGui::SameLine();

    ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    valueChanged |= ImGui::DragFloat((NameString("##Z") + label).CStr(), &v.z, .025f);

    ImGui::TextColored(ImVec4(1, 1, 1, 1), "W"); ImGui::SameLine();
    ImGui::SetNextItemWidth(inputFieldsWidth);
    valueChanged |= ImGui::DragFloat((NameString("##W") + label).CStr(), &v.w, .025f);

    return valueChanged;
}

bool ImGui::InputQuat(const char *label, LQuat &q, bool asEulerAngles) {

    if (asEulerAngles) {
        float roll, pitch, yaw;
        q.ToEuler(roll, pitch, yaw);

        roll = RadiansToDegrees(roll);
        pitch = RadiansToDegrees(pitch);
        yaw = RadiansToDegrees(yaw);

        const float inputFieldsWidth = ImGui::GetContentRegionAvail().x * 0.25f;

        bool valueChanged = false;

        ImGui::Text(label);

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        valueChanged |= ImGui::DragFloat((NameString("##X") + label).CStr(), &roll, .1f); ImGui::SameLine();

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        // Clamping for gimbal lock prevention
        valueChanged |= ImGui::DragFloat((NameString("##Y") + label).CStr(), &pitch, .1f, -89.9f, 89.9f); ImGui::SameLine();

        ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        valueChanged |= ImGui::DragFloat((NameString("##Z") + label).CStr(), &yaw, .1f);

        if (valueChanged) {

            roll = DegreesToRadians(roll);
            pitch = DegreesToRadians(pitch);
            yaw = DegreesToRadians(yaw);

            q = LQuat::FromEuler(roll, pitch, yaw);
        }

        return valueChanged;
    }
    else
    {
        const float inputFieldsWidth = ImGui::GetContentRegionAvail().x * 0.25f;

        bool valueChanged = false;

        ImGui::Text(label);

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        valueChanged |= ImGui::DragFloat((NameString("##X") + label).CStr(), &q.x, .025f); ImGui::SameLine();

        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        valueChanged |= ImGui::DragFloat((NameString("##Y") + label).CStr(), &q.y, .025f); ImGui::SameLine();

        ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        valueChanged |= ImGui::DragFloat((NameString("##Z") + label).CStr(), &q.z, .025f);

        ImGui::TextColored(ImVec4(1, 1, 1, 1), "T"); ImGui::SameLine();
        ImGui::SetNextItemWidth(inputFieldsWidth);
        valueChanged |= ImGui::DragFloat((NameString("##T") + label).CStr(), &q.t, .025f);

        if (valueChanged) {
            q = q.Normalize();
        }

        return valueChanged;
    }
}

bool ImGui::IsInsidePreviousItem(const LVec2& position) {
    const ImVec2 rectMin = ImGui::GetItemRectMin();
    const ImVec2 rectMax = ImGui::GetItemRectMax();
    return position.x >= rectMin.x && position.x <= rectMax.x && position.y >= rectMin.y && position.y <= rectMax.y;
}
bool ImGui::IsInsideWindow(const LVec2& position) {
    const ImVec2 winPos = ImGui::GetWindowPos();
    const ImVec2 winSize = ImGui::GetWindowSize();
    return position.x >= winPos.x && position.x <= winPos.x + winSize.x && position.y >= winPos.y && position.y <= winPos.y + winSize.y;
}
bool ImGui::IsItemDisabled() {
    ASSERT(ImGui::GetCurrentContext() != nullptr);
    return (ImGui::GetCurrentContext()->CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
}

bool ImGui::MainMenuBarItem(const char* menuName, const char* name, const char* shortcut, bool* isSelected) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(menuName)) {
            ImGui::MenuItem(name, shortcut, isSelected);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    return *isSelected;
}
