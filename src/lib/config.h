/**************************************************************************************************/
// centralization of engine data
//
// last update: 2008.01.22 (c) by MichiSoft TM
/**************************************************************************************************/

#if !defined(X_CONFIG_H__INCLUDED_)
#define X_CONFIG_H__INCLUDED_


#define _X_USE_HUI_
#define _X_USE_NET_
//#define _X_USE_NIX_
#define _X_USE_VULKAN_
#define _X_USE_IMAGE_
#define _X_USE_SCRIPT_
//#define _X_USE_SOUND_
#define _X_USE_ANY_


//#####################################################################
// Hui-API
//
// graphical user interface in the hui/* files
//#####################################################################

#define HUI_USE_GTK_ON_WINDOWS		// use gtk instead of windows api on windows




//#####################################################################
// Nix-API
//
// graphics support in the nix/* files
//#####################################################################


//#define NIX_ALLOW_VIDEO_TEXTURE			1		// allow Avi-videos as texture?



//#####################################################################
// Sound-API
//
// sound support...
//#####################################################################


//#define SOUND_ALLOW_OPENAL				1
//#define SOUND_ALLOW_OGG					1
//#define SOUND_ALLOW_FLAC				1

//#####################################################################
// X9-Engine
//
// components each with its own file
// uncomment the ones with their files in this project
//#####################################################################

//#define _X_ALLOW_X_					1


#define USE_ODE			1
//#define _X_ALLOW_PHYSICS_DEBUG_		1


#endif


