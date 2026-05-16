# 性能基准

测试条件：400×400, 300 spp, RTX 5060 Laptop

## 场景对比

| 场景 | CPU | GPU | 加速比 |
|------|-----|-----|--------|
| 墙体灯光（~30 tris） | 41.7s | 0.75s | 55× |
| 球体（0 tris） | 78.3s | 1.2s | 65× |
| 复杂 OBJ（5874 tris） | 136.7s | 11.6s | 12x |

## 复杂 OBJ 优化记录

| 版本 | 优化 | BVH 节点 | GPU 时间 | vs CPU |
|------|------|---------|----------|--------|
| 初版 | 场景级 Object BVH | 19 | 206s | 0.5× |
| 优化1 | 三角形级 BVH（每叶≤4 tris） | 2937 | 11.6s | — |
| 优化2 | 去掉 NEE 预检查遍历 | 2937 | 7.6s | 18× |
| 优化3 | SAH BVH 构建 | 2937 | 5.4s | 25× |
| 优化4 | Triangle SOA 拆分 | 2937 | 5.3s | 26× |

> 从 206s → 5.3s，累计 39× 加速

## 优化4 详解：Triangle SOA 拆分

### 做了什么

把 `GPUTriangle`（140 字节）拆成 `TriangleGeo`（60B 顶点+AABB）和 `TriangleMeta`（80B 法线+UV+材质）。BVH 粗筛和 Möller-Trumbore 求交只加载热数据，命中后才读冷数据。

### 改动文件

- `Triangle.cuh`：GPUTriangle → TriangleGeo + TriangleMeta，函数签名全部加 meta 参数
- `BVH.cuh`：BVHTraverse 增加 meta 参数
- `PathTracing.cuh`：PathTracingGPU/PlaneSampleLight 全部更新
- `kernel.cu/cuh`：cudaRender 增加 cpuMeta 参数和 d_meta 分配
- `main.cpp`：gpuTriangles → gpuGeo + gpuMeta 分离收集

### 效果有限的原因

当前场景 BVH 已有 2937 节点、每叶最多 4 个三角。光线大部分时间在 AABB 粗筛阶段流动（节点级），Möller-Trumbore 求交只在少数叶子触发。三角形数据尺寸缩短对"节点遍历密集、三角求交稀疏"的场景效果微弱。

### 后续方向

SOA 的收益在三角形求交密集的场景（如树叶少、每叶三角形多的旧版 BVH）更显著。当前瓶颈已不在三角形数据带宽，而在 BVH 遍历本身——下一步考虑 Wavefront 或 Shared Memory BVH。
