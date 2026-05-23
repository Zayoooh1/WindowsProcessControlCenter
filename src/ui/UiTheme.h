#pragma once

#include <imgui.h>

struct HWND__;
using HWND = HWND__*;

namespace wpcc::ui
{
    struct Palette
    {
        ImVec4 background;
        ImVec4 panel;
        ImVec4 panelAlt;
        ImVec4 panelRaised;
        ImVec4 border;
        ImVec4 borderSoft;
        ImVec4 text;
        ImVec4 textMuted;
        ImVec4 textSubtle;
        ImVec4 accent;
        ImVec4 accentHover;
        ImVec4 accentSoft;
        ImVec4 success;
        ImVec4 warning;
        ImVec4 danger;
        ImVec4 neutral;
    };

    struct LayoutMetrics
    {
        float appPadding;
        float panelPadding;
        float panelGap;
        float sectionGap;
        float topBarHeight;
        float sidebarWidth;
        float detailsPanelWidth;
        float tableRowHeight;
        float searchHeight;
        float actionButtonHeight;
        float badgeRounding;
        float panelRounding;
    };

    const Palette& GetPalette();
    const LayoutMetrics& GetLayoutMetrics();
    void LoadFonts(HWND hwnd);
    void ApplyStyle(HWND hwnd);
}
