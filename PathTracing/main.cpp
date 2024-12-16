
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "Vector.hpp"
#include "Scene.hpp"
#include "Renderer.hpp"
#include "Boll.hpp"
#include "Plane.hpp"
#include "Object.hpp"
#include <iostream>
#include <chrono>
#include "AABB.hpp"

int spp = 50;
char PATH[999] = "E:/all/12月16号多线程多物体测试100.ppm";
int MAX_RENDER_DEPTH = 6;
int sceneHW = 100;
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
	micro1->ior = 20;
	micro1->roughness = 0.3;

	Material* micro_white = new Material(Vector3f(0.725f, 0.71f, 0.63f), MIRCO);
	micro_white->ior = 20;
	micro_white->roughness = 0.3;

	Material* micro2 = new Material(Vector3f(0.14f, 0.35f, 0.091f), MIRCO);
	micro2->ior = 18;
	micro2->roughness = 0.3;

	Material* white = new Material(Vector3f(0.725f, 0.71f, 0.63f), DIFFUSE);

	Material* light1 = new Material(Vector3f(2, 2, 2), DIFFUSE);
	light1->SetLight(Vector3f(0.747f + 0.058f, 0.747f + 0.258f, 0.747f)* 12.0f +  Vector3f(0.740f + 0.287f, 0.740f + 0.160f, 0.740f) * 21.6f + Vector3f(0.737f + 0.642f, 0.737f + 0.159f, 0.737f) * 23.4);

	Material* jinzi = new Material(Vector3f(0.6, 0.1, 0.1), REFLC);

	Material* jinzi2 = new Material(Vector3f(0.14f, 0.30f, 0.091f), REFLC);

	Material* redRefract = new Material(Vector3f(0.7, 0.1, 0.1), REFRACT);
	redRefract->ior = 3.20;


	//建立物体（球 ， 面 ， 光源）
	Boll b1(Vector3f(0,  -4.0 + 0.3 + 4, 37.0), 5, redRefract);
	Boll b4(Vector3f(6.0, -4.0 + 0.3, 40.0), 5.3, redRefract);
	Plane L(Vector3f(0.0, 13.99, 42.0), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 5, light1);
	Plane faceReflact(Vector3f(4, -4.0 + 0.3 + 4, 37.0), Vector3f(-1, 1, 0), Vector3f(1, 1, 0), 6, jinzi);
	Plane down(Vector3f(0.0, -9.9 , 40.0), Vector3f(0.0, 1.0, 0.0), Vector3f(0, 0, 1.0), 50, white);
	Plane back(Vector3f(0.0, 1, 53.0), Vector3f(0.0, 0.0, -1.0), Vector3f(0, 1, 0.0), 50, white);
	Plane right(Vector3f(13.0, 1, 40.0), Vector3f(-1.0, 0.0, 0.0), Vector3f(0, 1, 0.0), 34, green);
	Plane left(Vector3f(-13.0, 1, 40.0), Vector3f(1.0, 0.0, 0.0), Vector3f(0, 1, 0.0), 26, red);
	Plane top(Vector3f(0.0, 14, 40.0), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 26, white);

	for( int i = -8 ; i<= 8  ; i += 1)
		for (int j = -8; j <= 8; j+= 1)
		{
			Boll* b = new Boll(Vector3f((float)i, (float)j, 40.0), 0.4, micro1);
			scene.Add(b);
		}


	scene.Add(&L);
	scene.Add(&right);
	//scene.Add(&b1);
	//scene.Add(&faceReflact);
	//scene.Add(&b4);
	scene.Add(&left);
	scene.Add(&top);
	scene.Add(&down);
	scene.Add(&back);

	scene.BuildAccl();
	Renderer r;

	auto start = std::chrono::system_clock::now();
	r.Render(scene);
	auto stop = std::chrono::system_clock::now();


	std::cout << "渲染耗时:" << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds\n";
	return 0;
}