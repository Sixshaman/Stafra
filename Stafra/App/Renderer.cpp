#include "Renderer.hpp"
#include "..\Util.hpp"

Renderer::Renderer(): mNeedRedraw(false), mNeedRedrawClickRule(false)
{
	CreateDevice();
}

Renderer::~Renderer()
{
}

void Renderer::ResizePreviewArea(uint32_t newWidth, uint32_t newHeight)
{
}

void Renderer::NeedRedraw()
{
	mNeedRedraw = true;
}

bool Renderer::GetNeedRedraw() const
{
	return mNeedRedraw;
}

bool Renderer::ConsumeNeedRedraw()
{
	if(mNeedRedraw)
	{
		mNeedRedraw = false;
		return true;
	}
	else
	{
		return false;
	}
}

void Renderer::NeedRedrawClickRule()
{
	mNeedRedrawClickRule = true;
}

bool Renderer::GetNeedRedrawClickRule() const
{
	return mNeedRedrawClickRule;
}

bool Renderer::ConsumeNeedRedrawClickRule()
{
	if(mNeedRedrawClickRule)
	{
		mNeedRedrawClickRule = false;
		return true;
	}
	else
	{
		return false;
	}
}

bool Renderer::GetClickRuleGridVisible() const
{
	return false;
}

void Renderer::SetClickRuleGridVisible(bool bVisible)
{
}

void Renderer::DrawPreview()
{
}

void Renderer::DrawClickRule()
{
}

ID3D11Device* Renderer::GetDevice() const
{
	return mDevice.Get();
}

ID3D11DeviceContext* Renderer::GetDeviceContext() const
{
	return mDeviceContext.Get();
}

void Renderer::SetCurrentBoard(ID3D11ShaderResourceView* srv)
{
}

void Renderer::SetCurrentClickRule(ID3D11ShaderResourceView* srv)
{
}

void Renderer::CreateDevice()
{
	UINT flags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(mDXGIFactory.GetAddressOf())));

	Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter = nullptr;
	ThrowIfFailed(mDXGIFactory->EnumAdapters1(0, pAdapter.GetAddressOf()));

	ThrowIfFailed(D3D11CreateDevice(pAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, mDevice.GetAddressOf(), nullptr, mDeviceContext.GetAddressOf()));

	ThrowIfFailed(mDevice->SetExceptionMode(D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR));
}