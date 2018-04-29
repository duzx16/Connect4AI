#include "uct.h"
#include "Judge.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>
#include <sys/time.h>

using std::cout;

#if FILE_OUTPUT && !NO_OUTPUT
std::ostream &output = std::ofstream("log.txt");
#else
std::ostream &output = cout;
#endif

const int max_search_num = 5000000;
const int max_size = 2000000;
const double time_limit = 2.5;

inline int compute_next_player(int player)
{
    if (player == 1)
        return 2;
    else
        return 1;
}

inline double time_diff(const timeval &t1, const timeval &t2)
{
    return t1.tv_sec - t2.tv_sec + static_cast<double>(t1.tv_usec - t2.tv_usec) / 1000000;
}

//进行UCT搜索的函数
Point UCT::uctSearch()
{
    timeval begin_time, current_time;
    struct timezone zone;
    gettimeofday(&begin_time, &zone);
    srand(time(NULL));
    Node *v0 = init_node();
    int search_num = 0;
    while (search_num < max_search_num)
    {
#if TIME_LIMIT
        if (search_num % 10000 == 0)
        {
            gettimeofday(&current_time, &zone);
            if (time_diff(current_time, begin_time) > time_limit)
                break;
        }
#endif
        Node *vl = treePolicy(v0);
#if VALUE_JUDGE
        double reward = biasPolicy(vl->player);
#else
        double reward = defaultPolicy(vl->player);
#endif
        backUp(vl, reward);
        clear();
        search_num += 1;
    }
#if !NO_OUTPUT
    output << search_num << "\n";
#endif
    factory.clear();
    Point action = bestAction(v0);
    return action;
}

//对树进行扩展的函数
Node *UCT::treePolicy(Node *v)
{
    do
    {
#ifdef EXPAND_STEP
        if (v->N > EXPAND_STEP && !v->child_actions.empty() && factory.empty())
#else
            if (!v->child_actions.empty() && factory.empty())
#endif
        {
            return expand(v);
        } else
        {
            if (!v->children.empty())
            {
                v = bestChild(v);
                take_action(v->action, v->player);
            } else
                break;
        }
    } while (_winner == -1);
    return v;
}

Node *UCT::expand(Node *v)
{
    int choice = v->child_actions.pop_back();
    Point action(_state_top[choice] - 1, choice);
    Node *child = factory.newNode(v, v->next_player(), action);
    v->children.push_back(child);
    take_action(action, child->player);
    if (_winner == -1)
    {
        for (int i = 0; i < _N; ++i)
        {
            if (_state_top[i] > 0)
                child->child_actions.push_back(i);
        }
    }
    return child;
}

Node *UCT::bestChild(Node *v)
{
    double max_ucb = -1e16;
    Node *max_node = nullptr;
    for (int i = 0; i < v->children.size(); ++i)
    {
        auto &it = v->children[i];
#if APPLY_UCT
        double ucb = it->Q / it->N + 0.7 * _share_log / sqrt(it->N);
#else
        double ucb = it->Q / it->N;
#endif
        if (not max_node || ucb > max_ucb)
        {
            max_ucb = ucb;
            max_node = it;
        }
    }
    return max_node;
}

double UCT::defaultPolicy(int player)
{
    int current_player = compute_next_player(player), feasible_actions[MAX_N], feasible_num = 0, choice, chosen_y;
    for (int i = 0; i < _N; ++i)
    {
        if (_state_top[i] > 0)
        {
            feasible_actions[feasible_num++] = i;
        }
    }
    while (_winner == -1)
    {
        Point action{-1, -1};
#if MUST_WIN
        int next_player = compute_next_player(current_player);
        for (choice = 0; choice < feasible_num; ++choice)
        {
            chosen_y = feasible_actions[choice];
            Point aim_action(_state_top[chosen_y] - 1, chosen_y);
            if (judgeWin(action.x, action.y, _M, _N, _state_board, current_player))
            {
                action = aim_action;
                break;
            } else if (action.x < 0 and judgeWin(action.x, action.y, _M, _N, _state_board, next_player))
            {
                action = aim_action;
            }
        }
        if (action.x < 0)
#endif
        {
            choice = rand() % feasible_num;
            chosen_y = feasible_actions[choice];
            action = Point(_state_top[chosen_y] - 1, chosen_y);
        }
        take_action(action, current_player);
        if (_state_top[chosen_y] == 0)
        {
            feasible_num--;
            for (int i = choice; i < feasible_num; ++i)
                feasible_actions[i] = feasible_actions[i + 1];
        }
        current_player = compute_next_player(current_player);

    }
    if (_winner == player)
        return 1;
    else if (_winner == 0)
        return 0;
    else
        return -1;
}

#if VALUE_JUDGE

