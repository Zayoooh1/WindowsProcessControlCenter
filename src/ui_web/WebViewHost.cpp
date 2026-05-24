#include "ui_web/WebViewHost.h"

#include <wrl/event.h>

#include <ShlObj.h>
#include <commdlg.h>
#include <shobjidl.h>

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
}

namespace wpcc
{
    bool WebViewHost::Initialize(HWND hwnd)
    {
        m_hwnd = hwnd;

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
                    case WebMessageType::LoadProfiles:
                        HandleLoadProfiles();
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
            const std::wstring message = m_bridge.BuildProcessSnapshotMessage(processes);
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

    void WebViewHost::HandleLoadProfiles()
    {
        if (!m_webView)
        {
            return;
        }

        const ProfileLoadResult result = ProfileStore::LoadProfiles();
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

        const ProfileLoadResult loadResult = ProfileStore::LoadProfiles();
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
