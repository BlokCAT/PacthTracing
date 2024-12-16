#pragma once 
#include <cmath>
#include <random>
#include <iostream>
#undef M_PI
#define M_PI 3.141592653589793f
#undef RussianRoulette
#define RussianRoulette 0.8


inline float clamp(const float &lo, const float &hi, const float &v)
{
	return std::max(lo, std::min(hi, v));
}

inline bool sloveEquation(float a, float b, float c  , float &t1 , float &t2)  //通过这个计算是否有解，有解直接返回算出来的值
{
	float deta = std::sqrt(b * b - (4 * a * c));
	if (deta < 0.00001)return false;
	if (-0.00001 <= deta  && deta < 0.00001)
	{
		t1 = -b / (2 * a);
		t2 = t1;
		return true;
	}
	if (deta >= 0.00001)
	{
		t1 = (-b + deta) / (2 * a);
		t2 = (-b - deta) / (2 * a);
		if (t1 > t2) std::swap(t1 ,t2);
		return true;
	}
}

inline static float RandomFloat()
{
	static std::random_device d;
	static std::mt19937 res(d());
	static std::uniform_real_distribution<float> D(0.f, 1.f);
	return D(res);
}

inline void ChangeO(Vector3f &t1, Vector3f &t2, Vector3f &t3, Vector3f &t4)
{
	if (t1.x == -0)t1.x = 0;
	if (t1.y == -0)t1.y = 0;
	if (t1.z == -0)t1.z = 0;
	if (t2.x == -0)t2.x = 0;
	if (t2.y == -0)t2.y = 0;
	if (t2.z == -0)t2.z = 0;
	if (t3.x == -0)t3.x = 0;
	if (t3.y == -0)t3.y = 0;
	if (t3.z == -0)t3.z = 0;
}


inline void UpdateProgress(float progress)
{
	int barWidth = 70;

	std::cout << "[";
	int pos = barWidth * progress;


	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) std::cout << "=";
		else if (i == pos) std::cout << ">";
		else std::cout << " ";
	}
	std::cout << "] " << int(progress * 100.0) << " %\r";
	std::cout.flush();
};