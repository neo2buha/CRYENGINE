#pragma once

#include "EntityComponentFactory.h"

// Wrapper of an entity component to allow exposing managed equivalents to native code
class CManagedEntityComponent final : public IEntityComponent
{
public:
	CManagedEntityComponent(const CManagedEntityComponentFactory& factory);

	virtual ~CManagedEntityComponent() {}

	// IEntityComponent
	virtual void PreInit(const SInitParams& params) override;
	virtual void Initialize() override;

	virtual	void ProcessEvent(SEntityEvent &event) override;
	virtual uint64 GetEventMask() const override { return m_factory.m_eventMask; }
	// ~IEntityComponent

	CMonoObject* GetObject() const { return m_pMonoObject.get(); }
	const CManagedEntityComponentFactory& GetManagedFactoy() const { return m_factory; }

protected:
	const CManagedEntityComponentFactory& m_factory;
	std::shared_ptr<CMonoObject> m_pMonoObject;
};
