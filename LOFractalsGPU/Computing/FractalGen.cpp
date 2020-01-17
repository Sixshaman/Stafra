#include "FractalGen.hpp"
#include "../Util.hpp"
#include <iostream>
#include <d3dcompiler.h>
#include <vector>
#include <fstream>
#include <sstream>
#include "EqualityChecker.hpp"
#include "StabilityCalculator.hpp"
#include "Downscaler.hpp"
#include "FinalTransform.hpp"
#include "ClickRules.hpp"
#include "InitialStates.hpp"
#include "BoardLoader.hpp"
#include "BoardSaver.hpp"
#include "../App/Renderer.hpp"

FractalGen::FractalGen(Renderer* renderer): mRenderer(renderer), mDefaultBoardWidth(1023), mDefaultBoardHeight(1023), mVideoFrameWidth(1), mVideoFrameHeight(1)
{
	ID3D11Device*    device = mRenderer->GetDevice();
	ID3D11DeviceContext* dc = mRenderer->GetDeviceContext();

	mStabilityCalculator = std::make_unique<StabilityCalculator>(device);

	mDownscaler       = std::make_unique<Downscaler>(device);
	mFinalTransformer = std::make_unique<FinalTransformer>(device);
	mEqualityChecker  = std::make_unique<EqualityChecker>(device);

	mClickRules    = std::make_unique<ClickRules>(device);
	mInitialStates = std::make_unique<InitialStates>(device);
	mBoardLoader   = std::make_unique<BoardLoader>(device);
	mBoardSaver    = std::make_unique<BoardSaver>();
}

FractalGen::~FractalGen()
{
}

void FractalGen::SetDefaultBoardWidth(uint32_t width)
{
	mDefaultBoardWidth = width;
}

void FractalGen::SetDefaultBoardHeight(uint32_t height)
{
	mDefaultBoardHeight = height;
}

void FractalGen::SetVideoFrameWidth(uint32_t width)
{
	mVideoFrameWidth = width;
}

void FractalGen::SetVideoFrameHeight(uint32_t height)
{
	mVideoFrameHeight = height;
}

uint32_t FractalGen::GetSolutionPeriod() const
{
	return mStabilityCalculator->GetDefaultSolutionPeriod();
}

void FractalGen::DoComputingPreparations(const std::wstring& initialBoardFile, const std::wstring& clickRuleFile, const std::wstring& restrictionFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> initialBoardTex;
	mBoardLoader->LoadBoardFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), initialBoardFile, initialBoardTex.GetAddressOf());

	if(!initialBoardTex)
	{
		mInitialStates->CreateBoard(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), mDefaultBoardWidth, mDefaultBoardHeight, BoardClearMode::FOUR_SIDES, initialBoardTex.GetAddressOf());
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleTex;
	mBoardLoader->LoadClickRuleFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), clickRuleFile, clickRuleTex.GetAddressOf());

	if(!clickRuleTex)
	{
		mClickRules->InitDefault(mRenderer->GetDevice());
	}
	else
	{
		mClickRules->CreateFromTexture(mRenderer->GetDevice(), clickRuleTex.Get());
	}

	mClickRules->Bake(mRenderer->GetDeviceContext());

	Microsoft::WRL::ComPtr<ID3D11Texture2D> restrictionTex;
	mBoardLoader->LoadBoardFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), restrictionFile, restrictionTex.GetAddressOf());

	mDownscaler->PrepareForDownscaling(mRenderer->GetDevice(), mVideoFrameWidth, mVideoFrameHeight);
	mFinalTransformer->PrepareForTransform(mRenderer->GetDevice(), mDefaultBoardWidth, mDefaultBoardHeight);
	mEqualityChecker->PrepareForCalculations(mRenderer->GetDevice(), mDefaultBoardWidth, mDefaultBoardHeight);

	mStabilityCalculator->PrepareForCalculations(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), initialBoardTex.Get(), restrictionTex.Get());

	mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
	mRenderer->NeedRedrawClickRule();
}

void FractalGen::Tick(uint32_t spawnPeriod, const std::wstring& videoFrameFile)
{
	bool saveFrame = !videoFrameFile.empty();
	bool drawFrame = mRenderer->CanDrawBoard();
	
	ID3D11ShaderResourceView* clickRuleBufferSRV  = nullptr;
	ID3D11ShaderResourceView* clickRuleCounterSRV = nullptr;
	if(!mClickRules->IsDefault())
	{
		clickRuleBufferSRV  = mClickRules->GetClickRuleBufferSRV();
		clickRuleCounterSRV = mClickRules->GetClickRuleBufferCounterSRV();
	}

	mStabilityCalculator->StabilityNextStep(mRenderer->GetDeviceContext(), clickRuleBufferSRV, clickRuleCounterSRV, spawnPeriod);
	if(saveFrame || drawFrame)
	{
		mFinalTransformer->ComputeTransform(mRenderer->GetDeviceContext(), mStabilityCalculator->GetLastStabilityState(), spawnPeriod);
	}

	if(saveFrame)
	{
		mDownscaler->DownscalePicture(mRenderer->GetDeviceContext(), mFinalTransformer->GetTransformedSRV());

		Microsoft::WRL::ComPtr<ID3D11Texture2D> downscaledTex;
		mDownscaler->GetDownscaledSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(downscaledTex.GetAddressOf()));

		mBoardSaver->SaveBoardToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), downscaledTex.Get(), videoFrameFile);
	}

	if(drawFrame)
	{
		mRenderer->SetCurrentBoard(mFinalTransformer->GetTransformedSRV());
		mRenderer->NeedRedraw();
	}
}

void FractalGen::FinishComputing(uint32_t spawnPeriod, const std::wstring& stabilityFile)
{
	if(!stabilityFile.empty())
	{
		mFinalTransformer->ComputeTransform(mRenderer->GetDeviceContext(), mStabilityCalculator->GetLastStabilityState(), spawnPeriod);

		Microsoft::WRL::ComPtr<ID3D11Texture2D> downscaledTex;
		mFinalTransformer->GetTransformedSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(downscaledTex.GetAddressOf()));

		mBoardSaver->SaveBoardToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), downscaledTex.Get(), stabilityFile);
	}
}