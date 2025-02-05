#pragma once 


#ifndef RAYTRACING_TRI_H
#define RAYTRACING_TRI_H
#include <cmath>
#include <array>
#include <tuple>
#include <iostream>
#include "Material.hpp"
#include "Vector.hpp"
#include "Tool.hpp"
#include "Ray.hpp"
#include "HitPoint.hpp"
#include "Object.hpp"


class Triangle :public Object
{

public:
	Vector3f N;
	Vector3f node1, node2, node3;
	array<Vector3f, 3> NodeNormal;
	array<Vector2f, 3> TextureUV;
	bool isSmoothShading;
	Triangle(const Vector3f &_node1, const  Vector3f &_node2, 
		const Vector3f &_node3 ,  Material *_m , array<Vector3f, 3>& _NnodeNormal ,  bool _isSmoothShading)
		:node1(_node1), node2(_node2), node3(_node3)
	{
		isSmoothShading = _isSmoothShading;
		NodeNormal = _NnodeNormal;
		m = _m;
		e1 = node2 - node1;
		e2 = node3 - node2;
		e3 = node1 - node3;
		N = crossProduct(e1 , e2).normalized();
		arae = 0.5 * (crossProduct((e1 * -1), e2).len());
		setAABB();
	}
	Triangle(const Vector3f& _node1, const  Vector3f& _node2, const Vector3f& _node3, Material* _m , 
		array<Vector3f, 3> &_NnodeNormal , array<Vector2f, 3> &_TextureUV, bool _isSmoothShading)
		:node1(_node1), node2(_node2), node3(_node3)
	{
		isSmoothShading = _isSmoothShading;
		m = _m;
		NodeNormal = _NnodeNormal;  
		TextureUV = _TextureUV;
		e1 = node2 - node1;
		e2 = node3 - node2;
		e3 = node1 - node3;
		N = crossProduct(e1, e2).normalized();
		arae = 0.5 * (crossProduct((e1 * -1), e2).len());
		setAABB();
	}

	Vector3f getHitColor(const Vector3f& hitpos)
	{
		std::tuple<float, float, float> tup = computeBarycentric3D(node1, node2, node3, hitpos);
		Vector2f hitUV = (TextureUV[0] * std::get<0>(tup)) + (TextureUV[1] * std::get<1>(tup)) + (TextureUV[2] * std::get<2>(tup));
		if (m->isTexture)
			return m->texture.getColorAt(hitUV.x, hitUV.y);
		else
			return m->Kd;
	}

	void getHitPoint(Ray &ray, HitPoint &res)	
	{
		/*
		|t | 	  1		|S2	E2|
		|b1| =  -----   |S1	S |
		|b2|	S1 E1	|S2	D |

		E1 = P1 - P0
		E2 = P2 - P0
		S = O - P0
		S1 = D X E2
		S2 = S X E1
		*/

		Vector3f E1 = node2 - node1 ,
				 E2 = node3 - node1 ,
				 S = ray.pos - node1 , 
				 S1 = crossProduct( ray.dir , E2),
				 S2 = crossProduct(S , E1 );
		if (dotProduct(S1, E1) < 0.0001 && dotProduct(S1, E1) > -0.0001) return;
		float temp = 1.0 / dotProduct(S1, E1);
		float tmp_t = temp * dotProduct(S2, E2);
		float b1 = temp * dotProduct(S1, S);
		float b2 = temp * dotProduct(S2, ray.dir);
		/*cout << "b1:" << b1 << " b2:" << b2 << endl;
		cout << "tmp_t："  << tmp_t << endl;*/
		if (b1 < 0 || b1 >= 1)return;
		if (b2 < 0 || b2 >= 1)return;
		if (b1 + b2 >= 1) return;

		if (tmp_t > 0 && tmp_t < res.distance )
		{
			

			if (!isSmoothShading)
			{
				res.distance = tmp_t;
				res.happened = true;
				res.hitcoord = ray.Xt_pos(tmp_t);
				//N
				if (dotProduct(ray.dir, this->N) > 0)
					res.hitN = this->N *- 1 ;
				else
					res.hitN = (this->N * 1 );
				res.m = m;
				res.hitColor = getHitColor(res.hitcoord);
			}
			else  //通过三角形点的差值算击中点的法线
			{
				
				Vector3f hit = ray.Xt_pos(tmp_t);
				std::tuple<float, float, float> tup = computeBarycentric3D(node1, node2, node3, hit);
				Vector3f tempN = (NodeNormal[0] * std::get<0>(tup)) + (NodeNormal[1] * std::get<1>(tup)) + (NodeNormal[2] * std::get<2>(tup));
				if (dotProduct(ray.dir, tempN) < 0)
				{
					res.distance = tmp_t;
					res.happened = true;
					res.hitcoord = hit;
					res.hitN = tempN;
					res.m = m;
					res.hitColor = getHitColor(res.hitcoord);
				}
				else if(this->m->mtype == REFRACT)
				{
					res.distance = tmp_t;
					res.happened = true;
					res.hitcoord = hit;
					res.hitN = tempN* -1;
					res.m = m;
					res.hitColor = getHitColor(res.hitcoord);
				}	
				else
				{
					res.happened = false;
				}
			}	
		}
	}
	float getAra()
	{
		return this->arae;
	}
	AABB getAABB()
	{
		return this->box;
	}
	void setAABB()
	{
		// 定义临时变量存储三个顶点坐标
		float minX = std::min({ node1.x, node2.x, node3.x });
		float minY = std::min({ node1.y, node2.y, node3.y });
		float minZ = std::min({ node1.z, node2.z, node3.z });

		float maxX = std::max({ node1.x, node2.x, node3.x });
		float maxY = std::max({ node1.y, node2.y, node3.y });
		float maxZ = std::max({ node1.z, node2.z, node3.z });

		// 创建最小点和最大点
		Vector3f minP(minX, minY, minZ);
		Vector3f maxP(maxX, maxY, maxZ);

		// 更新包围盒
		box.pMin = minP;
		box.pMax = maxP;
		box.getCen() = (minP + maxP) / 2.0f; // 确保除法结果为浮点数
	}
	void SampleLight(HitPoint &hp, float &pdf_L)
	{
		//do it in the future
	}
private:
	
	Vector3f e1, e2, e3,  cen;
	float arae;
	AABB box;
};
#endif