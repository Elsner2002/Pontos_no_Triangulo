template <typename T>
struct QuadtreeNode {
	QuadtreeNode *nw, *ne, *sw, *se;
	T *data;

	QuadtreeNode(T *data) {
		this->data = data;
		nw = ne = sw = se = nullptr;
	}

	QuadtreeNode(
		QuadtreeNode *nw, QuadtreeNode *ne,
		QuadtreeNode *sw, QuadtreeNode *se
	) {
		this->nw = nw;
		this->ne = ne;
		this->sw = sw;
		this->se = se;
		this->data = nullptr;
	}

	bool isLeaf();
};

template <typename T>
bool QuadtreeNode<T>::isLeaf() {
	return nw == nullptr;
}

