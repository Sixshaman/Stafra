#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>

class Renderer
{
public:
	Renderer(int gpuIndex = 0);
	virtual ~Renderer();

	virtual void ResizePreviewArea(uint32_t newWidth, uint32_t newHeight);

	void NeedRedraw();
	bool GetNeedRedraw() const;
	bool ConsumeNeedRedraw();

	void NeedRedrawClickRule();
	bool GetNeedRedrawClickRule() const;
	bool ConsumeNeedRedrawClickRule();

	virtual bool GetClickRuleGridVisible() const;
	virtual void SetClickRuleGridVisible(bool bVisible);

	virtual void DrawPreview();
	virtual void DrawClickRule();

	ID3D11Device*        GetDevice()        const;
	ID3D11DeviceContext* GetDeviceContext() const;

	std::wstring GetAdapterName() const;

	virtual void SetCurrentBoard(ID3D11ShaderResourceView* srv);
	virtual void SetCurrentClickRule(ID3D11ShaderResourceView* srv);

private:
	void CreateDevice(int gpuIndex);

protected:
	Microsoft::WRL::ComPtr<IDXGIFactory1> mDXGIFactory;

	Microsoft::WRL::ComPtr<ID3D11Device>        mDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext;

	std::wstring mAdapterName;

	bool mNeedRedraw;
	bool mNeedRedrawClickRule;
};