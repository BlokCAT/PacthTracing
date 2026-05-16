
#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <chrono>
#include "Renderer.hpp"
#include "Boll.hpp"
#include "Plane.hpp"
#include "MeshTriangle.hpp"
#include "Texture.hpp"
#include "cuda/kernel.cuh"
#include <ctime>
#include <direct.h>
int spp = 300;
char PATH[999];
string HDRIPATH = "../model\\HDRI\\room.jpg";
int MAX_RENDER_DEPTH = 6;
int sceneHW = 400;
int isThread = 1;
bool AntiAliasing = false; //反走样
int useGPU = 1;


//没有导入的复杂obj,只有干净的墙体灯场景： CPU time: 41654 ；GPU time: 753 ms
/*
导入了复杂obj:
GPU time :  11668 ms
CPU time:  136728 ms
*/
//只使用自己的球体的GPU：1.2s ;CPU time: 78277 ms
//
int main()
{
	_mkdir("../SHOW.assets/res");
	time_t now = time(nullptr);
	tm* t = localtime(&now);
	snprintf(PATH, sizeof(PATH), "../SHOW.assets/res/spot1_%02d%02d_%02d%02d%02d.ppm",
		t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

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
	Boll b1(Vector3f(-5.7,  -4.0 + 0.3 , 30.0), 3.6, Refract);
	Boll b4(Vector3f(5.7, -4.0 + 0.3, 30.0), 3.6, jinzi);
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

	// ============================================================
	//  GPU 渲染管线
	//  分三步：
	//    第 1 步 — 收集数据：把所有 CPU 物体转成 GPU 平面数组
	//    第 2 步 — 重建 BVH：在三角形级别建一棵更深更均匀的树
	//    第 3 步 — 启动渲染：cudaMemcpy → kernel launch → 取回 → 写盘
	// ============================================================

	if (useGPU) {
		// ---- 第 1 步：准备 GPU 端数据容器 ----
		// 这些 vector 存储的是"拍平后的连续数组"，后面会整块 cudaMemcpy 到 GPU
		std::vector<GPUTriangle>  gpuTriangles;  // 全局三角形数组
		std::vector<GPUSphere>    gpuSpheres;    // 全局球体数组
		std::vector<GPUMaterial>  gpuMaterials;  // 全局材质数组

		// triMap: Object* → (在 gpuTriangles 中的起始索引, 三角形个数)
		// 仅用于重建 BVH 时查询每个叶子节点的三角形范围
		std::unordered_map<Object*, std::pair<int,int>> triMap;

		// sphereMap: Object* → 在 gpuSpheres 中的索引
		// 重建 BVH 时告诉叶子"这个节点包含一个球体"
		std::unordered_map<Object*, int> sphereMap;

		// lightTriIndices: 光源三角形的索引列表（NEE 采样时随机选一个）
		std::vector<int> lightTriIndices;

		// matMap: CPU Material* → GPU materials[] 中的索引（去重用）
		std::unordered_map<Material*, int> matMap;

		// ---- 辅助函数：把 CPU Material 转成 GPUMaterial ----
		// 同一个 CPU Material 对象只转一次，后续直接用索引
		auto getOrCreateMat = [&](Material* m) -> int {
			auto it = matMap.find(m);
			if (it != matMap.end()) return it->second;
			int idx = (int)gpuMaterials.size();
			GPUMaterial gm;
			gm.mtype = m->mtype; gm.Kd = m->Kd; gm.Ks = m->Ks; gm.Ka = m->Ka;
			gm.islight = m->islight; gm.isTexture = m->isTexture;
			gm.roughness = m->roughness; gm.ior = m->ior;
			gm.Illum = m->Illum; gm.Ns = m->Ns;
			gm.lightIntensity = m->lightIntensity;
			gpuMaterials.push_back(gm);
			matMap[m] = idx;
			return idx;
		};

		// ---- 遍历场景中所有物体，转成 GPU 格式 ----
		for (Object* obj : scene.objs) {
			MeshTriangle* mesh = dynamic_cast<MeshTriangle*>(obj);
			Plane* plane = dynamic_cast<Plane*>(obj);

			if (mesh) {
				// ---- OBJ 模型 → GPUTriangle[] ----
				// MeshTriangle 内部已经有拆好的 Triangle 列表，逐个拷贝
				int start = (int)gpuTriangles.size();
				for (auto& t : mesh->triangles) {
					GPUTriangle gt;
					gt.v0=t.node1; gt.v1=t.node2; gt.v2=t.node3;
					gt.n0=t.NodeNormal[0]; gt.n1=t.NodeNormal[1]; gt.n2=t.NodeNormal[2];
					gt.uv0=t.TextureUV[0]; gt.uv1=t.TextureUV[1]; gt.uv2=t.TextureUV[2];
					gt.materialIdx = getOrCreateMat(t.m);
					gt.faceNormal = t.N; gt.bounds = t.getAABB();
					gt.isSmoothShading = t.isSmoothShading;
					gpuTriangles.push_back(gt);
				}
				int cnt = (int)gpuTriangles.size() - start;
				if (cnt > 0) triMap[obj] = {start, cnt};

			} else if (plane) {
				// ---- 矩形平面 → 2 个 GPUTriangle ----
				// 一个矩形 = 2 个三角面 (v1,v2,v3) + (v1,v3,v4)
				int start = (int)gpuTriangles.size();
				int matIdx = getOrCreateMat(plane->m);
				Vector3f N = plane->N;
				GPUTriangle t1, t2;
				t1.v0=plane->node1; t1.v1=plane->node2; t1.v2=plane->node3;
				t2.v0=plane->node1; t2.v1=plane->node3; t2.v2=plane->node4;
				for (auto* pt : {&t1, &t2}) {
					pt->n0=pt->n1=pt->n2=N;
					pt->uv0=pt->uv1=pt->uv2=Vector2f(0,0);
					pt->materialIdx=matIdx; pt->faceNormal=N; pt->isSmoothShading=false;
					// 手动计算 AABB（取三个顶点的 min/max）
					float mx=fmaxf(pt->v0.x,fmaxf(pt->v1.x,pt->v2.x));
					float my=fmaxf(pt->v0.y,fmaxf(pt->v1.y,pt->v2.y));
					float mz=fmaxf(pt->v0.z,fmaxf(pt->v1.z,pt->v2.z));
					float nx=fminf(pt->v0.x,fminf(pt->v1.x,pt->v2.x));
					float ny=fminf(pt->v0.y,fminf(pt->v1.y,pt->v2.y));
					float nz=fminf(pt->v0.z,fminf(pt->v1.z,pt->v2.z));
					pt->bounds = AABB(Vector3f(mx,my,mz), Vector3f(nx,ny,nz));
				}
				gpuTriangles.push_back(t1); gpuTriangles.push_back(t2);
				triMap[obj] = {start, 2};
				// 如果这个平面是光源 → 记录到 lightTriIndices
				if (plane->m->islight) {
					lightTriIndices.push_back(start);
					lightTriIndices.push_back(start+1);
				}

			} else if (Boll* bollObj = dynamic_cast<Boll*>(obj)) {
				// ---- 球体 → GPUSphere ----
				// 不用拆三角形，GPU 端用解析公式直接求交
				int sIdx = (int)gpuSpheres.size();
				GPUSphere gs;
				gs.center = bollObj->cen;
				gs.radius = bollObj->r;
				gs.materialIdx = getOrCreateMat(bollObj->m);
				gs.bounds = bollObj->getAABB();
				gpuSpheres.push_back(gs);
				sphereMap[obj] = sIdx;
			}
		}

		// ---- 第 2 步：重建 BVH（三角形级别）----
		// 为什么废弃场景级 BVH？
		//   场景 BVH 只有 19 个节点，每个叶子 ~300 个三角形
		//   GPU warp 内 for 循环等最慢线程拖死所有人
		// 改为每 ≤4 个三角形一个叶子 → ~2937 节点 → 叶子均匀 → warp 友好
		struct TriLeaf : public Object {
			int s, n;
			AABB b;
			TriLeaf(int ss, int nn, const AABB& bb) : s(ss), n(nn), b(bb) {}
			AABB getAABB() override { return b; }
			float getAra() override { return 0; }
			void getHitPoint(Ray&, HitPoint&) override {}
			void setAABB() override {}
			void SampleLight(HitPoint&, float&) override {}
			Vector3f getHitColor(const Vector3f&) override { return Vector3f(0); }
		};

		std::vector<TriLeaf*> leaves;
		std::vector<Object*>  leafPtrs;

		// 把全局三角形数组按每 4 个一组打包成 BVH 叶子
		for (int i = 0; i < (int)gpuTriangles.size(); i += 4) {
			int cnt = (i + 4 <= (int)gpuTriangles.size())
				? 4 : ((int)gpuTriangles.size() - i);
			AABB mb = gpuTriangles[i].bounds;
			for (int k = 1; k < cnt; k++)
				mb = AmalgamateTowBox(mb, gpuTriangles[i + k].bounds);
			auto* L = new TriLeaf(i, cnt, mb);
			leaves.push_back(L);
			leafPtrs.push_back(L);
		}

		std::vector<GPUBVHNode> gpuBVH;

		if (leaves.size() <= 1) {
			// 场景太小，单节点 BVH
			GPUBVHNode r;
			r.bounds    = leaves.empty() ? AABB() : leaves[0]->b;
			r.triStart  = leaves.empty() ? 0 : leaves[0]->s;
			r.triCount  = leaves.empty() ? 0 : leaves[0]->n;
			r.sphereIdx = -1;
			gpuBVH.push_back(r);
		} else {
			// 用现有的 CPU BVH 构建器，但输入不再是 Object*，
			// 而是我们包装的 TriLeaf 数组
			BVHstruct b2(leafPtrs);
			b2.BuiltBVH(1);
			std::unordered_map<Object*, std::pair<int, int>> m2;
			for (auto* L : leaves) m2[L] = { L->s, L->n };
			// 拍平时融入 sphereMap → 球体也挂到 BVH 叶子
			gpuBVH = b2.flattenBVH(m2, sphereMap);
		}

		for (auto* L : leaves) delete L;

		printf("GPU: %zu tris, %zu spheres, %zu mats, %zu BVH nodes, %d lights\n",
			gpuTriangles.size(), gpuSpheres.size(), gpuMaterials.size(),
			gpuBVH.size(), (int)lightTriIndices.size());

		// ---- 第 3 步：启动 GPU 渲染 ----
		// cudaRender() 做的事：
		//   cudaMalloc → cudaMemcpy(Host→Device) → renderKernel<<<>>>
		//   → cudaDeviceSynchronize → cudaMemcpy(Device→Host) → PPM 写盘
		auto start = std::chrono::system_clock::now();
		cudaRender(gpuTriangles, gpuSpheres, gpuMaterials, gpuBVH, lightTriIndices,
		           scene.w, scene.h, spp, MAX_RENDER_DEPTH,
		           0.3f, -0.8f, 8.0f, tanf(M_PI/9.0f), PATH);
		auto stop = std::chrono::system_clock::now();
		std::cout << "GPU time: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count() << " ms\n";
	} else {
	Renderer r;



	auto start = std::chrono::system_clock::now();
	r.Render(scene);
	auto stop = std::chrono::system_clock::now();

	std::cout << "CPU time: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " ms\n";
	}
	delete red, green, blue, white, micro1, micro2, Refract, jinzi, jinzi2, micro_white, light1, yellow;

	return 0;
}
