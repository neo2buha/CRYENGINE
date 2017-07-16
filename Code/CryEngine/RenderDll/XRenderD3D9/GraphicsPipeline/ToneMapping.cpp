// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ToneMapping.h"

#include "SunShafts.h"
#include "ColorGrading.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"

void CToneMappingStage::Init()
{
}

void CToneMappingStage::Execute()
{
	// TODO: Support HDR debug modes

	PROFILE_LABEL_SCOPE("TONEMAPPING");

	CD3D9Renderer* pRenderer = gcpRendD3D;

	bool bSunShafts = false;
	bool bHighQualitySunshafts = false;
	bool bColorGrading = false;
	bool bBloomEnabled = CRenderer::CV_r_HDRBloom && CRenderer::CV_r_PostProcess;

	CShader* pShader = CShaderMan::s_shHDRPostProcess;
	CTexture* pSunShaftsTex = CTexture::s_ptexBlack;
	CTexture* pColorChartTex = CTexture::s_ptexBlack;

	CSunShaftsStage* pSunShaftsStage = (CSunShaftsStage*)pRenderer->GetGraphicsPipeline().GetStage(eStage_Sunshafts);
	bSunShafts = pSunShaftsStage->IsActive();
	pSunShaftsTex = pSunShaftsStage->GetFinalOutputRT();

	CColorGradingStage* pColorGradingStage = (CColorGradingStage*)pRenderer->GetGraphicsPipeline().GetStage(eStage_ColorGrading);
	if (CTexture* pColorChartTexTentative = pColorGradingStage->GetColorChart())
	{
		bColorGrading = true;
		pColorChartTex = pColorChartTexTentative;
	}

	int featureMask = ((int)bSunShafts << 1) | ((int)bColorGrading << 2) | ((int)bBloomEnabled << 3) |
	                  ((CRenderer::CV_r_HDREyeAdaptationMode & 0xF) << 5) | ((CRenderer::CV_r_HDRDebug & 0xF) << 9);

	if (m_passToneMapping.InputChanged(featureMask, pSunShaftsTex->GetTextureID(), CTexture::s_ptexCurLumTexture->GetTextureID(), pColorChartTex->GetTextureID()))
	{
		uint64 rtMask = 0;
		if (CRenderer::CV_r_HDREyeAdaptationMode == 2)
			rtMask |= g_HWSR_MaskBit[HWSR_SAMPLE4];
		if (bColorGrading)
			rtMask |= g_HWSR_MaskBit[HWSR_SAMPLE1];
		if (CRenderer::CV_r_HDRDebug == 5)
			rtMask |= g_HWSR_MaskBit[HWSR_DEBUG0];

		static CCryNameTSCRC techToneMapping("HDRFinalPass");
		m_passToneMapping.SetTechnique(pShader, techToneMapping, rtMask);
		m_passToneMapping.SetRenderTarget(0, CTexture::s_ptexSceneDiffuse);
		m_passToneMapping.SetState(GS_NODEPTHTEST);
		m_passToneMapping.SetFlags(CPrimitiveRenderPass::ePassFlags_RequireVrProjectionConstants);

		CTexture* pBloomTex = bBloomEnabled ? CTexture::s_ptexHDRFinalBloom : CTexture::s_ptexBlack;

		m_passToneMapping.SetTextureSamplerPair(0, CTexture::s_ptexHDRTarget, EDefaultSamplerStates::LinearClamp);
		m_passToneMapping.SetTextureSamplerPair(1, CTexture::s_ptexCurLumTexture, EDefaultSamplerStates::LinearClamp);
		m_passToneMapping.SetTextureSamplerPair(2, pBloomTex, EDefaultSamplerStates::LinearClamp);
		m_passToneMapping.SetTextureSamplerPair(5, CTexture::s_ptexZTarget, EDefaultSamplerStates::PointClamp);
		m_passToneMapping.SetTextureSamplerPair(7, CTexture::s_ptexVignettingMap, EDefaultSamplerStates::LinearClamp);
		m_passToneMapping.SetTextureSamplerPair(8, pColorChartTex, EDefaultSamplerStates::LinearClamp);
		m_passToneMapping.SetTextureSamplerPair(9, pSunShaftsTex, EDefaultSamplerStates::LinearClamp);
		m_passToneMapping.SetRequireWorldPos(true);
		m_passToneMapping.SetRequirePerViewConstantBuffer(true);
	}

	static CCryNameR eyeAdaptationName("HDREyeAdaptation");
	static CCryNameR filmCurveName("HDRFilmCurve");
	static CCryNameR colorBalanceName("HDRColorBalance");
	static CCryNameR bloomColorName("HDRBloomColor");
	static CCryNameR shaftsSunColName("SunShafts_SunCol");

	m_passToneMapping.BeginConstantUpdate();

	Vec4 hdrSetupParams[5];
	gEnv->p3DEngine->GetHDRSetupParams(hdrSetupParams);

	m_passToneMapping.SetConstant(eyeAdaptationName, CRenderer::CV_r_HDREyeAdaptationMode == 2 ? hdrSetupParams[4] : hdrSetupParams[3], eHWSC_Pixel);
	m_passToneMapping.SetConstant(filmCurveName, hdrSetupParams[0], eHWSC_Pixel);
	m_passToneMapping.SetConstant(colorBalanceName, hdrSetupParams[2], eHWSC_Pixel);
	m_passToneMapping.SetConstant(bloomColorName, hdrSetupParams[1] * Vec4(Vec3(1.0f / 8.0f), 1.0f), eHWSC_Pixel); // Division by 8.0f was done in shader before, remove this at some point

	Vec4 shaftsSunCol(0, 0, 0, 0);
	if (bSunShafts)
	{
		Vec4 sunShaftParams[2];
		pSunShaftsStage->GetCompositionParams(sunShaftParams[0], sunShaftParams[1]);
		Vec3 sunColor = gEnv->p3DEngine->GetSunColor();
		sunColor.Normalize();
		sunColor.SetLerp(Vec3(sunShaftParams[0].x, sunShaftParams[0].y, sunShaftParams[0].z), sunColor, sunShaftParams[1].w);
		shaftsSunCol = Vec4(sunColor * sunShaftParams[1].z, 1);
	}
	m_passToneMapping.SetConstant(shaftsSunColName, shaftsSunCol, eHWSC_Pixel);

	m_passToneMapping.Execute();

	m_pColorChartTex = pColorChartTex; // Keep a ref count on color chart to make sure it is destroyed after m_passToneMapping
}

