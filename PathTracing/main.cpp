
#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <chrono>
#include "Renderer.hpp"
#include "Boll.hpp"
#include "Plane.hpp"
#include "MeshTriangle.hpp"

int spp = 6;
char PATH[999] = "E:/all/图3.14.ppm";
int MAX_RENDER_DEPTH = 6;
int sceneHW = 1200;
int isThread = 1;

int main()
{
	Scene scene(sceneHW, sceneHW,BVH);//w  h
	//建立材质
	Material* red = new Material(Vector3f(0.9, 0.1, 0.1), DIFFUSE);
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
	Material *micro1 = new Material(Vector3f(0.6, 0.1, 0.1), MIRCO);
	micro1->ior = 900;
	micro1->roughness = 1;
	Material* micro_white = new Material(Vector3f(0.725f, 0.71f, 0.63f), MIRCO);
	micro_white->ior = 20;
	micro_white->roughness = 0.0001;
	Material* micro2 = new Material(Vector3f(0.14f, 0.35f, 0.091f), MIRCO);
	micro2->ior = 18;
	micro2->roughness = 0.3;
	Material* white = new Material(Vector3f(0.925f, 0.91f, 0.93f), DIFFUSE);
	Material* light1 = new Material(Vector3f(2, 2, 2), DIFFUSE);
	light1->SetLight(Vector3f(0.747f + 0.058f, 0.747f + 0.258f, 0.747f)* 19.0f +  Vector3f(0.740f + 0.287f, 0.740f + 0.160f, 0.740f) * 19.6f + Vector3f(0.737f + 0.642f, 0.737f + 0.159f, 0.737f) * 19.4);
	Material* jinzi = new Material(Vector3f(0.6, 0.1, 0.1), REFLC);
	Material* jinzi2 = new Material(Vector3f(0.14f, 0.30f, 0.091f), REFLC);
	Material* redRefract = new Material(Vector3f(0.7, 0.7, 0.7), REFRACT);
	redRefract->ior = 1.90;


	//建立物体（球 ， 面 ， 光源）
	Boll b1(Vector3f(-7,  -4.0 + 0.3 , 31.0), 3, jinzi);
	Boll b4(Vector3f(6.0, -4.0 + 0.3, 34.0), 3, redRefract);
	Plane L(Vector3f(0.0, 13.99, 42.0), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 5, light1);
	Plane faceReflact(Vector3f(4, -4.0 + 0.3 + 4, 37.0), Vector3f(-1, 1, 0), Vector3f(1, 1, 0), 6, jinzi);
	Plane down(Vector3f(0.0, -9.9 , 40.0), Vector3f(0.0, 1.0, 0.0), Vector3f(0, 0, 1.0), 50, white);
	Plane back(Vector3f(0.0, 1, 53.0), Vector3f(0.0, 0.0, -1.0), Vector3f(0, 1, 0.0), 50, white);
	Plane right(Vector3f(13.0, 1, 40.0), Vector3f(-1.0, 0.0, 0.0), Vector3f(0, 1, 0.0), 34, green);
	Plane left(Vector3f(-13.0, 1, 40.0), Vector3f(1.0, 0.0, 0.0), Vector3f(0, 1, 0.0), 26, red);
	Plane top(Vector3f(0.0, 14, 40.0), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 26, white);
	Triangle triangle1(Vector3f(6, -8, 40), Vector3f(9, -6, 40), Vector3f(0, -2, 46), red , false);
	Triangle triangle2(Vector3f(6, -8, 40), Vector3f(0, -2, 46), Vector3f(-6, -8, 40),red , false);
	MeshTriangle test("E:\\PathTracing\\model\\猴头.obj", red, false);


	//scene.Add(&triangle1);
	//scene.Add(&triangle2);

	scene.Add(&L);
	scene.Add(&right);
	scene.Add(&b1);
	scene.Add(&test);
	//scene.Add(&test);
	scene.Add(&b4);
	scene.Add(&left);
	scene.Add(&top);
	scene.Add(&down);
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




	//cout << " 按下任意键开始渲染" << endl;
	//int as = 0;
	//cin >> as;
	//cout << "start renderer" << endl;
	auto start = std::chrono::system_clock::now();
	r.Render(scene);
	auto stop = std::chrono::system_clock::now();


	std::cout << endl <<  "渲染耗时:" << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " seconds\n";
	return 0;
}