/* Copyright (c) <2003-2021> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __D_RAYCAST_NOTYFY_H__
#define __D_RAYCAST_NOTYFY_H__

#include "ndCollisionStdafx.h"
#include "ndBody.h"
#include "ndContact.h"

D_MSV_NEWTON_ALIGN_32
class ndRayCastNotify
{
	public: 
	ndRayCastNotify()
		:m_param(dFloat32 (1.0f))
	{
	}

	virtual ~ndRayCastNotify()
	{
	}

	D_COLLISION_API bool TraceShape(const dVector& globalOrigin, const dVector& globalDestination, const ndShapeInstance& shapeInstance, const dMatrix& shapeGlobal);

	virtual dUnsigned32 OnRayPrecastAction(const ndBody* const, const ndShapeInstance* const)
	{
		return 1;
	}

	virtual dFloat32 OnRayCastAction(const ndContactPoint& contact, dFloat32 intersetParam) = 0;


	ndContactPoint m_contact;
	dFloat32 m_param;
} D_GCC_NEWTON_ALIGN_32;

D_MSV_NEWTON_ALIGN_32
class ndRayCastClosestHitCallback: public ndRayCastNotify
{
	public:
	ndRayCastClosestHitCallback()
		:ndRayCastNotify()
	{
	}

	dUnsigned32 OnRayPrecastAction(const ndBody* const body, const ndShapeInstance* const)
	{
		// do not let player capsule picking
		return ((ndBody*)body)->GetAsBodyPlayerCapsule() ? 0 : 1;
	}

	dFloat32 OnRayCastAction(const ndContactPoint& contact, dFloat32 intersetParam)
	{
		if (intersetParam < m_param)
		{
			m_contact = contact;
			m_param = intersetParam;
		}
		return intersetParam;
	}
} D_GCC_NEWTON_ALIGN_32 ;


#endif
