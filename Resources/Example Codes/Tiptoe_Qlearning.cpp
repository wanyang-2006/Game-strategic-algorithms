#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <ctime>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdlib>
#include "jsoncpp/json.h"

using namespace std;
int currBotColor;   //我所执子颜色 1:黑 -1:白
int gridInfo[3][3]; // x y记录棋子状态
int blackPieceCount = 0, whitePieceCount = 0;
string blackName = "", whiteName = "";
inline bool inMap(int x, int y)
{
    if (x < 0 || x > 2 || y < 0 || y > 2)
    {
        return false;
    }
    return true;
}
//在坐标处落子，检查是否合法或模拟落子
int ProcStep(int x, int y, int color)
{
    if (color == 0)
        return false;
    if (!inMap(x, y))
        return false;
    if (gridInfo[x][y] != 0)
        return false;
    if (color == 1)
        blackPieceCount++;
    else
        whitePieceCount++;

    gridInfo[x][y] = color;
    return true;
}
int chechIfWin(int gi[3][3])
{ // ret val: 0 No one wins, 1/-1 black/white wins
    vector<int> sum;
    int s1 = 0, s2 = 0;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            s1 += gi[i][j];
            s2 += gi[j][i];
        }
        sum.push_back(s1);
        sum.push_back(s2);
        s1 = 0, s2 = 0;
    }

    for (int i = 0; i < 3; i++)
    {
        s1 += gi[i][i];
        s2 += gi[i][2 - i];
    }
    sum.push_back(s1);
    sum.push_back(s2);

    for (auto &v : sum)
    {
        if (v == 3)
            return 1;
        if (v == -3)
            return -1;
    }
    return 0;
}

class QLearningAgent
{
private:
    unordered_map<string, array<double, 9>> qTable;
    double alpha;
    double gamma;
    double epsilon;

    string encodeState(const array<int, 9> &board) const
    {
        string s;
        s.reserve(9);
        for (int i = 0; i < 9; i++)
        {
            // 采用'0'/'1'/'2'编码空格/己方/对方，压缩状态字符串
            char c = board[i] == 0 ? '0' : (board[i] == 1 ? '1' : '2');
            s.push_back(c);
        }
        return s;
    }

    vector<int> getLegalActions(const array<int, 9> &board) const
    {
        vector<int> acts;
        for (int i = 0; i < 9; i++)
            if (board[i] == 0)
                acts.push_back(i);
        return acts;
    }

