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

#include "ndSandboxStdafx.h"
#include "ndTargaToOpenGl.h"
#include "ndDemoMesh.h"
#include "ndLoadFbxMesh.h"
#include "ndDemoSkinMesh.h"
#include "ndPhysicsUtils.h"
#include "ndPhysicsWorld.h"
#include "ndDemoEntityManager.h"
#include "ndAnimationSequence.h"

using namespace ofbx;

#define D_ANIM_BASE_FREQ dFloat32 (30.0f)

fbxDemoEntity::fbxDemoEntity(ndDemoEntity* const parent)
	:ndDemoEntity(dGetIdentityMatrix(), parent)
	,m_fbxMeshEffect(nullptr)
{
}

fbxDemoEntity::fbxDemoEntity(const fbxDemoEntity& source)
	:ndDemoEntity(source)
	,m_fbxMeshEffect(nullptr)
{
}

fbxDemoEntity::~fbxDemoEntity()
{
	if (m_fbxMeshEffect)
	{
		delete m_fbxMeshEffect;
	}
}

ndDemoEntity* fbxDemoEntity::CreateClone() const
{
	return new fbxDemoEntity(*this);
}

void fbxDemoEntity::CleanIntermediate()
{
	if (m_fbxMeshEffect)
	{
		delete m_fbxMeshEffect;
		m_fbxMeshEffect = nullptr;
	}

	for (fbxDemoEntity* child = (fbxDemoEntity*)GetChild(); child; child = (fbxDemoEntity*)child->GetSibling())
	{
		child->CleanIntermediate();
	}
}

void fbxDemoEntity::BuildRenderMeshes(ndDemoEntityManager* const scene)
{
	dInt32 stack = 1;
	fbxDemoEntity* entBuffer[1024];
	entBuffer[0] = this;
	while (stack)
	{
		stack--;
		fbxDemoEntity* const ent = entBuffer[stack];
	
		if (ent->m_fbxMeshEffect)
		{
			ndDemoMeshInterface* mesh;
			if (!ent->m_fbxMeshEffect->GetCluster().GetCount())
			{
				mesh = new ndDemoMesh(ent->GetName().GetStr(), ent->m_fbxMeshEffect, scene->GetShaderCache());
			}
			else
			{
				mesh = new ndDemoSkinMesh(ent, ent->m_fbxMeshEffect, scene->GetShaderCache());
			}
			ent->SetMesh(mesh, ent->GetMeshMatrix());
			mesh->Release();

			if ((ent->GetName().Find("hidden") >= 0) || (ent->GetName().Find("Hidden") >= 0))
			{
				mesh->m_isVisible = false;
			}
		}
	
		for (fbxDemoEntity* child = (fbxDemoEntity*)ent->GetChild(); child; child = (fbxDemoEntity*)child->GetSibling())
		{
			entBuffer[stack] = child;
			stack++;
		}
	}
}

void fbxDemoEntity::ApplyTransform(const dMatrix& transform)
{
	dInt32 stack = 1;
	fbxDemoEntity* entBuffer[1024];
	entBuffer[0] = this;
	dMatrix invTransform(transform.Inverse4x4());
	while (stack)
	{
		stack--;
		fbxDemoEntity* const ent = entBuffer[stack];

		dMatrix entMatrix(invTransform * ent->GetRenderMatrix() * transform);
		ent->SetRenderMatrix(entMatrix);

		dQuaternion rot(entMatrix);
		ent->SetMatrix(rot, entMatrix.m_posit);
		ent->SetMatrix(rot, entMatrix.m_posit);

		if (ent->m_fbxMeshEffect)
		{
			dMatrix meshMatrix(invTransform * ent->GetMeshMatrix() * transform);
			ent->SetMeshMatrix(meshMatrix);
			ent->m_fbxMeshEffect->ApplyTransform(transform);
		}

		for (fbxDemoEntity* child = (fbxDemoEntity*)ent->GetChild(); child; child = (fbxDemoEntity*)child->GetSibling())
		{
			entBuffer[stack] = child;
			stack++;
		}
	}
}

class fbxGlobalNodeMap : public dTree<fbxDemoEntity*, const ofbx::Object*>
{
};

class fbxImportStackData
{
	public:
	fbxImportStackData()
	{
	}

	fbxImportStackData(const ofbx::Object* const fbxNode, ndDemoEntity* const parentNode)
		:m_fbxNode(fbxNode)
		,m_parentNode(parentNode)
	{
	}

	const ofbx::Object* m_fbxNode;
	ndDemoEntity* m_parentNode;
};

static dMatrix GetCoordinateSystemMatrix(ofbx::IScene* const fbxScene)
{
	const ofbx::GlobalSettings* const globalSettings = fbxScene->getGlobalSettings();

	dMatrix convertMatrix(dGetIdentityMatrix());

	dFloat32 scaleFactor = globalSettings->UnitScaleFactor;
	convertMatrix[0][0] = dFloat32(scaleFactor / 100.0f);
	convertMatrix[1][1] = dFloat32(scaleFactor / 100.0f);
	convertMatrix[2][2] = dFloat32(scaleFactor / 100.0f);

	dMatrix axisMatrix(dGetZeroMatrix());
	axisMatrix.m_up[globalSettings->UpAxis] = dFloat32(globalSettings->UpAxisSign);
	axisMatrix.m_front[globalSettings->FrontAxis] = dFloat32(globalSettings->FrontAxisSign);
	axisMatrix.m_right = axisMatrix.m_front.CrossProduct(axisMatrix.m_up);
	axisMatrix = axisMatrix.Transpose();
	
	convertMatrix = axisMatrix * convertMatrix;

	return convertMatrix;
}

