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

#ifndef __D_JOINT_SLIDER_H__
#define __D_JOINT_SLIDER_H__

#include "ndNewtonStdafx.h"
#include "ndJointBilateralConstraint.h"

#define D_SLIDER_PENETRATION_LIMIT			dFloat32 (0.2f) 
#define D_SLIDER_PENETRATION_RECOVERY_SPEED dFloat32 (0.1f) 

class ndJointSlider: public ndJointBilateralConstraint
{
	public:
	D_CLASS_RELECTION(ndJointSlider);
	D_NEWTON_API ndJointSlider(const dMatrix& pinAndPivotFrame, ndBodyKinematic* const child, ndBodyKinematic* const parent);
	D_NEWTON_API virtual ~ndJointSlider();

	D_NEWTON_API dFloat32 GetSpeed() const;
	D_NEWTON_API dFloat32 GetPosit() const;
	D_NEWTON_API dFloat32 GetFriction() const;

	D_NEWTON_API void SetFriction(dFloat32 friction);
	D_NEWTON_API void EnableLimits(bool state, dFloat32 minLimit, dFloat32 maxLimit);
	D_NEWTON_API void SetAsSpringDamper(bool state, dFloat32 regularizer, dFloat32 spring, dFloat32 damper);

	private:
	void SubmitConstraintLimits(ndConstraintDescritor& desc, const dMatrix& matrix0, const dMatrix& matrix1);
	void SubmitConstraintLimitSpringDamper(ndConstraintDescritor& desc, const dMatrix& matrix0, const dMatrix& matrix1);

	protected:
	D_NEWTON_API void JacobianDerivative(ndConstraintDescritor& desc);

	dFloat32 m_posit;
	dFloat32 m_speed;
	dFloat32 m_springK;
	dFloat32 m_damperC;
	dFloat32 m_minLimit;
	dFloat32 m_maxLimit;
	dFloat32 m_friction;
	dFloat32 m_springDamperRegularizer;

	bool m_hasLimits;
	bool m_isSpringDamper;
};

#endif 

