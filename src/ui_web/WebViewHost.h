#pragma once

#include "core/ProcessActions.h"
#include "core/GpuPreferenceManager.h"
#include "core/ProcessProvider.h"
#include "core/ProfileStore.h"
#include "ui_web/WebMessageBridge.h"

#include <Windows.h>
#include <unknwn.h>
#include <WebView2.h>
#include <wrl/client.h>

#include <filesystem>
#include <string>

namespace wpcc
{
    class WebViewHost
    {
    public:
        static constexpr UINT ChooseExecutableWindowMessage = WM_APP + 101;

        bool Initialize(HWND hwnd);
        void Resize();
        void Shutdown();
        void RefreshProcesses();
        void ChooseExecutable();

    private:
        void OnEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment);
        void OnControllerCreated(HRESULT result, ICoreWebView2Controller* controller);
        void ConfigureWebView();
        void NavigateToFrontend();
        void SendProcessSnapshot();
        void HandleSetCpuPriority(std::wstring_view messageJson);
        void HandleTerminateProcess(std::wstring_view messageJson);
        void HandleFreezeProcess(std::wstring_view messageJson);
        void HandleResumeProcess(std::wstring_view messageJson);
        void HandleSetGpuPreference(std::wstring_view messageJson);
        void HandleLoadProfiles();
        void HandleSaveProfiles(std::wstring_view messageJson);
        void HandleExportProfilesToFile(std::wstring_view messageJson);
        void SendError(std::string_view message);
        void ShowInitializationError(std::wstring_view message) const;

        std::filesystem::path GetExecutableDirectory() const;
        std::filesystem::path GetFrontendIndexPath() const;
        std::wstring PathToFileUri(const std::filesystem::path& path) const;
        std::wstring HResultToMessage(HRESULT result) const;
        bool IsAllowedNavigationUri(std::wstring_view uri) const;

        HWND m_hwnd = nullptr;
        Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_environment;
        Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controller;
        Microsoft::WRL::ComPtr<ICoreWebView2> m_webView;
        EventRegistrationToken m_messageToken{};
        EventRegistrationToken m_navigationToken{};
        bool m_messageHandlerRegistered = false;
        bool m_navigationHandlerRegistered = false;
        ProcessProvider m_processProvider;
        ProcessActions m_processActions;
        GpuPreferenceManager m_gpuPreferenceManager;
        WebMessageBridge m_bridge;
    };
}
