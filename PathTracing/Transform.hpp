#pragma once

#include "Vector.hpp"
#include <cmath>

// ============================================================
//  Transform — 物体空间变换（CPU 端，构造时预变换顶点）
//  顺序：Scale → Rotate(Z→Y→X) → Translate
// ============================================================
struct Transform {
	Vector3f position = {0, 0, 0};
	Vector3f scale    = {1, 1, 1};
	Vector3f rotation = {0, 0, 0};   // Euler 角度（度）

	// 对单个顶点应用变换
	Vector3f apply(const Vector3f& v) const {
		float sx = v.x * scale.x;
		float sy = v.y * scale.y;
		float sz = v.z * scale.z;

		// 绕 Z 轴
		float rz = rotation.z * (float)M_PI / 180.0f;
		float cosZ = cosf(rz), sinZ = sinf(rz);
		float x1 = sx * cosZ - sy * sinZ;
		float y1 = sx * sinZ + sy * cosZ;
		float z1 = sz;

		// 绕 Y 轴
		float ry = rotation.y * (float)M_PI / 180.0f;
		float cosY = cosf(ry), sinY = sinf(ry);
		float x2 = x1 * cosY + z1 * sinY;
		float y2 = y1;
		float z2 = -x1 * sinY + z1 * cosY;

		// 绕 X 轴
		float rx = rotation.x * (float)M_PI / 180.0f;
		float cosX = cosf(rx), sinX = sinf(rx);
		float x3 = x2;
		float y3 = y2 * cosX - z2 * sinX;
		float z3 = y2 * sinX + z2 * cosX;

		return Vector3f(x3 + position.x, y3 + position.y, z3 + position.z);
	}

	// 对 Vector3f 只做平移+缩放（不旋转，用于球体位置）
	Vector3f applyPos(const Vector3f& v) const {
		return Vector3f(v.x * scale.x + position.x,
		                v.y * scale.y + position.y,
		                v.z * scale.z + position.z);
	}
};
