// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/IConsole.h>
#include <CrySystem/ISystem.h>

#include <CrySystem/VR/IHMDManager.h>

namespace CryVR
{
class CVars
{
private:
public:
	// Shared variables:
	static int hmd_info;
	static int hmd_social_screen;
	static int hmd_social_screen_keep_aspect;
<<<<<<< HEAD
	static int hmd_driver;
	static int hmd_post_inject_camera;

#if defined(INCLUDE_OCULUS_SDK) // Oculus-specific variables:
	static int   hmd_low_persistence;
	static int   hmd_dynamic_prediction;
	static float hmd_ipd;
	static int   hmd_queue_ahead;
	static int   hmd_projection;
	static int   hmd_perf_hud;
	static float hmd_projection_screen_dist;
#endif                           // defined(INCLUDE_OCULUS_SDK)

#if defined(INCLUDE_OPENVR_SDK) // OpenVR-specific variables:
	static int   hmd_reference_point;
	static float hmd_quad_distance;
	static float hmd_quad_width;
	static int hmd_quad_absolute;
#endif                           // defined(INCLUDE_OPENVR_SDK)
=======
	static int hmd_tracking_origin;
	static float hmd_resolution_scale;
	static ICVar* pSelectedHmdNameVar;
>>>>>>> upstream/stabilisation

	static void OnHmdRecenter(IConsoleCmdArgs* pArgs) 
	{ 
		gEnv->pSystem->GetHmdManager()->RecenterPose();
	}

public:
	static void Register()
	{
		REGISTER_CVAR2("hmd_info", &hmd_info, hmd_info,
		               VF_NULL, "Shows hmd related profile information.");

		REGISTER_CVAR2("hmd_social_screen", &hmd_social_screen, hmd_social_screen,
		               VF_NULL, "Selects the social screen mode: \n"
		                        "-1- Off\n"
		                        "0 - Distorted dual image\n"
		                        "1 - Undistorted dual image\n"
		                        "2 - Undistorted left eye\n"
		                        "3 - Undistorted right eye\n"
		               );

		REGISTER_CVAR2("hmd_social_screen_keep_aspect", &hmd_social_screen_keep_aspect, hmd_social_screen_keep_aspect,
		               VF_NULL, "Keep aspect ratio when displaying images on social screen: \n"
		                        "0 - Off\n"
		                        "1 - On\n"
		               );

		REGISTER_CVAR2("hmd_tracking_origin", &hmd_tracking_origin, hmd_tracking_origin,
			VF_NULL, "Determine HMD tracking origin point.\n"
			"0 - Camera (/Actor's head)\n"
			"1 - Actor's feet\n");

		REGISTER_CVAR2("hmd_resolution_scale", &hmd_resolution_scale, hmd_resolution_scale,
			VF_NULL, "Scales rendered resolution");

<<<<<<< HEAD
		REGISTER_COMMAND("hmd_recenter_pose", &OnHmdRecenter,
		                 VF_NULL, "Recenters sensor orientation of the HMD.");

#if defined(INCLUDE_OCULUS_SDK)
		REGISTER_CVAR2("hmd_low_persistence", &hmd_low_persistence, hmd_low_persistence,
		               VF_NULL, "Enables low persistence mode.");

		REGISTER_CVAR2("hmd_dynamic_prediction", &hmd_dynamic_prediction, hmd_dynamic_prediction,
		               VF_NULL, "Enables dynamic prediction based on internally measured latency.");

		REGISTER_CVAR2("hmd_ipd", &hmd_ipd, hmd_ipd,
		               VF_NULL, "HMD Interpupillary distance. If set to a value lower than zero it reads the IPD from the HMD device");

		REGISTER_CVAR2("hmd_queue_ahead", &hmd_queue_ahead, hmd_queue_ahead,
		               VF_NULL, "Enable/Disable Queue Ahead for Oculus");

		REGISTER_CVAR2("hmd_projection", &hmd_projection, eHmdProjection_Stereo,
		               VF_NULL, "Selects the way the image is projected into the hmd: \n"
		                        "0 - normal stereoscopic mode\n"
		                        "1 - monoscopic (cinema-like)\n"
		                        "2 - monoscopic (head-locked)\n"
		               );

		REGISTER_CVAR2("hmd_projection_screen_dist", &hmd_projection_screen_dist, 0.f,
		               VF_NULL, "If >0 it forces the 'cinema screen' distance to the HMD when using 'monoscopic (cinema-like)' projection");

		REGISTER_CVAR2("hmd_perf_hud", &hmd_perf_hud, 0,
		               VF_NULL, "Performance HUD Display for HMDs \n"
		                        "0 - off\n"
		                        "1 - Summary\n"
		                        "2 - Latency timing\n"
		                        "3 - App Render timing\n"
		                        "4 - Compositor Render timing\n"
		                        "5 - Version Info\n"
		               );
#endif // defined(INCLUDE_OCULUS_SDK)

#if defined(INCLUDE_OPENVR_SDK)
		REGISTER_CVAR2("hmd_reference_point", &hmd_reference_point, hmd_reference_point,
		               VF_NULL, "HMD center reference point.\n"
		                        "0 - Camera (/Actor's head)\n"
		                        "1 - Actor's feet\n");

		REGISTER_CVAR2("hmd_quad_distance", &hmd_quad_distance, hmd_quad_distance, VF_NULL, "Distance between eyes and UI quad");

		REGISTER_CVAR2("hmd_quad_width", &hmd_quad_width, hmd_quad_width, VF_NULL, "Width of the UI quad in meters");

		REGISTER_CVAR2("hmd_quad_absolute", &hmd_quad_absolute, hmd_quad_absolute, VF_NULL, "Should quads be placed relative to the HMD or in absolute tracking space? (Default = 1: Absolute UI positioning)");
#endif // defined(INCLUDE_OPENVR_SDK)
=======
		pSelectedHmdNameVar = REGISTER_STRING("hmd_device", "", VF_NULL, 
						"Specifies the name of the VR device to use\nAvailable options depend on VR plugins registered with the engine");
>>>>>>> upstream/stabilisation

		REGISTER_COMMAND("hmd_recenter_pose", &OnHmdRecenter,
		                 VF_NULL, "Recenters sensor orientation of the HMD.");
	}

