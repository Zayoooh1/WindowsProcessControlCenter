#pragma once

#include "core/ProcessActions.h"
#include "core/GpuPreferenceManager.h"
#include "core/ProcessProvider.h"
#include "core/ProfileStore.h"
#include "core/AutoApplyEngine.h"
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
        static constexpr UINT WM_DOWNLOAD_PROGRESS = WM_APP + 102;
        static constexpr UINT WM_DOWNLOAD_COMPLETE = WM_APP + 103;

        struct ProgressData {
            uint32_t downloaded;
            uint32_t total;
        };

        bool Initialize(HWND hwnd, AutoApplyEngine* autoApplyEngine);
        void Resize();
        void Shutdown();
        void RefreshProcesses();
        void ChooseExecutable();
        void OnDownloadComplete(bool success, const std::wstring& filePath);
        void NotifyDownloadProgress(uint32_t downloadedBytes, uint32_t totalBytes);

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
        void HandleGetProfiles();
        void HandleSaveProfiles(std::wstring_view messageJson);
        void HandleExportProfilesToFile(std::wstring_view messageJson);
        void HandleApplyProfile(std::wstring_view messageJson);
        void HandleExecuteInstaller(std::wstring_view messageJson);
        void HandleDownloadUpdate(std::wstring_view messageJson);
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
        AutoApplyEngine* m_autoApplyEngine = nullptr;
        WebMessageBridge m_bridge;
    };
}
