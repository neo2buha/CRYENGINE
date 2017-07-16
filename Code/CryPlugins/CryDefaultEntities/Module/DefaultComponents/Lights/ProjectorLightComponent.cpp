#include "StdAfx.h"
#include "ProjectorLightComponent.h"

#include <CrySystem/IProjectManager.h>
#include <CryGame/IGameFramework.h>
#include <ILevelSystem.h>
#include <Cry3DEngine/IRenderNode.h>

#include <array>

namespace Cry
{
namespace DefaultComponents
{
void CProjectorLightComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
}

void CProjectorLightComponent::ReflectType(Schematyc::CTypeDesc<CProjectorLightComponent>& desc)
{
	desc.SetGUID(CProjectorLightComponent::IID());
	desc.SetEditorCategory("Lights");
	desc.SetLabel("Projector Light");
	desc.SetDescription("Emits light from its position in a general direction, constrained to a specified angle");
	desc.SetIcon("icons:ObjectTypes/light.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach, IEntityComponent::EFlags::ClientOnly });

	desc.AddMember(&CProjectorLightComponent::m_bActive, 'actv', "Active", "Active", "Determines whether the light is enabled", true);
	desc.AddMember(&CProjectorLightComponent::m_radius, 'radi', "Radius", "Range", "Determines whether the range of the point light", 10.f);
	desc.AddMember(&CProjectorLightComponent::m_angle, 'angl', "Angle", "Angle", "Maximum angle to emit light to, from the light's forward axis.", 45.0_degrees);

	desc.AddMember(&CProjectorLightComponent::m_projectorOptions, 'popt', "ProjectorOptions", "Projector Options", nullptr, CProjectorLightComponent::SProjectorOptions());

	desc.AddMember(&CProjectorLightComponent::m_color, 'colo', "Color", "Color", "Color emission information", CPointLightComponent::SColor());
	desc.AddMember(&CProjectorLightComponent::m_shadows, 'shad', "Shadows", "Shadows", "Shadow casting settings", CPointLightComponent::SShadows());
	desc.AddMember(&CProjectorLightComponent::m_options, 'opt', "Options", "Options", "Specific Light Options", CPointLightComponent::SOptions());
	desc.AddMember(&CProjectorLightComponent::m_animations, 'anim', "Animations", "Animations", "Light style / animation properties", CPointLightComponent::SAnimations());
}

static void ReflectType(Schematyc::CTypeDesc<CProjectorLightComponent::SProjectorOptions>& desc)
{
	desc.SetGUID("{705FA6D1-CC00-45A5-8E51-78AF6CA14D2D}"_cry_guid);
	desc.AddMember(&CProjectorLightComponent::SProjectorOptions::m_nearPlane, 'near', "NearPlane", "Near Plane", nullptr, 0.f);
	desc.AddMember(&CProjectorLightComponent::SProjectorOptions::m_texturePath, 'tex', "Texture", "Projected Texture", "Path to a texture we want to emit", "");
	desc.AddMember(&CProjectorLightComponent::SProjectorOptions::m_materialPath, 'mat', "Material", "Material", "Path to a material we want to apply to the projector", "");
}

static void ReflectType(Schematyc::CTypeDesc<CProjectorLightComponent::SFlare>& desc)
{
	desc.SetGUID("{DE4B89DD-B436-47EC-861F-4A5F3E831594}"_cry_guid);
	desc.AddMember(&CProjectorLightComponent::SFlare::m_angle, 'angl', "Angle", "Angle", nullptr, 360.0_degrees);
	desc.AddMember(&CProjectorLightComponent::SFlare::m_texturePath, 'tex', "Texture", "Flare Texture", "Path to the flare texture we want to use", "");
}

void CProjectorLightComponent::Initialize()
{
	if (!m_bActive)
	{
		FreeEntitySlot();

		return;
	}

	CDLight light;

	light.m_nLightStyle = m_animations.m_style;
	light.SetAnimSpeed(m_animations.m_speed);

	light.SetPosition(ZERO);
	light.m_Flags = DLF_DEFERRED_LIGHT | DLF_PROJECT;

	light.m_fRadius = m_radius;

	light.m_fLightFrustumAngle = m_angle.ToDegrees();
	light.m_fProjectorNearPlane = m_projectorOptions.m_nearPlane;

	light.SetLightColor(m_color.m_color * m_color.m_diffuseMultiplier);
	light.SetSpecularMult(m_color.m_specularMultiplier);

	light.m_fHDRDynamic = 0.f;

	if (m_options.m_bAffectsOnlyThisArea)
		light.m_Flags |= DLF_THIS_AREA_ONLY;

	if (m_options.m_bIgnoreVisAreas)
		light.m_Flags |= DLF_IGNORES_VISAREAS;

	if (m_options.m_bVolumetricFogOnly)
		light.m_Flags |= DLF_VOLUMETRIC_FOG_ONLY;

	if (m_options.m_bAffectsVolumetricFog)
		light.m_Flags |= DLF_VOLUMETRIC_FOG;

	if (m_options.m_bAmbient)
		light.m_Flags |= DLF_AMBIENT;

	//TODO: Automatically add DLF_FAKE when using beams or flares

	if (m_shadows.m_castShadowSpec != EMiniumSystemSpec::Disabled && (int)gEnv->pSystem->GetConfigSpec() >= (int)m_shadows.m_castShadowSpec)
	{
		light.m_Flags |= DLF_CASTSHADOW_MAPS;

		light.SetShadowBiasParams(1.f, 1.f);
		light.m_fShadowUpdateMinRadius = light.m_fRadius;

		float shadowUpdateRatio = 1.f;
		light.m_nShadowUpdateRatio = max((uint16)1, (uint16)(shadowUpdateRatio * (1 << DL_SHADOW_UPDATE_SHIFT)));
	}
	else
		light.m_Flags &= ~DLF_CASTSHADOW_MAPS;

	light.m_fAttenuationBulbSize = m_options.m_attenuationBulbSize;

	light.m_fFogRadialLobe = m_options.m_fogRadialLobe;

	const char* szProjectorTexturePath = m_projectorOptions.GetTexturePath();
	if (szProjectorTexturePath[0] == '\0')
	{
		szProjectorTexturePath = "%ENGINE%/EngineAssets/Textures/lights/softedge.dds";
	}

	const char* pExt = PathUtil::GetExt(szProjectorTexturePath);
	if (!stricmp(pExt, "swf") || !stricmp(pExt, "gfx") || !stricmp(pExt, "usm") || !stricmp(pExt, "ui"))
	{
		light.m_pLightDynTexSource = gEnv->pRenderer->EF_LoadDynTexture(szProjectorTexturePath, false);
	}
	else
	{
		light.m_pLightImage = gEnv->pRenderer->EF_LoadTexture(szProjectorTexturePath, FT_DONT_STREAM);
	}

	if ((light.m_pLightImage == nullptr || !light.m_pLightImage->IsTextureLoaded()) && light.m_pLightDynTexSource == nullptr)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Light projector texture %s not found, disabling projector component for entity %s", szProjectorTexturePath, m_pEntity->GetName());
		FreeEntitySlot();
		return;
	}

	if (m_flare.HasTexturePath())
	{
		int nLensOpticsId;

		if (gEnv->pOpticsManager->Load(m_flare.GetTexturePath(), nLensOpticsId))
		{
			IOpticsElementBase* pOptics = gEnv->pOpticsManager->GetOptics(nLensOpticsId);
			CRY_ASSERT(pOptics != nullptr);

			if (pOptics != nullptr)
			{
				light.SetLensOpticsElement(pOptics);

				float flareAngle = m_flare.m_angle.ToDegrees();

				if (flareAngle != 0)
				{
					int modularAngle = ((int)flareAngle) % 360;
					if (modularAngle == 0)
						light.m_LensOpticsFrustumAngle = 255;
					else
						light.m_LensOpticsFrustumAngle = (uint8)(flareAngle * (255.0f / 360.0f));
				}
				else
				{
					light.m_LensOpticsFrustumAngle = 0;
				}
			}
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Flare lens optics %s for projector component in entity %s doesn't exist!", m_flare.GetTexturePath(), GetEntity()->GetName());
			light.SetLensOpticsElement(nullptr);
		}
	}

	// Load the light source into the entity
	m_pEntity->LoadLight(GetOrMakeEntitySlotId(), &light);

	if (m_projectorOptions.HasMaterialPath())
	{
		// Allow setting a specific material for the light in this slot, for example to set up beams
		if (IMaterial* pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_projectorOptions.GetMaterialPath(), false))
		{
			m_pEntity->SetSlotMaterial(GetEntitySlotId(), pMaterial);
		}
	}