static dInt32 GetChildrenNodes(const ofbx::Object* const node, ofbx::Object** buffer)
{
	dInt32 count = 0;
	dInt32 index = 0;
	while (ofbx::Object* child = node->resolveObjectLink(index))
	{
		if (child->isNode())
		{
			buffer[count] = child;
			count++;
			dAssert(count < 1024);
		}
		index++;
	}
	return count;
}

static dMatrix ofbxMatrix2dMatrix(const ofbx::Matrix& fbxMatrix)
{
	dMatrix matrix;
	for (dInt32 i = 0; i < 4; i++)
	{
		for (dInt32 j = 0; j < 4; j++)
		{
			matrix[i][j] = dFloat32 (fbxMatrix.m[i * 4 + j]);
		}
	}
	return matrix;
}

static fbxDemoEntity* LoadHierarchy(ofbx::IScene* const fbxScene, fbxGlobalNodeMap& nodeMap)
{
	dInt32 stack = 0;
	ofbx::Object* buffer[1024];
	fbxImportStackData nodeStack[1024];
	const ofbx::Object* const rootNode = fbxScene->getRoot();
	dAssert(rootNode);
	stack = GetChildrenNodes(rootNode, buffer);

	fbxDemoEntity* rootEntity = nullptr;
	if (stack > 1)
	{
		rootEntity = new fbxDemoEntity(nullptr);
		rootEntity->SetName("dommyRoot");
	}

	for (dInt32 i = 0; i < stack; i++)
	{
		ofbx::Object* const child = buffer[stack - i - 1];
		nodeStack[i] = fbxImportStackData(child, rootEntity);
	}

	while (stack)
	{
		stack--;
		fbxImportStackData data(nodeStack[stack]);

		fbxDemoEntity* const node = new fbxDemoEntity(data.m_parentNode);
		if (!rootEntity)
		{
			rootEntity = node;
		}

		dMatrix localMatrix(ofbxMatrix2dMatrix(data.m_fbxNode->getLocalTransform()));

		node->SetName(data.m_fbxNode->name);
		node->SetRenderMatrix(localMatrix);

		nodeMap.Insert(node, data.m_fbxNode);
		const dInt32 count = GetChildrenNodes(data.m_fbxNode, buffer);
		for (dInt32 i = 0; i < count; i++) 
		{
			ofbx::Object* const child = buffer[count - i - 1];
			nodeStack[stack] = fbxImportStackData(child, node);
			stack++;
			dAssert(stack < dInt32(sizeof(nodeStack) / sizeof(nodeStack[0])));
		}
	}
	return rootEntity;
}

static void ImportMaterials(const ofbx::Mesh* const fbxMesh, ndMeshEffect* const mesh)
{
	dArray<ndMeshEffect::dMaterial>& materialArray = mesh->GetMaterials();
	
	dInt32 materialCount = fbxMesh->getMaterialCount();
	if (materialCount == 0)
	{
		ndMeshEffect::dMaterial defaultMaterial;
		materialArray.PushBack(defaultMaterial);
	}
	else
	{
		for (dInt32 i = 0; i < materialCount; i++)
		{
			ndMeshEffect::dMaterial material;
			const ofbx::Material* const fbxMaterial = fbxMesh->getMaterial(i);
			dAssert(fbxMaterial);

			ofbx::Color color = fbxMaterial->getDiffuseColor();
			material.m_diffuse = dVector(color.r, color.g, color.b, 1.0f);
			
			color = fbxMaterial->getAmbientColor();
			material.m_ambient = dVector(color.r, color.g, color.b, 1.0f);
			
			color = fbxMaterial->getSpecularColor();
			material.m_specular = dVector(color.r, color.g, color.b, 1.0f);
			
			material.m_opacity = dFloat32(fbxMaterial->getOpacityFactor());
			material.m_shiness = dFloat32(fbxMaterial->getShininess());
			
			const ofbx::Texture* const texture = fbxMaterial->getTexture(ofbx::Texture::DIFFUSE);
			if (texture)
			{
				char textName[1024];
				ofbx::DataView dataView = texture->getRelativeFileName();
				dataView.toString(textName);
				char* namePtr = strrchr(textName, '\\');
				if (!namePtr)
				{
					namePtr = strrchr(textName, '/');
				}
				if (namePtr)
				{
					namePtr++;
				}
				else
				{
					namePtr = textName;
				}
				strncpy(material.m_textureName, namePtr, sizeof(material.m_textureName));
			}
 			else
			{
				strcpy(material.m_textureName, "default.tga");
			}
			materialArray.PushBack(material);
		}
	}
}

