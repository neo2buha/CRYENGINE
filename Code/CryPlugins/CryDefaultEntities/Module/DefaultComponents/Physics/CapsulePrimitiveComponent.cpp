#include "StdAfx.h"
#include "CapsulePrimitiveComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
namespace DefaultComponents
{
void CCapsulePrimitiveComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
{
}

void CCapsulePrimitiveComponent::ReflectType(Schematyc::CTypeDesc<CCapsulePrimitiveComponent>& desc)
{
	desc.SetGUID(CCapsulePrimitiveComponent::IID());
	desc.SetEditorCategory("Physics");
	desc.SetLabel("Capsule Collider");
	desc.SetDescription("Adds a capsule collider to the physical entity, requires a Simple Physics or Character Controller component.");
	desc.SetIcon("icons:ObjectTypes/object.ico");
	desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

	desc.AddMember(&CCapsulePrimitiveComponent::m_radius, 'radi', "Radius", "Radius", "Radius of the capsule", 0.5f);
	desc.AddMember(&CCapsulePrimitiveComponent::m_height, 'heig', "Height", "Height", "Height of the capsule", 1.f);

	desc.AddMember(&CCapsulePrimitiveComponent::m_mass, 'mass', "Mass", "Mass", "Mass of the object in kg, note that this cannot be set at the same time as density. Both being set to 0 means no physics.", 10.f);
	desc.AddMember(&CCapsulePrimitiveComponent::m_density, 'dens', "Density", "Density", "Density of the object, note that this cannot be set at the same time as mass. Both being set to 0 means no physics.", 0.f);

	desc.AddMember(&CCapsulePrimitiveComponent::m_surfaceTypeName, 'surf', "Surface", "Surface Type", "Surface type assigned to this object, determines its physical properties", "");
}
}
}
