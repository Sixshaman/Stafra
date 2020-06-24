#include "DisplayRenderer.hpp"
#include "..\Util.hpp"

#define FLAG_CLICK_RULE_GRID_VISIBLE 0x01

DisplayRenderer::DisplayRenderer(int gpuIndex, HWND previewWnd, HWND clickRuleWnd): Renderer(gpuIndex), mCurrentBoardSRV(nullptr), mCurrentClickRuleSRV(nullptr)
{
	mCBufferParamsClickRuleCopy.Flags = FLAG_CLICK_RULE_GRID_VISIBLE;

	CreateSwapChains(previewWnd, clickRuleWnd);
	LoadShaders();
}

DisplayRenderer::~DisplayRenderer()
{
}

void DisplayRenderer::ResizePreviewArea(uint32_t newWidth, uint32_t newHeight)
{
	mPreviewRTV.Reset();
	InitRenderAreaSize(mPreviewSwapChain.Get(), newWidth, newHeight, mPreviewRTV.GetAddressOf(), &mPreviewViewport);
	NeedRedraw();
}

bool DisplayRenderer::GetClickRuleGridVisible() const
{
	return mCBufferParamsClickRuleCopy.Flags & FLAG_CLICK_RULE_GRID_VISIBLE;
}

void DisplayRenderer::SetClickRuleGridVisible(bool bVisible)
{
	if(bVisible)
	{
		mCBufferParamsClickRuleCopy.Flags |= FLAG_CLICK_RULE_GRID_VISIBLE;
	}
	else
	{
		mCBufferParamsClickRuleCopy.Flags &= ~(FLAG_CLICK_RULE_GRID_VISIBLE);
	}

	NeedRedrawClickRule();
}

void DisplayRenderer::SetCurrentBoard(ID3D11ShaderResourceView* srv)
{
	mCurrentBoardSRV = srv;
}

void DisplayRenderer::SetCurrentClickRule(ID3D11ShaderResourceView* srv)
{
	mCurrentClickRuleSRV = srv;
}

void DisplayRenderer::DrawPreview()
{
	ID3D11RenderTargetView* rtvs[] = { mPreviewRTV.Get() };
	mDeviceContext->OMSetRenderTargets(1, rtvs, nullptr);

	D3D11_VIEWPORT viewports[] = { mPreviewViewport };
	mDeviceContext->RSSetViewports(1, viewports);

	FLOAT clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	mDeviceContext->ClearRenderTargetView(mPreviewRTV.Get(), clearColor);

	ID3D11Buffer* vertexBuffers[] = { nullptr };
	UINT          strides[] = { 0 };
	UINT          offsets[] = { 0 };

	mDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	mDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

	mDeviceContext->IASetInputLayout(nullptr);
	mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	mDeviceContext->VSSetShader(mRenderVertexShader.Get(), nullptr, 0);
	mDeviceContext->PSSetShader(mRenderPixelShader.Get(), nullptr, 0);

	ID3D11ShaderResourceView* boardSRVs[] = { mCurrentBoardSRV };
	mDeviceContext->PSSetShaderResources(0, 1, boardSRVs);

	ID3D11SamplerState* samplers[] = { mBoardSampler.Get() };
	mDeviceContext->PSSetSamplers(0, 1, samplers);

	mDeviceContext->Draw(4, 0);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr };
	mDeviceContext->PSSetShaderResources(0, 1, nullSRVs);

	mPreviewSwapChain->Present(0, 0);

	mNeedRedraw = false;
}

void DisplayRenderer::DrawClickRule()
{
	Utils::UpdateBuffer(mCBufferParamsClickRule.Get(), mCBufferParamsClickRuleCopy, mDeviceContext.Get());

	ID3D11RenderTargetView* rtvs[] = { mClickRuleRTV.Get() };
	mDeviceContext->OMSetRenderTargets(1, rtvs, nullptr);

	D3D11_VIEWPORT viewports[] = { mClickRuleViewport };
	mDeviceContext->RSSetViewports(1, viewports);

	FLOAT clearColor[] = { 1.0f, 0.7f, 0.7f, 1.0f };
	mDeviceContext->ClearRenderTargetView(mClickRuleRTV.Get(), clearColor);

	ID3D11Buffer* vertexBuffers[] = {nullptr};
	UINT          strides[]       = {0};
	UINT          offsets[]       = {0};

	mDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	mDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

	mDeviceContext->IASetInputLayout(nullptr);
	mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	mDeviceContext->VSSetShader(mRenderVertexShader.Get(),         nullptr, 0);
	mDeviceContext->PSSetShader(mRenderClickRulePixelShader.Get(), nullptr, 0);

	ID3D11Buffer* cbuffers[] = { mCBufferParamsClickRule.Get() };
	mDeviceContext->PSSetConstantBuffers(0, 1, cbuffers);

	ID3D11ShaderResourceView* boardSRVs[] = {mCurrentClickRuleSRV};
	mDeviceContext->PSSetShaderResources(0, 1, boardSRVs);

	mDeviceContext->Draw(4, 0);

	ID3D11Buffer* nullCbuffers[] = { nullptr };
	mDeviceContext->PSSetConstantBuffers(0, 1, nullCbuffers);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr };
	mDeviceContext->PSSetShaderResources(0, 1, nullSRVs);

	mClickRuleSwapChain->Present(0, 0);

	mNeedRedrawClickRule = false;
}

