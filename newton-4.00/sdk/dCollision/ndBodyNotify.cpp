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

#include "dCoreStdafx.h"
#include "ndCollisionStdafx.h"
#include "ndBodyNotify.h"
#include "ndBodyKinematic.h"

ndBodyNotify::ndBodyNotify(const nd::TiXmlNode* const rootNode)
	:dClassAlloc()
	,m_body(nullptr)
{
	m_defualtGravity = xmlGetVector3(rootNode, "gravity");
}

void ndBodyNotify::OnApplyExternalForce(dInt32, dFloat32)
{
	ndBodyKinematic* const body = GetBody()->GetAsBodyKinematic();
	dAssert(body);
	if (body->GetInvMass() > 0.0f)
	{
		dVector massMatrix(body->GetMassMatrix());
		dVector force (m_defualtGravity.Scale(massMatrix.m_w));
		body->SetForce(force);
		body->SetTorque(dVector::m_zero);

		//dVector L(body->CalculateAngularMomentum());
		//dTrace(("%f %f %f\n", L.m_x, L.m_y, L.m_z));
	}
}

void ndBodyNotify::Save(nd::TiXmlElement* const rootNode, const char* const) const
{
	nd::TiXmlElement* const paramNode = new nd::TiXmlElement("ndBodyNotify");
	rootNode->LinkEndChild(paramNode);
	xmlSaveParam(paramNode, "gravity", m_defualtGravity);
}
