#include "ui_web/WebViewHost.h"

#include <wrl/event.h>

#include <ShlObj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <thread>
#include <urlmon.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <vector>

using Microsoft::WRL::Callback;

namespace
{
    std::wstring Quote(std::wstring_view value)
    {
        return L"\"" + std::wstring(value) + L"\"";
    }

    void AppLog(const std::wstring& message)
    {
        std::wcout << L"[WPCC LOG] " << message << std::endl;
        OutputDebugStringW((L"[WPCC LOG] " + message + L"\n").c_str());
    }

    class DownloadProgressCallback : public IBindStatusCallback {
    public:
        DownloadProgressCallback(HWND hwnd) : m_hwnd(hwnd), m_refCount(1) {}

        // IUnknown
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
            if (riid == IID_IUnknown || riid == IID_IBindStatusCallback) {
                *ppvObject = this;
                AddRef();
                return S_OK;
            }
            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
        ULONG STDMETHODCALLTYPE Release() override {
            ULONG ref = InterlockedDecrement(&m_refCount);
            if (ref == 0) delete this;
            return ref;
        }

        // IBindStatusCallback
        HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD, IBinding*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetPriority(LONG*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE OnLowResource(DWORD) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG, LPCWSTR) override {
            wpcc::WebViewHost::ProgressData* data = new wpcc::WebViewHost::ProgressData{ulProgress, ulProgressMax};
            PostMessageW(m_hwnd, wpcc::WebViewHost::WM_DOWNLOAD_PROGRESS, 0, reinterpret_cast<LPARAM>(data));
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT, LPCWSTR) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD, DWORD, FORMATETC*, STGMEDIUM*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID, IUnknown*) override { return E_NOTIMPL; }

    private:
        HWND m_hwnd;
        ULONG m_refCount;
    };
}

