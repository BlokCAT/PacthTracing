#pragma once 
#ifndef PathTracing_MESH_H
#define PathTracing_MESH_H

#include "Triangle.hpp"
#include "BVHStruct.hpp"
#include "OBJ_Loader.hpp"
#include <array>

class MeshTriangle :public Object
{
private:
	float area = 0;
	AABB box;
	BVHstruct meshBVH;
public:
	bool isSmoothShading = false;
	vector<Triangle> triangles;
	MeshTriangle( const string &path ,  Material *_m ,bool _isSmoothShading)
	{
		m = _m;
		isSmoothShading = _isSmoothShading;
		objl::Loader Loader;

		bool loadout = Loader.LoadFile(path);
		if (loadout)
		{
			
			objl::Mesh curMesh = Loader.LoadedMeshes[0];

			Vector3f boxMAX(std::numeric_limits<long>::min(),
				std::numeric_limits<long>::min(),
				std::numeric_limits<long>::min());
			Vector3f boxMIN(std::numeric_limits<long>::max(),
				std::numeric_limits<long>::max(),
				std::numeric_limits<long>::max());
			
			array<Vector3f , 3> tempVertices;
			array<Vector3f, 3> tempVerticesNormal;


			//遍历所有索引
			for (int i = 0; i < curMesh.Indices.size(); i += 3)
			{
				
				for (int k = 0; k < 3; k++)
				{
					tempVertices[k] = Vector3f(curMesh.Vertices[curMesh.Indices[i + k]].Position.X ,
						curMesh.Vertices[curMesh.Indices[i + k]].Position.Z,
						curMesh.Vertices[curMesh.Indices[i + k]].Position.Y);
					tempVerticesNormal[k] = Vector3f(curMesh.Vertices[curMesh.Indices[i + k]].Normal.X,
						curMesh.Vertices[curMesh.Indices[i + k]].Normal.Z,
						curMesh.Vertices[curMesh.Indices[i + k]].Normal.Y);
				}

				triangles.emplace_back(tempVertices[0], tempVertices[1],
									tempVertices[2], m , tempVerticesNormal , isSmoothShading);

				for (int h = 0; h < 3; h++)
				{
					boxMAX.x = fmax(boxMAX.x, tempVertices[h].x);
					boxMAX.y = fmax(boxMAX.y, tempVertices[h].y);
					boxMAX.z = fmax(boxMAX.z, tempVertices[h].z);
					boxMIN.x = fmin(boxMIN.x, tempVertices[h].x);
					boxMIN.y = fmin(boxMIN.y, tempVertices[h].y);
					boxMIN.z = fmin(boxMIN.z, tempVertices[h].z);
				}
			}
			this->box = AABB(boxMAX, boxMIN);
			
			for (auto &tt : triangles)
			{
				area += tt.getAra();
				meshBVH.objects.push_back(&tt);
			}

			meshBVH.BuiltBVH(1);
			cout << "这个模型有的面数量：" << triangles.size() << endl;
			cout << "meshBVHsize: " << meshBVH.objects.size() << endl;
		}
		else
		{
			cout << "Failed to Load File";
		}
	}

	void getHitPoint(Ray &ray, HitPoint &res)
	{
//		for (int i = 0; i < triangles.size(); i++)
//		{
//			triangles[i].getHitPoint( ray , res);
//;		}
		meshBVH.gethitposition(ray, meshBVH.root, res);
	 }

	float getAra()
	{
		return this->area;
	 }
	AABB getAABB()
	{
		return this->box;
	 }
	void setAABB()
	{
		return;
	 }
	void SampleLight(HitPoint& hp, float& pdf_L)
	{
		//TODO
		return;

	 }
};


#endif // PathTracing_MESH_H