	// Fix light orientation to point along the forward axis
	// This has to be done since lights in the engine currently emit from the right axis for some reason.
	m_pEntity->SetSlotLocalTM(GetEntitySlotId(), Matrix34::Create(Vec3(1.f), Quat::CreateRotationZ(gf_PI * 0.5f), ZERO));

	uint32 slotFlags = m_pEntity->GetSlotFlags(GetEntitySlotId());
	UpdateGIModeEntitySlotFlags((uint8)m_options.m_giMode, slotFlags);
	m_pEntity->SetSlotFlags(GetEntitySlotId(), slotFlags);
}

void CProjectorLightComponent::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED)
	{
		Initialize();
	}
}

uint64 CProjectorLightComponent::GetEventMask() const
{
	return BIT64(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);
}

#ifndef RELEASE
void CProjectorLightComponent::Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const
{
	if (context.bSelected)
	{
		Matrix34 slotTransform = GetWorldTransformMatrix();

		float distance = m_radius;
		float size = distance * tan(m_angle.ToRadians());

		std::array<Vec3, 4> points = 
		{ {
			Vec3(size, distance, size),
			Vec3(-size, distance, size),
			Vec3(-size, distance, -size),
			Vec3(size, distance, -size)
		} };

		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(slotTransform.GetTranslation(), context.debugDrawInfo.color, slotTransform.TransformPoint(points[0]), context.debugDrawInfo.color);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(slotTransform.GetTranslation(), context.debugDrawInfo.color, slotTransform.TransformPoint(points[1]), context.debugDrawInfo.color);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(slotTransform.GetTranslation(), context.debugDrawInfo.color, slotTransform.TransformPoint(points[2]), context.debugDrawInfo.color);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(slotTransform.GetTranslation(), context.debugDrawInfo.color, slotTransform.TransformPoint(points[3]), context.debugDrawInfo.color);

		Vec3 p1 = slotTransform.TransformPoint(points[0]);
		Vec3 p2;
		for (int i = 0; i < points.size(); i++)
		{
			int j = (i + 1) % points.size();
			p2 = slotTransform.TransformPoint(points[j]);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(p1, context.debugDrawInfo.color, p2, context.debugDrawInfo.color);
			p1 = p2;
		}
	}
}
#endif

void CProjectorLightComponent::SProjectorOptions::SetTexturePath(const char* szPath)
{
	m_texturePath = szPath;
}

void CProjectorLightComponent::SProjectorOptions::SetMaterialPath(const char* szPath)
{
	m_materialPath = szPath;
}

void CProjectorLightComponent::SFlare::SetTexturePath(const char* szPath)
{
	m_texturePath = szPath;
}

void CProjectorLightComponent::Enable(bool bEnable) 
{
	m_bActive = bEnable; 

	Initialize();
}
}
}
