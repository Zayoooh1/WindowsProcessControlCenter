#include "ui/UiLayer.h"

#include "core/ProcessProvider.h"
#include "ui/UiTheme.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>

namespace
{
    std::string ToLowerAscii(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        return value;
    }

    ImVec4 WithAlpha(ImVec4 color, float alpha)
    {
        color.w = alpha;
        return color;
    }

    void DrawBadge(const char* label, const ImVec4& fill, const ImVec4& textColor)
    {
        const auto& metrics = wpcc::ui::GetLayoutMetrics();
        const ImVec2 textSize = ImGui::CalcTextSize(label);
        const ImVec2 padding(10.0f, 4.0f);
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const ImVec2 size(textSize.x + padding.x * 2.0f, textSize.y + padding.y * 2.0f);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::ColorConvertFloat4ToU32(fill), metrics.badgeRounding);
        drawList->AddText(ImVec2(pos.x + padding.x, pos.y + padding.y), ImGui::ColorConvertFloat4ToU32(textColor), label);
        ImGui::Dummy(size);
    }

    void DrawSectionHeader(const char* title)
    {
        const auto& palette = wpcc::ui::GetPalette();
        const auto& metrics = wpcc::ui::GetLayoutMetrics();
        ImGui::PushStyleColor(ImGuiCol_Text, palette.text);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(1.0f, 5.0f));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(1.0f, metrics.sectionGap * 0.55f));
    }

    std::string EllipsizeMiddle(const std::string& value, float maxWidth)
    {
        if (value.empty() || ImGui::CalcTextSize(value.c_str()).x <= maxWidth)
        {
            return value;
        }

        if (maxWidth <= ImGui::CalcTextSize("...").x)
        {
            return "...";
        }

        const size_t keepTail = std::min<size_t>(value.size(), 38);
        for (size_t keepHead = std::min<size_t>(value.size(), 28); keepHead > 4; --keepHead)
        {
            const std::string candidate = value.substr(0, keepHead) + "..." + value.substr(value.size() - keepTail);
            if (ImGui::CalcTextSize(candidate.c_str()).x <= maxWidth)
            {
                return candidate;
            }
        }

        for (size_t tail = std::min<size_t>(value.size(), 34); tail > 6; --tail)
        {
            const std::string candidate = "..." + value.substr(value.size() - tail);
            if (ImGui::CalcTextSize(candidate.c_str()).x <= maxWidth)
            {
                return candidate;
            }
        }

        return "...";
    }

    void DrawClippedTextWithTooltip(const std::string& value, float maxWidth, const char* fallback = "Unavailable")
    {
        const std::string displayValue = value.empty() ? fallback : value;
        const std::string clipped = EllipsizeMiddle(displayValue, maxWidth);
        ImGui::TextUnformatted(clipped.c_str());

        if (clipped != displayValue && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", displayValue.c_str());
        }
    }

    void DrawInfoRow(const char* label, const std::string& value)
    {
        const auto& palette = wpcc::ui::GetPalette();
        const auto& metrics = wpcc::ui::GetLayoutMetrics();
        ImGui::PushStyleColor(ImGuiCol_Text, palette.textSubtle);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();

        const float valueWidth = ImGui::GetContentRegionAvail().x;
        DrawClippedTextWithTooltip(value, valueWidth);
        ImGui::Dummy(ImVec2(1.0f, metrics.sectionGap * 0.45f));
    }

    void DrawDisabledActionButton(const char* label)
    {
        const auto& metrics = wpcc::ui::GetLayoutMetrics();
        const float width = ImGui::GetContentRegionAvail().x;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::BeginDisabled(true);
        ImGui::Button(label, ImVec2(width, metrics.actionButtonHeight));
        ImGui::EndDisabled();
        ImGui::PopStyleVar();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Not implemented yet");
        }
    }

    void DrawSearchBox(std::array<char, 128>& buffer, float width)
    {
        const auto& palette = wpcc::ui::GetPalette();
        const auto& metrics = wpcc::ui::GetLayoutMetrics();
        ImGui::PushStyleColor(ImGuiCol_FrameBg, palette.panelRaised);
        ImGui::PushStyleColor(ImGuiCol_Border, palette.border);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f, 10.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f);
        ImGui::SetNextItemWidth(width);
        ImGui::InputTextWithHint("##ProcessSearch", "Search by process, PID, or executable path", buffer.data(), buffer.size());
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        if ((max.y - min.y) < metrics.searchHeight)
        {
            ImGui::Dummy(ImVec2(1.0f, metrics.searchHeight - (max.y - min.y)));
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    ImVec4 BadgeFillForStatus(const std::string& status)
    {
        const auto& palette = wpcc::ui::GetPalette();
        if (status == "Accessible")
        {
            return WithAlpha(palette.success, 0.20f);
        }
        if (status == "Access denied")
        {
            return WithAlpha(palette.warning, 0.22f);
        }
        if (status == "Protected/System")
        {
            return WithAlpha(palette.danger, 0.20f);
        }
        return WithAlpha(palette.neutral, 0.20f);
    }

    ImVec4 BadgeTextForStatus(const std::string& status)
    {
        const auto& palette = wpcc::ui::GetPalette();
        if (status == "Accessible")
        {
            return palette.success;
        }
        if (status == "Access denied")
        {
            return palette.warning;
        }
        if (status == "Protected/System")
        {
            return palette.danger;
        }
        return palette.textMuted;
    }

    ImVec4 BadgeFillForPriority(const std::string& priority)
    {
        const auto& palette = wpcc::ui::GetPalette();
        if (priority == "High" || priority == "Realtime")
        {
            return WithAlpha(palette.danger, 0.18f);
        }
        if (priority == "Above normal")
        {
            return WithAlpha(palette.warning, 0.18f);
        }
        if (priority == "Below normal" || priority == "Idle")
        {
            return WithAlpha(palette.neutral, 0.18f);
        }
        return WithAlpha(palette.accent, 0.16f);
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace wpcc
{
    UiLayer::UiLayer()
    {
        RefreshProcesses();
    }

    UiLayer::~UiLayer()
    {
        if (m_initialized)
        {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            m_initialized = false;
        }
    }

    bool UiLayer::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;

        ui::LoadFonts(hwnd);
        ui::ApplyStyle(hwnd);

        if (!ImGui_ImplWin32_Init(hwnd))
        {
            return false;
        }

        if (!ImGui_ImplDX11_Init(device, context))
        {
            ImGui_ImplWin32_Shutdown();
            return false;
        }

        m_initialized = true;
        return true;
    }

    void UiLayer::Render()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        constexpr ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("WindowsProcessControlCenter.Root", nullptr, windowFlags);
        ImGui::PopStyleVar();

        const auto& metrics = ui::GetLayoutMetrics();
        ImGui::SetCursorPos(ImVec2(metrics.appPadding, metrics.appPadding));
        RenderTopBar();

        ImGui::SetCursorPos(ImVec2(metrics.appPadding, metrics.appPadding + metrics.topBarHeight + metrics.panelGap));
        RenderSidebar();

        ImGui::SetCursorPos(ImVec2(metrics.appPadding + metrics.sidebarWidth + metrics.panelGap, metrics.appPadding + metrics.topBarHeight + metrics.panelGap));
        RenderProcessesView();
        RenderToast();

        ImGui::End();

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    bool UiLayer::HandleWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        return ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam) != 0;
    }

    void UiLayer::RenderTopBar()
    {
        const auto& palette = ui::GetPalette();
        const auto& metrics = ui::GetLayoutMetrics();
        const float width = ImGui::GetContentRegionAvail().x - metrics.appPadding;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, palette.panel);
        ImGui::BeginChild("TopBar", ImVec2(width, metrics.topBarHeight), true);

        ImGui::SetCursorPos(ImVec2(metrics.panelPadding, 14.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, palette.text);
        ImGui::TextUnformatted("Windows Process Control Center");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, palette.textSubtle);
        ImGui::TextUnformatted("Read-only process monitor");
        ImGui::PopStyleColor();

        const float controlsWidth = 360.0f;
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - controlsWidth - metrics.panelPadding, 17.0f));
        DrawBadge("User mode", WithAlpha(palette.accent, 0.18f), palette.accentHover);
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, palette.textMuted);
        ImGui::Text("%zu processes", m_processes.size());
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 12.0f);
        if (ImGui::Button("Refresh", ImVec2(96.0f, 34.0f)))
        {
            RefreshProcesses();
            m_toastMessage = "Process list refreshed";
            m_toastTimer = 2.0f;
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void UiLayer::RenderSidebar()
    {
        const auto& palette = ui::GetPalette();
        const auto& metrics = ui::GetLayoutMetrics();
        const float height = ImGui::GetContentRegionAvail().y - metrics.appPadding;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, palette.panel);
        ImGui::BeginChild("Sidebar", ImVec2(metrics.sidebarWidth, height), true);

        ImGui::SetCursorPos(ImVec2(metrics.panelPadding, metrics.panelPadding));
        ImGui::PushStyleColor(ImGuiCol_Text, palette.textSubtle);
        ImGui::TextUnformatted("NAVIGATION");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(1.0f, 12.0f));

        constexpr const char* items[] = {
            "Dashboard",
            "Processes",
            "Rules / Profiles",
            "Settings",
            "About",
        };

        for (int index = 0; index < static_cast<int>(std::size(items)); ++index)
        {
            ImGui::PushID(index);
            const bool selected = m_selectedNavigationIndex == index;
            const ImVec2 itemPos = ImGui::GetCursorScreenPos();
            const ImVec2 itemSize(metrics.sidebarWidth - metrics.panelPadding * 2.0f, 42.0f);

            if (selected)
            {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(itemPos, ImVec2(itemPos.x + itemSize.x, itemPos.y + itemSize.y), ImGui::ColorConvertFloat4ToU32(palette.accentSoft), 9.0f);
                drawList->AddRectFilled(ImVec2(itemPos.x + 1.0f, itemPos.y + 8.0f), ImVec2(itemPos.x + 4.0f, itemPos.y + itemSize.y - 8.0f), ImGui::ColorConvertFloat4ToU32(palette.accent), 2.0f);
            }

            ImGui::SetCursorPosX(metrics.panelPadding + 8.0f);
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, WithAlpha(palette.panelRaised, 0.72f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, palette.accentSoft);
            ImGui::PushStyleColor(ImGuiCol_Text, selected ? palette.text : palette.textMuted);
            if (ImGui::Selectable(items[index], selected, 0, ImVec2(itemSize.x - 8.0f, itemSize.y)))
            {
                m_selectedNavigationIndex = index;
                if (index != 1)
                {
                    ShowNotImplementedToast(items[index]);
                }
            }
            ImGui::PopStyleColor(4);
            ImGui::PopID();
            ImGui::Dummy(ImVec2(1.0f, 5.0f));
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void UiLayer::RenderProcessesView()
    {
        RebuildFilteredProcessIndices();

        const auto& palette = ui::GetPalette();
        const auto& metrics = ui::GetLayoutMetrics();
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        const float availableHeight = ImGui::GetContentRegionAvail().y;
        const bool showDetails = availableWidth >= 900.0f;
        const float detailsWidth = showDetails ? metrics.detailsPanelWidth : 0.0f;
        const float gapWidth = showDetails ? metrics.panelGap : 0.0f;
        const float mainPanelWidth = std::max(availableWidth - detailsWidth - gapWidth - metrics.appPadding, 520.0f);
        const float panelHeight = availableHeight - metrics.appPadding;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, palette.panel);
        ImGui::BeginChild("ProcessesContent", ImVec2(mainPanelWidth, panelHeight), true);
        ImGui::SetCursorPos(ImVec2(metrics.panelPadding, metrics.panelPadding));

        RenderProcessToolbar(mainPanelWidth - metrics.panelPadding * 2.0f);
        RenderProcessTable();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (showDetails)
        {
            ImGui::SameLine(0.0f, metrics.panelGap);
            RenderDetailsPanel();
        }
    }

    void UiLayer::RenderProcessToolbar(float contentWidth)
    {
        const auto& palette = ui::GetPalette();
        ImGui::PushStyleColor(ImGuiCol_Text, palette.text);
        ImGui::TextUnformatted("Processes");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, palette.textSubtle);
        ImGui::Text("%zu shown from %zu active processes", m_filteredProcessIndices.size(), m_processes.size());
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
        DrawSearchBox(m_searchBuffer, std::min(520.0f, contentWidth));
        ImGui::Dummy(ImVec2(1.0f, ui::GetLayoutMetrics().sectionGap));
    }

    void UiLayer::RenderProcessTable()
    {
        const auto& palette = ui::GetPalette();
        const auto& metrics = ui::GetLayoutMetrics();
        const ImVec2 tableSize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
        constexpr ImGuiTableFlags tableFlags =
            ImGuiTableFlags_BordersInnerH |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_Reorderable |
            ImGuiTableFlags_Hideable |
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_ScrollX |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_PadOuterX;

        if (!ImGui::BeginTable("ProcessTable", 6, tableFlags, tableSize))
        {
            return;
        }

        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 84.0f);
        ImGui::TableSetupColumn("Process", ImGuiTableColumnFlags_WidthFixed, 170.0f);
        ImGui::TableSetupColumn("Executable Path", ImGuiTableColumnFlags_WidthFixed, 300.0f);
        ImGui::TableSetupColumn("CPU Priority", ImGuiTableColumnFlags_WidthFixed, 132.0f);
        ImGui::TableSetupColumn("Admin", ImGuiTableColumnFlags_WidthFixed, 82.0f);
        ImGui::TableSetupColumn("Access", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableHeadersRow();

        for (size_t filteredIndex = 0; filteredIndex < m_filteredProcessIndices.size(); ++filteredIndex)
        {
            const size_t processIndex = m_filteredProcessIndices[filteredIndex];
            const ProcessInfo& process = m_processes[processIndex];
            const bool selected = m_selectedProcessPid == process.pid;

            ImGui::TableNextRow(ImGuiTableRowFlags_None, metrics.tableRowHeight);
            if (selected)
            {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::ColorConvertFloat4ToU32(WithAlpha(palette.accent, 0.18f)));
            }

            ImGui::TableSetColumnIndex(0);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
            ImGui::PushID(static_cast<int>(processIndex));
            const std::string rowLabel = std::to_string(process.pid) + "##row";
            if (ImGui::Selectable(rowLabel.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, metrics.tableRowHeight - 9.0f)))
            {
                m_selectedProcessPid = process.pid;
            }
            if (ImGui::IsItemHovered() && !selected)
            {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::ColorConvertFloat4ToU32(WithAlpha(palette.panelRaised, 0.55f)));
            }
            ImGui::PopID();

            ImGui::TableSetColumnIndex(1);
            DrawClippedTextWithTooltip(process.name, ImGui::GetContentRegionAvail().x);

            ImGui::TableSetColumnIndex(2);
            ImGui::PushStyleColor(ImGuiCol_Text, palette.textMuted);
            DrawClippedTextWithTooltip(process.executablePath, ImGui::GetContentRegionAvail().x);
            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(3);
            DrawBadge(process.cpuPriority.c_str(), BadgeFillForPriority(process.cpuPriority), palette.text);

            ImGui::TableSetColumnIndex(4);
            DrawBadge(process.likelyRequiresAdmin ? "Likely" : "No", process.likelyRequiresAdmin ? WithAlpha(palette.warning, 0.20f) : WithAlpha(palette.neutral, 0.16f), process.likelyRequiresAdmin ? palette.warning : palette.textMuted);

            ImGui::TableSetColumnIndex(5);
            DrawBadge(process.accessStatus.c_str(), BadgeFillForStatus(process.accessStatus), BadgeTextForStatus(process.accessStatus));
        }

        ImGui::EndTable();
    }

    void UiLayer::RenderDetailsPanel()
    {
        const auto& palette = ui::GetPalette();
        const auto& metrics = ui::GetLayoutMetrics();
        const ProcessInfo* selected = FindSelectedProcess();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, palette.panelAlt);
        ImGui::BeginChild("DetailsPanel", ImVec2(metrics.detailsPanelWidth, ImGui::GetContentRegionAvail().y), true);
        ImGui::SetCursorPos(ImVec2(metrics.panelPadding, metrics.panelPadding));

        ImGui::PushStyleColor(ImGuiCol_Text, palette.text);
        ImGui::TextUnformatted("Process details");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, palette.textSubtle);
        ImGui::TextUnformatted("Read-only inspection");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(1.0f, metrics.sectionGap));

        if (selected == nullptr)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, palette.textMuted);
            ImGui::TextWrapped("Select a process to inspect its details.");
            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::PopStyleColor();
            return;
        }

        DrawSectionHeader("Basic information");
        ImGui::PushStyleColor(ImGuiCol_Text, palette.text);
        DrawClippedTextWithTooltip(selected->name, ImGui::GetContentRegionAvail().x);
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, palette.textMuted);
        ImGui::Text("PID %lu", selected->pid);
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(1.0f, metrics.sectionGap));

        DrawSectionHeader("Executable path");
        DrawClippedTextWithTooltip(selected->executablePath, ImGui::GetContentRegionAvail().x);
        ImGui::Dummy(ImVec2(1.0f, metrics.sectionGap));

        DrawSectionHeader("Runtime status");
        DrawInfoRow("Snapshot mode", "Read-only");

        DrawSectionHeader("CPU priority");
        DrawBadge(selected->cpuPriority.c_str(), BadgeFillForPriority(selected->cpuPriority), palette.text);
        ImGui::Dummy(ImVec2(1.0f, metrics.sectionGap));

        DrawSectionHeader("Access status");
        DrawBadge(selected->accessStatus.c_str(), BadgeFillForStatus(selected->accessStatus), BadgeTextForStatus(selected->accessStatus));
        ImGui::Dummy(ImVec2(1.0f, metrics.sectionGap));

        DrawSectionHeader("Admin requirement");
        DrawInfoRow("Future operations", selected->likelyRequiresAdmin ? "Likely required for some process actions" : "Not detected for this read-only snapshot");
        if (!selected->accessError.empty())
        {
            DrawInfoRow("Access error", selected->accessError);
        }

        DrawSectionHeader("Future actions");
        DrawDisabledActionButton("End Process");
        DrawDisabledActionButton("Freeze");
        DrawDisabledActionButton("Resume");
        DrawDisabledActionButton("Set CPU Priority");
        DrawDisabledActionButton("Set GPU Preference");

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void UiLayer::RenderToast()
    {
        if (m_toastTimer <= 0.0f)
        {
            return;
        }

        const auto& palette = ui::GetPalette();
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        m_toastTimer -= ImGui::GetIO().DeltaTime;

        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 330.0f, viewport->WorkPos.y + 84.0f));
        ImGui::SetNextWindowBgAlpha(0.97f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, palette.panelRaised);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 9.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 10.0f));
        ImGui::Begin("ActionStatusToast", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
        ImGui::TextUnformatted(m_toastMessage.c_str());
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    void UiLayer::RebuildFilteredProcessIndices()
    {
        m_filteredProcessIndices.clear();
        for (size_t index = 0; index < m_processes.size(); ++index)
        {
            if (MatchesSearch(m_processes[index]))
            {
                m_filteredProcessIndices.push_back(index);
            }
        }
    }

    void UiLayer::ShowNotImplementedToast(const char* actionName)
    {
        m_toastMessage = std::string(actionName) + ": Not implemented yet";
        m_toastTimer = 2.4f;
    }

    void UiLayer::RefreshProcesses()
    {
        ProcessProvider provider;
        m_processes = provider.LoadProcesses();
        RebuildFilteredProcessIndices();

        if (m_processes.empty())
        {
            m_selectedProcessPid = 0;
            return;
        }

        const bool selectedStillExists = std::any_of(m_processes.begin(), m_processes.end(), [this](const ProcessInfo& process) {
            return process.pid == m_selectedProcessPid;
        });

        if (!selectedStillExists)
        {
            m_selectedProcessPid = m_processes.front().pid;
        }
    }

    bool UiLayer::MatchesSearch(const ProcessInfo& process) const
    {
        const std::string query = ToLowerAscii(m_searchBuffer.data());
        if (query.empty())
        {
            return true;
        }

        const std::string pid = std::to_string(process.pid);
        const std::string name = ToLowerAscii(process.name);
        const std::string path = ToLowerAscii(process.executablePath);

        return pid.find(query) != std::string::npos ||
            name.find(query) != std::string::npos ||
            path.find(query) != std::string::npos;
    }

    const ProcessInfo* UiLayer::FindSelectedProcess() const
    {
        const auto selected = std::find_if(m_processes.begin(), m_processes.end(), [this](const ProcessInfo& process) {
            return process.pid == m_selectedProcessPid;
        });

        return selected != m_processes.end() ? &(*selected) : nullptr;
    }
}
