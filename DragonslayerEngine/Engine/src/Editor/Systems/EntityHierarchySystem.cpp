#include "Editor/Systems/EntityHierarchySystem.hpp"

#include "ECS/HierarchyDirectory.hpp"
#include "Core/Containers/ArrayHelper.hpp"

#include "Core/Log.hpp"
#include "ECS/EntityName.hpp"
#include "Editor/ComponentMetadata.hpp"
#include "Editor/Components/HierarchySelection.hpp"
#include "Editor/EditorGlobals.hpp"
#include "Editor/TimeScope.hpp"

#include "Runtime/Components/Transform.hpp"

// Helper ImGui Functions
namespace ImGui {
    static bool SelectableColoredBulletText(const char* label, bool selected, ImU32 col, ImGuiSelectableFlags flags = 0) {
        // Save the starting local cursor position
        ImVec2 startPos = ImGui::GetCursorPos();

        // Draw an empty Selectable to act as the clickable bounding box/background.
        bool itemSelected = ImGui::Selectable("##entity_selectable", selected, flags);

        // Save the end position so our layout remains intact
        ImVec2 endPos = ImGui::GetCursorPos();

        // Move the cursor back over the empty Selectable
        ImGui::SetCursorPos(startPos);

        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::Bullet();
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::Text("%s", label);

        // Restore the cursor to the bottom of this item so the next loop iteration starts correctly
        ImGui::SetCursorPos(endPos);

        return itemSelected;
    }

    static bool TreeNodeColored(const char* label, const ImU32 col, ImGuiTreeNodeFlags flags = 0) {

        ImGui::PushID(label);

        flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;

        const ImVec2 startPosLocal = ImGui::GetCursorPos();
        const ImVec2 startPosScreen = ImGui::GetCursorScreenPos();

        // Get the standard width reserved for the arrow/indentation
        const float labelSpacing = ImGui::GetTreeNodeToLabelSpacing();

        // Draw the TreeNode (hiding the default arrow)
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 0));
        const bool isOpen = ImGui::TreeNodeEx("##treeNodeColored", flags);
        ImGui::PopStyleColor();

        const ImVec2 endPos = ImGui::GetCursorPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Icon Dimensions
        const float textHeight = ImGui::GetTextLineHeight();
        const float iconWidth = textHeight * 0.8f;
        const float iconHeight = textHeight * 0.7f;

        // Centering the icon horizontally within the 'labelSpacing' area
        // and vertically within the text line height
        const float iconPosX = startPosScreen.x + (labelSpacing - iconWidth) * 0.5f;
        const float iconPosY = startPosScreen.y + (textHeight - iconHeight) * 0.5f;

        const ImVec2 pMin = ImVec2(iconPosX, iconPosY);

        const float rounding = textHeight * 0.1f;
        const float tabHeight = iconHeight * 0.25f;
        const float tabWidth = iconWidth * 0.4f;

        // Draw the top-left folder tab
        // We extend the Y max by 1.0f to overlap the body and prevent a faint anti-aliasing seam
        drawList->AddRectFilled(
            pMin,
            ImVec2(pMin.x + tabWidth, pMin.y + tabHeight + 1.0f),
            col,
            rounding,
            ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight
        );

        // Standard flags for the main folder body: Round everything except the top-left where the tab sits
        constexpr ImDrawFlags bodyRoundingFlags = ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersBottomRight | ImDrawFlags_RoundCornersTopRight;

        // Draw the back of the folder
        drawList->AddRectFilled(
            ImVec2(pMin.x, pMin.y + tabHeight),
            ImVec2(pMin.x + iconWidth, pMin.y + iconHeight),
            col,
            rounding,
            bodyRoundingFlags
        );

        constexpr float shadowThickness = 1.5;
        constexpr ImU32 shadowCol = IM_COL32(0, 0, 0, 150);
        if (isOpen) {

            // Draw the front flap
            const ImVec2 points[4] = {
                ImVec2(pMin.x, pMin.y + iconHeight),
                ImVec2(pMin.x + iconWidth * .2f, pMin.y + iconHeight * 0.5f),
                ImVec2(pMin.x + iconWidth * 1.2f, pMin.y + iconHeight * 0.5f),
                ImVec2(pMin.x + iconWidth, pMin.y + iconHeight),
            };

            drawList->AddConvexPolyFilled(points, 4, col);

            const ImVec2 adjustedPoint2(pMin.x + iconWidth * 1.1f, pMin.y + iconHeight * 0.5f);

            drawList->AddLine(points[0], points[1], shadowCol, shadowThickness);
            drawList->AddLine(points[1], adjustedPoint2, shadowCol, shadowThickness);
        }
        else
        {
            // Draw folder opening shadow
            const ImVec2 points[2] = {
                ImVec2(pMin.x, pMin.y + tabHeight * .8f),
                ImVec2(pMin.x + tabWidth, pMin.y + tabHeight * .8f)
            };

            drawList->AddLine(points[0], points[1], shadowCol, shadowThickness);
        }

        // Draw the label precisely where standard text would start
        ImGui::SetCursorPos(ImVec2(startPosLocal.x + labelSpacing, startPosLocal.y));
        ImGui::TextUnformatted(label);

        // Restore the cursor
        ImGui::SetCursorPos(endPos);

        return isOpen;
    }

    static void TreeNodeColoredPop() {
        ImGui::PopID();
    }
}

