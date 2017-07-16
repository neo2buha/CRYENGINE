#pragma once

#include "DefaultComponents/Geometry/AdvancedAnimationComponent.h"

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CCharacterControllerComponent
			: public IEntityComponent
#ifndef RELEASE
			, public IEntityComponentPreviewer
#endif
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void Initialize() final;

			virtual void ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;

#ifndef RELEASE
			virtual IEntityComponentPreviewer* GetPreviewer() final { return this; }
#endif
			// ~IEntityComponent

#ifndef RELEASE
			// IEntityComponentPreviewer
			virtual void SerializeProperties(Serialization::IArchive& archive) final {}

			virtual void Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext &context) const final;
			// ~IEntityComponentPreviewer
#endif

		public:
			static void ReflectType(Schematyc::CTypeDesc<CCharacterControllerComponent>& desc);

			CCharacterControllerComponent() = default;
			virtual ~CCharacterControllerComponent();

			static CryGUID& IID()
			{
				static CryGUID id = "{98183F31-A685-43CD-92A9-815274F0A81C}"_cry_guid;
				return id;
			}

			bool IsOnGround() const { return m_bOnGround; }
			const Schematyc::UnitLength<Vec3>& GetGroundNormal() const { return m_groundNormal; }

			virtual void AddVelocity(const Vec3& velocity)
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
				{
					pe_action_move moveAction;

					// Apply movement request directly to velocity
					moveAction.iJump = 2;
					moveAction.dir = velocity;

					// Dispatch the movement request
					pPhysicalEntity->Action(&moveAction);
				}
			}

			virtual void SetVelocity(const Vec3& velocity)
			{
				if (IPhysicalEntity* pPhysicalEntity = m_pEntity->GetPhysicalEntity())
				{
					pe_action_move moveAction;

					// Override velocity
					moveAction.iJump = 1;
					moveAction.dir = velocity;

					// Dispatch the movement request
					pPhysicalEntity->Action(&moveAction);
				}
			}

			const Vec3& GetVelocity() const { return m_velocity; }
			Vec3 GetMoveDirection() const { return m_velocity.GetNormalized(); }

			bool IsWalking() const { return m_velocity.GetLength2D() > 0.2f && m_bOnGround; }
			
			virtual void Physicalize()
			{
				// Physicalize the player as type Living.
				// This physical entity type is specifically implemented for players
				SEntityPhysicalizeParams physParams;
				physParams.type = PE_LIVING;
				physParams.nSlot = GetOrMakeEntitySlotId();

				physParams.mass = m_physics.m_mass;

				pe_player_dimensions playerDimensions;

				// Prefer usage of a cylinder
				playerDimensions.bUseCapsule = m_physics.m_bCapsule ? 1 : 0;

				// Specify the size of our capsule
				playerDimensions.sizeCollider = m_physics.m_colliderSize;

				// Keep pivot at the player's feet (defined in player geometry) 
				playerDimensions.heightPivot = 0.f;
				// Offset collider upwards
				playerDimensions.heightCollider = 1.f;
				playerDimensions.groundContactEps = 0.004f;

				physParams.pPlayerDimensions = &playerDimensions;

				pe_player_dynamics playerDynamics;
				playerDynamics.mass = physParams.mass;
				playerDynamics.kAirControl = m_movement.m_airControlRatio;
				playerDynamics.kAirResistance = m_movement.m_airResistance;
				playerDynamics.kInertia = m_movement.m_inertia;
				playerDynamics.kInertiaAccel = m_movement.m_inertiaAcceleration;

				playerDynamics.maxClimbAngle = m_movement.m_maxClimbAngle.ToDegrees();
				playerDynamics.maxJumpAngle = m_movement.m_maxJumpAngle.ToDegrees();
				playerDynamics.minFallAngle = m_movement.m_minFallAngle.ToDegrees();
				playerDynamics.minSlideAngle = m_movement.m_minSlideAngle.ToDegrees();

				playerDynamics.maxVelGround = m_movement.m_maxGroundVelocity;

				physParams.pPlayerDynamics = &playerDynamics;

				m_pEntity->Physicalize(physParams);

				m_pEntity->UpdateComponentEventMask(this);
			}

			virtual void Ragdollize()
			{
				SEntityPhysicalizeParams physParams;
				physParams.type = PE_ARTICULATED;

				physParams.mass = m_physics.m_mass;
				physParams.nSlot = GetEntitySlotId();

				physParams.bCopyJointVelocities = true;

				m_pEntity->Physicalize(physParams);
			}

			struct SPhysics
			{
				inline bool operator==(const SPhysics &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				Schematyc::PositiveFloat m_mass = 80.f;
				Vec3 m_colliderSize = Vec3(0.45f, 0.45f, 0.935f * 0.5f);
				bool m_bCapsule = true;

				bool m_bSendCollisionSignal = false;
			};

			struct SMovement
			{
				inline bool operator==(const SMovement &rhs) const { return 0 == memcmp(this, &rhs, sizeof(rhs)); }

				Schematyc::Range<0, 1> m_airControlRatio = 0.f;
				Schematyc::Range<0, 10000> m_airResistance = 0.2f;
				Schematyc::Range<0, 10000> m_inertia = 8.f;
				Schematyc::Range<0, 10000> m_inertiaAcceleration = 8.f;

				CryTransform::CClampedAngle<0, 90> m_maxClimbAngle = 50.0_degrees;
				CryTransform::CClampedAngle<0, 90> m_maxJumpAngle = 50.0_degrees;
				CryTransform::CClampedAngle<0, 90> m_minFallAngle = 80.0_degrees;
				CryTransform::CClampedAngle<0, 90> m_minSlideAngle = 70.0_degrees;

				Schematyc::Range<0, 10000> m_maxGroundVelocity = 16.f;
			};

			virtual SPhysics& GetPhysicsParameters() { return m_physics; }
			const SPhysics& GetPhysicsParameters() const { return m_physics; }

			virtual SMovement& GetMovementParameters() { return m_movement; }
			const SMovement& GetMovementParameters() const { return m_movement; }

		protected:
			bool m_bNetworked = false;
			
			SPhysics m_physics;
			SMovement m_movement;

			bool m_bOnGround = false;
			Schematyc::UnitLength<Vec3> m_groundNormal = Vec3(0, 0, 1);

			Vec3 m_velocity = ZERO;
		};
	}
}