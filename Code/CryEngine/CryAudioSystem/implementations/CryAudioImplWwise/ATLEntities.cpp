// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"
#include <PoolObject.h>
#include <ATLEntities.h>
#include <IAudioImpl.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
AkGameObjectID CObject::s_dummyGameObjectId = static_cast<AkGameObjectID>(-2);

// AK callbacks
void EndEventCallback(AkCallbackType callbackType, AkCallbackInfo* pCallbackInfo)
{
	if (callbackType == AK_EndOfEvent)
	{
		CEvent* const pAudioEvent = static_cast<CEvent* const>(pCallbackInfo->pCookie);

		if (pAudioEvent != nullptr)
		{
			gEnv->pAudioSystem->ReportFinishedEvent(pAudioEvent->m_atlEvent, true);
		}
	}
}

void PrepareEventCallback(
  AkUniqueID eventId,
  void const* pBankPtr,
  AKRESULT wwiseResult,
  AkMemPoolId memPoolId,
  void* pCookie)
{
	CEvent* const pEvent = static_cast<CEvent* const>(pCookie);

	if (pEvent != nullptr)
	{
		pEvent->m_id = eventId;
		gEnv->pAudioSystem->ReportFinishedEvent(pEvent->m_atlEvent, wwiseResult == AK_Success);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::Update()
{
	ERequestStatus result = ERequestStatus::Failure;

	if (m_bNeedsToUpdateEnvironments)
	{
		result = PostEnvironmentAmounts();
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::Set3DAttributes(SObject3DAttributes const& attributes)
{
	AkSoundPosition soundPos;
	FillAKObjectPosition(attributes.transformation, soundPos);

	AKRESULT const wwiseResult = AK::SoundEngine::SetPosition(m_id, soundPos);

	if (!IS_WWISE_OK(wwiseResult))
	{
		g_implLogger.Log(ELogType::Warning, "Wwise SetPosition failed with AKRESULT: %d", wwiseResult);
		return ERequestStatus::Failure;
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetEnvironment(IEnvironment const* const pIEnvironment, float const amount)
{
	static float const envEpsilon = 0.0001f;
	ERequestStatus result = ERequestStatus::Failure;
	SEnvironment const* const pEnvironment = static_cast<SEnvironment const* const>(pIEnvironment);

	if (pEnvironment != nullptr)
	{
		switch (pEnvironment->type)
		{
		case EEnvironmentType::AuxBus:
			{
				float const currentAmount = stl::find_in_map(m_environemntImplAmounts, pEnvironment->busId, -1.0f);

				if ((currentAmount == -1.0f) || (fabs(currentAmount - amount) > envEpsilon))
				{
					m_environemntImplAmounts[pEnvironment->busId] = amount;
					m_bNeedsToUpdateEnvironments = true;
				}

				result = ERequestStatus::Success;
				break;
			}
		case EEnvironmentType::Rtpc:
			{
				AkRtpcValue const rtpcValue = static_cast<AkRtpcValue>(pEnvironment->multiplier * amount + pEnvironment->shift);
				AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(pEnvironment->rtpcId, rtpcValue, m_id);

				if (IS_WWISE_OK(wwiseResult))
				{
					result = ERequestStatus::Success;
				}
				else
				{
					g_implLogger.Log(
					  ELogType::Warning,
					  "Wwise failed to set the Rtpc %u to value %f on object %u in SetEnvironement()",
					  pEnvironment->rtpcId,
					  rtpcValue,
					  m_id);
				}

				break;
			}
		default:
			{
				CRY_ASSERT(false); //Unknown AudioEnvironmentImplementation type
			}
		}
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid EnvironmentData passed to the Wwise implementation of SetEnvironment");
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetParameter(IParameter const* const pIParameter, float const value)
{
	ERequestStatus result = ERequestStatus::Failure;
	SParameter const* const pParameter = static_cast<SParameter const* const>(pIParameter);

	if (pParameter != nullptr)
	{
		AkRtpcValue rtpcValue = static_cast<AkRtpcValue>(pParameter->mult * value + pParameter->shift);

		AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(pParameter->id, rtpcValue, m_id);

		if (IS_WWISE_OK(wwiseResult))
		{
			result = ERequestStatus::Success;
		}
		else
		{
			g_implLogger.Log(
			  ELogType::Warning,
			  "Wwise failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T,
			  pParameter->id,
			  static_cast<AkRtpcValue>(value),
			  m_id);
		}
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid RtpcData passed to the Wwise implementation of SetParameter");
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetSwitchState(ISwitchState const* const pISwitchState)
{
	ERequestStatus result = ERequestStatus::Failure;

	SSwitchState const* const pSwitchState = static_cast<SSwitchState const* const>(pISwitchState);

	if (pSwitchState != nullptr)
	{
		switch (pSwitchState->type)
		{
		case ESwitchType::StateGroup:
			{
				AKRESULT const wwiseResult = AK::SoundEngine::SetState(
				  pSwitchState->stateOrSwitchGroupId,
				  pSwitchState->stateOrSwitchId);

				if (IS_WWISE_OK(wwiseResult))
				{
					result = ERequestStatus::Success;
				}
				else
				{
					g_implLogger.Log(
					  ELogType::Warning,
					  "Wwise failed to set the StateGroup %" PRISIZE_T "to state %" PRISIZE_T,
					  pSwitchState->stateOrSwitchGroupId,
					  pSwitchState->stateOrSwitchId);
				}

				break;
			}
		case ESwitchType::SwitchGroup:
			{
				AkGameObjectID const gameObjectId = m_id != AK_INVALID_GAME_OBJECT ? m_id : s_dummyGameObjectId;

				AKRESULT const wwiseResult = AK::SoundEngine::SetSwitch(
				  pSwitchState->stateOrSwitchGroupId,
				  pSwitchState->stateOrSwitchId,
				  gameObjectId);

				if (IS_WWISE_OK(wwiseResult))
				{
					result = ERequestStatus::Success;
				}
				else
				{
					g_implLogger.Log(
					  ELogType::Warning,
					  "Wwise failed to set the SwitchGroup %" PRISIZE_T " to state %" PRISIZE_T " on object %" PRISIZE_T,
					  pSwitchState->stateOrSwitchGroupId,
					  pSwitchState->stateOrSwitchId,
					  gameObjectId);
				}

				break;
			}
		case ESwitchType::Rtpc:
			{
				AkGameObjectID const gameObjectId = m_id != AK_INVALID_GAME_OBJECT ? m_id : s_dummyGameObjectId;

				AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(
				  pSwitchState->stateOrSwitchGroupId,
				  static_cast<AkRtpcValue>(pSwitchState->rtpcValue),
				  gameObjectId);

				if (IS_WWISE_OK(wwiseResult))
				{
					result = ERequestStatus::Success;
				}
				else
				{
					g_implLogger.Log(
					  ELogType::Warning,
					  "Wwise failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T,
					  pSwitchState->stateOrSwitchGroupId,
					  static_cast<AkRtpcValue>(pSwitchState->rtpcValue),
					  gameObjectId);
				}

				break;
			}
		case ESwitchType::None:
			{
				break;
			}
		default:
			{
				g_implLogger.Log(ELogType::Warning, "Unknown ESwitchType: %" PRISIZE_T, pSwitchState->type);

				break;
			}
		}
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid SwitchState passed to the Wwise implementation of SetSwitchState");
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	ERequestStatus result = ERequestStatus::Failure;

	AKRESULT const wwiseResult = AK::SoundEngine::SetObjectObstructionAndOcclusion(
	  m_id,
	  0,                                // only set the obstruction/occlusion for the default listener for now
	  static_cast<AkReal32>(occlusion), // Currently used on obstruction until the ATL produces a correct obstruction value.
	  static_cast<AkReal32>(occlusion));

	if (IS_WWISE_OK(wwiseResult))
	{
		result = ERequestStatus::Success;
	}
	else
	{
		g_implLogger.Log(
		  ELogType::Warning,
		  "Wwise failed to set Obstruction %f and Occlusion %f on object %" PRISIZE_T,
		  obstruction,
		  occlusion,
		  m_id);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::ExecuteTrigger(ITrigger const* const pITrigger, IEvent* const pIEvent)
{
	ERequestStatus result = ERequestStatus::Failure;

	CTrigger const* const pTrigger = static_cast<CTrigger const* const>(pITrigger);
	CEvent* const pEvent = static_cast<CEvent* const>(pIEvent);

	if ((pTrigger != nullptr) && (pEvent != nullptr))
	{
		AkGameObjectID gameObjectId = AK_INVALID_GAME_OBJECT;

		if (m_id != AK_INVALID_GAME_OBJECT)
		{
			gameObjectId = m_id;
			PostEnvironmentAmounts();
		}
		else
		{
			// If ID is invalid, then it is the global audio object
			gameObjectId = CObject::s_dummyGameObjectId;
		}

		AkPlayingID const id = AK::SoundEngine::PostEvent(pTrigger->m_id, gameObjectId, AK_EndOfEvent, &EndEventCallback, pEvent);

		if (id != AK_INVALID_PLAYING_ID)
		{
			pEvent->m_state = EEventState::Playing;
			pEvent->m_id = id;
			result = ERequestStatus::Success;
		}
		else
		{
			// if posting an Event failed, try to prepare it, if it isn't prepared already
			g_implLogger.Log(ELogType::Warning, "Failed to Post Wwise event %" PRISIZE_T, pEvent->m_id);
		}
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid AudioObjectData, ATLTriggerData or EventData passed to the Wwise implementation of ExecuteTrigger.");
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::StopAllTriggers()
{
	AkGameObjectID const gameObjectId = m_id != AK_INVALID_GAME_OBJECT ? m_id : CObject::s_dummyGameObjectId;
	AK::SoundEngine::StopAll(gameObjectId);
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetName(char const* const szName)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	StopAllTriggers();

	AKRESULT wwiseResult = AK::SoundEngine::UnregisterGameObj(m_id);
	CRY_ASSERT(wwiseResult == AK_Success);

	wwiseResult = AK::SoundEngine::RegisterGameObj(m_id, szName);
	CRY_ASSERT(wwiseResult == AK_Success);

	return ERequestStatus::SuccessNeedsRefresh;
#else
	return ERequestStatus::Success;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::PostEnvironmentAmounts()
{
	ERequestStatus result = ERequestStatus::Failure;
	AkAuxSendValue auxValues[AK_MAX_AUX_PER_OBJ];
	uint32 auxIndex = 0;

	CObject::EnvironmentImplMap::iterator iEnvPair = m_environemntImplAmounts.begin();
	CObject::EnvironmentImplMap::const_iterator const iEnvStart = m_environemntImplAmounts.begin();
	CObject::EnvironmentImplMap::const_iterator const iEnvEnd = m_environemntImplAmounts.end();

	if (m_environemntImplAmounts.size() <= AK_MAX_AUX_PER_OBJ)
	{
		for (; iEnvPair != iEnvEnd; ++auxIndex)
		{
			float const amount = iEnvPair->second;

			auxValues[auxIndex].auxBusID = iEnvPair->first;
			auxValues[auxIndex].fControlValue = amount;

			// If an amount is zero, we still want to send it to the middleware, but we also want to remove it from the map.
			if (amount == 0.0f)
			{
				m_environemntImplAmounts.erase(iEnvPair++);
			}
			else
			{
				++iEnvPair;
			}
		}
	}
	else
	{
		// sort the environments in order of decreasing amounts and take the first AK_MAX_AUX_PER_OBJ worth
		typedef std::set<std::pair<AkAuxBusID, float>, SEnvPairCompare> TEnvPairSet;
		TEnvPairSet cEnvPairs(iEnvStart, iEnvEnd);

		TEnvPairSet::const_iterator iSortedEnvPair = cEnvPairs.begin();
		TEnvPairSet::const_iterator const iSortedEnvEnd = cEnvPairs.end();

		for (; (iSortedEnvPair != iSortedEnvEnd) && (auxIndex < AK_MAX_AUX_PER_OBJ); ++iSortedEnvPair, ++auxIndex)
		{
			auxValues[auxIndex].auxBusID = iSortedEnvPair->first;
			auxValues[auxIndex].fControlValue = iSortedEnvPair->second;
		}

		//remove all Environments with 0.0 amounts
		while (iEnvPair != iEnvEnd)
		{
			if (iEnvPair->second == 0.0f)
			{
				m_environemntImplAmounts.erase(iEnvPair++);
			}
			else
			{
				++iEnvPair;
			}
		}
	}

	CRY_ASSERT(auxIndex <= AK_MAX_AUX_PER_OBJ);

	AKRESULT const wwiseResult = AK::SoundEngine::SetGameObjectAuxSendValues(m_id, auxValues, auxIndex);

	if (IS_WWISE_OK(wwiseResult))
	{
		result = ERequestStatus::Success;
	}
	else
	{
		g_implLogger.Log(ELogType::Warning, "Wwise SetGameObjectAuxSendValues failed on object %" PRISIZE_T " with AKRESULT: %d", m_id, wwiseResult);
	}

	m_bNeedsToUpdateEnvironments = false;
	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CEvent::Stop()
{
	switch (m_state)
	{
	case EEventState::Playing:
		{
			AK::SoundEngine::StopPlayingID(m_id, 10);
			break;
		}
	default:
		{
			// Stopping an event of this type is not supported!
			CRY_ASSERT(false);
			break;
		}
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CListener::Set3DAttributes(SObject3DAttributes const& attributes)
{
	ERequestStatus result = ERequestStatus::Failure;
	AkListenerPosition listenerPos;
	FillAKListenerPosition(attributes.transformation, listenerPos);
	AKRESULT const wwiseResult = AK::SoundEngine::SetListenerPosition(listenerPos, m_id);

	if (IS_WWISE_OK(wwiseResult))
	{
		result = ERequestStatus::Success;
	}
	else
	{
		g_implLogger.Log(ELogType::Warning, "Wwise SetListenerPosition failed with AKRESULT: %" PRISIZE_T, wwiseResult);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Load() const
{
	return SetLoaded(true);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Unload() const
{
	return SetLoaded(false);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::LoadAsync(IEvent* const pIEvent) const
{
	return SetLoadedAsync(pIEvent, true);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::UnloadAsync(IEvent* const pIEvent) const
{
	return SetLoadedAsync(pIEvent, false);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::SetLoaded(bool const bLoad) const
{
	ERequestStatus result = ERequestStatus::Failure;
	AkUniqueID id = m_id;
	AKRESULT const wwiseResult = AK::SoundEngine::PrepareEvent(bLoad ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload, &id, 1);

	if (IS_WWISE_OK(wwiseResult))
	{
		result = ERequestStatus::Success;
	}
	else
	{
		g_implLogger.Log(
		  ELogType::Warning,
		  "Wwise PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %" PRISIZE_T,
		  bLoad ? "Preparation_Load" : "Preparation_Unload",
		  m_id,
		  wwiseResult);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::SetLoadedAsync(IEvent* const pIEvent, bool const bLoad) const
{
	ERequestStatus result = ERequestStatus::Failure;

	CEvent* const pEvent = static_cast<CEvent*>(pIEvent);

	if (pEvent != nullptr)
	{
		AkUniqueID id = m_id;
		AKRESULT const wwiseResult = AK::SoundEngine::PrepareEvent(
		  bLoad ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload,
		  &id,
		  1,
		  &PrepareEventCallback,
		  pEvent);

		if (IS_WWISE_OK(wwiseResult))
		{
			pEvent->m_id = m_id;
			pEvent->m_state = EEventState::Unloading;
			result = ERequestStatus::Success;
		}
		else
		{
			g_implLogger.Log(
			  ELogType::Warning,
			  "Wwise PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %d",
			  bLoad ? "Preparation_Load" : "Preparation_Unload",
			  m_id,
			  wwiseResult);
		}
	}
	else
	{
		g_implLogger.Log(ELogType::Error,
		                 "Invalid IEvent passed to the Wwise implementation of %sTriggerAsync",
		                 bLoad ? "Load" : "Unprepare");
	}

	return result;
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
