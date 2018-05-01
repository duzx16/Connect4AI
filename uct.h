#ifndef AI_PROJECT_UCT_H
#define AI_PROJECT_UCT_H

//用于调整策略的宏
#define TIME_LIMIT 1
#define MUST_WIN 1
#define VALUE_JUDGE 1
#define APPLY_UCT 1
//#define EXPAND_STEP 20

//用于改变输出的宏
#define FILE_OUTPUT 0
#define NO_OUTPUT 1
#define STEP_RECORD 0

#include "Point.h"
#include "Judge.h"
#include <vector>

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

    void remove(T data)
    {
        int i = 0;
        for (i = 0; i < _size; ++i)
        {
            if (_data[i] == data)
                break;
        }
        --_size;
        for (; i < _size; ++i)
        {
            _data[i] = _data[i + 1];
        }

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

    void add_actions(Node *v);

    Point bestAction(Node *v);

    Node *treePolicy(Node *v);

    Node *expand(Node *v);

    Node *bestChild(Node *v);

    double defaultPolicy(int player);

    void backUp(Node *v, double reward);

    void clear();

    void take_action(const Point &action, int player);

#if VALUE_JUDGE

    double biasPolicy(int player);

    int actionScore(const int x, const int y, int player);

    int actionScore(const int x, const int y);
#endif
};

#if VALUE_JUDGE
inline int UCT::actionScore(const int x, const int y, int player)
{
    int score = 0, small_i, small_j, large_i, large_j, current_count = 1, count = 0, next_player = compute_next_player(
            player);
    bool limited = false;
    for (small_j = y - 1; small_j > y - 4 && small_j > 0; --small_j)
    {
        if (_state_board[x][small_j] == player)
            current_count += 1;
        else if (_state_board[x][small_j] == player or (x == _noX and small_j == _noY))
            break;

    }
    for (large_j = y + 1; large_j < _N && large_j < small_j + 4; large_j++)
    {
        if (_state_board[x][large_j] == player)
            current_count += 1;
        else if (_state_board[x][large_j] == player or (x == _noX and large_j == _noY))
        {
            limited = true;
            break;
        }
    }
    if (!limited)
    {
        do
        {
            count = myMax(count, current_count);
            if (_state_board[x][++small_j] == player)
                current_count--;
            if (_state_board[x][++large_j] == player)
                current_count++;
            else if (_state_board[x][large_j] == next_player or (x == _noX and large_j == _noY))
                break;
        } while (large_j <= 1);
    }
    return score;
}

inline int UCT::actionScore(const int x, const int y)
{
    int count[2][4] = {0};
    bool find[2] = {false, false};
    for (int i = y - 1; i > y - 4 && i > 0; --i)
    {
        if (_state_board[x][i] == 1 && !find[2])
        {
            for (int j = 0; j < 4 - y + i; ++j)
            {
                count[1][j + 1] += 1;
                count[2][j + 1] = 0;
            }
        }
    }
}
#endif

#endif //AI_PROJECT_UCT_H
