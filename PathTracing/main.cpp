
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
#include "Camera.hpp"
#include <ctime>
#include <direct.h>
int spp = 250;
char PATH[999];
string HDRIPATH = "../model\\HDRI\\room.jpg";
int MAX_RENDER_DEPTH = 6;
int sceneHW = 400;
int isThread = 1;
bool AntiAliasing = false; //反走样
int useGPU = 1;


int main()
{
	_mkdir("../SHOW.assets/res");
	time_t now = time(nullptr);
	tm* t = localtime(&now);
	snprintf(PATH, sizeof(PATH), "../SHOW.assets/res/spot0_%02d%02d_%02d%02d%02d.ppm",
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
	MeshTriangle bunny("../model\\Scene\\spot.obj" , red , true);

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

	Camera cam;  // 默认值匹配旧硬编码: pos(0.3,-0.8,8), fov=40, lookAt沿+Z

	// ========== GPU 渲染管线 ==========
	if (useGPU) {
		std::vector<TriangleGeo>  gpuGeo;     // 热数据：顶点+AABB
		std::vector<TriangleMeta> gpuMeta;    // 冷数据：法线+UV+材质索引
		std::vector<GPUSphere>    gpuSpheres;
		std::vector<GPUMaterial>  gpuMaterials;
		std::unordered_map<Object*, std::pair<int,int>> triMap;
		std::unordered_map<Object*, int> sphereMap;
		std::vector<int>          lightTriIndices;
		std::unordered_map<Material*, int> matMap;

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

		for (Object* obj : scene.objs) {
			MeshTriangle* mesh = dynamic_cast<MeshTriangle*>(obj);
			Plane* plane = dynamic_cast<Plane*>(obj);

			if (mesh) {
				int start = (int)gpuGeo.size();
				for (auto& t : mesh->triangles) {
					TriangleGeo geo;
					geo.v0=t.node1; geo.v1=t.node2; geo.v2=t.node3;
					geo.bounds = t.getAABB();
					gpuGeo.push_back(geo);
					TriangleMeta meta;
					meta.n0=t.NodeNormal[0]; meta.n1=t.NodeNormal[1]; meta.n2=t.NodeNormal[2];
					meta.uv0=t.TextureUV[0]; meta.uv1=t.TextureUV[1]; meta.uv2=t.TextureUV[2];
					meta.materialIdx = getOrCreateMat(t.m);
					meta.faceNormal = t.N;
					meta.isSmoothShading = t.isSmoothShading;
					gpuMeta.push_back(meta);
				}
				int cnt = (int)gpuGeo.size() - start;
				if (cnt > 0) triMap[obj] = {start, cnt};

			} else if (plane) {
				int start = (int)gpuGeo.size();
				int matIdx = getOrCreateMat(plane->m);
				Vector3f N = plane->N;
				Vector3f triVerts[2][3] = {
					{plane->node1,plane->node2,plane->node3},
					{plane->node1,plane->node3,plane->node4} };
				for (int ti = 0; ti < 2; ti++) {
					TriangleGeo geo; geo.v0=triVerts[ti][0]; geo.v1=triVerts[ti][1]; geo.v2=triVerts[ti][2];
					float mx=fmaxf(geo.v0.x,fmaxf(geo.v1.x,geo.v2.x));
					float my=fmaxf(geo.v0.y,fmaxf(geo.v1.y,geo.v2.y));
					float mz=fmaxf(geo.v0.z,fmaxf(geo.v1.z,geo.v2.z));
					float nx=fminf(geo.v0.x,fminf(geo.v1.x,geo.v2.x));
					float ny=fminf(geo.v0.y,fminf(geo.v1.y,geo.v2.y));
					float nz=fminf(geo.v0.z,fminf(geo.v1.z,geo.v2.z));
					geo.bounds = AABB(Vector3f(mx,my,mz), Vector3f(nx,ny,nz));
					gpuGeo.push_back(geo);
					TriangleMeta meta; meta.n0=meta.n1=meta.n2=N;
					meta.uv0=meta.uv1=meta.uv2=Vector2f(0,0);
					meta.materialIdx=matIdx; meta.faceNormal=N; meta.isSmoothShading=false;
					gpuMeta.push_back(meta);
				}
				triMap[obj] = {start, 2};
				if (plane->m->islight) {
					lightTriIndices.push_back(start);
					lightTriIndices.push_back(start+1);
				}
			} else if (Boll* bollObj = dynamic_cast<Boll*>(obj)) {
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

		// ---- 三角形级 BVH（每叶 ≤ 4 tris）----
		struct TriLeaf : public Object {
			int s, n; AABB b;
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
		for (int i = 0; i < (int)gpuGeo.size(); i += 4) {
			int cnt = (i + 4 <= (int)gpuGeo.size()) ? 4 : ((int)gpuGeo.size() - i);
			AABB mb = gpuGeo[i].bounds;
			for (int k = 1; k < cnt; k++) mb = AmalgamateTowBox(mb, gpuGeo[i+k].bounds);
			auto* L = new TriLeaf(i, cnt, mb);
			leaves.push_back(L); leafPtrs.push_back(L);
		}
		std::vector<GPUBVHNode> gpuBVH;
		if (leaves.size() <= 1) {
			GPUBVHNode r; r.bounds = leaves.empty() ? AABB() : leaves[0]->b;
			r.triStart = leaves.empty() ? 0 : leaves[0]->s;
			r.triCount = leaves.empty() ? 0 : leaves[0]->n;
			r.sphereIdx = -1; gpuBVH.push_back(r);
		} else {
			BVHstruct b2(leafPtrs); b2.BuiltBVH(1);
			std::unordered_map<Object*, std::pair<int,int>> m2;
			for (auto* L : leaves) m2[L] = {L->s, L->n};
			gpuBVH = b2.flattenBVH(m2, sphereMap);
		}
		for (auto* L : leaves) delete L;

		printf("GPU: %zu tris, %zu spheres, %zu mats, %zu BVH nodes, %d lights\n",
			gpuGeo.size(), gpuSpheres.size(), gpuMaterials.size(),
			gpuBVH.size(), (int)lightTriIndices.size());

		auto start = std::chrono::system_clock::now();
		cudaRender(gpuGeo, gpuMeta, gpuSpheres, gpuMaterials, gpuBVH, lightTriIndices,
		           scene.w, scene.h, spp, MAX_RENDER_DEPTH, cam, PATH);
		auto stop = std::chrono::system_clock::now();
		std::cout << "GPU time: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count() << " ms\n";
	} else {
	Renderer r;
	auto start = std::chrono::system_clock::now();
	r.Render(scene, cam);
	auto stop = std::chrono::system_clock::now();
	std::cout << "CPU time: " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " ms\n";
	}
	delete red, green, blue, white, micro1, micro2, Refract, jinzi, jinzi2, micro_white, light1, yellow;
	return 0;
}
