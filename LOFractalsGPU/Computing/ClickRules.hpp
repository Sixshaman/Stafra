#pragma once

#include <wrl/client.h>
#include <d3d11.h>
#include <string>

class ClickRules
{
	struct CBParamsStruct
	{
		uint32_t ClickX;
		uint32_t ClickY;
	};

public:
	ClickRules(ID3D11Device* device);
	~ClickRules();

	void GetDimensions(uint32_t& width, uint32_t& height);

	void InitDefault(ID3D11Device* device);
	void CreateFromTexture(ID3D11Device* device, ID3D11Texture2D* clickRuleTex);

	void EditCellState(ID3D11DeviceContext* dc, uint32_t x, uint32_t y);
	void Bake(ID3D11DeviceContext* dc); //Always call it if you're gonna use mClickRuleBufferSRV

	bool IsDefault() const;

	ID3D11ShaderResourceView* GetClickRuleImageSRV()         const;
	ID3D11ShaderResourceView* GetClickRuleBufferSRV()        const;
	ID3D11ShaderResourceView* GetClickRuleBufferCounterSRV() const;

private:
	void LoadShaders(ID3D11Device* device);

	void CreateClickRuleBuffer(ID3D11Device* device);

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mPrevClickRuleSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mNextClickRuleSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mPrevClickRuleUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mNextClickRuleUAV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  mClickRuleBufferSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mClickRuleBufferUAV;

	Microsoft::WRL::ComPtr<ID3D11Buffer>             mClickRuleCellCounterBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mClickRuleCellCounterBufferSRV;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mBakeClickRuleShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> mChangeCellStateShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer> mCBufferParams;
	CBParamsStruct                       mCBufferParamsCopy;

	uint32_t mClickRuleWidth;
	uint32_t mClickRuleHeight;

	bool mbIsDefault;
};