void CToneMappingStage::ExecuteFixedExposure()
{
	PROFILE_LABEL_SCOPE("TONEMAPPING_FIXED_EXPOSURE");

	CRenderView* pRenderView = RenderView();

	CTexture* pTargetTex = gcpRendD3D->GetCurrentTargetOutput();
	CTexture* pSrcTex = CTexture::s_ptexHDRTarget;

	const CRenderOutput* pOutput = pRenderView->GetRenderOutput();
	if (pOutput)
	{
		pSrcTex = pOutput->GetHDRTargetTexture();
	}

	auto& pass = m_passFixedExposureToneMapping;

	{
		uint64 rtMask = 0;
		static CCryNameTSCRC techToneMapping("HDRFinalPassFixedExposure");
		pass.SetPrimitiveFlags(CRenderPrimitive::eFlags_ReflectShaderConstants_VS);
		pass.SetTechnique(CShaderMan::s_shHDRPostProcess, techToneMapping, rtMask);
		pass.SetRenderTarget(0, pTargetTex);
		pass.SetState(GS_NODEPTHTEST);
		pass.SetFlags(CPrimitiveRenderPass::ePassFlags_RequireVrProjectionConstants);

		pass.SetTextureSamplerPair(0, pSrcTex, EDefaultSamplerStates::LinearClamp);
		pass.SetRequireWorldPos(true);
		pass.SetRequirePerViewConstantBuffer(true);
	}

	pass.BeginConstantUpdate();

#if 0
	// parameter for filmic response curve.
	static CCryNameR filmCurveName("HDRFilmCurve");
	Vec4 hdrSetupParams[5];
	gEnv->p3DEngine->GetHDRSetupParams(hdrSetupParams);
	pass.SetConstant(filmCurveName, hdrSetupParams[0], eHWSC_Pixel);
#endif

	pass.Execute();
}
