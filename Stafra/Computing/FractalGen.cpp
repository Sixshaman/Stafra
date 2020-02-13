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
#include "Boards.hpp"
#include "BoardLoader.hpp"
#include "BoardSaver.hpp"
#include "../App/Renderer.hpp"

FractalGen::FractalGen(Renderer* renderer): mRenderer(renderer), mDefaultBoardWidth(1023), mDefaultBoardHeight(1023), mVideoFrameWidth(1), mVideoFrameHeight(1), mSpawnPeriod(0), mbUseSmoothTransform(false)
{
	ID3D11Device*    device = mRenderer->GetDevice();
	ID3D11DeviceContext* dc = mRenderer->GetDeviceContext();

	mStabilityCalculator = std::make_unique<StabilityCalculator>(device);

	mDownscaler       = std::make_unique<Downscaler>(device);
	mFinalTransformer = std::make_unique<FinalTransformer>(device);
	mEqualityChecker  = std::make_unique<EqualityChecker>(device);

	mBoards        = std::make_unique<Boards>(device);
	mClickRules    = std::make_unique<ClickRules>(device);
	mBoardLoader   = std::make_unique<BoardLoader>(device);
	mBoardSaver    = std::make_unique<BoardSaver>();

	mBoards->Init4CornersBoard(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), mDefaultBoardWidth, mDefaultBoardHeight);
	mBoards->InitDefaultRestriction(mRenderer->GetDevice(), mRenderer->GetDeviceContext());
	mClickRules->InitDefault(mRenderer->GetDevice());
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

void FractalGen::SetSpawnPeriod(uint32_t spawn)
{
	mSpawnPeriod = spawn;
}

void FractalGen::SetUseSmooth(bool smooth)
{
	mbUseSmoothTransform = smooth;
}

void FractalGen::InitDefaultClickRule()
{
	mClickRules->InitDefault(mRenderer->GetDevice());
	mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
	mRenderer->NeedRedrawClickRule();
}

Utils::BoardLoadError FractalGen::LoadClickRuleFromFile(const std::wstring& clickRuleFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleTex;
	Utils::BoardLoadError loadErr = mBoardLoader->LoadClickRuleFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), clickRuleFile, clickRuleTex.GetAddressOf());

	if(loadErr == Utils::BoardLoadError::LOAD_SUCCESS)
	{
		mClickRules->CreateFromTexture(mRenderer->GetDevice(), clickRuleTex.Get());
		mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
		mRenderer->NeedRedrawClickRule();
	}

	return loadErr;
}

uint32_t FractalGen::GetLastFrameNumber() const
{
	return mStabilityCalculator->GetCurrentStep();
}

uint32_t FractalGen::GetSolutionPeriod() const
{
	return mStabilityCalculator->GetDefaultSolutionPeriod();
}

void FractalGen::EditClickRule(float normalizedX, float normalizedY)
{
	uint32_t clickRuleWidth = mClickRules->GetWidth();
	uint32_t clickRuleHeight = mClickRules->GetHeight();

	mClickRules->EditCellState(mRenderer->GetDeviceContext(), (uint32_t)(clickRuleWidth * normalizedX), (uint32_t)(clickRuleHeight * normalizedY));
	mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
	mRenderer->NeedRedrawClickRule();
}

void FractalGen::Init4CornersBoard()
{
	mBoards->Init4CornersBoard(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), mDefaultBoardWidth, mDefaultBoardHeight);
}

void FractalGen::Init4SidesBoard()
{
	mBoards->Init4SidesBoard(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), mDefaultBoardWidth, mDefaultBoardHeight);
}

Utils::BoardLoadError FractalGen::LoadBoardFromFile(const std::wstring& boardFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> initialBoardTex;
	Utils::BoardLoadError loadErr = mBoardLoader->LoadBoardFromFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), boardFile, initialBoardTex.GetAddressOf());

	if(loadErr == Utils::BoardLoadError::LOAD_SUCCESS)
	{
		mBoards->InitBoardFromTexture(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), initialBoardTex.Get());
	}

	return loadErr;
}

