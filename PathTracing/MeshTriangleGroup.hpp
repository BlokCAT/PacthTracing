#pragma once 
#ifndef PATHTRACING_GROUP_H
#define PATHTRACING_GROUP_H

#include "MeshTriangle.hpp"


//many MeshTriangle 
class MeshTriangleGroup :public MeshTriangle
{
	Material* MeshGroupMaterial = nullptr;
	vector< MeshTriangle> meshgroup;
	
	MeshTriangleGroup(const string& path)
	{

	}

	MeshTriangleGroup(const string& path,  Material * _m)
	{

	}

};

#endif // PATHTRACING_GROUP_H
