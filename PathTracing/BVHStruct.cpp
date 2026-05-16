#pragma
#include "BVHStruct.hpp"
#include <algorithm>
#include <iostream>

// ---- GPU 迁移辅助：递归收集节点数 ----
static int countBVHNodes(BVHnode* node) {
	if (!node) return 0;
	return 1 + countBVHNodes(node->lift) + countBVHNodes(node->right);
}

// ---- GPU 迁移辅助：递归拍平（预分配好数组后再填）----
static int flattenRecursive(BVHnode* node, std::vector<GPUBVHNode>& flat,
	const std::unordered_map<Object*, std::pair<int,int>>& triMap,
	const std::unordered_map<Object*, int>& sphereMap) {
	if (!node) return -1;

	int myIdx = (int)flat.size();
	flat.push_back({});
	GPUBVHNode& g = flat.back();

	g.bounds     = node->nodeBox;
	g.leftChild  = -1;
	g.rightChild = -1;
	g.triStart   = 0;
	g.triCount   = 0;
	g.sphereIdx  = -1;

	if (node->lift == nullptr && node->right == nullptr) {
		// 叶子：查三角形
		auto itT = triMap.find(node->obj);
		if (itT != triMap.end()) {
			g.triStart = itT->second.first;
			g.triCount = itT->second.second;
		}
		// 叶子：查球体
		auto itS = sphereMap.find(node->obj);
		if (itS != sphereMap.end()) {
			g.sphereIdx = itS->second;
		}
	} else {
		g.leftChild  = flattenRecursive(node->lift, flat, triMap, sphereMap);
		g.rightChild = flattenRecursive(node->right, flat, triMap, sphereMap);
	}
	return myIdx;
}

// ---- 公开接口：拍平 BVH ----
std::vector<GPUBVHNode> BVHstruct::flattenBVH(
	const std::unordered_map<Object*, std::pair<int,int>>& triMap,
	const std::unordered_map<Object*, int>& sphereMap) const {
	std::vector<GPUBVHNode> result;
	if (!root) return result;

	int total = countBVHNodes(root);
	result.reserve(total);
	flattenRecursive(root, result, triMap, sphereMap);
	return result;
}

void BVHstruct::BuiltBVH( int t )
{
	if (!objects.empty())
	{
		root = recursiveBuildBVH(objects ,t);
	}
	return;
}

void BVHstruct::deleteNode(BVHnode* node)
{
	if (node)
	{
		if (node->lift)deleteNode(node->lift);
		if (node->right)deleteNode(node->right);
		delete node;
	}
}

int BVHstruct::getnextTurn(int now)
{
	if (now == 3)return 1;
	return now + 1;
}

// ---- AABB 表面积（不含常数因子 2，用于 SAH 比较）----
static inline float AABB_SA(const AABB& box) {
	float dx = box.pMax.x - box.pMin.x;
	float dy = box.pMax.y - box.pMin.y;
	float dz = box.pMax.z - box.pMin.z;
	return dx*dy + dy*dz + dz*dx;  // 半表面积，比例等价
}