void DisplayRenderer::CreateSwapChains(HWND previewWnd, HWND clickRuleWnd)
{
	ThrowIfFailed(mDXGIFactory->MakeWindowAssociation(previewWnd,   DXGI_MWA_NO_WINDOW_CHANGES));
	ThrowIfFailed(mDXGIFactory->MakeWindowAssociation(clickRuleWnd, DXGI_MWA_NO_WINDOW_CHANGES));

	RECT previewWndowRect;
	GetClientRect(previewWnd, &previewWndowRect);

	RECT clickRukewWndowRect;
	GetClientRect(clickRuleWnd, &clickRukewWndowRect);

	uint32_t previewWidth  = previewWndowRect.right - previewWndowRect.left;
	uint32_t previewHeight = previewWndowRect.bottom - previewWndowRect.top;

	uint32_t clickRuleWidth  = clickRukewWndowRect.right - clickRukewWndowRect.left;
	uint32_t clickRuleHeight = clickRukewWndowRect.bottom - clickRukewWndowRect.top;

	CreateSwapChain(mDXGIFactory.Get(), previewWnd, previewWidth, previewHeight, mPreviewSwapChain.GetAddressOf());
	InitRenderAreaSize(mPreviewSwapChain.Get(), previewWidth, previewHeight, mPreviewRTV.GetAddressOf(), &mPreviewViewport);

	CreateSwapChain(mDXGIFactory.Get(), clickRuleWnd, clickRuleWidth, clickRuleHeight, mClickRuleSwapChain.GetAddressOf());
	InitRenderAreaSize(mClickRuleSwapChain.Get(), clickRuleWidth, clickRuleHeight, mClickRuleRTV.GetAddressOf(), &mClickRuleViewport);

	NeedRedraw();
	NeedRedrawClickRule();
}

void DisplayRenderer::LoadShaders()
{
	const std::wstring shaderDir = Utils::GetShaderPath() + L"Render\\";
	ThrowIfFailed(Utils::LoadShaderFromFile(mDevice.Get(), shaderDir + L"RenderVS.cso",          mRenderVertexShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(mDevice.Get(), shaderDir + L"RenderPS.cso",          mRenderPixelShader.GetAddressOf()));
	ThrowIfFailed(Utils::LoadShaderFromFile(mDevice.Get(), shaderDir + L"RenderClickRulePS.cso", mRenderClickRulePixelShader.GetAddressOf()));

	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(D3D11_SAMPLER_DESC));
	samDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.Filter   = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;

	ThrowIfFailed(mDevice->CreateSamplerState(&samDesc, mBoardSampler.GetAddressOf()));

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.Usage               = D3D11_USAGE_DYNAMIC;
	cbDesc.ByteWidth           = (sizeof(CBParamsClickRuleStruct) + 0xff) & (~0xff);
	cbDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags           = 0;
	cbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA cbData;
	cbData.pSysMem          = &mCBufferParamsClickRuleCopy;
	cbData.SysMemPitch      = 0;
	cbData.SysMemSlicePitch = 0;

	ThrowIfFailed(mDevice->CreateBuffer(&cbDesc, &cbData, &mCBufferParamsClickRule));	
}

void DisplayRenderer::CreateSwapChain(IDXGIFactory* factory, HWND hwnd, uint32_t width, uint32_t height, IDXGISwapChain** outSwapChain)
{
	DXGI_SWAP_CHAIN_DESC scDesc;
	scDesc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.BufferDesc.Width                   = width;
	scDesc.BufferDesc.Height                  = height;
	scDesc.BufferDesc.RefreshRate.Numerator   = 60;
	scDesc.BufferDesc.RefreshRate.Denominator = 1;
	scDesc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
	scDesc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
	scDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount                        = 2;
	scDesc.OutputWindow                       = hwnd;
	scDesc.SampleDesc.Count                   = 1;
	scDesc.SampleDesc.Quality                 = 0;
	scDesc.Windowed                           = TRUE;
	scDesc.SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Flags                              = 0;

	ThrowIfFailed(factory->CreateSwapChain(mDevice.Get(), &scDesc, outSwapChain));
}

void DisplayRenderer::InitRenderAreaSize(IDXGISwapChain* swapChain, uint32_t width, uint32_t height, ID3D11RenderTargetView** outRTV, D3D11_VIEWPORT* outViewport)
{
	ThrowIfFailed(swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	ThrowIfFailed(swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));
	ThrowIfFailed(mDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, outRTV));

	outViewport->TopLeftX = 0.0f;
	outViewport->TopLeftY = 0.0f;
	outViewport->Width    = (float)width;
	outViewport->Height   = (float)height;
	outViewport->MinDepth = 0.0f;
	outViewport->MaxDepth = 1.0f;
}