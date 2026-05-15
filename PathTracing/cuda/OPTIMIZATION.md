# CUDA 路径追踪性能分析与优化方向

## 当前性能数据

| 场景 | CPU | GPU | 加速比 |
|------|-----|-----|--------|
| 纯墙体灯光（~30 tris） | 41.7s | 0.75s | 55× |
| 纯球体（0 tris） | 78.3s | 1.2s | 65× |
| 复杂 OBJ（5874 tris, 19 BVH nodes） | 48.5s | 90.7s | 0.5× |

## 根因分析

### 瓶颈 1：Warp Divergence（最主要瓶颈）

一个 warp 32 个线程同时执行同一条指令。但每个线程的光线路径不同：

- 有的光线 2 次 bounce 就打到光源（提前终止）
- 有的光线 6 次 bounce 还在追（占用整个 warp）
- 有的光线走 REFLC 分支，有的走 DIFFUSE 分支
- REFRACT 的反射路径 vs 折射路径：完全不同的计算量

**影响**：warp 中只要有一个线程还在追长路径，其他 31 个线程空转等待。对于 5874 个三角形的场景，BVH 遍历花费的时间占主导，长路径线程拖死整个 warp。

### 瓶颈 2：叶子节点三角形过多

19 个 BVH 节点，5874 个三角形 → 平均每个叶子 300+ 个三角形。

```
GPU 端的 BVH 叶子处理：
for (int i = 0; i < node.triCount; i++) {   // triCount ≈ 300
    TriangleIntersect(triangles, node.triStart + i, ray, hp, materials);
}
```

一个 warp 里 32 个线程，有的叶子有 300 个三角，有的有 2 个。GPU 执行 for 循环时以 warp 为单位，32 个线程都必须等最长的那个循环跑完。

CPU 上这完全不是问题——for 循环是单线程的，编译器可以做自动向量化，而且 CPU 的分支预测 + 大 cache 对顺序访问极其友好。

### 瓶颈 3：每 bounce 3 次 BVH 遍历

```
1. 主光线遍历（找交点）
2. NEE shadow ray 遍历（直接光遮挡检测）
3. NEE 预检查遍历（半球采样方向的下一个命中是不是光源）
```

对 400×400×300 SPP × avg 3 bounces = 144M 像素-光线。每次 bounce 3 次遍历 = 432M 次 BVH 遍历。每次遍历都可能访问 O(log N) 到 O(N) 个节点。

### 瓶颈 4：全局内存随机访问

```
GPU 线程 0：访问 BVH nodes[0] → nodes[5] → nodes[12] → triangles[350]
GPU 线程 1：访问 BVH nodes[0] → nodes[5] → nodes[15] → triangles[2000]
GPU 线程 2：访问 BVH nodes[0] → nodes[8] → nodes[3]  → triangles[5000]
```

每个线程访问完全不同的内存地址。L1 cache 128KB/SM，L2 cache 2MB，32 个线程同时查不同的 BVH 节点，cache line 频繁抖动摇摆（thrashing）。

### 瓶颈 5：Occupancy 不足

- 每个线程状态：BVH stack[64] ints, RNGState, throughput, ray, hp, 局部变量
- 寄存器压力大 → 每个 SM 能同时跑的 warp 数量下降
- 6602 blocks × 256 threads = 1.69M 线程排队，但 RTX 5060 只有 ~20 SM
- 每个 SM 只能同时跑 2-4 个 warp → 大量线程在队列里等 → 无法隐藏内存延迟

### 瓶颈 6：BVH 构建质量

CPU 端 BVH 构建算法极其简单：
- 按单一轴排序，中分
- 不按 SAH（表面积启发）划分
- 递归深度不均匀

简单的 BVH 对 CPU 影响不大（CPU 遍历开销低），但对 GPU 是致命的——质量差的 BVH 意味着更多 AABB 测试和更多三角形测试。

## 为什么简单场景快？

- 三角形少 → 叶子小 → for 循环短 → warp divergence 影响小
- BVH 遍历路径短 → 内存访问少
- 球体场景使用解析求交 → 完全不用三角形循环
- GPU 的数学吞吐量远超 CPU（球体求交就是几个 FMA 指令）

## 优化方向（按预期效果排序）

### 1. 重构 BVH 构建（预期提升：2-3×）

- 改用 SAH（Surface Area Heuristic）构建
- 叶子节点控制三角形数量上限（如 ≤ 8 个）
- 深度均匀化

### 2. Wavefront Path Tracing（预期提升：2-5×）

- 当前：每个线程独立追完整路径
- 改为：所有线程同步推进，每个 depth 所有线程一起做 BVH 遍历
- 优势：同一 depth 的线程访存模式相似，cache 友好
- 优势：dead 线程不占用 warp（重新打包活跃线程）

### 3. 流压缩（Stream Compaction）（预期提升：1.5-3×）

- 每轮 BVH 遍历后，移除已终止的线程
- 重新打包活跃线程到连续的 warp
- 消除"等待已终止线程"的空转

### 4. 缓存 BVH 到 Shared Memory（预期提升：1.2-1.5×）

- BVH 节点只有 19 个（~1.2KB），整个树可放进 shared memory
- 每个 SM 一次加载，所有线程共享
- 消除全局内存访问

### 5. 减少 BVH 遍历次数（预期提升：1.2-1.5×）

- 合并 NEE shadow ray 和 NEE 预检查为一次遍历
- 或者：去掉 NEE 预检查，改为标记法（在 PathTracing 迭代中标记"来自 NEE 表面"）

### 6. 三角形数据 SOA 化（预期提升：1.1-1.3×）

- 当前 AOS（Array of Structures）：GPUTriangle {v0, v1, v2, n0, n1, n2, ...}
- 改为 SOA（Structure of Arrays）：所有 v0 连续存，所有 v1 连续存...
- Möller-Trumbore 每次只访问需要的字段，避免加载冗余数据到 cache line

### 7. 更深的 BVH、更小的叶子（预期提升：2-3×）

- 当前 19 节点 5874 三角形 = 每种叶子 ~300 tris
- 目标：~800 节点，每种叶子 ≤ 8 tris
- 更深但更窄的遍历 → warp 线程的 for 循环更均匀

## 建议实施顺序

```
第 1 步：BVH 构建优化（SAH + 限叶子三角数 ≤ 8）— 投入产出比最高
第 2 步：小 BVH 放 shared memory
第 3 步：合并 NEE shadow + 预检查为一次遍历
第 4 步：Wavefront + Stream Compaction（工程量最大，效果最好）
```
