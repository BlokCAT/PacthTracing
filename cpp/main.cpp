
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

int main()
{
	Vector3f v1(1 , 2 , 3);

	Scene scene(20 , 20);//w  h
	//��������
	Material* red = new Material(Vector3f(0.8, 0.1, 0.1), DIFFUSE);
	red->ior = 0.4;
	red->roughness = 0.2;
	Material* green = new Material(Vector3f(0.14f, 0.45f, 0.091f), DIFFUSE);
	green->ior = 0.4;
	green->roughness = 0.5;
	Material *micro1 = new Material(Vector3f(0.6, 0.1, 0.1), MIRCO);
	micro1->ior = 7;
	micro1->roughness = 0.3;
	Material* micro2 = new Material(Vector3f(0.14f, 0.45f, 0.091f), MIRCO);
	micro2->ior = 20;
	micro2->roughness = 0.3;

	Material* white = new Material(Vector3f(0.725f, 0.71f, 0.63f), DIFFUSE);
	Material* light1 = new Material(Vector3f(2, 2, 2), DIFFUSE);
	//light1->SetLight((Vector3f(0.747f + 0.058f, 0.747f + 0.258f, 0.747f)* 12.0f + Vector3f(0.740f + 0.287f, 0.740f + 0.160f, 0.740f) *17.6f  + Vector3f(0.737f + 0.642f, 0.737f + 0.159f, 0.737f) *20.4f ));
	light1->SetLight(Vector3f(90 , 90 , 80));
	Material* jinzi = new Material(Vector3f(0.6, 0.1, 0.1), REFLC);
	Material* jinzi2 = new Material(Vector3f(0.14f, 0.30f, 0.091f), REFLC);


	//�������壨�� �� �� �� ��Դ��
	Boll b1(Vector3f(-6.0,  -7.0 + 0.3, 40.0), 5.3, jinzi);
	Boll b4(Vector3f(6.0, -4.0 + 0.3, 40.0), 5.3, jinzi2);
	Plane L(Vector3f(0.0, 14, 42.0), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 6, light1);
	Plane down(Vector3f(0.0, -12 , 40.0), Vector3f(0.0, 1.0, 0.0), Vector3f(0, 0, 1.0), 26, white);
	Plane back(Vector3f(0.0, 1, 53.0), Vector3f(0.0, 0.0, -1.0), Vector3f(0, 1, 0.0), 26, white);
	Plane right(Vector3f(13.0, 1, 40.0), Vector3f(-1.0, 0.0, 0.0), Vector3f(0, 1, 0.0), 26, red);
	Plane left(Vector3f(-13.0, 1, 40.0), Vector3f(1.0, 0.0, 0.0), Vector3f(0, 1, 0.0), 26, green);
	Plane top(Vector3f(0.0, 14, 40.0), Vector3f(0.0, -1.0, 0.0), Vector3f(0, 0, 1.0), 26, white);

	//��������볡��
	scene.Add(&b1);
	scene.Add(&b4);
	scene.Add(&L);
	scene.Add(&down);
	scene.Add(&back);
	scene.Add(&right);
	scene.Add(&left);
	scene.Add(&top);

	Renderer r;


	

	auto start = std::chrono::system_clock::now();
	r.Render(scene);
	auto stop = std::chrono::system_clock::now();


	std::cout << "��Ⱦ��ʱ:" << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds\n";
	return 0;
}