static void ImportMeshNode(ofbx::Object* const fbxNode, fbxGlobalNodeMap& nodeMap)
{
	const ofbx::Mesh* const fbxMesh = (ofbx::Mesh*)fbxNode;

	dAssert(nodeMap.Find(fbxNode));
	fbxDemoEntity* const entity = nodeMap.Find(fbxNode)->GetInfo();
	ndMeshEffect* const mesh = new ndMeshEffect();
	mesh->SetName(fbxMesh->name);

	dMatrix pivotMatrix(ofbxMatrix2dMatrix(fbxMesh->getGeometricMatrix()));
	entity->SetMeshMatrix(pivotMatrix);
	entity->m_fbxMeshEffect = mesh;

	const ofbx::Geometry* const geom = fbxMesh->getGeometry();
	const ofbx::Vec3* const vertices = geom->getVertices();
	dInt32 indexCount = geom->getIndexCount();
	dInt32* const indexArray = new dInt32 [indexCount];
	memcpy(indexArray, geom->getFaceIndices(), indexCount * sizeof(dInt32));

	dInt32 faceCount = 0;
	for (dInt32 i = 0; i < indexCount; i++)
	{
		if (indexArray[i] < 0)
		{
			faceCount++;
		}
	}

	dInt32* const faceIndexArray = new dInt32[faceCount];
	dInt32* const faceMaterialArray = new dInt32[faceCount];

	ImportMaterials(fbxMesh, mesh);

	dInt32 count = 0;
	dInt32 faceIndex = 0;
	const dArray<ndMeshEffect::dMaterial>& materialArray = mesh->GetMaterials();
	dInt32 materialId = (materialArray.GetCount() <= 1) ? 0 : -1;
	for (dInt32 i = 0; i < indexCount; i++)
	{
		count++;
		if (indexArray[i] < 0)
		{
			indexArray[i] = -indexArray[i] - 1;
			faceIndexArray[faceIndex] = count;
			if (materialId == 0)
			{
				faceMaterialArray[faceIndex] = materialId;
			}
			else
			{
				dInt32 fbxMatIndex = geom->getMaterials()[faceIndex];
				faceMaterialArray[faceIndex] = fbxMatIndex;
			}
			count = 0;
			faceIndex++;
		}
	}

	ndMeshEffect::dMeshVertexFormat format;
	format.m_vertex.m_data = &vertices[0].x;
	format.m_vertex.m_indexList = indexArray;
	format.m_vertex.m_strideInBytes = sizeof(ofbx::Vec3);

	format.m_faceCount = faceCount;
	format.m_faceIndexCount = faceIndexArray;
	format.m_faceMaterial = faceMaterialArray;
	
	dArray<dVector> normalArray;
	if (geom->getNormals())
	{
		normalArray.Resize(indexCount);
		normalArray.SetCount(indexCount);
		const ofbx::Vec3* const normals = geom->getNormals();
		for (dInt32 i = 0; i < indexCount; ++i)
		{
			ofbx::Vec3 n = normals[i];
			normalArray[i] = dVector(dFloat32(n.x), dFloat32(n.y), dFloat32(n.z), dFloat32(0.0f));
		}

		format.m_normal.m_data = &normalArray[0].m_x;
		format.m_normal.m_indexList = indexArray;
		format.m_normal.m_strideInBytes = sizeof(dVector);
	}

	dArray<dVector> uvArray;
	if (geom->getUVs())
	{
		uvArray.Resize(indexCount);
		uvArray.SetCount(indexCount);
		const ofbx::Vec2* const uv = geom->getUVs();
		for (dInt32 i = 0; i < indexCount; ++i)
		{
			ofbx::Vec2 n = uv[i];
			uvArray[i] = dVector(dFloat32(n.x), dFloat32(n.y), dFloat32(0.0f), dFloat32(0.0f));
		}
		format.m_uv0.m_data = &uvArray[0].m_x;
		format.m_uv0.m_indexList = indexArray;
		format.m_uv0.m_strideInBytes = sizeof(dVector);
	}

	// import skin if there is any
	if (geom->getSkin())
	{
		const ofbx::Skin* const skin = geom->getSkin();
		dInt32 clusterCount = skin->getClusterCount();

		dTree <const ofbx::Cluster*, const Object*> clusterBoneMap;
		for (dInt32 i = 0; i < clusterCount; i++) 
		{
			const ofbx::Cluster* const cluster = skin->getCluster(i);
			const ofbx::Object* const link = cluster->getLink();
			clusterBoneMap.Insert(cluster, link);
		}

		for (int i = 0; i < clusterCount; i++)
		{
			const ofbx::Cluster* const fbxCluster = skin->getCluster(i);
			const ofbx::Object* const fbxBone = fbxCluster->getLink();
			if (nodeMap.Find(fbxBone))
			{
				ndMeshEffect::dVertexCluster* const cluster = mesh->CreateCluster(fbxBone->name);

				dAssert(fbxCluster->getIndicesCount() == fbxCluster->getWeightsCount());
				dInt32 clusterIndexCount = fbxCluster->getIndicesCount();
				const dInt32* const indices = fbxCluster->getIndices();
				const double* const weights = fbxCluster->getWeights();
				for (dInt32 j = 0; j < clusterIndexCount; j++)
				{
					cluster->m_vertexIndex.PushBack(indices[j]);
					cluster->m_vertexWeigh.PushBack(dFloat32 (weights[j]));
				}
			}
		}
	}

	mesh->BuildFromIndexList(&format);
	//mesh->RepairTJoints();

	delete[] faceMaterialArray;
	delete[] faceIndexArray;
	delete[] indexArray;
}

