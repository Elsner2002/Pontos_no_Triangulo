#include <algorithm>

#include "Ponto.h"

template <typename T>
struct QuadtreeNode {
	QuadtreeNode *nw, *ne, *sw, *se;
	T *data;
	Ponto min, max;

	QuadtreeNode(T *data, Ponto min, Ponto max) {
		this->data = new T(*data);
		nw = ne = sw = se = nullptr;
		this->min = min;
		this->max = max;
	}

	~QuadtreeNode() {
		delete nw;
		delete ne;
		delete sw;
		delete se;
		delete data;
	}

	QuadtreeNode(
		QuadtreeNode *nw, QuadtreeNode *ne,
		QuadtreeNode *sw, QuadtreeNode *se,
		Ponto min, Ponto max
	) {
		this->nw = nw;
		this->ne = ne;
		this->sw = sw;
		this->se = se;
		this->data = nullptr;
		this->min = min;
		this->max = max;
	}

	bool isLeaf();
};

template <typename T>
bool QuadtreeNode<T>::isLeaf() {
	return nw == nullptr;
}