namespace wpcc
{
    bool WebViewHost::Initialize(HWND hwnd, AutoApplyEngine* autoApplyEngine)
    {
        m_hwnd = hwnd;
        m_autoApplyEngine = autoApplyEngine;

        const std::filesystem::path userDataFolder = GetExecutableDirectory() / L"WebView2UserData";
        const HRESULT result = CreateCoreWebView2EnvironmentWithOptions(
            nullptr,
            userDataFolder.c_str(),
            nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](HRESULT environmentResult, ICoreWebView2Environment* environment) -> HRESULT {
                    OnEnvironmentCreated(environmentResult, environment);
                    return S_OK;
                })
                .Get());

        if (FAILED(result))
        {
            ShowInitializationError(L"Failed to start WebView2 environment: " + HResultToMessage(result));
            return false;
        }

        return true;
    }

    void WebViewHost::Resize()
    {
        if (!m_controller || m_hwnd == nullptr)
        {
            return;
        }

        RECT bounds{};
        GetClientRect(m_hwnd, &bounds);
        m_controller->put_Bounds(bounds);
    }

    void WebViewHost::Shutdown()
    {
        m_processActions.ResumeAllFrozenProcesses();

        if (m_webView && m_messageHandlerRegistered)
        {
            m_webView->remove_WebMessageReceived(m_messageToken);
            m_messageHandlerRegistered = false;
        }

        if (m_webView && m_navigationHandlerRegistered)
        {
            m_webView->remove_NavigationStarting(m_navigationToken);
            m_navigationHandlerRegistered = false;
        }

        if (m_controller)
        {
            m_controller->Close();
        }

        m_webView.Reset();
        m_controller.Reset();
        m_environment.Reset();
    }

    void WebViewHost::OnEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment)
    {
        if (FAILED(result) || environment == nullptr)
        {
            ShowInitializationError(L"WebView2 Runtime is not available or failed to initialize: " + HResultToMessage(result));
            return;
        }

        m_environment = environment;
        m_environment->CreateCoreWebView2Controller(
            m_hwnd,
            Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [this](HRESULT controllerResult, ICoreWebView2Controller* controller) -> HRESULT {
                    OnControllerCreated(controllerResult, controller);
                    return S_OK;
                })
                .Get());
    }

    void WebViewHost::OnControllerCreated(HRESULT result, ICoreWebView2Controller* controller)
    {
        if (FAILED(result) || controller == nullptr)
        {
            ShowInitializationError(L"Failed to create WebView2 controller: " + HResultToMessage(result));
            return;
        }

        m_controller = controller;
        m_controller->get_CoreWebView2(&m_webView);
        if (!m_webView)
        {
            ShowInitializationError(L"WebView2 controller did not provide a CoreWebView2 instance.");
            return;
        }

        ConfigureWebView();
        Resize();
        NavigateToFrontend();
    }

    void WebViewHost::ConfigureWebView()
    {
        Microsoft::WRL::ComPtr<ICoreWebView2Settings> settings;
        if (SUCCEEDED(m_webView->get_Settings(&settings)) && settings)
        {
            settings->put_AreDefaultContextMenusEnabled(FALSE);
            settings->put_IsStatusBarEnabled(FALSE);
#ifdef NDEBUG
            settings->put_AreDevToolsEnabled(FALSE);
#endif
        }

        m_webView->add_WebMessageReceived(
            Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                    LPWSTR message = nullptr;
                    if (FAILED(args->get_WebMessageAsJson(&message)) || message == nullptr)
                    {
                        SendError("Failed to read frontend message.");
                        return S_OK;
                    }

                    const std::wstring messageJson(message);
                    const WebMessageType messageType = m_bridge.ParseMessageType(messageJson);
                    CoTaskMemFree(message);

                    switch (messageType)
                    {
                    case WebMessageType::RefreshProcesses:
                        SendProcessSnapshot();
                        break;
                    case WebMessageType::SetCpuPriority:
                        HandleSetCpuPriority(messageJson);
                        break;
                    case WebMessageType::TerminateProcess:
                        HandleTerminateProcess(messageJson);
                        break;
                    case WebMessageType::FreezeProcess:
                        HandleFreezeProcess(messageJson);
                        break;
                    case WebMessageType::ResumeProcess:
                        HandleResumeProcess(messageJson);
                        break;
                    case WebMessageType::SetGpuPreference:
                        HandleSetGpuPreference(messageJson);
                        break;
                    case WebMessageType::GetProfiles:
                        HandleGetProfiles();
                        break;
                    case WebMessageType::SaveProfiles:
                        HandleSaveProfiles(messageJson);
                        break;
                    case WebMessageType::ExportProfilesToFile:
                        HandleExportProfilesToFile(messageJson);
                        break;
                    case WebMessageType::ChooseExecutable:
                        AppLog(L"Browse clicked");
                        PostMessageW(m_hwnd, ChooseExecutableWindowMessage, 0, 0);
                        break;
                    case WebMessageType::ApplyProfile:
                        HandleApplyProfile(messageJson);
                        break;
                    case WebMessageType::ExecuteInstaller:
                        HandleExecuteInstaller(messageJson);
                        break;
                    case WebMessageType::DownloadUpdate:
                        HandleDownloadUpdate(messageJson);
                        break;
                    default:
                        SendError("Unsupported frontend message.");
                        break;
                    }

                    return S_OK;
                })
                .Get(),
            &m_messageToken);
        m_messageHandlerRegistered = true;

        m_webView->add_NavigationStarting(
            Callback<ICoreWebView2NavigationStartingEventHandler>(
                [this](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                    LPWSTR uri = nullptr;
                    if (SUCCEEDED(args->get_Uri(&uri)) && uri != nullptr)
                    {
                        const bool allowed = IsAllowedNavigationUri(uri);
                        CoTaskMemFree(uri);
                        if (!allowed)
                        {
                            args->put_Cancel(TRUE);
                        }
                    }
                    return S_OK;
                })
                .Get(),
            &m_navigationToken);
        m_navigationHandlerRegistered = true;
    }

    void WebViewHost::NavigateToFrontend()
    {
        const std::filesystem::path indexPath = GetFrontendIndexPath();
        if (!std::filesystem::exists(indexPath))
        {
            ShowInitializationError(L"Frontend file was not found: " + indexPath.wstring());
            return;
        }

        const std::wstring uri = PathToFileUri(indexPath);
        const HRESULT result = m_webView->Navigate(uri.c_str());
        if (FAILED(result))
        {
            ShowInitializationError(L"Failed to load frontend from " + uri + L": " + HResultToMessage(result));
        }
    }

    void WebViewHost::RefreshProcesses()
    {
        SendProcessSnapshot();
    }

    void WebViewHost::SendProcessSnapshot()
    {
        try
        {
            std::vector<ProcessInfo> processes = m_processProvider.LoadProcesses();
            for (ProcessInfo& process : processes)
            {
                process.isFrozenByApp = m_processActions.IsFrozenByApp(process.pid);
                process.gpuPreference = m_gpuPreferenceManager.GetPreferenceForExecutablePath(process.executablePath);
            }
            std::vector<AutoApplyLog> logs;
            if (m_autoApplyEngine)
            {
                logs = m_autoApplyEngine->GetLogs();
            }
            const std::wstring message = m_bridge.BuildProcessSnapshotMessage(processes, logs);
            m_webView->PostWebMessageAsJson(message.c_str());
        }
        catch (...)
        {
            SendError("Failed to refresh process list.");
        }
    }

    void WebViewHost::HandleSetCpuPriority(std::wstring_view messageJson)
    {
        const SetCpuPriorityRequest request = m_bridge.ParseSetCpuPriorityRequest(messageJson);
        ProcessActionResult result = m_processActions.SetCpuPriority(request.pid, request.priority, request.confirmRealtime);

        if (m_webView)
        {
            const std::wstring actionResult = m_bridge.BuildActionResultMessage("setCpuPriority", result);
            m_webView->PostWebMessageAsJson(actionResult.c_str());
        }

        if (result.success)
        {
            SendProcessSnapshot();
        }
    }

    void WebViewHost::HandleTerminateProcess(std::wstring_view messageJson)
    {
        const TerminateProcessRequest request = m_bridge.ParseTerminateProcessRequest(messageJson);
        ProcessActionResult result = m_processActions.TerminateProcessByPid(request.pid, request.expectedName, request.confirmation);

        if (m_webView)
        {
            const std::wstring actionResult = m_bridge.BuildActionResultMessage("terminateProcess", result);
            m_webView->PostWebMessageAsJson(actionResult.c_str());
        }

        if (result.success)
        {
            SendProcessSnapshot();
        }
    }

    void WebViewHost::HandleFreezeProcess(std::wstring_view messageJson)
    {
        const FreezeProcessRequest request = m_bridge.ParseFreezeProcessRequest(messageJson);
        ProcessActionResult result = m_processActions.FreezeProcessByPid(request.pid, request.expectedName, request.confirmation);

        if (m_webView)
        {
            const std::wstring actionResult = m_bridge.BuildActionResultMessage("freezeProcess", result);
            m_webView->PostWebMessageAsJson(actionResult.c_str());
        }

        if (result.success)
        {
            SendProcessSnapshot();
        }
    }

    void WebViewHost::HandleResumeProcess(std::wstring_view messageJson)
    {
        const ResumeProcessRequest request = m_bridge.ParseResumeProcessRequest(messageJson);
        ProcessActionResult result = m_processActions.ResumeProcessByPid(request.pid, request.expectedName);

        if (m_webView)
        {
            const std::wstring actionResult = m_bridge.BuildActionResultMessage("resumeProcess", result);
            m_webView->PostWebMessageAsJson(actionResult.c_str());
        }

        if (result.success)
        {
            SendProcessSnapshot();
        }
    }

    void WebViewHost::HandleSetGpuPreference(std::wstring_view messageJson)
    {
        const SetGpuPreferenceRequest request = m_bridge.ParseSetGpuPreferenceRequest(messageJson);
        ProcessActionResult result = m_gpuPreferenceManager.SetPreferenceForProcess(request.pid, request.expectedName, request.executablePath, request.preference);

        if (m_webView)
        {
            const std::wstring actionResult = m_bridge.BuildActionResultMessage("setGpuPreference", result);
            m_webView->PostWebMessageAsJson(actionResult.c_str());
        }

        if (result.success)
        {
            SendProcessSnapshot();
        }
    }

    void WebViewHost::HandleGetProfiles()
    {
        if (!m_webView)
        {
            return;
        }

        const ProfileLoadResult result = ProfileStore::GetProfiles();
        const std::wstring response = m_bridge.BuildProfilesLoadedMessage(result.success, result.jsonContent, result.warning);
        m_webView->PostWebMessageAsJson(response.c_str());
    }

    void WebViewHost::HandleSaveProfiles(std::wstring_view messageJson)
    {
        if (!m_webView)
        {
            return;
        }

        const std::string profilesJson = m_bridge.ParseSaveProfilesRequest(messageJson);
        if (profilesJson.empty())
        {
            const std::wstring response = m_bridge.BuildProfilesSavedMessage(false, L"No profiles data received.");
            m_webView->PostWebMessageAsJson(response.c_str());
            return;
        }

        const ProfileSaveResult result = ProfileStore::SaveProfiles(profilesJson);
        const std::wstring response = m_bridge.BuildProfilesSavedMessage(result.success, result.warning);
        m_webView->PostWebMessageAsJson(response.c_str());
    }

    void WebViewHost::HandleExportProfilesToFile(std::wstring_view messageJson)
    {
        if (!m_webView)
        {
            return;
        }

        const std::string profilesJson = m_bridge.ParseSaveProfilesRequest(messageJson);

        bool cancelled = false;
        bool success = false;
        std::wstring warning;

        wchar_t filePath[MAX_PATH] = L"wpcc-profiles.json";
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hwnd;
        ofn.lpstrFile = filePath;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
        ofn.lpstrDefExt = L"json";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetSaveFileNameW(&ofn))
        {
            std::ofstream file(ofn.lpstrFile);
            if (file.is_open())
            {
                file << profilesJson;
                file.close();
                success = true;
            }
            else
            {
                warning = L"Could not write to the selected file.";
            }
        }
        else
        {
            cancelled = true;
        }

        const std::wstring response = m_bridge.BuildProfilesExportedMessage(success, cancelled, warning);
        m_webView->PostWebMessageAsJson(response.c_str());
    }

    void WebViewHost::HandleApplyProfile(std::wstring_view messageJson)
    {
        if (!m_webView)
        {
            return;
        }

        const std::string profileId = m_bridge.ParseApplyProfileRequest(messageJson);
        if (profileId.empty())
        {
            const std::wstring response = m_bridge.BuildProfileAppliedMessage("", false, 0, 0, 0, "No profile ID received.");
            m_webView->PostWebMessageAsJson(response.c_str());
            return;
        }

        const ProfileLoadResult loadResult = ProfileStore::GetProfiles();
        if (!loadResult.success)
        {
            const std::wstring response = m_bridge.BuildProfileAppliedMessage(profileId, false, 0, 0, 0, "Failed to load profiles storage.");
            m_webView->PostWebMessageAsJson(response.c_str());
            return;
        }

        const std::vector<Profile> profiles = ProfileStore::ParseProfilesJson(loadResult.jsonContent);
        const auto profileIt = std::find_if(profiles.begin(), profiles.end(), [&profileId](const Profile& p) {
            return p.id == profileId;
        });

        if (profileIt == profiles.end())
        {
            const std::wstring response = m_bridge.BuildProfileAppliedMessage(profileId, false, 0, 0, 0, "Profile not found.");
            m_webView->PostWebMessageAsJson(response.c_str());
            return;
        }

        const ApplyProfileResult result = m_processActions.ApplyProfile(*profileIt);

        const std::wstring response = m_bridge.BuildProfileAppliedMessage(
            profileId,
            result.success,
            result.matched,
            result.updated,
            result.failed,
            result.message
        );
        m_webView->PostWebMessageAsJson(response.c_str());

        SendProcessSnapshot();
    }

    void WebViewHost::HandleExecuteInstaller(std::wstring_view messageJson)
    {
        if (!m_webView)
        {
            return;
        }

        std::wstring filePath = m_bridge.ParseExecuteInstallerRequest(messageJson);
        if (filePath.empty())
        {
            SendError("Installer file path is empty.");
            return;
        }

        HINSTANCE result = ShellExecuteW(nullptr, L"open", filePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(result) <= 32)
        {
            SendError("Failed to launch the installer.");
            return;
        }

        PostQuitMessage(0);
    }

    void WebViewHost::HandleDownloadUpdate(std::wstring_view messageJson)
    {
        std::wstring url = m_bridge.ParseDownloadUpdateUrl(messageJson);
        if (url.empty())
        {
            SendError("Download URL is empty.");
            return;
        }

        HWND hwnd = m_hwnd;
        std::thread downloadThread([url, hwnd]() {
            wchar_t tempDir[MAX_PATH];
            if (GetTempPathW(MAX_PATH, tempDir) > 0)
            {
                std::filesystem::path targetPath = std::filesystem::path(tempDir) / L"WPCC_Update_Setup.exe";
                
                std::error_code ec;
                std::filesystem::create_directories(targetPath.parent_path(), ec);
                if (ec)
                {
                    std::string errMsg = ec.message();
                    std::wstring wmsg(errMsg.begin(), errMsg.end());
                    AppLog(L"Failed to create directories: " + wmsg);
                    OutputDebugStringW((L"[WPCC ERROR] Failed to create directory: " + wmsg + L"\n").c_str());
                }

                std::filesystem::remove(targetPath, ec);

                AppLog(L"Starting URLDownloadToFileW from URL: " + url + L" to path: " + targetPath.wstring());
                OutputDebugStringW((L"[WPCC LOG] Starting URLDownloadToFileW to " + targetPath.wstring() + L"\n").c_str());

                DownloadProgressCallback* cb = new DownloadProgressCallback(hwnd);
                HRESULT hr = URLDownloadToFileW(nullptr, url.c_str(), targetPath.c_str(), 0, cb);
                cb->Release();
                
                AppLog(L"URLDownloadToFileW completed with HRESULT: " + std::to_wstring(hr));
                OutputDebugStringW((L"[WPCC LOG] URLDownloadToFileW completed with HRESULT: " + std::to_wstring(hr) + L"\n").c_str());

                if (SUCCEEDED(hr))
                {
                    PostMessageW(hwnd, WebViewHost::WM_DOWNLOAD_COMPLETE, 1, reinterpret_cast<LPARAM>(new std::wstring(targetPath.wstring())));
                }
                else
                {
                    std::wostringstream wss;
                    wss << L"HRESULT 0x" << std::hex << std::uppercase << hr;
                    PostMessageW(hwnd, WebViewHost::WM_DOWNLOAD_COMPLETE, 0, reinterpret_cast<LPARAM>(new std::wstring(wss.str())));
                }
            }
            else
            {
                AppLog(L"GetTempPathW failed.");
                OutputDebugStringW(L"[WPCC ERROR] GetTempPathW failed.\n");
                PostMessageW(hwnd, WebViewHost::WM_DOWNLOAD_COMPLETE, 0, reinterpret_cast<LPARAM>(new std::wstring(L"Failed to resolve temporary directory path.")));
            }
        });
        downloadThread.detach();
    }

    void WebViewHost::OnDownloadComplete(bool success, const std::wstring& filePathOrError)
    {
        if (!m_webView)
        {
            return;
        }

        if (success)
        {
            std::wstring jsonMessage = m_bridge.BuildDownloadCompleteMessage(true, filePathOrError, L"");
            m_webView->PostWebMessageAsJson(jsonMessage.c_str());
        }
        else
        {
            std::wstring errorMessage = L"Native Downloader Error: " + filePathOrError;
            std::wstring jsonMessage = m_bridge.BuildDownloadCompleteMessage(false, L"", errorMessage);
            m_webView->PostWebMessageAsJson(jsonMessage.c_str());
        }
    }

    void WebViewHost::NotifyDownloadProgress(uint32_t downloadedBytes, uint32_t totalBytes)
    {
        if (!m_webView)
        {
            return;
        }

        std::wstring jsonMessage = m_bridge.BuildDownloadProgressMessage(downloadedBytes, totalBytes);
        m_webView->PostWebMessageAsJson(jsonMessage.c_str());
    }

    void WebViewHost::ChooseExecutable()
    {
        if (!m_webView)
        {
            return;
        }

        AppLog(L"Opening executable picker");

        bool cancelled = false;
        bool success = false;
        std::wstring path;
        std::wstring fileName;
        const std::string iconDataUrl;

        // Bring the host window to the foreground so the file dialog
        // appears on top of the WebView2 control and is not hidden.
        if (m_hwnd)
        {
            SetForegroundWindow(m_hwnd);
        }

        Microsoft::WRL::ComPtr<IFileOpenDialog> fileDialog;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fileDialog));
        if (SUCCEEDED(hr))
        {
            COMDLG_FILTERSPEC fileTypes[] = {
                { L"Executable Files (*.exe)", L"*.exe" },
                { L"All Files (*.*)", L"*.*" }
            };
            fileDialog->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);
            fileDialog->SetDefaultExtension(L"exe");
            
            FILEOPENDIALOGOPTIONS options;
            if (SUCCEEDED(fileDialog->GetOptions(&options)))
            {
                options |= FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
                fileDialog->SetOptions(options);
            }

            hr = fileDialog->Show(m_hwnd);
            if (SUCCEEDED(hr))
            {
                Microsoft::WRL::ComPtr<IShellItem> item;
                hr = fileDialog->GetResult(&item);
                if (SUCCEEDED(hr))
                {
                    wchar_t* filePath = nullptr;
                    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
                    if (SUCCEEDED(hr))
                    {
                        path = filePath;
                        CoTaskMemFree(filePath);
                        success = true;
                        AppLog(L"Picker selected: " + path);
                    }
                    else
                    {
                        AppLog(L"Picker failed: GetDisplayName error " + HResultToMessage(hr));
                    }
                }
                else
                {
                    AppLog(L"Picker failed: GetResult error " + HResultToMessage(hr));
                }
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            {
                cancelled = true;
                AppLog(L"Picker cancelled");
            }
            else
            {
                cancelled = true;
                AppLog(L"Picker failed: " + HResultToMessage(hr));
            }
        }
        else
        {
            // Fallback to GetOpenFileNameW if ComPtr/IFileOpenDialog fails
            std::array<wchar_t, 32768> filePath{};
            OPENFILENAMEW ofn = {};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = m_hwnd;
            ofn.lpstrFile = filePath.data();
            ofn.nMaxFile = static_cast<DWORD>(filePath.size());
            ofn.lpstrFilter = L"Executable Files\0*.exe\0All Files\0*.*\0";
            ofn.lpstrDefExt = L"exe";
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

            if (GetOpenFileNameW(&ofn))
            {
                path = filePath.data();
                success = true;
                AppLog(L"Picker selected: " + path);
            }
            else
            {
                cancelled = true;
                const DWORD err = CommDlgExtendedError();
                if (err == 0)
                {
                    AppLog(L"Picker cancelled");
                }
                else
                {
                    AppLog(L"Picker failed: GetOpenFileName error " + std::to_wstring(err));
                }
            }
        }

        if (success && !path.empty())
        {
            const size_t lastSlash = path.find_last_of(L"\\/");
            fileName = (lastSlash != std::wstring::npos) ? path.substr(lastSlash + 1) : path;
        }

        const std::wstring response = m_bridge.BuildExecutableChosenMessage(success, cancelled, path, fileName, iconDataUrl);
        m_webView->PostWebMessageAsJson(response.c_str());
    }
    void WebViewHost::SendError(std::string_view message)
    {
        if (!m_webView)
        {
            return;
        }

        const std::wstring errorMessage = m_bridge.BuildErrorMessage(message);
        m_webView->PostWebMessageAsJson(errorMessage.c_str());
    }

    void WebViewHost::ShowInitializationError(std::wstring_view message) const
    {
        MessageBoxW(m_hwnd, std::wstring(message).c_str(), L"WebView2 initialization error", MB_ICONERROR | MB_OK);
    }

    std::filesystem::path WebViewHost::GetExecutableDirectory() const
    {
        std::array<wchar_t, MAX_PATH> buffer{};
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0)
        {
            return std::filesystem::current_path();
        }

        return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
    }

    std::filesystem::path WebViewHost::GetFrontendIndexPath() const
    {
        return GetExecutableDirectory() / L"web" / L"index.html";
    }

    std::wstring WebViewHost::PathToFileUri(const std::filesystem::path& path) const
    {
        std::wstring uri = std::filesystem::absolute(path).wstring();
        std::replace(uri.begin(), uri.end(), L'\\', L'/');
        if (uri.rfind(L"//", 0) == 0)
        {
            return L"file:" + uri;
        }

        return L"file:///" + uri;
    }

    std::wstring WebViewHost::HResultToMessage(HRESULT result) const
    {
        wchar_t* buffer = nullptr;
        const DWORD size = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            static_cast<DWORD>(result),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&buffer),
            0,
            nullptr);

        if (size == 0 || buffer == nullptr)
        {
            return L"HRESULT 0x" + std::to_wstring(static_cast<unsigned long>(result));
        }

        std::wstring message(buffer, size);
        LocalFree(buffer);
        while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n' || message.back() == L'.' || message.back() == L' '))
        {
            message.pop_back();
        }

        return message;
    }

    bool WebViewHost::IsAllowedNavigationUri(std::wstring_view uri) const
    {
        return uri.rfind(L"file:///", 0) == 0 || uri.rfind(L"about:blank", 0) == 0;
    }
}
