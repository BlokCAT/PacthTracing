#pragma
#include "BVHStruct.hpp"
#include <algorithm>
#include <iostream>
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

BVHnode* BVHstruct::recursiveBuildBVH(vector<Object*> objs, int tt) //当前需要创建的节点的
{
	BVHnode* new_node = new BVHnode();
	AABB new_aabb;
	if (objs.size() == 0)return nullptr;
	else if (objs.size() == 1)
	{
		new_aabb = objs[0]->getAABB();
		new_node->area += objs[0]->getAra();
		new_node->nodeBox = new_aabb;
		new_node->objsCount = 1;
		new_node->obj = objs[0];
		//std::cout << "一个叶子节点\n";
		return new_node;
	}
	else if (objs.size() == 2)
	{
		vector<Object*> rightObj;
		vector<Object*> liftObj;

		if (tt == 1)
		{
			if (objs[0]->getAABB().getCen().x > objs[1]->getAABB().getCen().x)
			{
				rightObj.push_back(objs[0]);
				liftObj.push_back(objs[1]);
			}
			else
			{
				rightObj.push_back(objs[1]);
				liftObj.push_back(objs[0]);
			}
		}
		else if (tt == 2)
		{
			if (objs[0]->getAABB().getCen().y > objs[1]->getAABB().getCen().y)
			{
				rightObj.push_back(objs[0]);
				liftObj.push_back(objs[1]);
			}
			else
			{
				rightObj.push_back(objs[1]);
				liftObj.push_back(objs[0]);
			}
		}
		else
		{
			if (objs[0]->getAABB().getCen().z > objs[1]->getAABB().getCen().z)
			{
				rightObj.push_back(objs[0]);
				liftObj.push_back(objs[1]);
			}
			else
			{
				rightObj.push_back(objs[1]);
				liftObj.push_back(objs[0]);
			}
		}
		new_node->lift = recursiveBuildBVH(liftObj, getnextTurn(tt));
		new_node->right = recursiveBuildBVH(rightObj, getnextTurn(tt));

		new_node->area += (new_node->lift->area + new_node->right->area);
		new_node->nodeBox = AmalgamateTowBox(new_node->lift->nodeBox, new_node->right->nodeBox);
		new_node->objsCount += (new_node->lift->objsCount + new_node->right->objsCount);
		return new_node;
	}
	else
	{
		vector<Object*> rightObj;
		vector<Object*> liftObj;

		if (tt == 1)
		{
			sort(objs.begin(), objs.end(), [](auto a, auto b)
				{
					return a->getAABB().getCen().x < b->getAABB().getCen().x;
				});
		}
		else if (tt == 2)
		{
			sort(objs.begin(), objs.end(), [](auto a, auto b)
				{
					return a->getAABB().getCen().y < b->getAABB().getCen().y;
				});
		}
		else
		{
			sort(objs.begin(), objs.end(), [](auto a, auto b)
				{
					return a->getAABB().getCen().z < b->getAABB().getCen().z;
				});
		}

		auto beginning = objs.begin();
		auto middling = objs.begin() + (objs.size() / 2);
		auto ending = objs.end();

		liftObj = vector<Object*>(beginning , middling);
		rightObj = vector<Object*>(middling, ending);

		new_node->lift = recursiveBuildBVH(liftObj, getnextTurn(tt));
		new_node->right = recursiveBuildBVH(rightObj, getnextTurn(tt));

		new_node->area += (new_node->lift->area + new_node->right->area);
		new_node->nodeBox = AmalgamateTowBox(new_node->lift->nodeBox, new_node->right->nodeBox);
		new_node->objsCount += (new_node->lift->objsCount + new_node->right->objsCount);
		return new_node;
	}
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