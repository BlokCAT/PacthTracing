
#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <chrono>
#include "Renderer.hpp"
#include "Boll.hpp"
#include "Plane.hpp"
#include "MeshTriangle.hpp"
#include "Texture.hpp"
int spp = 200;
char PATH[999] = "E:/all/EnverimentMAP.ppm";
string HDRIPATH = "E:\\PathTracing\\model\\HDRI\\room.jpg";
int MAX_RENDER_DEPTH = 6;
int sceneHW = 200;
int isThread = 1;
bool AntiAliasing = false; //反走样
int main()
{
	Scene scene(sceneHW, sceneHW,BVH );//w  h
	//建立材质
	Material* red = new Material(Vector3f(0.83, 0.1, 0.1), DIFFUSE);
	red->ior = 1.3;
	red->roughness = 0.2;
	Material* green = new Material(Vector3f(0.14f, 0.45f, 0.091f), DIFFUSE); 
	green->ior = 5;
	green->roughness = 0.5;	
	Material* blue = new Material(Vector3f(0.14f, 0.15f, 0.51f), DIFFUSE);
	green->ior = 5;
	green->roughness = 0.5;
	Material* yellow= new Material(Vector3f(0.34f, 0.35f, 0.02f), DIFFUSE);
	green->ior = 5;
	green->roughness = 0.5;
	Material *micro1 = new Material(Vector3f(0.8, 0.1, 0.1), MIRCO);
	micro1->ior = 2000;
	micro1->roughness = 0.5;
	Material* micro_white = new Material(Vector3f(0.725f, 0.71f, 0.63f), MIRCO);
	micro_white->ior = 20;
	micro_white->roughness = 0.6;
	Material* micro2 = new Material(Vector3f(0.14f, 0.35f, 0.091f), MIRCO);
	micro2->ior = 18;
	micro2->roughness = 0.3;
	Material* white = new Material(Vector3f(1.0f, 1.0f, 1.0f), DIFFUSE);
	Material* light1 = new Material(Vector3f(2, 2, 2), DIFFUSE);
	light1->SetLight(Vector3f(0.747f + 0.058f, 0.747f + 0.258f, 0.747f)* 19.0f +  Vector3f(0.740f + 0.287f, 0.740f + 0.160f, 0.740f) * 19.6f + Vector3f(0.737f + 0.642f, 0.737f + 0.159f, 0.737f) * 19.4);
	Material* light2 = new Material(Vector3f(2, 2, 2), DIFFUSE);
	light2->SetLight(Vector3f(20, 20, 18));
	Material* jinzi = new Material(Vector3f(0.6, 0.1, 0.1), REFLC);
	Material* jinzi2 = new Material(Vector3f(0.14f, 0.30f, 0.091f), REFLC);
	Material* Refract = new Material(Vector3f(0.7, 0.7, 0.7), REFRACT);
	Refract->ior = 1.30;

	//建立物体（球 ， 面 ， 光源）
	Boll b1(Vector3f(0,  -4.0 + 0.3 , 45.0), 5, red);
	Boll b4(Vector3f(6.0, -4.0 + 0.3, 34.0), 3, jinzi);
	Plane L(Vector3f(-8.1, 12.3, 35.0), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 8.1, light2);
	Plane L2(Vector3f(0.0, 12.3, 44.0), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 8.1, light2);
	Plane L3(Vector3f(8.1, 12.3, 35.6), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 8.1, light2);

	MeshTriangle back("E:\\PathTracing\\model\\Scene\\back.obj" , white, false);
	MeshTriangle top("E:\\PathTracing\\model\\Scene\\top.obj", white, false);
	MeshTriangle down("E:\\PathTracing\\model\\Scene\\down.obj", white, false);
	MeshTriangle right("E:\\PathTracing\\model\\Scene\\right.obj", green, false);
	MeshTriangle left("E:\\PathTracing\\model\\Scene\\left.obj", red, false);
	MeshTriangle bunny("E:\\PathTracing\\model\\Scene\\spot.obj" , true);

	//scene.Add(&triangle1);
	//scene.Add(&triangle2);
	//scene.Add(&b4);
	//scene.Add(&b1);
	scene.Add(&L);
	scene.Add(&L2);
	scene.Add(&L3);
	scene.Add(&right);
	scene.Add(&back);
	scene.Add(&left);
	scene.Add(&top);
	scene.Add(&down);
	scene.Add(&bunny);
	scene.Add(&back);
	scene.BuildAccl();
	Renderer r;
	

	////三角形测试没有问题
	//HitPoint res;
	//Ray ry( Vector3f (0.1 , 19 , 0.1) , Vector3f( 0 , -1 , 0));
	//Triangle t(Vector3f(6, 0, 0), Vector3f(0, 3, 8), Vector3f(-6, 0, 0), red);
	//cout << t.N.x  << " " << t.N.y  << " " << t.N.z;
	//cout << endl << t.getAra() << endl;
	//cout << t.getAABB().pMax.x << " " << t.getAABB().pMax.y << " " << t.getAABB().pMax.z << " " << t.getAABB().pMin.x << " " << t.getAABB().pMin.y << " " <<t.getAABB().pMin.z << " " << endl;
	//t.getHitPoint(ry, res);
	//if (res.happened)
	//{
	//	cout << res.hitcoord.x << " " << res.hitcoord.y << " " << res.hitcoord.z << endl; 
	//}



	
	//mesh测试

	//MeshTriangle test("E:\\PathTracing\\model\\三棱椎.obj", red , false);
	//cout <<"MAXXX:" <<  test.getAABB().pMax.showVec() << endl;
	//cout << "MINNN:" << test.getAABB().pMax.showVec() << endl;
	//cout << "cen:" << test.getAABB().getCen().showVec() << endl;

	//for (int i = 0; i < test.triangles.size(); i++)
	//{
	//	cout << i << ":  " << test.triangles[i].node1.showVec() << "顶点法线:" << test.triangles[i].NodeNormal[0].showVec() << endl;
	//	cout << " :  " << test.triangles[i].node2.showVec() << "顶点法线:" << test.triangles[i].NodeNormal[1].showVec() << endl;
	//	cout << " :  " << test.triangles[i].node3.showVec() << "顶点法线:" << test.triangles[i].NodeNormal[2].showVec() << endl;
	//	cout << "N:  " << test.triangles[i].N.showVec() << endl;
	//}

	//Vector3f t(1, 0, 0); Vector3f tt(-3, 3, 0); Vector3f ttt(0, 3, 0);
	//cout << TriangleArea(t, tt, ttt);
	//cout << "bvhsize:" << test.meshBVH.objects.size() << endl; 
	
	/*HitPoint res;
	Ray ry( Vector3f (0.5 , 19 , 0.5) , Vector3f( 0 , -1 , 0));
	test.getHitPoint(ry, res);
	if (res.happened)
	{
		cout << "hit point:" << res.hitcoord.x << " " << res.hitcoord.y << " " << res.hitcoord.z << endl;
		cout << "distance:" << res.distance << endl;
		cout << "Kd:" << res.m->Kd.x << endl;
		if(res.m->mtype == DIFFUSE)
			cout << "type:DIFFUSE" << endl;
		else
		{
			cout << "NO" << endl;
		}
	}
	else
	{ 
		cout << "no hit any face!" << endl;
	}*/



	//Texture 测试
	//Texture pp("C:\\Users\\DELL\\Desktop\\TEST.png");

	//cout << pp.getColorAt(1.0, 0).showVec() << endl;



	//cout << " 按下任意键开始渲染" << endl;
	//int as = 0;
	//cin >> as;
	//cout << "start renderer" << endl;



	auto start = std::chrono::system_clock::now();
	r.Render(scene);
	auto stop = std::chrono::system_clock::now();


	std::cout << endl <<  "渲染耗时:" << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " seconds\n";
	delete red, green, blue, white, micro1, micro2, Refract, jinzi, jinzi2, micro_white, light1, yellow;

	return 0;
}