void FractalGen::ResetComputingParameters()
{
	mClickRules->Bake(mRenderer->GetDeviceContext());
	mStabilityCalculator->PrepareForCalculations(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), mBoards->GetInitialBoardTex());

	uint32_t boardWidth  = mStabilityCalculator->GetBoardWidth();
	uint32_t boardHeight = mStabilityCalculator->GetBoardHeight();

	uint32_t clickRuleWidth  = mClickRules->GetWidth();
	uint32_t clickRuleHeight = mClickRules->GetHeight();

	mDownscaler->PrepareForDownscaling(mRenderer->GetDevice(), mVideoFrameWidth, mVideoFrameHeight);
	mFinalTransformer->PrepareForTransform(mRenderer->GetDevice(), boardWidth, boardHeight);
	mEqualityChecker->PrepareForCalculations(mRenderer->GetDevice(), boardWidth, boardHeight);

	mBoardSaver->PrepareStagingTextures(mRenderer->GetDevice(), boardWidth, boardHeight, mVideoFrameWidth, mVideoFrameHeight, clickRuleWidth, clickRuleHeight);

	mRenderer->SetCurrentClickRule(mClickRules->GetClickRuleImageSRV());
	mRenderer->NeedRedrawClickRule();

	mFinalTransformer->ComputeTransform(mRenderer->GetDeviceContext(), mStabilityCalculator->GetLastStabilityState(), mSpawnPeriod, mbUseSmoothTransform);
	mRenderer->SetCurrentBoard(mFinalTransformer->GetTransformedSRV());
	mRenderer->NeedRedraw();
}

void FractalGen::Tick()
{
	ID3D11ShaderResourceView* clickRuleBufferSRV  = nullptr;
	ID3D11ShaderResourceView* clickRuleCounterSRV = nullptr;
	if(!mClickRules->IsDefault())
	{
		clickRuleBufferSRV  = mClickRules->GetClickRuleBufferSRV();
		clickRuleCounterSRV = mClickRules->GetClickRuleBufferCounterSRV();
	}

	mStabilityCalculator->StabilityNextStep(mRenderer->GetDeviceContext(), clickRuleBufferSRV, clickRuleCounterSRV, mBoards->GetRestrictionSRV(), mSpawnPeriod);
	mFinalTransformer->ComputeTransform(mRenderer->GetDeviceContext(), mStabilityCalculator->GetLastStabilityState(), mSpawnPeriod, mbUseSmoothTransform);

	mRenderer->SetCurrentBoard(mFinalTransformer->GetTransformedSRV());
	mRenderer->NeedRedraw();
}

void FractalGen::SaveCurrentVideoFrame(const std::wstring& videoFrameFile)
{
	mDownscaler->DownscalePicture(mRenderer->GetDeviceContext(), mFinalTransformer->GetTransformedSRV());

	Microsoft::WRL::ComPtr<ID3D11Texture2D> downscaledTex;
	mDownscaler->GetDownscaledSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(downscaledTex.GetAddressOf()));

	mBoardSaver->SaveVideoFrameToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), downscaledTex.Get(), videoFrameFile);
}

void FractalGen::SaveCurrentStep(const std::wstring& stabilityFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> stabilityTex;
	mFinalTransformer->GetTransformedSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(stabilityTex.GetAddressOf()));

	mBoardSaver->SaveBoardToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), stabilityTex.Get(), stabilityFile);
}

void FractalGen::SaveClickRule(const std::wstring& clickRuleFile)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> clickRuleTex;
	mClickRules->GetClickRuleImageSRV()->GetResource(reinterpret_cast<ID3D11Resource**>(clickRuleTex.GetAddressOf()));

	mBoardSaver->SaveClickRuleToFile(mRenderer->GetDevice(), mRenderer->GetDeviceContext(), clickRuleTex.Get(), clickRuleFile);
}