static fbxDemoEntity* FbxToEntity(ofbx::IScene* const fbxScene)
{
	fbxGlobalNodeMap nodeMap;
	fbxDemoEntity* const entity = LoadHierarchy(fbxScene, nodeMap);

	fbxGlobalNodeMap::Iterator iter(nodeMap);
	for (iter.Begin(); iter; iter++) 
	{
		ofbx::Object* const fbxNode = (ofbx::Object*)iter.GetKey();
		ofbx::Object::Type type = fbxNode->getType();
		switch (type)
		{
			case ofbx::Object::Type::MESH:
			{
				ImportMeshNode(fbxNode, nodeMap);
				break;
			}

			case ofbx::Object::Type::NULL_NODE:
			{
				break;
			}

			case ofbx::Object::Type::LIMB_NODE:
			{
				break;
			}

			//case FbxNodeAttribute::eLine:
			//{
			//	ImportLineShape(fbxScene, ngdScene, fbxNode, node, meshCache, materialCache, textureCache, usedMaterials);
			//	break;
			//}
			//case FbxNodeAttribute::eNurbsCurve:
			//{
			//	ImportNurbCurveShape(fbxScene, ngdScene, fbxNode, node, meshCache, materialCache, textureCache, usedMaterials);
			//	break;
			//}
			//case FbxNodeAttribute::eMarker:
			//case FbxNodeAttribute::eNurbs:
			//case FbxNodeAttribute::ePatch:
			//case FbxNodeAttribute::eCamera:
			//case FbxNodeAttribute::eCameraStereo:
			//case FbxNodeAttribute::eCameraSwitcher:
			//case FbxNodeAttribute::eLight:
			//case FbxNodeAttribute::eOpticalReference:
			//case FbxNodeAttribute::eOpticalMarker:
			//case FbxNodeAttribute::eTrimNurbsSurface:
			//case FbxNodeAttribute::eBoundary:
			//case FbxNodeAttribute::eNurbsSurface:
			//case FbxNodeAttribute::eShape:
			//case FbxNodeAttribute::eLODGroup:
			//case FbxNodeAttribute::eSubDiv:
			//case FbxNodeAttribute::eCachedEffect:
			//case FbxNodeAttribute::eUnknown:
			default:
				dAssert(0);
				break;
		}
	}

	return entity;
}

static void FreezeScale(fbxDemoEntity* const entity)
{
	dInt32 stack = 1;
	fbxDemoEntity* entBuffer[1024];
	dMatrix parentMatrix[1024];
	entBuffer[0] = entity;
	parentMatrix[0] = dGetIdentityMatrix();
	while (stack)
	{
		stack--;
		dMatrix scaleMatrix(parentMatrix[stack]);
		fbxDemoEntity* const ent = entBuffer[stack];

		dMatrix transformMatrix;
		dMatrix stretchAxis;
		dVector scale;
		dMatrix matrix(ent->GetRenderMatrix() * scaleMatrix);
		matrix.PolarDecomposition(transformMatrix, scale, stretchAxis);
		ent->SetRenderMatrix(transformMatrix);
		scaleMatrix = dMatrix(dGetIdentityMatrix(), scale, stretchAxis);

		if (ent->m_fbxMeshEffect)
		{
			matrix = ent->GetMeshMatrix() * scaleMatrix;
			matrix.PolarDecomposition(transformMatrix, scale, stretchAxis);
			ent->SetMeshMatrix(transformMatrix);
			dMatrix meshMatrix(dGetIdentityMatrix(), scale, stretchAxis);
			ent->m_fbxMeshEffect->ApplyTransform(meshMatrix);
		}

		for (fbxDemoEntity* child = (fbxDemoEntity*)ent->GetChild(); child; child = (fbxDemoEntity*)child->GetSibling())
		{
			entBuffer[stack] = child;
			parentMatrix[stack] = scaleMatrix;
			stack++;
		}
	}
}

