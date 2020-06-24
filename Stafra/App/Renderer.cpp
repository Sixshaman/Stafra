#include "Renderer.hpp"
#include "..\Util.hpp"
#include <vector>

Renderer::Renderer(int gpuIndex): mNeedRedraw(false), mNeedRedrawClickRule(false)
{
	CreateDevice(gpuIndex);
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
	mDeviceContext->Flush(); //Just to not make the console version stall
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

std::wstring Renderer::GetAdapterName() const
{
	return mAdapterName;
}

void Renderer::SetCurrentBoard(ID3D11ShaderResourceView* srv)
{
}

void Renderer::SetCurrentClickRule(ID3D11ShaderResourceView* srv)
{
}

void Renderer::CreateDevice(int gpuIndex)
{
	UINT flags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(mDXGIFactory.GetAddressOf())));

	if(gpuIndex < 0) //WARP
	{
		ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, mDevice.GetAddressOf(), nullptr, mDeviceContext.GetAddressOf()));

		mAdapterName = L"WARP";
	}
	else
	{
		std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> adapters;

		Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter = nullptr;
		for(int index = 0; mDXGIFactory->EnumAdapters1(index, pAdapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; index++)
		{
			adapters.push_back(pAdapter);
			pAdapter.Reset();
		}

		//Clamp the value
		if(gpuIndex >= adapters.size())
		{
			gpuIndex = adapters.size() - 1;
		}

		Microsoft::WRL::ComPtr<IDXGIAdapter1> pSelectedAdapter = adapters[gpuIndex];
		ThrowIfFailed(D3D11CreateDevice(pSelectedAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, mDevice.GetAddressOf(), nullptr, mDeviceContext.GetAddressOf()));

		DXGI_ADAPTER_DESC1 selectedAdapterDesc;
		ThrowIfFailed(pSelectedAdapter->GetDesc1(&selectedAdapterDesc));

		mAdapterName = selectedAdapterDesc.Description;
	}

	ThrowIfFailed(mDevice->SetExceptionMode(D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR));
}