double UCT::biasPolicy(int player)
{
    int current_player = compute_next_player(
            player), feasible_actions[MAX_N], feasible_num = 0, choice, chosen_y, action_scores[MAX_N], total_score;
    for (int i = 0; i < _N; ++i)
    {
        if (_state_top[i] > 0)
        {
            feasible_actions[feasible_num++] = i;
        }
    }
    while (_winner == -1)
    {
        total_score = 0;
        for (choice = 0; choice < feasible_num; ++choice)
        {
            chosen_y = feasible_actions[choice];
            action_scores[choice] = valueJudge(_state_top[chosen_y] - 1, chosen_y, _M, _N, _state_board,
                                               current_player);
            action_scores[choice] += valueJudge(_state_top[chosen_y] - 1, chosen_y, _M, _N, _state_board,
                                                compute_next_player(current_player));
            total_score += action_scores[choice];
        }
        Point action{-1, -1};
        {
            int randnum = rand() % total_score;
            int limit = 0;
            for (choice = 0; choice < feasible_num; ++choice)
            {
                limit += action_scores[choice];
                if (randnum < limit)
                    break;

            }
            chosen_y = feasible_actions[choice];
            action = Point(_state_top[chosen_y] - 1, chosen_y);
        }
        take_action(action, current_player);
        if (_state_top[chosen_y] == 0)
        {
            feasible_num--;
            for (int i = choice; i < feasible_num; ++i)
                feasible_actions[i] = feasible_actions[i + 1];
        }
        current_player = compute_next_player(current_player);

    }
    if (_winner == player)
        return 1;
    else if (_winner == 0)
        return 0;
    else
        return -1;
}

#endif

void UCT::backUp(Node *v, double reward)
{
    while (v != nullptr)
    {
        v->N += 1;
        v->Q += reward;
        reward = -reward;
        v = v->parent;
    }
    _total_num += 1;
#if APPLY_UCT
    _share_log = sqrt(log(_total_num));
#endif
}

void UCT::clear()
{
    for (int i = 0; i < _M; ++i)
        for (int j = 0; j < _N; ++j)
            _state_board[i][j] = _init_board[i][j];
    for (int i = 0; i < _N; ++i)
        _state_top[i] = _init_top[i];
    _winner = -1;
}

Node *UCT::init_node()
{
    Node *node = factory.newNode();
    node->player = compute_next_player(_player);
    for (int i = 0; i < _N; ++i)
    {
        if (_state_top[i] > 0)
            node->child_actions.push_back(i);
    }
    return node;
}

void UCT::take_action(const Point &action, int player)
{
    _state_board[action.x][action.y] = player;
    _state_top[action.y] -= 1;
    if (action.y == _noY && _state_top[action.y] - 1 == _noX)
        _state_top[action.y] -= 1;
    if (judgeWin(action.x, action.y, _M, _N, _state_board, player))
    {
        _winner = player;
    } else if (isTie(_N, _state_top))
    {
        _winner = 0;
    }
}

Point UCT::bestAction(Node *v)
{
    Node *child = bestChild(v);
    if (child)
        return bestChild(v)->action;
    else
        return {-1, -1};
}

void UCT::print_state()
{
    for (int i = 0; i < _M; ++i)
    {
        for (int j = 0; j < _N; ++j)
            cout << _state_board[i][j] << " ";
        cout << "\n";
    }
    cout << "\n";
}

UCT::UCT()
{
    _init_board = new int *[MAX_M];
    _state_board = new int *[MAX_M];
    _init_top = new int[MAX_N];
    _state_top = new int[MAX_N];
    for (int i = 0; i < MAX_M; ++i)
    {
        _init_board[i] = new int[MAX_N];
        _state_board[i] = new int[MAX_N];
    }
}

void UCT::init(int M, int N, int **board, const int *top, int noX, int noY, int player)
{
    _M = M;
    _N = N;
    _noX = noX;
    _noY = noY;
    _winner = -1;
    _player = player;
    _total_num = 0;
    _share_log = 0.0;
    for (int i = 0; i < N; ++i)
    {
        _state_top[i] = _init_top[i] = top[i];
    }
    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            _init_board[i][j] = _state_board[i][j] = board[i][j];
        }
    }
}

inline int Node::next_player()
{
    return compute_next_player(player);
}

inline Node *Factory::newNode(Node *parent, int player, Point action)
{
    if (top == total_size)
    {
        cout << "Doom!!!!\n";
    }
    Node *v = space + top;
    top += 1;
    v->N = 0;
    v->Q = 0;
    v->action = action;
    v->parent = parent;
    v->player = player;
    v->children.clear();
    v->child_actions.clear();
    return v;
}


Factory::Factory() : space(new Node[max_size + 10]), total_size(max_size + 10), top(0)
{
#if !NO_OUTPUT
    output << "Factory built\n";
#endif
}

Factory::~Factory()
{
    delete[]space;
#if !NO_OUTPUT
    output << "Factory destroyed\n";
#endif
}
