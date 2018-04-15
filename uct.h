#ifndef AI_PROJECT_UCT_H
#define AI_PROJECT_UCT_H

#include "Point.h"
#include <vector>

#define FILE_OUTPUT 1
#define NO_OUTPUT 0
#define MUST_WIN 1
#define TIME_TEST 1
#define STEP_RECORD 0
#define DEBUG 0

double getTime();
int compute_next_player(int player);

// the tree node for UCT
struct Node {
    Point action;
    double Q;
    int N;
    int player;
    Node *parent;
    std::vector<Node *> children;
    std::vector<Point> child_actions;

    Node() : Q(0), N(0), action(0, 0), parent(nullptr), player(1) {}

    int next_player();
};

// the factory for space pool
class Factory
{
public:
    Factory();
    ~Factory();
    Node * newNode();
    bool empty(){return top < total_size;}
    void clear(){top = 0;}
private:
    Node * space;
    int total_size;
    int top;
};

class UCT {
public:
    Point uctSearch();

    UCT();
    void init(int M, int N, int **board, const int *top, int noX, int noY, int player);

private:
    Factory factory;
    int **_init_board, **_state_board;
    int _winner;
    int _player;
    int _noX, _noY, _M, _N;
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

    void judge_winner(const Point &action, int player);

    void print_state();

};

#endif //AI_PROJECT_UCT_H