    int checkWinnerOnBoard(const array<int, 9> &board) const
    {
        static const int lines[8][3] = {
            {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
            {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
            {0, 4, 8}, {2, 4, 6}};
        for (int i = 0; i < 8; i++)
        {
            int a = lines[i][0], b = lines[i][1], c = lines[i][2];
            int s = board[a] + board[b] + board[c];
            if (s == 3)
                return 1;
            if (s == -3)
                return -1;
        }
        return 0;
    }

    int epsilonGreedyAction(const string &state, const vector<int> &legalActions) const
    {
        if (legalActions.empty())
            return -1;

        if ((double)rand() / RAND_MAX < epsilon)
            return legalActions[rand() % legalActions.size()];

        auto it = qTable.find(state);
        double bestQ = -1e18;
        vector<int> bestActs;
        for (int a : legalActions)
        {
            double q = 0.0;
            if (it != qTable.end())
                q = it->second[a];
            if (q > bestQ + 1e-12)
            {
                bestQ = q;
                bestActs.clear();
                bestActs.push_back(a);
            }
            else if (fabs(q - bestQ) <= 1e-12)
            {
                bestActs.push_back(a);
            }
        }
        return bestActs[rand() % bestActs.size()];
    }

    double maxQ(const string &state, const vector<int> &legalActions) const
    {
        if (legalActions.empty())
            return 0.0;
        auto it = qTable.find(state);
        if (it == qTable.end())
            return 0.0;

        double best = -1e18;
        for (int a : legalActions)
            if (it->second[a] > best)
                best = it->second[a];
        return best;
    }

    void updateQ(const string &state, int action, double reward, const string &nextState, const vector<int> &nextLegal, bool done)
    {
        if (qTable.find(state) == qTable.end())
            qTable[state].fill(0.0);
        if (!done && qTable.find(nextState) == qTable.end())
            qTable[nextState].fill(0.0);

        double target = reward;
        if (!done)
            target += gamma * maxQ(nextState, nextLegal);

        qTable[state][action] += alpha * (target - qTable[state][action]);
    }

public:
    QLearningAgent(double a = 0.12, double g = 0.95, double e = 0.10)
    {
        alpha = a;
        gamma = g;
        epsilon = e;
    }

    void preTrain(int episodes)
    {
        for (int ep = 0; ep < episodes; ep++)
        {
            array<int, 9> board;
            board.fill(0);

            // 随机决定是否让随机对手先手，增强状态覆盖
            if (rand() % 2 == 0)
            {
                vector<int> acts = getLegalActions(board);
                int oppAct = acts[rand() % acts.size()];
                board[oppAct] = -1;
            }

            while (true)
            {
                vector<int> legalActions = getLegalActions(board);
                if (legalActions.empty())
                    break;

                string state = encodeState(board);
                int action = epsilonGreedyAction(state, legalActions);
                if (action < 0)
                    break;

                board[action] = 1;
                int winner = checkWinnerOnBoard(board);
                if (winner == 1)
                {
                    updateQ(state, action, 1.0, "", vector<int>(), true);
                    break;
                }

                vector<int> afterAgentActs = getLegalActions(board);
                if (afterAgentActs.empty())
                {
                    updateQ(state, action, 0.0, "", vector<int>(), true);
                    break;
                }

                // 随机对手行动
                int oppAct = afterAgentActs[rand() % afterAgentActs.size()];
                board[oppAct] = -1;
                winner = checkWinnerOnBoard(board);
                if (winner == -1)
                {
                    updateQ(state, action, -1.0, "", vector<int>(), true);
                    break;
                }

                vector<int> nextLegal = getLegalActions(board);
                if (nextLegal.empty())
                {
                    updateQ(state, action, 0.0, "", vector<int>(), true);
                    break;
                }

                string nextState = encodeState(board);
                updateQ(state, action, 0.0, nextState, nextLegal, false);
            }
        }
    }

    int pickAction(const array<int, 9> &board) const
    {
        vector<int> legalActions = getLegalActions(board);
        if (legalActions.empty())
            return -1;

        string state = encodeState(board);
        auto it = qTable.find(state);
        if (it == qTable.end())
            return legalActions[rand() % legalActions.size()];

        double bestQ = -1e18;
        vector<int> bestActs;
        for (int a : legalActions)
        {
            double q = it->second[a];
            if (q > bestQ + 1e-12)
            {
                bestQ = q;
                bestActs.clear();
                bestActs.push_back(a);
            }
            else if (fabs(q - bestQ) <= 1e-12)
            {
                bestActs.push_back(a);
            }
        }
        return bestActs[rand() % bestActs.size()];
    }
};

int main()
{
    int x, y;

    // 读入JSON
    string str;
    getline(cin, str);
    Json::Reader reader;
    Json::Value input;
    reader.parse(str, input);

    //==================解析json中的内容0=====================\\
	// 分析自己收到的输入和自己过往的输出，并恢复状态
    int turnID = input["responses"].size();
    currBotColor = input["requests"][(Json::Value::UInt)0]["x"].asInt() < 0 ? 1 : -1; // 第一回合收到坐标是-1, -1，说明我是黑方
    for (int i = 0; i < turnID; i++)
    {
        // 根据这些输入输出逐渐恢复状态到当前回合
        x = input["requests"][i]["x"].asInt();
        y = input["requests"][i]["y"].asInt();
        if (x >= 0)
            ProcStep(x, y, -currBotColor); // 模拟对方落子
        x = input["responses"][i]["x"].asInt();
        y = input["responses"][i]["y"].asInt();
        if (x >= 0)
            ProcStep(x, y, currBotColor); // 模拟己方落子
    }

    // 看看自己本回合输入
    x = input["requests"][turnID]["x"].asInt();
    y = input["requests"][turnID]["y"].asInt();
    if (x >= 0)
        ProcStep(x, y, -currBotColor); // 模拟对方落子

    // 找出合法落子点
    int possiblePos[9][2], posCount = 0, choice; // value表示的是这九个地方落子的估值

    for (y = 0; y < 3; y++)
        for (x = 0; x < 3; x++)
            if (gridInfo[x][y] == 0)
            {
                possiblePos[posCount][0] = x;
                possiblePos[posCount++][1] = y;
            }
    // 做出决策（你只需修改以下部分）

    int resultX = -2, resultY = -2;
    if (posCount > 0)
    {
        static bool seeded = false;
        if (!seeded)
        {
            srand((unsigned int)time(0));
            seeded = true;
        }

        QLearningAgent agent;
        agent.preTrain(15000);

        // 将当前局面转换为“己方=1、对方=-1”的标准化表示
        array<int, 9> board;
        for (int yy = 0; yy < 3; yy++)
        {
            for (int xx = 0; xx < 3; xx++)
            {
                int v = gridInfo[xx][yy];
                if (v == 0)
                    board[yy * 3 + xx] = 0;
                else if (v == currBotColor)
                    board[yy * 3 + xx] = 1;
                else
                    board[yy * 3 + xx] = -1;
            }
        }

        int action = agent.pickAction(board);
        if (action >= 0)
        {
            resultX = action % 3;
            resultY = action / 3;
        }
        else
        {
            choice = rand() % posCount;
            resultX = possiblePos[choice][0];
            resultY = possiblePos[choice][1];
        }
    }
    else
    {
        resultX = -1;
        resultY = -1;
    }

    // 决策结束，输出结果（你只需修改以上部分）

    Json::Value ret;
    ret["response"]["x"] = resultX;
    ret["response"]["y"] = resultY;
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;
    return 0;
}