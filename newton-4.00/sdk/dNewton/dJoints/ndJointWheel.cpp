/* Copyright (c) <2003-2019> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include "dCoreStdafx.h"
#include "ndNewtonStdafx.h"
#include "ndJointWheel.h"

ndJointWheel::ndJointWheel(const dMatrix& pinAndPivotFrame, ndBodyKinematic* const child, ndBodyKinematic* const parent, const ndWheelDescriptor& info)
	:ndJointBilateralConstraint(7, child, parent, pinAndPivotFrame)
	,m_baseFrame(m_localMatrix1)
	,m_info(info)
	,m_posit(dFloat32 (0.0f))
	,m_speed(dFloat32(0.0f))
	,m_brakeTorque(dFloat32(0.0f))
{
	//static int xxxxxxxxxxxx;
	//xxxxx = xxxxxxxxxxxx;
	//xxxxxxxxxxxx++;
}

ndJointWheel::~ndJointWheel()
{
}

void ndJointWheel::SetBrakeTorque(dFloat32 torque)
{
	m_brakeTorque = torque;
}

void ndJointWheel::SetSteeringAngle(dFloat32 steeringAngle)
{
	dMatrix tireMatrix;
	dMatrix chassisMatrix;

	const dMatrix steeringMatrix(dYawMatrix(steeringAngle));
	m_localMatrix1 = steeringMatrix * m_baseFrame;

	CalculateGlobalMatrix(tireMatrix, chassisMatrix);

	const dVector relPosit(tireMatrix.m_posit - chassisMatrix.m_posit);
	const dFloat32 distance = relPosit.DotProduct(chassisMatrix.m_up).GetScalar();
	const dFloat32 spinAngle = -CalculateAngle(tireMatrix.m_up, chassisMatrix.m_up, chassisMatrix.m_front);

	dMatrix newTireMatrix(dPitchMatrix(spinAngle) * chassisMatrix);
	newTireMatrix.m_posit = chassisMatrix.m_posit + chassisMatrix.m_up.Scale(distance);

	const dMatrix tireBodyMatrix(m_localMatrix0.Inverse() * newTireMatrix);
	m_body0->SetMatrix(tireBodyMatrix);
}

void ndJointWheel::JacobianDerivative(ndConstraintDescritor& desc)
{
	dMatrix matrix0;
	dMatrix matrix1;

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix(matrix0, matrix1);

	// calculate position and speed	
	const dVector veloc0(m_body0->GetVelocityAtPoint(matrix0.m_posit));
	const dVector veloc1(m_body1->GetVelocityAtPoint(matrix1.m_posit));

	const dVector& pin = matrix1[0];
	const dVector& p0 = matrix0.m_posit;
	const dVector& p1 = matrix1.m_posit;
	const dVector prel(p0 - p1);
	const dVector vrel(veloc0 - veloc1);

	m_speed = vrel.DotProduct(matrix1.m_up).GetScalar();
	m_posit = prel.DotProduct(matrix1.m_up).GetScalar();
	const dVector projectedPoint = p1 + pin.Scale(pin.DotProduct(prel).GetScalar());

	const dFloat32 angle0 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_up);
	const dFloat32 angle1 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right);

	AddLinearRowJacobian(desc, p0, projectedPoint, matrix1[0]);
	AddLinearRowJacobian(desc, p0, projectedPoint, matrix1[2]);
	AddAngularRowJacobian(desc, matrix1.m_up, angle0);
	AddAngularRowJacobian(desc, matrix1.m_right, angle1);
	
	// add suspension spring damper row
	AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1.m_up);
	SetMassSpringDamperAcceleration(desc, m_info.m_regularizer, m_info.m_springK, m_info.m_damperC);

	// set tire rotation axel joint, break or load transfer
	//AddAngularRowJacobian(desc, matrix1.m_front, dFloat32(0.0f));
	//const dVector tireOmega(m_body0->GetOmega());
	//const dVector chassisOmega(m_body1->GetOmega());
	//dVector relOmega(tireOmega - chassisOmega);
	//dFloat32 rpm = relOmega.DotProduct(matrix1.m_front).GetScalar();
	//SetMotorAcceleration(desc, -rpm * desc.m_invTimestep);
	if (m_brakeTorque > dFloat32(0.0f))
	{
		AddAngularRowJacobian(desc, matrix1.m_front, dFloat32(0.0f));
		const dVector tireOmega(m_body0->GetOmega());
		const dVector chassisOmega(m_body1->GetOmega());
		dVector relOmega(tireOmega - chassisOmega);
		dFloat32 rpm = relOmega.DotProduct(matrix1.m_front).GetScalar();
		SetMotorAcceleration(desc, -rpm * desc.m_invTimestep);

		SetLowerFriction(desc, -m_brakeTorque);
		SetHighFriction(desc, m_brakeTorque);
	}
	//else
	//{
	//	if (xxxxx < 2)
	//	{
	//		AddAngularRowJacobian(desc, matrix1.m_front, dFloat32(0.0f));
	//		const dVector tireOmega(m_body0->GetOmega());
	//		const dVector chassisOmega(m_body1->GetOmega());
	//		dVector relOmega(tireOmega - chassisOmega);
	//		dFloat32 rpm = relOmega.DotProduct(matrix1.m_front).GetScalar();
	//		SetMotorAcceleration(desc, -rpm * desc.m_invTimestep);
	//
	//		SetLowerFriction(desc, -1500.0f);
	//		SetHighFriction(desc, 1500.0f);
	//	}
	//}

	// add suspension limits alone the vertical axis 
	const dFloat32 x = m_posit + m_speed * desc.m_timestep;
	if (x < m_info.m_minLimit)
	{
		AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1.m_up);
		SetLowerFriction(desc, dFloat32(0.0f));
		const dFloat32 stopAccel = GetMotorZeroAcceleration(desc);
		SetMotorAcceleration(desc, stopAccel);
	}
	else if (x > m_info.m_maxLimit)
	{
		AddLinearRowJacobian(desc, matrix0.m_posit, matrix1.m_posit, matrix1.m_up);
		SetHighFriction(desc, dFloat32(0.0f));
		const dFloat32 stopAccel = GetMotorZeroAcceleration(desc);
		SetMotorAcceleration(desc, stopAccel);
	}
}
