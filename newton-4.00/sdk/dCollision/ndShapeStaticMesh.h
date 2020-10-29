/* Copyright (c) <2003-2019> <Julio Jerez, Newton Game Dynamics>
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

#ifndef __D_COLLISION_MESH_H__
#define __D_COLLISION_MESH_H__

#include "ndCollisionStdafx.h"
#include "ndShape.h"

class ndBodyKinematic;
class ndShapeInstance;
class ndContactSolver;

#define D_MAX_COLLIDING_FACES		512
#define D_MAX_COLLIDING_INDICES		(D_MAX_COLLIDING_FACES * (4 * 2 + 3))


class ndShapeStaticMesh;
typedef void (*dgCollisionMeshCollisionCallback) (const ndBodyKinematic* const bodyWithTreeCollision, const ndBodyKinematic* const body, dInt32 faceID, 
												  dInt32 vertexCount, const dFloat32* const vertex, dInt32 vertexStrideInBytes); 

D_MSV_NEWTON_ALIGN_32 
class ndPolygonMeshDesc: public dFastAabbInfo
{
	public:
	class dgMesh
	{
		public:
		dInt32 m_globalFaceIndexCount[D_MAX_COLLIDING_FACES];
		dInt32 m_globalFaceIndexStart[D_MAX_COLLIDING_FACES];
		dFloat32 m_globalHitDistance[D_MAX_COLLIDING_FACES];
	};

	// colliding box in polygonSoup local space
	ndPolygonMeshDesc()
		:dFastAabbInfo()
		,m_boxDistanceTravelInMeshSpace(dFloat32 (0.0f))
		,m_maxT(dFloat32 (1.0f))
		,m_doContinuesCollisionTest(false)
	{
	}

	ndPolygonMeshDesc(ndContactSolver& proxy, void* const userData);

	void SetDistanceTravel (const dVector& distanceInGlobalSpace)
	{
		const dMatrix& soupMatrix = m_polySoupInstance->GetGlobalMatrix();
		m_boxDistanceTravelInMeshSpace = m_polySoupInstance->GetInvScale() * soupMatrix.UnrotateVector(distanceInGlobalSpace * m_convexInstance->GetInvScale());
		if (m_boxDistanceTravelInMeshSpace.DotProduct(m_boxDistanceTravelInMeshSpace).GetScalar() < dFloat32 (1.0e-2f)) {
			m_doContinuesCollisionTest = false;
		}
	}

	dInt32 GetFaceIndexCount(dInt32 indexCount) const
	{
		return indexCount * 2 + 3;
	}

	const dInt32* GetAdjacentFaceEdgeNormalArray(const dInt32* const faceIndexArray, dInt32 indexCount) const
	{
		return &faceIndexArray[indexCount + 2];
	}


	dInt32 GetNormalIndex(const dInt32* const faceIndexArray, dInt32 indexCount) const
	{
		return faceIndexArray[indexCount + 1];
	}

	dInt32 GetFaceId(const dInt32* const faceIndexArray, dInt32 indexCount) const
	{
		return faceIndexArray[indexCount];
	}

	dFloat32 GetFaceSize(const dInt32* const faceIndexArray, dInt32 indexCount) const
	{
		dInt32 size = faceIndexArray[indexCount * 2 + 2];
		return dFloat32 ((size >= 1) ? size : dFloat32 (1.0f));
	}

	dFloat32 GetSeparetionDistance() const
	{
		return m_separationDistance[0] * m_polySoupInstance->GetScale().GetScalar();
	}

	void SortFaceArray ();

	dVector m_boxDistanceTravelInMeshSpace;
	dInt32 m_threadNumber;
	dInt32 m_faceCount;
	dInt32 m_vertexStrideInBytes;
	dFloat32 m_skinThickness;
	void* m_userData;
	//ndBodyKinematic *m_objBody;
	//ndBodyKinematic *m_polySoupBody;
	ndShapeInstance* m_convexInstance;
	ndShapeInstance* m_polySoupInstance;
	dFloat32* m_vertex;
	dInt32* m_faceIndexCount;
	dInt32* m_faceVertexIndex;

	// private data;
	dInt32* m_faceIndexStart;
	dFloat32* m_hitDistance;
	const ndShapeStaticMesh* m_me;
	dInt32 m_globalIndexCount;
	dFloat32 m_maxT;
	bool m_doContinuesCollisionTest;
	dInt32 m_globalFaceVertexIndex[D_MAX_COLLIDING_INDICES];
	dgMesh m_meshData;
} D_GCC_NEWTON_ALIGN_32;

#if 0
D_MSV_NEWTON_ALIGN_32 
class dgCollisionMeshRayHitDesc
{
	public:
	dgCollisionMeshRayHitDesc ()
		:m_matrix (dGetIdentityMatrix())
	{
	}

	dVector m_localP0; 
	dVector m_localP1; 
	dVector m_normal;
	dUnsigned64 m_userId;
	void*  m_userData;
	void*  m_altenateUserData;
	dMatrix m_matrix;
}D_GCC_NEWTON_ALIGN_32;
#endif

class ndShapeStaticMesh: public ndShape
{
	public:
	D_COLLISION_API ndShapeStaticMesh(ndShapeID id);
	D_COLLISION_API virtual ~ndShapeStaticMesh();

	virtual void GetCollidingFaces(ndPolygonMeshDesc* const data) const = 0;
	protected:
	virtual dFloat32 GetVolume() const;
	virtual dFloat32 GetBoxMinRadius() const;
	virtual dFloat32 GetBoxMaxRadius() const;
	virtual ndShapeStaticMesh* GetAsShapeStaticMeshShape();
	virtual dVector SupportVertex(const dVector& dir, dInt32* const vertexIndex) const;
	virtual dVector SupportVertexSpecial(const dVector& dir, dFloat32 skinThickness, dInt32* const vertexIndex) const;
	virtual dVector SupportVertexSpecialProjectPoint(const dVector& point, const dVector& dir) const;
	virtual dInt32 CalculatePlaneIntersection(const dVector& normal, const dVector& point, dVector* const contactsOut) const;
	virtual dVector CalculateVolumeIntegral(const dMatrix& globalMatrix, const dVector& plane, const ndShapeInstance& parentScale) const;

	D_COLLISION_API virtual void CalcAABB(const dMatrix& matrix, dVector& p0, dVector& p1) const;
	D_COLLISION_API dInt32 CalculatePlaneIntersection(const dFloat32* const vertex, const dInt32* const index, dInt32 indexCount, dInt32 strideInFloat, const dPlane& localPlane, dVector* const contactsOut) const;

	D_MSV_NEWTON_ALIGN_32 
	class ndMeshVertexListIndexList
	{
		public:
		dInt32* m_indexList;
		dInt32* m_userDataList;
		dFloat32* m_veterxArray;
		dInt32 m_triangleCount; 
		dInt32 m_maxIndexCount;
		dInt32 m_vertexCount;
		dInt32 m_vertexStrideInBytes;
	} D_GCC_NEWTON_ALIGN_32;

#if 0
	ndShapeStaticMesh (dgWorld* const world, dgDeserialize deserialization, void* const userData, dInt32 revisionNumber);
	
	virtual void GetVertexListIndexList (const dVector& p0, const dVector& p1, ndMeshVertexListIndexList &data) const = 0;
	void SetDebugCollisionCallback (dgCollisionMeshCollisionCallback debugCallback);
	dgCollisionMeshCollisionCallback GetDebugCollisionCallback() const { return m_debugCallback;} 

	protected:
	virtual void SetCollisionBBox (const dVector& p0, const dVector& p1);

	private:
	virtual dInt32 CalculateSignature () const;
	virtual void DebugCollision  (const dMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const;

	virtual void GetCollisionInfo(dgCollisionInfo* const info) const;
	virtual void Serialize(dgSerialize callback, void* const userData) const;


#ifdef DG_DEBUG_AABB
	dVector BoxSupportMapping  (const dVector& dir) const;
#endif

	protected:
	dgCollisionMeshCollisionCallback m_debugCallback;
	friend class dgWorld;
	friend class ndShapeInstance;
#endif
};

inline dFloat32 ndShapeStaticMesh::GetVolume() const
{
	return dFloat32(0.0f);
}

inline dFloat32 ndShapeStaticMesh::GetBoxMinRadius() const
{
	return dFloat32(0.0f);
}

inline dFloat32 ndShapeStaticMesh::GetBoxMaxRadius() const
{
	return dFloat32(0.0f);
}

inline dVector ndShapeStaticMesh::SupportVertex(const dVector& dir, dInt32* const vertexIndex) const
{
	dAssert(0);
	return dVector::m_zero;
}

inline dVector ndShapeStaticMesh::SupportVertexSpecial(const dVector& dir, dFloat32 skinThickness, dInt32* const vertexIndex) const
{
	dAssert(0);
	return SupportVertex(dir, vertexIndex);
}

inline dVector ndShapeStaticMesh::SupportVertexSpecialProjectPoint(const dVector& point, const dVector& dir) const
{ 
	return point; 
}

inline dInt32 ndShapeStaticMesh::CalculatePlaneIntersection(const dVector& normal, const dVector& point, dVector* const contactsOut) const
{
	return 0;
}

inline dVector ndShapeStaticMesh::CalculateVolumeIntegral(const dMatrix& globalMatrix, const dVector& plane, const ndShapeInstance& parentScale) const
{
	return dVector::m_zero;
}

inline ndShapeStaticMesh* ndShapeStaticMesh::GetAsShapeStaticMeshShape()
{ 
	return this; 
}

#endif 



