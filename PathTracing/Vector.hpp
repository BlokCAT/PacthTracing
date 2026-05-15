#pragma once

// GPU/CPU 共享定义
#include "cuda/Vector.cuh"

// ---- 以下仅主机端调试用 ----
#include <string>

inline std::string showVec3(const Vector3f& v) {
    return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + " )";
}

inline std::string showVec2(const Vector2f& v) {
    return "(" + std::to_string(v.x) + "," + std::to_string(v.y) + " )";
}