namespace HierarchyStyle {
    static ImU32 EntityIconColor = IM_COL32(100, 100, 100, 255);
    static ImU32 DirectoryIconColor = IM_COL32(179 + 50, 146 + 50, 79 + 50, 255);
}

static void DisplayEntities(Vault& vault, const ImGuiTextFilter& textFilter, const Entity* entities, uint32 entityCount) {

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(entityCount));

    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            Entity entity = entities[i];

            NameString entityName = "Entity " + NameString(entity);

            // Entity has name component so use that instead
            if (const EntityName* entityNameComponent = vault.TryGetComponent<EntityName>(entity)) {
                entityName = entityNameComponent->string;
            }

            // Only display entities which pass the text filter
            if (!textFilter.PassFilter(entityName.CStr())) {
                continue;
            }

            const bool isSelected = gHierarchySelection.selectedEntity == entity;

            ImGui::PushID(static_cast<int>(entity));

            // Draw entity
            if (ImGui::SelectableColoredBulletText(
                entityName.CStr(),
                isSelected,
                HierarchyStyle::EntityIconColor,
                ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_AllowDoubleClick)) {

                const bool doubleClicked = ImGui::IsMouseDoubleClicked(0);

                // Entity was already selected so we deselect it
                if (!doubleClicked && isSelected) {
                    gHierarchySelection.selectedEntity = ECS::InvalidEntity;
                    gHierarchySelection.doubleClickedEntity = false;

                    ImGui::PopID();
                    continue;
                }

                gHierarchySelection.selectedEntity = entity;
                gHierarchySelection.doubleClickedEntity = doubleClicked;
                }

            ImGui::PopID();
        }
    }
    clipper.End();
}

struct HierarchyDirectoryNode {

    NameString name;
    TempArray<uint32> childDirectories; // Indices pointing to the NodeStorage
    TempArray<Entity> entities;

    explicit HierarchyDirectoryNode(const NameString& name) :
        name(name),
        childDirectories(8),
        entities(1) {}

