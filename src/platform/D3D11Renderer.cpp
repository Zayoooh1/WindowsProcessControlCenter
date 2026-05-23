#include "platform/D3D11Renderer.h"

#include <iterator>

namespace wpcc
{
    bool D3D11Renderer::Initialize(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc{};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hwnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        constexpr D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0,
        };

        D3D_FEATURE_LEVEL selectedFeatureLevel{};
        const HRESULT result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            featureLevels,
            static_cast<UINT>(std::size(featureLevels)),
            D3D11_SDK_VERSION,
            &swapChainDesc,
            m_swapChain.GetAddressOf(),
            m_device.GetAddressOf(),
            &selectedFeatureLevel,
            m_context.GetAddressOf());

        if (FAILED(result))
        {
            return false;
        }

        m_initialized = CreateRenderTarget();
        return m_initialized;
    }

    void D3D11Renderer::Shutdown()
    {
        CleanupRenderTarget();
        m_swapChain.Reset();
        m_context.Reset();
        m_device.Reset();
        m_initialized = false;
    }

    void D3D11Renderer::BeginFrame(float r, float g, float b, float a)
    {
        const float clearColor[] = {r, g, b, a};
        m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
        m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    }

    void D3D11Renderer::EndFrame()
    {
        m_swapChain->Present(1, 0);
    }

    void D3D11Renderer::Resize(UINT width, UINT height)
    {
        if (!m_initialized || width == 0 || height == 0)
        {
            return;
        }

        CleanupRenderTarget();
        m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget();
    }

    bool D3D11Renderer::CreateRenderTarget()
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        if (FAILED(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()))))
        {
            return false;
        }

        return SUCCEEDED(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf()));
    }

    void D3D11Renderer::CleanupRenderTarget()
    {
        m_renderTargetView.Reset();
    }
}
