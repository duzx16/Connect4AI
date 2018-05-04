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

//给1返2给2返1
inline int compute_next_player(int player)
{
    if (player == 1)
        return 2;
    else
        return 1;
}

//求两个时间戳之间的时间差（秒）
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
#if STEP_RECORD
    print_state();
#endif
#endif
    factory.clear();
    Point action = bestAction(v0);
#if ACTION_RECORD
    for (int i = 0; i < v0->children.size(); ++i)
    {
        output << v0->children[i]->upper_bound(_share_log) << " " << v0->children[i]->N << "\n";
    }
    output << "\n";
#endif
    return action;
}

//对树进行扩展的函数
Node *UCT::treePolicy(Node *v)
{
    do
    {
#if EXPAND_STEP > 0
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

//为某一节点扩展子节点
Node *UCT::expand(Node *v)
{
    int choice = v->child_actions.pop_back();
    Point action(_state_top[choice] - 1, choice);
    Node *child = factory.newNode(v, v->next_player(), action);
    v->children.push_back(child);
    take_action(action, child->player);
    if (_winner == -1)
    {
        add_actions(child);
    }
    return child;
}

//按照UCT返回最佳子节点
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

//返回最佳动作
Point UCT::bestAction(Node *v)
{
    //Node *child = bestChild(v);
    Node *child = nullptr;
    int max_N = -1;
    for (int i = 0; i < v->children.size(); ++i)
    {
        auto &it = v->children[i];
        if (it->N > max_N)
        {
            max_N = it->N;
            child = it;
        }
    }
    if (child)
        return child->action;
    else
        return {-1, -1};
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
        int next_player = compute_next_player(current_player), test_y;
        for (int j = 0; j < feasible_num; ++j)
        {
            test_y = feasible_actions[j];
            Point aim_action(_state_top[test_y] - 1, test_y);
            if (judgeWin(aim_action.x, aim_action.y, _M, _N, _state_board, current_player))
            {
                action = aim_action;
                chosen_y = test_y;
                choice = j;
                break;
            } else if (action.x < 0 and judgeWin(aim_action.x, aim_action.y, _M, _N, _state_board, next_player))
            {
                action = aim_action;
                chosen_y = test_y;
                choice = j;
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

//将模拟结果回溯
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

//将board和top回到初始状态
void UCT::clear()
{
    for (int i = 0; i < _M; ++i)
        for (int j = 0; j < _N; ++j)
            _state_board[i][j] = _init_board[i][j];
    for (int i = 0; i < _N; ++i)
        _state_top[i] = _init_top[i];
    _winner = -1;
}

//初始化子节点
Node *UCT::init_node()
{
    Node *node = factory.newNode();
    node->player = compute_next_player(_player);
    add_actions(node);
    return node;
}

//为某个节点设置child_actions
void UCT::add_actions(Node *v)
{
    int next_player = compute_next_player(v->player);
#if MUST_WIN
    bool must_lose = false;
#endif
    for (int i = 0; i < _N; ++i)
    {
        if (_state_top[i] > 0)
        {
#if MUST_WIN
            if (judgeWin(_state_top[i] - 1, i, _M, _N, _state_board, next_player))
            {
                v->child_actions.clear();
                v->child_actions.push_back(i);
                break;
            } else if (judgeWin(_state_top[i] - 1, i, _M, _N, _state_board, v->player))
            {
                if (not must_lose)
                {
                    v->child_actions.clear();
                    must_lose = true;
                }
                v->child_actions.push_back(i);
            } else if (not must_lose)
#endif
            {
                v->child_actions.push_back(i);
            }

        }
    }

}

//采取某个落子并判定胜负
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

//打印当前局面
void UCT::print_state()
{
    for (int i = 0; i < _M; ++i)
    {
        for (int j = 0; j < _N; ++j)
            if (i == _noX and j == _noY)
                cout << "x ";
            else
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

double Node::upper_bound(double log_total)
{
    return Q / N + 0.7 * log_total / sqrt(N);
}

//返回某个节点
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