    // Static so we don't carry `this` across Emplace-induced reallocations of nodeStorage.
    // nodeIndex addresses the node inside nodeStorage and is re-resolved after each growth.
    static void InsertEntity(TempArray<HierarchyDirectoryNode>& nodeStorage, uint32 nodeIndex, Entity entity, const HierarchyDirectory& directory) {

        if (directory.Folders.IsEmpty()) {
            return;
        }

        InsertEntityRecursive(nodeStorage, nodeIndex, entity, directory, 0);
    }

private:
    static void InsertEntityRecursive(TempArray<HierarchyDirectoryNode>& nodeStorage, uint32 nodeIndex, Entity entity, const HierarchyDirectory& directory, uint32 folderIndex) { // NOLINT(*-no-recursion)

        const uint32 stopSearchIndex = directory.Folders.GetSize();

        if (folderIndex == stopSearchIndex) {
            nodeStorage[nodeIndex].entities.Add(entity);
            return;
        }

        const NameString& dirName = directory.Folders[folderIndex];

        const uint32 childCount = nodeStorage[nodeIndex].childDirectories.GetSize();
        for (uint32 i = 0; i < childCount; ++i) {
            const uint32 childIndex = nodeStorage[nodeIndex].childDirectories[i];
            if (nodeStorage[childIndex].name == dirName) {
                InsertEntityRecursive(nodeStorage, childIndex, entity, directory, folderIndex + 1);
                return;
            }
        }

        // Emplace may reallocate nodeStorage; any reference we hold would dangle, so re-resolve via indices after this point.
        nodeStorage.Emplace(dirName);
        const uint32 newNodeIndex = nodeStorage.GetSize() - 1;
        nodeStorage[nodeIndex].childDirectories.Add(newNodeIndex);
        InsertEntityRecursive(nodeStorage, newNodeIndex, entity, directory, folderIndex + 1);
    }
};

static void DisplayHierarchyDirectories(Vault& vault, const ImGuiTextFilter& textFilter, TempArray<HierarchyDirectoryNode>& nodes, HierarchyDirectoryNode& parentNode) { // NOLINT(*-no-recursion)

    if (nodes.IsEmpty()) {
        return;
    }

    for (uint32 childNodeIndex : parentNode.childDirectories) {

        HierarchyDirectoryNode& childNode = nodes[childNodeIndex];

        // Force directories to be displayed as open when text searching for an entity
        if (textFilter.IsActive()) {
            ImGui::SetNextItemOpen(true); \
        }

        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 8.0f);

        if (ImGui::TreeNodeColored(childNode.name.CStr(), HierarchyStyle::DirectoryIconColor)) {

            // Display subdirectories in directory
            DisplayHierarchyDirectories(vault, textFilter, nodes, childNode);

            // Display entities in directory
            DisplayEntities(vault, textFilter, childNode.entities.GetData(), childNode.entities.GetSize());

            ImGui::TreePop();
        } ImGui::TreeNodeColoredPop(); // Necessary since it pops the id that was pushed in TreeNodeColored!

        ImGui::PopStyleVar();
    }
}

