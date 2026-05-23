#include "ui/UiTheme.h"

#include <Windows.h>

namespace
{
    ImVec4 ColorFromBytes(int r, int g, int b, int a = 255)
    {
        return ImVec4(
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f);
    }

    float GetWindowScale(HWND hwnd)
    {
        if (hwnd == nullptr)
        {
            return 1.0f;
        }

        const UINT dpi = GetDpiForWindow(hwnd);
        return dpi > 0 ? static_cast<float>(dpi) / 96.0f : 1.0f;
    }
}

namespace wpcc::ui
{
    const Palette& GetPalette()
    {
        static const Palette palette{
            ColorFromBytes(15, 18, 24),
            ColorFromBytes(23, 28, 36),
            ColorFromBytes(27, 33, 42),
            ColorFromBytes(33, 40, 50),
            ColorFromBytes(58, 67, 80),
            ColorFromBytes(42, 50, 62),
            ColorFromBytes(235, 238, 244),
            ColorFromBytes(161, 170, 183),
            ColorFromBytes(105, 116, 130),
            ColorFromBytes(105, 151, 188),
            ColorFromBytes(129, 174, 210),
            ColorFromBytes(37, 58, 76),
            ColorFromBytes(91, 176, 137),
            ColorFromBytes(212, 164, 84),
            ColorFromBytes(215, 103, 103),
            ColorFromBytes(116, 128, 144),
        };

        return palette;
    }

    const LayoutMetrics& GetLayoutMetrics()
    {
        static const LayoutMetrics metrics{
            16.0f,
            18.0f,
            16.0f,
            14.0f,
            72.0f,
            232.0f,
            320.0f,
            46.0f,
            38.0f,
            36.0f,
            7.0f,
            12.0f,
        };

        return metrics;
    }

    void LoadFonts(HWND hwnd)
    {
        ImGuiIO& io = ImGui::GetIO();
        const float dpiScale = GetWindowScale(hwnd);
        const float fontSize = 17.0f * dpiScale;

        ImFontConfig config{};
        config.OversampleH = 3;
        config.OversampleV = 2;
        config.PixelSnapH = false;

        ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", fontSize, &config);
        if (font == nullptr)
        {
            font = io.Fonts->AddFontDefault();
        }

        io.FontDefault = font;
    }

    void ApplyStyle(HWND hwnd)
    {
        const Palette& palette = GetPalette();
        const float dpiScale = GetWindowScale(hwnd);
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 0.0f;
        style.ChildRounding = 12.0f;
        style.FrameRounding = 8.0f;
        style.PopupRounding = 10.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding = 8.0f;
        style.TabRounding = 7.0f;
        style.WindowBorderSize = 0.0f;
        style.ChildBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.ItemSpacing = ImVec2(10.0f, 11.0f);
        style.ItemInnerSpacing = ImVec2(9.0f, 7.0f);
        style.WindowPadding = ImVec2(0.0f, 0.0f);
        style.FramePadding = ImVec2(14.0f, 9.0f);
        style.CellPadding = ImVec2(16.0f, 10.0f);
        style.ScrollbarSize = 12.0f;
        style.DisabledAlpha = 0.48f;
        style.ScaleAllSizes(dpiScale);

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = palette.text;
        colors[ImGuiCol_TextDisabled] = palette.textSubtle;
        colors[ImGuiCol_WindowBg] = palette.background;
        colors[ImGuiCol_ChildBg] = palette.panel;
        colors[ImGuiCol_PopupBg] = palette.panelRaised;
        colors[ImGuiCol_Border] = palette.borderSoft;
        colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        colors[ImGuiCol_FrameBg] = ColorFromBytes(30, 36, 45);
        colors[ImGuiCol_FrameBgHovered] = ColorFromBytes(38, 46, 57);
        colors[ImGuiCol_FrameBgActive] = ColorFromBytes(44, 54, 67);
        colors[ImGuiCol_TitleBg] = palette.panel;
        colors[ImGuiCol_TitleBgActive] = palette.panel;
        colors[ImGuiCol_Button] = ColorFromBytes(38, 58, 76);
        colors[ImGuiCol_ButtonHovered] = ColorFromBytes(48, 72, 94);
        colors[ImGuiCol_ButtonActive] = ColorFromBytes(43, 65, 85);
        colors[ImGuiCol_Header] = ColorFromBytes(35, 43, 54);
        colors[ImGuiCol_HeaderHovered] = ColorFromBytes(43, 52, 65);
        colors[ImGuiCol_HeaderActive] = ColorFromBytes(49, 60, 75);
        colors[ImGuiCol_TableHeaderBg] = ColorFromBytes(28, 35, 44);
        colors[ImGuiCol_TableBorderStrong] = palette.borderSoft;
        colors[ImGuiCol_TableBorderLight] = ColorFromBytes(31, 38, 48);
        colors[ImGuiCol_TableRowBg] = ColorFromBytes(21, 26, 34);
        colors[ImGuiCol_TableRowBgAlt] = ColorFromBytes(24, 30, 38);
        colors[ImGuiCol_CheckMark] = palette.accent;
        colors[ImGuiCol_SliderGrab] = palette.accent;
        colors[ImGuiCol_Separator] = palette.borderSoft;
        colors[ImGuiCol_ScrollbarBg] = ColorFromBytes(16, 20, 26);
        colors[ImGuiCol_ScrollbarGrab] = ColorFromBytes(46, 55, 68);
        colors[ImGuiCol_ScrollbarGrabHovered] = ColorFromBytes(58, 69, 84);
        colors[ImGuiCol_ScrollbarGrabActive] = ColorFromBytes(70, 83, 101);
    }
}
