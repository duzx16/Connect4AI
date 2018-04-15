#include <iostream>
#include <cstdlib>
#include "uct.h"
#include "Judge.h"

using std::cout;

int main()
{
    srand(time(NULL));
    for(int n=0;n<100;++n)
    {
        int M = rand() % 4 + 9, N = rand() % 4 + 9, noX = rand() % M, noY = rand() % N;
        M = 12;
        N = 12;
        int **board = new int *[M];
        int *top = new int[N];
        for (int i = 0; i < M; ++i)
        {
            board[i] = new int[N];
            for (int j = 0; j < N; ++j)
            {
                board[i][j] = 0;
                std::string a;
                std::cin >> a;
                if (a == ".")
                    board[i][j] = 0;
                else if (a == "A")
                    board[i][j] = 1;
                else if (a == "B")
                    board[i][j] = 2;
                else
                {
                    board[i][j] = 0;
                    noX = i;
                    noY = j;
                }
            }
        }
        for (int i = 0; i < N; ++i)
        {
            top[i] = 0;
            for (int j = M - 1; j >= 0; --j)
            {
                if (board[j][i] == 0 and (noX != j or noY != i))
                {
                    top[i] = j + 1;
                    break;
                }
            }
        }
        UCT uct_tree;
        for (int i = 0; i < M * N - 1; ++i)
        {
            double begin_time = getTime();
            int player = 2 - i % 2;
            uct_tree.init(M, N, board, top, noX, noY, player);
            Point action = uct_tree.uctSearch();
            if ((getTime() - begin_time) > 3)
            {
                cout << player << "Timeout\n";
            }
            cout << getTime() - begin_time << "\n";
            cout << action.x << " " << action.y << "\n";
            board[action.x][action.y] = player;
            top[action.y] -= 1;
            if (top[action.y] - 1 == noX && action.y == noY)
                top[action.y] -= 1;
            /*for (int i = 0; i < M; ++i)
            {
                for (int j = 0; j < N; ++j)
                    cout << board[i][j] << " ";
                cout << "\n";
            }
            cout << "\n";*/
            if (player == 1 and userWin(action.x, action.y, M, N, board))
            {
                cout << "1 win\n";
                break;
            }
            else if (player == 2 and machineWin(action.x, action.y, M, N, board))
            {
                cout << "2 win\n";
                break;
            }
            else if(isTie(N, top))
            {
                cout <<"Tie\n";
                break;

            }
        }
    }
    return 0;
}