void EntityHierarchySystem::Update(ThreadContext& threadContext, Vault& vault) {
    if (!threadContext.IsMainThread()) {
        return;
    }

    gHierarchySelection.doubleClickedEntity = false;

    static bool showHierarchy = true;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Entity Hierarchy", "", &showHierarchy);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (!showHierarchy) {
        return;
    }

    if (ImGui::Begin("Hierarchy", &showHierarchy)) {

        if (ImGui::BeginChild("##tree", ImVec2(300, 0), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened)) {

            // Entity Name Filter
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
            ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
            static ImGuiTextFilter textFilter;
            if (ImGui::InputTextWithHint("##Filter", "incl,-excl", textFilter.InputBuf, IM_ARRAYSIZE(textFilter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll))
                textFilter.Build();
            ImGui::PopItemFlag();
            ImGui::Separator();

            // Gather entities in root dir
            TempArray<Entity> entitiesInRootDir(128);
            for (const Entity& entity : vault.GetAllEntities()) {

                if (vault.HasComponent<HierarchyDirectory>(entity)) {
                    continue;
                }

                entitiesInRootDir.Add(entity);
            }


            // Gather entities with directories
            TempArray<HierarchyDirectoryNode> hierarchyDirectoryNodes(64);
            hierarchyDirectoryNodes.Emplace("Root");
            constexpr uint32 ROOT_DIRECTORY_NODE_INDEX = 0;
            for (auto& [hierarchyDirectory, entity] : vault.GetComponents<HierarchyDirectory>()) {

                if (hierarchyDirectory.Folders.IsEmpty()) {
                    entitiesInRootDir.Emplace(entity);
                }

                HierarchyDirectoryNode::InsertEntity(hierarchyDirectoryNodes, ROOT_DIRECTORY_NODE_INDEX, entity, hierarchyDirectory);
            }

            // Display entities inside directories, resolve root from storage here since any Emplace above may have relocated it.
            DisplayHierarchyDirectories(vault, textFilter, hierarchyDirectoryNodes, hierarchyDirectoryNodes[ROOT_DIRECTORY_NODE_INDEX]);

            // Display entities in root directory
            DisplayEntities(vault, textFilter, entitiesInRootDir.GetData(), entitiesInRootDir.GetSize());

            // TODO Fix this hack
            // Without it the scroll bar does not scroll as further down as it should leaving some elements invisible
            // Only happens when entity has a lot of components
            ImGui::Dummy(ImVec2(0.0f, 200.0f));
        }
        ImGui::EndChild();

        // Right side: draw properties
        ImGui::SameLine();

        if (ImGui::BeginChild("##EntityView")) {

            if (vault.IsEntityValid(gHierarchySelection.selectedEntity)) {

                const Entity selectedEntity = gHierarchySelection.selectedEntity;

                NameString entityName;

                // Entity has name component so use that instead
                if (EntityName* entityNameComponent = vault.TryGetComponent<EntityName>(selectedEntity)) {
                    entityName = entityNameComponent->string;
                    if (ImGui::InputText("##entityName", entityName.Data(), NAME_STRING_LENGTH)) {
                        entityNameComponent->string = entityName.CStr();
                    }
                } else {
                    entityName = "Entity " + NameString(selectedEntity);
                    if (ImGui::InputText("##entityName", entityName.Data(), NAME_STRING_LENGTH)) {
                        vault.EmplaceComponent<EntityName>(selectedEntity, entityName);
                    }
                }

                ImGui::TextDisabled("UID: %d", selectedEntity);
                ImGui::Separator();

                // Draw Transform first
                if (Transform* transform = vault.TryGetComponent<Transform>(selectedEntity)) {
                    transform->OnEditorUI();
                }

                for (ComponentMetadata& componentMetadata: GetComponentsMetadata()) {

                    if (componentMetadata.getComponentFlags().isDisplayable) {

                        // Transform was drawn already so just skip it!
                        if (componentMetadata.componentName == "Transform") {
                            continue;
                        }

                        if (void* component = componentMetadata.getComponent(vault, selectedEntity)) {
                            if (componentMetadata.onEditorUI(component))
                            {
                                componentMetadata.destroyComponent(vault, selectedEntity);
                            }
                        }
                    }
                }

                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

                const char* componentsPopupID = "componentsPopup";
                // Button uses all available horizontal space
                if (ImGui::Button("Add Component", ImVec2(-FLT_MIN, 0.0f))) {
                    ImGui::OpenPopup(componentsPopupID);
                }
                ImGui::PopStyleVar();

                if (ImGui::BeginPopup(componentsPopupID))
                {
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
                    ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
                    static ImGuiTextFilter textFilter;
                    if (ImGui::InputTextWithHint("##Filter", "incl,-excl", textFilter.InputBuf, IM_ARRAYSIZE(textFilter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll))
                        textFilter.Build();
                    ImGui::PopItemFlag();
                    ImGui::Separator();

                    for (ComponentMetadata& componentMetadata: GetComponentsMetadata()) {

                        if (componentMetadata.getComponentFlags().isDisplayable) {

                            if (!textFilter.PassFilter(componentMetadata.componentName.CStr()))
                            {
                                continue;
                            }

                            // Only display components entity does not already have
                            if (!componentMetadata.getComponent(vault, selectedEntity)) {
                                if (ImGui::Selectable(componentMetadata.componentName.CStr())) {
                                    componentMetadata.createComponent(vault, selectedEntity);
                                }
                            }
                        }
                    }

                    ImGui::EndPopup();
                }


                // TODO Fix this hack
                // Without it the scroll bar does not scroll as further down as it should leaving some elements invisible
                // Only happens when entity has a lot of components
                ImGui::Dummy(ImVec2(0.0f, 200.0f));
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();
}