	static void Unregister()
	{
		if (IConsole* const pConsole = gEnv->pConsole)
		{
			pConsole->UnregisterVariable("hmd_info");
			pConsole->UnregisterVariable("hmd_social_screen");
<<<<<<< HEAD
			pConsole->UnregisterVariable("hmd_driver");
			pConsole->UnregisterVariable("hmd_recenter_pose");
#if defined(INCLUDE_OCULUS_SDK)
			pConsole->UnregisterVariable("hmd_low_persistence");
			pConsole->UnregisterVariable("hmd_dynamic_prediction");
			pConsole->UnregisterVariable("hmd_ipd");
			pConsole->UnregisterVariable("hmd_queue_ahead");
			pConsole->UnregisterVariable("hmd_projection");
			pConsole->UnregisterVariable("hmd_projection_screen_dist");
			pConsole->UnregisterVariable("hmd_perf_hud");
#endif  // defined(INCLUDE_OCULUS_SDK)
#if defined(INCLUDE_OPENVR_SDK)
			pConsole->UnregisterVariable("hmd_reference_point");
			pConsole->UnregisterVariable("hmd_quad_distance");
			pConsole->UnregisterVariable("hmd_quad_width");
			pConsole->UnregisterVariable("hmd_quad_absolute");
#endif  // defined(INCLUDE_OPENVR_SDK)
=======
			pConsole->UnregisterVariable("hmd_social_screen_keep_aspect");
			pConsole->UnregisterVariable("hmd_tracking_origin");
			pConsole->UnregisterVariable("hmd_resolution_scale");
			pConsole->UnregisterVariable("hmd_device");

			pConsole->RemoveCommand("hmd_recenter_pose");
>>>>>>> upstream/stabilisation
		}

	}
};
}
