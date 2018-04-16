#ifndef AI_PROJECT_UCT_H
#define AI_PROJECT_UCT_H

#include "Point.h"
#include <vector>

//用于调整策略的宏
#define TIME_LIMIT 1
#define MUST_WIN 1

//用于改变输出的宏
#define FILE_OUTPUT 0
#define NO_OUTPUT 0
#define STEP_RECORD 0

#define MAX_M 12
#define MAX_N 12

double getTime();

int compute_next_player(int player);

// the tree node for UCT

template<typename T, int N>
class FixedVector {
private:
    T _data[N];
    unsigned _size = 0;
public:

    bool empty() { return _size == 0; }

    void clear() { _size = 0; }

    void push_back(const T &t) { _data[_size++] = t; }

    unsigned size() { return _size; }

    T &operator[](unsigned i) { return _data[i]; }

    T pop_back()
    {
        T t = _data[--_size];
        return t;
    }
};

struct Node {
    Point action;
    double Q;
    int N;
    int player;
    Node *parent;
    FixedVector<Node *, MAX_N> children;
    FixedVector<int, MAX_N> child_actions;

    Node() : Q(0), N(0), action(0, 0), parent(nullptr), player(1) {}

    int next_player();
};

// the factory for space pool
class Factory {
public:
    Factory();

    ~Factory();

    Node *newNode(Node *parent = nullptr, int player = 1, Point action = {-1, -1});

    bool empty() { return top < total_size; }

    void clear() { top = 0; }

private:
    Node *space;
    int total_size;
    int top;
};

class UCT {
public:
    Point uctSearch();

    UCT();

    void init(int M, int N, int **board, const int *top, int noX, int noY, int player);

    void print_state();

private:
    Factory factory;
    int **_init_board, **_state_board;
    int _winner;
    int _player;
    int _noX, _noY, _M, _N;
    int _total_num;
    double _share_log;
    int *_init_top, *_state_top;

    Node *init_node();

    Point bestAction(Node *v);

    Node *treePolicy(Node *v);

    Node *expand(Node *v);

    Node *bestChild(Node *v);

    double defaultPolicy(int player);

    void backUp(Node *v, double reward);

    void clear();

    void take_action(const Point &action, int player);
};

#endif //AI_PROJECT_UCT_H
