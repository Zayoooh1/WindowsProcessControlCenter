#pragma once

#include "core/ProcessInfo.h"

#include <Windows.h>
#include <d3d11.h>

#include <array>
#include <string>
#include <vector>

namespace wpcc
{
    class UiLayer
    {
    public:
        UiLayer();
        ~UiLayer();

        bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
        void Render();
        bool HandleWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    private:
        void RenderTopBar();
        void RenderSidebar();
        void RenderProcessesView();
        void RenderProcessToolbar(float contentWidth);
        void RenderProcessTable();
        void RenderDetailsPanel();
        void RenderToast();
        void RebuildFilteredProcessIndices();
        void RefreshProcesses();
        void ShowNotImplementedToast(const char* actionName);
        bool MatchesSearch(const ProcessInfo& process) const;
        const ProcessInfo* FindSelectedProcess() const;

        std::vector<ProcessInfo> m_processes;
        std::vector<size_t> m_filteredProcessIndices;
        std::array<char, 128> m_searchBuffer{};
        unsigned long m_selectedProcessPid = 0;
        int m_selectedNavigationIndex = 1;
        std::string m_toastMessage;
        float m_toastTimer = 0.0f;
        bool m_initialized = false;
    };
}
