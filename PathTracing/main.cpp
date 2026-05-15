
#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <chrono>
#include "Renderer.hpp"
#include "Boll.hpp"
#include "Plane.hpp"
#include "MeshTriangle.hpp"
#include "Texture.hpp"
int spp = 150;
char PATH[999] = "../SHOW.assets/output2026_spot_refract_big.ppm";
string HDRIPATH = "../model\\HDRI\\room.jpg";
int MAX_RENDER_DEPTH = 6;
int sceneHW = 1300;
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
	light2->SetLight(Vector3f(12, 12, 10));
	Material* jinzi = new Material(Vector3f(0.6, 0.1, 0.1), REFLC);
	Material* jinzi2 = new Material(Vector3f(0.14f, 0.30f, 0.091f), REFLC);
	Material* Refract = new Material(Vector3f(0.7, 0.7, 0.7), REFRACT);
	Refract->ior = 1.60;

	//建立物体（球 ， 面 ， 光源）
	Boll b1(Vector3f(0,  -4.0 + 0.3 , 31.0), 6.0, Refract);
	Boll b4(Vector3f(6.0, -4.0 + 0.3, 34.0), 3, jinzi);
	Plane L(Vector3f(-8.1, 12.3, 35.0), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 8.1, light2);
	Plane L2(Vector3f(0.0, 12.3, 44.0), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 8.1, light2);
	Plane L3(Vector3f(8.1, 12.3, 35.6), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 8.1, light2);

	MeshTriangle back("../model\\Scene\\back.obj" , white, false); 
	MeshTriangle top("../model\\Scene\\top.obj", white, false);
	MeshTriangle down("../model\\Scene\\down.obj", white, false);
	MeshTriangle right("../model\\Scene\\right.obj", blue, false);
	MeshTriangle left("../model\\Scene\\left.obj", yellow, false);
	MeshTriangle bunny("../model\\Scene\\spot.obj" , Refract, true);

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



	auto start = std::chrono::system_clock::now();
	r.Render(scene);
	auto stop = std::chrono::system_clock::now();

	std::cout << endl <<  "use time:" << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " seconds\n";
	delete red, green, blue, white, micro1, micro2, Refract, jinzi, jinzi2, micro_white, light1, yellow;

	return 0;
}
