/* Copyright (c) <2003-2021> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently


#ifndef _TOLLBOX_STDAFX_H_
#define _TOLLBOX_STDAFX_H_

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

#define USE_STATIC_MESHES_DEBUG_COLLISION
//#define USE_TEST_ALL_FACE_USER_RAYCAST_CALLBACK

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

	#undef APIENTRY
	#define GLFW_EXPOSE_NATIVE_WIN32
	#define GLFW_EXPOSE_NATIVE_WGL

	#include <windows.h>
	#include <commctrl.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <malloc.h>
	#include <memory.h>
	#include <time.h>
	#include <tchar.h>
	#include <crtdbg.h>
	
	#include <glatter.h>
	#include <GL/glu.h>
	#include <GL/gl.h>
	#include <imgui.h>
	#include <GLFW/glfw3.h>
	#include <GLFW/glfw3native.h>

	// audio library support
	#include <AL/al.h>
	#include <AL/alc.h>

#endif
	
#if (defined (_POSIX_VER) || defined (_POSIX_VER_64))
	#include <stdlib.h>
	#include <unistd.h>
	#include <time.h>

	#include <glatter.h>
	#include <GL/glu.h>
	#include <GL/gl.h>
	#include <imgui.h>
	#include <GLFW/glfw3.h>

	// audio library support
	#include <AL/al.h>
	#include <AL/alc.h>
#endif

#ifdef _MACOSX_VER
	#include <CoreFoundation/CoreFoundation.h> 
	#include <unistd.h>
	#include <GL/glew.h>
	#include <OpenGL/glu.h>
	#include <OpenGL/gl.h>

	// audio library support
	#include <OpenAl/al.h>
	#include <OpenAl/alc.h>
#endif

// some thierd party lib includes
#include <ofbx.h>

// SDK includes
#include <ndNewton.h>

#ifndef _MSC_VER
	#ifndef stricmp
		#define stricmp strcasecmp
	#endif

	#ifndef _strlwr
		inline char* _strlwr (char* const ptr) 
		{ 
			char* ret = ptr; 
			while (*ret != '\0')
			{ 
				*ret = char (tolower (*ret));
				ret ++;
			} 
			return ptr; 
		}
	#endif
#endif


//#define DEMO_CHECK_ASYN_UPDATE
#define dRAND_MAX		0x00ffffff

dFloat32 dRand();
void dSetRandSeed (dUnsigned32 seed);
dFloat32 dGaussianRandom (dFloat32 amp);

inline dInt32 dTwosPower (dInt32 x)
{
	dInt32 rval=1;
	for (; rval < x; rval *= 2);
	return rval;
}


// for some reason specifying a relative does not seem to work in Linus
// and i have to specify a absolute path
// #define ASSETS_PATH "."
//void GetAplicationDirectory (char* const aplicationDir);
void dGetWorkingFileName (const char* const name, char* const outPathName);

// endian conversion
dUnsigned32 SWAP_INT32(dUnsigned32 x);
dUnsigned16 SWAP_INT16(dUnsigned16 x);
void SWAP_FLOAT32_ARRAY (void* const array, dInt32 count);
#endif 

