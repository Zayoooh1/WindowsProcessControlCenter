#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace wpcc
{
    class D3D11Renderer
    {
    public:
        bool Initialize(HWND hwnd);
        void Shutdown();

        void BeginFrame(float r, float g, float b, float a);
        void EndFrame();
        void Resize(UINT width, UINT height);

        bool IsInitialized() const { return m_initialized; }
        ID3D11Device* GetDevice() const { return m_device.Get(); }
        ID3D11DeviceContext* GetContext() const { return m_context.Get(); }

    private:
        bool CreateRenderTarget();
        void CleanupRenderTarget();

        Microsoft::WRL::ComPtr<ID3D11Device> m_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
        Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        bool m_initialized = false;
    };
}
