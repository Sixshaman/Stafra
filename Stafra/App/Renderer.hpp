#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>

class Renderer
{
public:
	Renderer(HWND previewWnd, HWND clickRuleWnd);
	~Renderer();

	void ResizePreviewArea(uint32_t newWidth, uint32_t newHeight);

	bool CanDrawBoard() const;

	void NeedRedraw();
	bool GetNeedRedraw() const;

	void NeedRedrawClickRule();
	bool GetNeedRedrawClickRule() const;

	void DrawPreview();   //Only to be called from the background thread
	void DrawClickRule(); //Only to be called from the background thread

	void SetCurrentBoard(ID3D11ShaderResourceView* srv);
	void SetCurrentClickRule(ID3D11ShaderResourceView* srv);

	ID3D11Device*        GetDevice()        const;
	ID3D11DeviceContext* GetDeviceContext() const;

private:
	void CreateDeviceAndSwapChains(HWND previewWnd, HWND clickRuleWnd);
	void LoadShaders();

	void CreateSwapChain(IDXGIFactory* factory, HWND hwnd, uint32_t width, uint32_t height, IDXGISwapChain** outSwapChain);
	void InitRenderAreaSize(IDXGISwapChain* swapChain, uint32_t width, uint32_t height, ID3D11RenderTargetView** outRTV, D3D11_VIEWPORT* outViewport);

private:
	Microsoft::WRL::ComPtr<ID3D11Device>        mDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext;

	Microsoft::WRL::ComPtr<IDXGISwapChain> mPreviewSwapChain;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mClickRuleSwapChain;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mPreviewRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mClickRuleRTV;

	D3D11_VIEWPORT mPreviewViewport;
	D3D11_VIEWPORT mClickRuleViewport;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> mBoardSampler;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> mRenderVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  mRenderPixelShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  mRenderClickRulePixelShader;

	ID3D11ShaderResourceView* mCurrentBoardSRV;     //Non-owning observer pointer
	ID3D11ShaderResourceView* mCurrentClickRuleSRV; //Non-owning observer pointer

	bool mNeedRedraw;
	bool mNeedRedrawClickRule;
};