fbxDemoEntity* LoadFbxMesh(const char* const meshName)
{
	char outPathName[1024];
	dGetWorkingFileName(meshName, outPathName);

	FILE* fp = fopen(outPathName, "rb");
	if (!fp)
	{
		dAssert(0);
		return nullptr;
	}

	size_t readBytes = 0;
	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	dArray<ofbx::u8> content;
	content.SetCount(file_size);
	readBytes = fread(&content[0], 1, file_size, fp);
	ofbx::IScene* const fbxScene = ofbx::load(&content[0], file_size, (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);

	const dMatrix convertMatrix(GetCoordinateSystemMatrix(fbxScene));
	fbxDemoEntity* const entity = FbxToEntity(fbxScene);
	FreezeScale(entity);
	entity->ApplyTransform(convertMatrix);

	fbxScene->destroy();
	return entity;
}

class dFbxAnimationTrack
{
	public:
	class dCurveValue
	{
		public:
		dFloat32 m_x;
		dFloat32 m_y;
		dFloat32 m_z;
		dFloat32 m_time;
	};

	class dCurve : public dList <dCurveValue>
	{
		public:
		dCurve()
			:dList <dCurveValue>()
		{
		}

		dCurveValue Evaluate(dFloat32 t) const
		{
			for (dNode* ptr = GetFirst(); ptr->GetNext(); ptr = ptr->GetNext()) 
			{
				dCurveValue& info1 = ptr->GetNext()->GetInfo();
				if (info1.m_time >= t) 
				{
					dCurveValue& info0 = ptr->GetInfo();
					dCurveValue val;
					dFloat32 param = (t - info0.m_time) / (info1.m_time - info0.m_time);
					val.m_x = info0.m_x + (info1.m_x - info0.m_x) * param;
					val.m_y = info0.m_y + (info1.m_y - info0.m_y) * param;
					val.m_z = info0.m_z + (info1.m_z - info0.m_z) * param;
					val.m_time = info0.m_time + (info1.m_time - info0.m_time) * param;
					return val;
				}
			}
			dAssert(0);
			return dCurveValue();
		}
	};

	dFbxAnimationTrack()
	{
	}

	~dFbxAnimationTrack()
	{
	}

	//const dList<dCurveValue>& GetScales() const;
	//const dList<dCurveValue>& GetPositions() const;
	//const dList<dCurveValue>& GetRotations() const;

	void AddKeyframe(dFloat32 time, const dMatrix& matrix)
	{
		dVector scale;
		dVector euler0;
		dVector euler1;
		dMatrix transform;
		dMatrix eigenScaleAxis;
		matrix.PolarDecomposition(transform, scale, eigenScaleAxis);
		transform.CalcPitchYawRoll(euler0, euler1);

		AddScale(time, scale.m_x, scale.m_y, scale.m_z);
		AddPosition(time, matrix.m_posit.m_x, matrix.m_posit.m_y, matrix.m_posit.m_z);
		AddRotation(time, euler0.m_x, euler0.m_y, euler0.m_z);
	}

	dMatrix GetKeyframe(dFloat32 time) const
	{
		dCurveValue scale(m_scale.Evaluate(time));
		dCurveValue position(m_position.Evaluate(time));
		dCurveValue rotation(m_rotation.Evaluate(time));

		dMatrix scaleMatrix(dGetIdentityMatrix());
		scaleMatrix[0][0] = scale.m_x;
		scaleMatrix[1][1] = scale.m_y;
		scaleMatrix[2][2] = scale.m_z;
		dMatrix matrix(scaleMatrix * dPitchMatrix(rotation.m_x) * dYawMatrix(rotation.m_y) * dRollMatrix(rotation.m_z));
		matrix.m_posit = dVector(position.m_x, position.m_y, position.m_z, 1.0f);
		return matrix;
	}

	void OptimizeCurves()
	{
		if (m_scale.GetCount()) 
		{
			OptimizeCurve(m_scale);
		}
		if (m_position.GetCount()) 
		{
			OptimizeCurve(m_position);
		}
		if (m_rotation.GetCount()) 
		{
			for (dCurve::dNode* node = m_rotation.GetFirst(); node->GetNext(); node = node->GetNext()) 
			{
				const dCurveValue& value0 = node->GetInfo();
				dCurveValue& value1 = node->GetNext()->GetInfo();
				value1.m_x = FixAngleAlias(value0.m_x, value1.m_x);
				value1.m_y = FixAngleAlias(value0.m_y, value1.m_y);
				value1.m_z = FixAngleAlias(value0.m_z, value1.m_z);
				//dTrace(("%d %f %f %f\n", m_rotation.GetCount(), value0.m_x * dRadToDegree, value0.m_y * dRadToDegree, value0.m_z * dRadToDegree));
			}

			OptimizeCurve(m_rotation);
		}
	}

	void ApplyTransform(const dMatrix& transform)
	{
		dMatrix invert(transform.Inverse4x4());
		dCurve::dNode* scaleNode = m_scale.GetFirst();
		dCurve::dNode* positNode = m_position.GetFirst();
		for (dCurve::dNode* rotationNode = m_rotation.GetFirst(); rotationNode; rotationNode = rotationNode->GetNext()) 
		{
			dVector euler0;
			dVector euler1;

			dCurveValue& scaleValue = scaleNode->GetInfo();
			dCurveValue& positValue = positNode->GetInfo();
			dCurveValue& rotationValue = rotationNode->GetInfo();

			dMatrix scaleMatrix(dGetIdentityMatrix());
			scaleMatrix[0][0] = scaleValue.m_x;
			scaleMatrix[1][1] = scaleValue.m_y;
			scaleMatrix[2][2] = scaleValue.m_z;
			dMatrix m(scaleMatrix * dPitchMatrix(rotationValue.m_x) * dYawMatrix(rotationValue.m_y) * dRollMatrix(rotationValue.m_z));
			m.m_posit = dVector(positValue.m_x, positValue.m_y, positValue.m_z, 1.0f);
			dMatrix matrix(invert * m * transform);

			dVector scale;
			dMatrix output;
			dMatrix eigenScaleAxis;
			matrix.PolarDecomposition(output, scale, eigenScaleAxis);
			output.CalcPitchYawRoll(euler0, euler1);
			//dTrace(("%d %f %f %f\n", m_rotation.GetCount(), euler0.m_x * dRadToDegree, euler0.m_y * dRadToDegree, euler0.m_z * dRadToDegree));

			scaleValue.m_x = scale.m_x;
			scaleValue.m_y = scale.m_y;
			scaleValue.m_z = scale.m_z;

			rotationValue.m_x = euler0.m_x;
			rotationValue.m_y = euler0.m_y;
			rotationValue.m_z = euler0.m_z;

			positValue.m_x = output.m_posit.m_x;
			positValue.m_y = output.m_posit.m_y;
			positValue.m_z = output.m_posit.m_z;

			positNode = positNode->GetNext();
			scaleNode = scaleNode->GetNext();
		}
	}

	private:
	void AddScale(dFloat32 time, dFloat32 x, dFloat32 y, dFloat32 z)
	{
		dCurveValue& value = m_scale.Append()->GetInfo();
		value.m_x = x;
		value.m_y = y;
		value.m_z = z;
		value.m_time = time;
	}

	void AddPosition(dFloat32 time, dFloat32 x, dFloat32 y, dFloat32 z)
	{
		dCurveValue& value = m_position.Append()->GetInfo();
		value.m_x = x;
		value.m_y = y;
		value.m_z = z;
		value.m_time = time;
	}

	void AddRotation(dFloat32 time, dFloat32 x, dFloat32 y, dFloat32 z)
	{
		dCurveValue& value = m_rotation.Append()->GetInfo();
		value.m_x = x;
		value.m_y = y;
		value.m_z = z;
		value.m_time = time;
	}

	dFloat32 FixAngleAlias(dFloat32 angleA, dFloat32 angleB) const
	{
		dFloat32 sinA = dSin(angleA);
		dFloat32 cosA = dCos(angleA);
		dFloat32 sinB = dSin(angleB);
		dFloat32 cosB = dCos(angleB);

		dFloat32 num = sinB * cosA - cosB * sinA;
		dFloat32 den = cosA * cosB + sinA * sinB;
		angleB = angleA + dAtan2(num, den);
		return angleB;
	}

	dFloat32 Interpolate(dFloat32 x0, dFloat32 t0, dFloat32 x1, dFloat32 t1, dFloat32 t) const
	{
		return x0 + (x1 - x0) * (t - t0) / (t1 - t0);
	}

	void OptimizeCurve(dList<dCurveValue>& curve)
	{
		const dFloat32 tol = 5.0e-5f;
		const dFloat32 tol2 = tol * tol;
		for (dCurve::dNode* node0 = curve.GetFirst(); node0->GetNext(); node0 = node0->GetNext()) 
		{
			const dCurveValue& value0 = node0->GetInfo();
			for (dCurve::dNode* node1 = node0->GetNext()->GetNext(); node1; node1 = node1->GetNext()) 
			{
				const dCurveValue& value1 = node1->GetPrev()->GetInfo();
				const dCurveValue& value2 = node1->GetInfo();
				dVector p1(value1.m_x, value1.m_y, value1.m_z, dFloat32(0.0f));
				dVector p2(value2.m_x, value2.m_y, value2.m_z, dFloat32(0.0f));
		
				dFloat32 dist_x = value1.m_x - Interpolate(value0.m_x, value0.m_time, value2.m_x, value2.m_time, value1.m_time);
				dFloat32 dist_y = value1.m_y - Interpolate(value0.m_y, value0.m_time, value2.m_y, value2.m_time, value1.m_time);
				dFloat32 dist_z = value1.m_z - Interpolate(value0.m_z, value0.m_time, value2.m_z, value2.m_time, value1.m_time);
		
				dVector err(dist_x, dist_y, dist_z, 0.0f);
				dFloat32 mag2 = err.DotProduct(err).GetScalar();
				if (mag2 > tol2) 
				{
					break;
				}
				curve.Remove(node1->GetPrev());
			}
		}
	}

	dCurve m_scale;
	dCurve m_position;
	dCurve m_rotation;

	friend class dFbxAnimation;
};

class dFbxAnimation : public dTree <dFbxAnimationTrack, dString>
{
	public:
	dFbxAnimation()
		:dTree <dFbxAnimationTrack, dString>()
		,m_length(0.0f)
		,m_timestep(0.0f)
		,m_framesCount(0)
	{
	}

	dFbxAnimation(const dFbxAnimation& source, fbxDemoEntity* const entity, const dMatrix& matrix)
		:dTree <dFbxAnimationTrack, dString>()
		,m_length(source.m_length)
		,m_timestep(source.m_timestep)
		,m_framesCount(source.m_framesCount)
	{
		Iterator iter(source);
		for (iter.Begin(); iter; iter++)
		{
			Insert(iter.GetKey());
		}

		FreezeScale(entity, source);
		ApplyTransform(matrix);
		OptimizeCurves();
	}

	void OptimizeCurves()
	{
		Iterator iter(*this);
		for (iter.Begin(); iter; iter++)
		{
			dFbxAnimationTrack& track = iter.GetNode()->GetInfo();
			track.OptimizeCurves();
		}
	}

	void FreezeScale(fbxDemoEntity* const entity, const dFbxAnimation& source)
	{
		dMatrix parentMatrixStack[1024];
		fbxDemoEntity* stackPool[1024];

		dFloat32 deltaTimeAcc = dFloat32 (0.0f);
		for (dInt32 i = 0; i < m_framesCount; i++)
		{
			for (fbxDemoEntity* node = (fbxDemoEntity*)entity->GetFirst(); node; node = (fbxDemoEntity*)node->GetNext())
			{
				const dFbxAnimation::dNode* const aniNode = source.Find(node->GetName());
				if (aniNode)
				{
					const dFbxAnimationTrack& track = aniNode->GetInfo();
					const dMatrix matrix(track.GetKeyframe(deltaTimeAcc));
					node->SetMeshMatrix(matrix);
				}
			}

			dInt32 stack = 1;
			stackPool[0] = entity;
			parentMatrixStack[0] = dGetIdentityMatrix();
			while (stack)
			{
				stack--;
				dMatrix parentMatrix(parentMatrixStack[stack]);
				fbxDemoEntity* const rootNode = stackPool[stack];

				dMatrix transform(rootNode->GetMeshMatrix() * parentMatrix);

				dMatrix matrix;
				dMatrix stretchAxis;
				dVector scale;
				transform.PolarDecomposition(matrix, scale, stretchAxis);

				dMatrix scaledAxis(dGetIdentityMatrix());
				scaledAxis[0][0] = scale[0];
				scaledAxis[1][1] = scale[1];
				scaledAxis[2][2] = scale[2];
				dMatrix newParentMatrix(stretchAxis * scaledAxis);

				dFbxAnimation::dNode* const aniNode = Find(rootNode->GetName());
				if (aniNode)
				{
					dFbxAnimationTrack& track = aniNode->GetInfo();
					track.AddKeyframe(deltaTimeAcc, matrix);
				}

				for (fbxDemoEntity* node = (fbxDemoEntity*)rootNode->GetChild(); node; node = (fbxDemoEntity*)node->GetSibling())
				{
					stackPool[stack] = node;
					parentMatrixStack[stack] = newParentMatrix;
					stack++;
				}
			}
			deltaTimeAcc += m_timestep;
		}
	}

	void ApplyTransform(const dMatrix& transform)
	{
		Iterator iter(*this);
		for (iter.Begin(); iter; iter++)
		{
			dFbxAnimationTrack& track = iter.GetNode()->GetInfo();
			track.ApplyTransform(transform);
		}
	}

	ndAnimationSequence* CreateSequence(const char* const name) const
	{
		ndAnimationSequence* const sequence = new ndAnimationSequence;
		sequence->SetName(name);
		sequence->m_period = m_length;

		Iterator iter(*this);
		for (iter.Begin(); iter; iter++)
		{
			const dFbxAnimationTrack& fbxTrack = iter.GetNode()->GetInfo();
			ndAnimationKeyFramesTrack* const track = sequence->AddTrack();

			track->m_name = iter.GetKey();
			//dTrace(("name: %s\n", track->m_name.GetStr()));

			const dFbxAnimationTrack::dCurve& position = fbxTrack.m_position;
			for (dFbxAnimationTrack::dCurve::dNode* node = position.GetFirst(); node; node = node->GetNext())
			{
				dFbxAnimationTrack::dCurveValue& keyFrame = node->GetInfo();
				track->m_position.m_time.PushBack(keyFrame.m_time);
				track->m_position.PushBack(dVector(keyFrame.m_x, keyFrame.m_y, keyFrame.m_z, dFloat32(1.0f)));
				//dTrace(("%f %f %f %f\n", keyFrame.m_time, keyFrame.m_x, keyFrame.m_y, keyFrame.m_z));
			}

			const dFbxAnimationTrack::dCurve& rotation = fbxTrack.m_rotation;
			for (dFbxAnimationTrack::dCurve::dNode* node = rotation.GetFirst(); node; node = node->GetNext())
			{
				dFbxAnimationTrack::dCurveValue& keyFrame = node->GetInfo();
				track->m_rotation.m_time.PushBack(keyFrame.m_time);
				const dMatrix transform(dPitchMatrix(keyFrame.m_x) * dYawMatrix(keyFrame.m_y) * dRollMatrix(keyFrame.m_z));
				const dQuaternion quat(transform);
				dAssert(quat.DotProduct(quat).GetScalar() > 0.999f);
				dAssert(quat.DotProduct(quat).GetScalar() < 1.001f);
				track->m_rotation.PushBack(quat);
				//dTrace(("%f %f %f %f %f\n", keyFrame.m_time, quat.m_x, quat.m_y, quat.m_z, quat.m_w));
			}
		}

		return sequence;
	}

	dFloat32 m_length;
	dFloat32 m_timestep;
	dInt32 m_framesCount;
};

static void LoadAnimationCurve(ofbx::IScene* const, const ofbx::Object* const bone, const ofbx::AnimationLayer* const animLayer, dFbxAnimation& animation)
{
	const ofbx::AnimationCurveNode* const scaleNode = animLayer->getCurveNode(*bone, "Lcl Scaling");
	const ofbx::AnimationCurveNode* const rotationNode = animLayer->getCurveNode(*bone, "Lcl Rotation");
	const ofbx::AnimationCurveNode* const translationNode = animLayer->getCurveNode(*bone, "Lcl Translation");

	dFbxAnimationTrack& track = animation.Insert(bone->name)->GetInfo();

	Vec3 scale;
	Vec3 rotation;
	Vec3 translation;

	dFloat32 timeAcc = 0.0f;
	dFloat32 timestep = animation.m_timestep;

	dVector scale1;
	dVector euler0;
	dVector euler1;
	dMatrix transform;
	dMatrix eigenScaleAxis;
	dMatrix boneMatrix(ofbxMatrix2dMatrix(bone->getLocalTransform()));
	boneMatrix.PolarDecomposition(transform, scale1, eigenScaleAxis);
	transform.CalcPitchYawRoll(euler0, euler1);
	for (dInt32 i = 0; i < animation.m_framesCount; i++)
	{
		scale.x = scale1.m_x;
		scale.y = scale1.m_y;
		scale.z = scale1.m_z;
		rotation.x = euler0.m_x;
		rotation.y = euler0.m_y;
		rotation.z = euler0.m_z;
		translation.x = transform.m_posit.m_x;
		translation.y = transform.m_posit.m_y;
		translation.z = transform.m_posit.m_z;

		if (scaleNode)
		{
			scale = scaleNode->getNodeLocalTransform(timeAcc);
		}
		if (rotationNode)
		{
			rotation = rotationNode->getNodeLocalTransform(timeAcc);
		}
		if (translationNode)
		{
			translation = translationNode->getNodeLocalTransform(timeAcc);
		}
		dMatrix matrix(ofbxMatrix2dMatrix(bone->evalLocal(translation, rotation, scale)));
		track.AddKeyframe(timeAcc, matrix);

		timeAcc += timestep;
	}
}

static void LoadAnimationLayer(ofbx::IScene* const fbxScene, const ofbx::AnimationLayer* const animLayer, dFbxAnimation& animation)
{
	dInt32 stack = 0;
	ofbx::Object* stackPool[1024];
	const ofbx::Object* const rootNode = fbxScene->getRoot();
	dAssert(rootNode);
	stack = GetChildrenNodes(rootNode, stackPool);

	const ofbx::TakeInfo* const animationInfo = fbxScene->getTakeInfo(0);
	//animation.m_length = dFloat32(animationInfo->local_time_to - animationInfo->local_time_from);

	dFloat32 period = dFloat32(animationInfo->local_time_to - animationInfo->local_time_from);
	dFloat32 framesFloat = period * D_ANIM_BASE_FREQ;

	dInt32 frames = dInt32(dFloor(framesFloat));
	dAssert(frames > 0);
	dFloat32 timestep = period / frames;

	animation.m_length = period;
	animation.m_timestep = timestep;
	animation.m_framesCount = frames;

	while (stack)
	{
		stack--;
		ofbx::Object* const bone = stackPool[stack];
		LoadAnimationCurve(fbxScene, bone, animLayer, animation);

		stack += GetChildrenNodes(bone, &stackPool[stack]);
		dAssert(stack < dInt32(sizeof(stackPool) / sizeof(stackPool[0]) - 64));
	}
}

static void LoadAnimation(ofbx::IScene* const fbxScene, dFbxAnimation& animation)
{
	dInt32 animationCount = fbxScene->getAnimationStackCount();
	for (int i = 0; i < animationCount; i++)
	{
		const ofbx::AnimationStack* const animStack = fbxScene->getAnimationStack(i);
		
		dInt32 layerCount = 0;
		while (const ofbx::AnimationLayer* const animLayer = animStack->getLayer(layerCount))
		{
			LoadAnimationLayer(fbxScene, animLayer, animation);
			layerCount++;
		}
	}

	animation.OptimizeCurves();
}

ndAnimationSequence* LoadFbxAnimation(const char* const fileName)
{
	char outPathName[1024];
	dGetWorkingFileName(fileName, outPathName);

	FILE* fp = fopen(outPathName, "rb");
	if (!fp)
	{
		dAssert(0);
		return nullptr;
	}

	size_t readBytes = 0;
	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	dArray<ofbx::u8> content;
	content.SetCount(file_size);
	readBytes = fread(&content[0], 1, file_size, fp);
	ofbx::IScene* const fbxScene = ofbx::load(&content[0], file_size, (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);

	const dMatrix convertMatrix(GetCoordinateSystemMatrix(fbxScene));

	dFbxAnimation animation;
	LoadAnimation(fbxScene, animation);
	fbxDemoEntity* const entity = FbxToEntity(fbxScene);
	dFbxAnimation newAnimation(animation, entity, convertMatrix);

	delete entity;
	fbxScene->destroy();

	return newAnimation.CreateSequence(fileName);
}