BVHnode* BVHstruct::recursiveBuildBVH(vector<Object*> objs, int tt) // SAH 构建
{
	BVHnode* new_node = new BVHnode();
	if (objs.size() == 0) return nullptr;

	// 计算父节点包围盒 + 总面积
	AABB parentBox = objs[0]->getAABB();
	float totalArea = objs[0]->getAra();
	for (int i = 1; i < (int)objs.size(); i++) {
		parentBox = AmalgamateTowBox(parentBox, objs[i]->getAABB());
		totalArea += objs[i]->getAra();
	}

	if (objs.size() == 1) {
		new_node->nodeBox = parentBox;
		new_node->area = totalArea;
		new_node->objsCount = 1;
		new_node->obj = objs[0];
		return new_node;
	}

	// ======== SAH：扫所有切分点，选成本最低的 ========
	float parentSA = AABB_SA(parentBox);
	float bestCost = 1e30f;
	int    bestAxis = 0;
	int    bestSplit = 0;

	for (int axis = 0; axis < 3; axis++) {
		// 按当前轴排序
		if (axis == 0) {
			sort(objs.begin(), objs.end(), [](auto a, auto b) {
				return a->getAABB().getCen().x < b->getAABB().getCen().x; });
		} else if (axis == 1) {
			sort(objs.begin(), objs.end(), [](auto a, auto b) {
				return a->getAABB().getCen().y < b->getAABB().getCen().y; });
		} else {
			sort(objs.begin(), objs.end(), [](auto a, auto b) {
				return a->getAABB().getCen().z < b->getAABB().getCen().z; });
		}

		// 从右往左扫，预存右半边的 AABB 和数量
		AABB rightBox = objs.back()->getAABB();
		vector<float> rightSA(objs.size());
		rightSA.back() = AABB_SA(rightBox);
		for (int i = (int)objs.size() - 2; i >= 0; i--) {
			rightBox = AmalgamateTowBox(rightBox, objs[i]->getAABB());
			rightSA[i] = AABB_SA(rightBox);
		}

		// 从左往右扫，算每个切分点的 SAH 成本
		AABB leftBox = objs[0]->getAABB();
		for (int split = 1; split < (int)objs.size(); split++) {
			float cost = AABB_SA(leftBox) * (float)split
			           + rightSA[split] * (float)(objs.size() - split);
			if (cost < bestCost) {
				bestCost = cost;
				bestAxis = axis;
				bestSplit = split;
			}
			leftBox = AmalgamateTowBox(leftBox, objs[split]->getAABB());
		}
	}

	// 如果 SAH 认为叶子更省，或只有 2 个物体（强制分），直接做叶子
	float leafCost = parentSA * (float)objs.size();
	if (bestSplit == 0 || (bestCost >= leafCost && objs.size() <= 4)) {
		// 小节点直接做叶子，把多个物体挂到一个节点上
		new_node->nodeBox = parentBox;
		new_node->area = totalArea;
		new_node->objsCount = (int)objs.size();
		new_node->obj = objs[0];  // 只存第一个，flatten 时通过 triMap 查区间
		return new_node;
	}

	// 按最佳轴重新排序
	if (bestAxis == 0) {
		sort(objs.begin(), objs.end(), [](auto a, auto b) {
			return a->getAABB().getCen().x < b->getAABB().getCen().x; });
	} else if (bestAxis == 1) {
		sort(objs.begin(), objs.end(), [](auto a, auto b) {
			return a->getAABB().getCen().y < b->getAABB().getCen().y; });
	} else {
		sort(objs.begin(), objs.end(), [](auto a, auto b) {
			return a->getAABB().getCen().z < b->getAABB().getCen().z; });
	}

	vector<Object*> leftObjs (objs.begin(),            objs.begin() + bestSplit);
	vector<Object*> rightObjs(objs.begin() + bestSplit, objs.end());

	new_node->lift  = recursiveBuildBVH(leftObjs,  getnextTurn(tt));
	new_node->right = recursiveBuildBVH(rightObjs, getnextTurn(tt));

	new_node->area = new_node->lift->area + new_node->right->area;
	new_node->nodeBox = AmalgamateTowBox(new_node->lift->nodeBox, new_node->right->nodeBox);
	new_node->objsCount = new_node->lift->objsCount + new_node->right->objsCount;
	return new_node;
}

void BVHstruct::gethitposition(Ray &ray, BVHnode *tree , HitPoint &hp)
{
	if (tree->nodeBox.IsHitbox(ray))
	{
		if (tree->lift == nullptr || tree->right == nullptr)
		{
			tree->obj->getHitPoint(ray, hp);
			return;
		}
		else
		{
			HitPoint l;
			gethitposition(ray, tree->lift , l);
			HitPoint r;
			gethitposition(ray, tree->right , r);
			if (l.distance < r.distance)
			{
				hp.distance = l.distance;
				hp.happened = l.happened;
				hp.hitcoord = l.hitcoord;
				hp.hitN = l.hitN;
				hp.m = l.m;
				hp.hitColor = l.hitColor;
			}
			else
			{
				hp.distance = r.distance;
				hp.happened = r.happened;
				hp.hitcoord = r.hitcoord;
				hp.hitN = r.hitN;
				hp.m = r.m;
				hp.hitColor = r.hitColor;
			}
			return;
		}
	}
}


void BVHstruct::getHitposition( Ray &ray , HitPoint &hp) //对外接口
{
	if (objects.size() == 0) return;
	gethitposition(ray, root , hp);
	return;
}

void BVHstruct::samplelight(float now_area, HitPoint &hp, float &pdf_L, BVHnode* tree)
{
	// set it in the future
}

void  BVHstruct::SampleLight(HitPoint &hp, float &pdf_L) //对外接口
{
	/*float all_area = root->area;
	float gs = RandomFloat();
	float aim_area = all_area * gs;
	samplelight(aim_area, hp , pdf_L , root);*/
}
