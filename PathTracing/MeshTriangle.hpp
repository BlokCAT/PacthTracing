#pragma once 
#ifndef PathTracing_MESH_H
#define PathTracing_MESH_H

#include "Triangle.hpp"
#include "BVHStruct.hpp"
#include "OBJ_Loader.h"
#include <array>

class MeshTriangle :public Object
{
private:
	float area = 0;
	AABB box;
	BVHstruct meshBVH;
	Material * tempMat = new Material();
public:
	bool isSmoothShading = false;
	vector<Triangle> triangles;

	//tow ways to load obj
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


			//������������
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
		}
		else
		{
			std::cerr << "Failed to Load OBJ File";
		}
	}

	//second ways with OBJ MTL
	MeshTriangle(const string &path,  bool _isSmoothShading)
	{
		isSmoothShading = _isSmoothShading;
		objl::Loader Loader;

		bool loadout = Loader.LoadFile(path);
		if (loadout)
		{
			objl::Mesh curMesh = Loader.LoadedMeshes[0];
			cout << "MeshTriangl::�����ж��Ƿ�������" << endl;
			if (curMesh.MeshMaterial.map_Kd != "")
			{
				cout << "OBJ������" << endl;
				tempMat->mtype = BLENDER;
				tempMat->isTexture = true;
				tempMat->ior = curMesh.MeshMaterial.Ni; //�����ò���ior
				tempMat->Ka = Vector3f(curMesh.MeshMaterial.Ka.X, curMesh.MeshMaterial.Ka.Y, curMesh.MeshMaterial.Ka.Z);
				tempMat->Ks = Vector3f(curMesh.MeshMaterial.Ks.X, curMesh.MeshMaterial.Ks.Y, curMesh.MeshMaterial.Ks.Z);
				tempMat->Illum = curMesh.MeshMaterial.illum;
				tempMat->Ns = curMesh.MeshMaterial.Ns;
				tempMat->texture = Texture(curMesh.MeshMaterial.map_Kd);
				m = tempMat;
				cout << "mtl����ȫ����ȡ������" << endl;
				cout << "������ʾ��ȡ������w����λ�ã�" << m->texture.texture_path<< endl;
			}
			else
			{
				cout << "OBJû������" << endl;
				m->mtype = BLENDER;
				m->ior = curMesh.MeshMaterial.Ni; //�����ò���ior
				m->Ka = Vector3f(curMesh.MeshMaterial.Ka.X, curMesh.MeshMaterial.Ka.Y, curMesh.MeshMaterial.Ka.Z);
				m->Kd = Vector3f(curMesh.MeshMaterial.Kd.X, curMesh.MeshMaterial.Kd.Y, curMesh.MeshMaterial.Kd.Z);
				m->Ks = Vector3f(curMesh.MeshMaterial.Ks.X, curMesh.MeshMaterial.Ks.Y, curMesh.MeshMaterial.Ks.Z);
				m->Illum = curMesh.MeshMaterial.illum;
				m->Ns = curMesh.MeshMaterial.Ns;
				cout << "mtl����ȫ����ȡ������" << endl;
				cout << "������ʾ��ȡ������Kd��ֵ��" << m->Kd.x << endl;
			}
			
			Vector3f boxMAX(std::numeric_limits<long>::min(),
				std::numeric_limits<long>::min(),
				std::numeric_limits<long>::min());
			Vector3f boxMIN(std::numeric_limits<long>::max(),
				std::numeric_limits<long>::max(),
				std::numeric_limits<long>::max());

			array<Vector3f, 3> tempVertices;
			array<Vector3f, 3> tempVerticesNormal;
			array<Vector2f, 3> tempVerticesTexture;

			//������������,���������εĶ��㣬����ķ��ߣ������UV����
			for (int i = 0; i < curMesh.Indices.size(); i += 3)
			{

				for (int k = 0; k < 3; k++)
				{
					tempVertices[k] = Vector3f(curMesh.Vertices[curMesh.Indices[i + k]].Position.X,
						curMesh.Vertices[curMesh.Indices[i + k]].Position.Z,
						curMesh.Vertices[curMesh.Indices[i + k]].Position.Y);
					tempVerticesNormal[k] = Vector3f(curMesh.Vertices[curMesh.Indices[i + k]].Normal.X,
						curMesh.Vertices[curMesh.Indices[i + k]].Normal.Z,
						curMesh.Vertices[curMesh.Indices[i + k]].Normal.Y);
					tempVerticesTexture[k] = Vector2f(curMesh.Vertices[curMesh.Indices[i + k]].TextureCoordinate.X,
						curMesh.Vertices[curMesh.Indices[i + k]].TextureCoordinate.Y);
				}

				triangles.emplace_back(tempVertices[0], tempVertices[1],
					tempVertices[2], m, tempVerticesNormal , tempVerticesTexture , isSmoothShading);

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

			for (auto& tt : triangles)
			{
				area += tt.getAra();
				meshBVH.objects.push_back(&tt);
			}

			meshBVH.BuiltBVH(1);
			cout << "�������ģ���е���������" << triangles.size() << endl;
		}
		else
		{
			cout << "objģ�ͼ���ʧ��";
		}
	}

	~MeshTriangle()
	{
		delete tempMat;
	}

	Vector3f getHitColor(const Vector3f &hitpos)
	{
		//no use
		return Vector3f(0);
	}

	void getHitPoint(Ray &ray, HitPoint &res)
	{
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
