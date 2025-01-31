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
#include "ndNewtonStdafx.h"
#include "ndMultiBodyVehicleDifferential.h"

ndMultiBodyVehicleDifferential::ndMultiBodyVehicleDifferential(ndBodyKinematic* const differential, ndBodyKinematic* const chassis, dFloat32 slipOmegaLock)
	:ndJointBilateralConstraint(2, differential, chassis, differential->GetMatrix())
	,m_limitedSlipOmega(slipOmegaLock)
{
	dAssert(slipOmegaLock >= 0.0f);
}

void ndMultiBodyVehicleDifferential::AlignMatrix()
{
	dMatrix matrix0;
	dMatrix matrix1;
	CalculateGlobalMatrix(matrix0, matrix1);

	//matrix1.m_posit += matrix1.m_up.Scale(1.0f);

	m_body0->SetMatrix(matrix1);
	m_body0->SetVelocity(m_body1->GetVelocity());

	dVector omega0(m_body0->GetOmega());
	dVector omega1(m_body1->GetOmega());
	dVector omega(
		matrix1.m_front.Scale(matrix1.m_front.DotProduct(omega0).GetScalar()) +
		matrix1.m_up.Scale(matrix1.m_up.DotProduct(omega0).GetScalar()) +
		matrix1.m_right.Scale(matrix1.m_right.DotProduct(omega1).GetScalar()));

	m_body0->SetOmega(omega);
}

void ndMultiBodyVehicleDifferential::JacobianDerivative(ndConstraintDescritor& desc)
{
	dMatrix matrix0;
	dMatrix matrix1;
	CalculateGlobalMatrix(matrix0, matrix1);

	//one rows to restrict rotation around around the parent coordinate system
	const dFloat32 angle = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right);
	AddAngularRowJacobian(desc, matrix1.m_right, angle);

	const dVector omega0(m_body0->GetOmega());
	const dVector omega1(m_body1->GetOmega());

	dFloat32 slipOmega = matrix1.m_up.DotProduct(omega0 - omega1).GetScalar();
	if (dAbs(slipOmega) > m_limitedSlipOmega) 
	{
		AddAngularRowJacobian(desc, matrix1.m_up, dFloat32 (0.0f));
		ndJacobian& jacobian = desc.m_jacobian[desc.m_rowsCount - 1].m_jacobianM1;
		jacobian.m_angular = dVector::m_zero;
		if (slipOmega > m_limitedSlipOmega)
		{
			slipOmega -= m_limitedSlipOmega;
			dFloat32 alpha = slipOmega * desc.m_invTimestep;
			SetMotorAcceleration(desc, -alpha);
			SetHighFriction(desc, dFloat32(0.0f));
		} 
		else
		{
			dAssert(slipOmega < -m_limitedSlipOmega);
			slipOmega += m_limitedSlipOmega;
			dFloat32 alpha = slipOmega * desc.m_invTimestep;
			SetMotorAcceleration(desc, -alpha);
			SetLowerFriction(desc, dFloat32(0.0f));
		}
	}
}

