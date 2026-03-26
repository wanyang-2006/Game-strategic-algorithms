#include <iostream>             // 提供标准输入输出流（cin/cout）
#include <sstream>              // 提供字符串流（本文件当前未使用，但常用于字符串解析）
#include <string>               // 提供 std::string
#include <cmath>                // 提供 fabs 等数学函数
#include <ctime>                // 提供 time() 用于随机种子
#include <vector>               // 提供动态数组 std::vector
#include <array>                // 提供定长数组 std::array
#include <unordered_map>        // 提供哈希表 std::unordered_map
#include <cstdlib>              // 提供 rand/srand/RAND_MAX
#include "jsoncpp/json.h"      // 引入 jsoncpp，读写 Bot 协议 JSON

using namespace std;            // 使用标准命名空间，简化代码书写

int currBotColor;               // 当前 Bot 执子颜色：1 表示黑方，-1 表示白方
int gridInfo[3][3];             // 棋盘状态：gridInfo[x][y]，0 空，1 黑，-1 白
int blackPieceCount = 0;        // 当前局面黑子数量（用于状态统计）
int whitePieceCount = 0;        // 当前局面白子数量（用于状态统计）
string blackName = "";          // 黑方名称（当前逻辑未使用）
string whiteName = "";          // 白方名称（当前逻辑未使用）

// 段落总结：
// 1) 以上是依赖库与全局状态定义。
// 2) 棋盘使用 3x3 二维数组保存。
// 3) 颜色编码统一为 1/-1/0，后续判断胜负和训练都依赖该约定。

inline bool inMap(int x, int y) // 判断坐标是否在 3x3 棋盘范围内
{
    if (x < 0 || x > 2 || y < 0 || y > 2) // 只允许 [0,2] 区间
    {
        return false;                      // 越界，返回 false
    }
    return true;                           // 未越界，返回 true
}

// 段落总结：
// inMap 只负责边界检查，不修改任何状态。

// 在坐标处落子：可用于真实落子，也可用于“回放历史”时模拟落子
int ProcStep(int x, int y, int color)
{
    if (color == 0)                        // 颜色不能为 0（0 代表空格）
        return false;                      // 非法颜色，落子失败
    if (!inMap(x, y))                      // 坐标越界
        return false;                      // 越界落子失败
    if (gridInfo[x][y] != 0)               // 目标格已有棋子
        return false;                      // 重复占位，落子失败

    if (color == 1)                        // 若为黑子
        blackPieceCount++;                 // 黑子数量 +1
    else                                   // 否则按白子处理（color == -1）
        whitePieceCount++;                 // 白子数量 +1

    gridInfo[x][y] = color;                // 写入棋盘状态
    return true;                           // 落子成功
}

// 段落总结：
// ProcStep 把“合法性检查 + 棋盘更新 + 棋子计数更新”封装在一起。

int chechIfWin(int gi[3][3])               // 检测胜负：返回 0(未分胜负)、1(黑胜)、-1(白胜)
{
    vector<int> sum;                       // 保存 8 条线（3 行 + 3 列 + 2 对角线）的和
    int s1 = 0, s2 = 0;                    // s1 用于行和，s2 用于列和

    for (int i = 0; i < 3; i++)            // 遍历每一行/列
    {
        for (int j = 0; j < 3; j++)        // 聚合第 i 行与第 i 列
        {
            s1 += gi[i][j];                // 累加第 i 行
            s2 += gi[j][i];                // 累加第 i 列
        }
        sum.push_back(s1);                 // 记录该行的和
        sum.push_back(s2);                 // 记录该列的和
        s1 = 0, s2 = 0;                    // 为下一行/列重置
    }

    for (int i = 0; i < 3; i++)            // 计算两条对角线
    {
        s1 += gi[i][i];                    // 主对角线
        s2 += gi[i][2 - i];                // 副对角线
    }
    sum.push_back(s1);                     // 保存主对角线和
    sum.push_back(s2);                     // 保存副对角线和

    for (auto &v : sum)                    // 检查是否存在绝对值为 3 的线
    {
        if (v == 3)                        // 三个黑子连线
            return 1;                      // 黑胜
        if (v == -3)                       // 三个白子连线
            return -1;                     // 白胜
    }
    return 0;                              // 无人连线，暂未分胜负
}

// 段落总结：
// chechIfWin 利用“同线三格求和”的技巧来判断三连，逻辑简洁且高效。

class QLearningAgent
{
private:
    unordered_map<string, array<double, 9>> qTable; // Q 表：状态字符串 -> 9 个动作的 Q 值
    double alpha;                                    // 学习率，控制新信息覆盖旧信息的速度
    double gamma;                                    // 折扣因子，衡量未来收益权重
    double epsilon;                                  // epsilon-greedy 中的随机探索概率

    string encodeState(const array<int, 9> &board) const // 将棋盘状态编码成字符串键
    {
        string s;                                     // 准备状态字符串
        s.reserve(9);                                 // 预分配容量，避免重复扩容
        for (int i = 0; i < 9; i++)                   // 遍历 9 个格子
        {
            // 采用字符编码：'0' 空位，'1' 己方，'2' 对方
            char c = board[i] == 0 ? '0' : (board[i] == 1 ? '1' : '2');
            s.push_back(c);                           // 拼接到状态字符串
        }
        return s;                                     // 返回可哈希状态表示
    }

    // 段落总结：
    // encodeState 把棋盘变成哈希键，便于在 unordered_map 中快速读写 Q 值。

    vector<int> getLegalActions(const array<int, 9> &board) const // 找出所有合法动作（空位）
    {
        vector<int> acts;                             // 保存合法动作下标（0~8）
        for (int i = 0; i < 9; i++)                  // 枚举每个格子
            if (board[i] == 0)                        // 空格才可下
                acts.push_back(i);                    // 记录合法动作
        return acts;                                  // 返回合法动作列表
    }

    // 段落总结：
    // getLegalActions 负责动作空间裁剪，保证后续决策不落到非法位置。

    int checkWinnerOnBoard(const array<int, 9> &board) const // 在一维棋盘上判断胜负
    {
        static const int lines[8][3] = {              // 8 条三连线的下标组合
            {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
            {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
            {0, 4, 8}, {2, 4, 6}};

        for (int i = 0; i < 8; i++)                   // 检查每条线
        {
            int a = lines[i][0], b = lines[i][1], c = lines[i][2]; // 取三格索引
            int s = board[a] + board[b] + board[c];   // 求和判断三连
            if (s == 3)                                // 己方三连
                return 1;
            if (s == -3)                               // 对方三连
                return -1;
        }
        return 0;                                      // 无胜者
    }

    // 段落总结：
    // checkWinnerOnBoard 与 chechIfWin 思路一致，只是输入换成了一维数组。

    int epsilonGreedyAction(const string &state, const vector<int> &legalActions) const // epsilon-greedy 选动作
    {
        if (legalActions.empty())                      // 没有可行动作
            return -1;                                 // 返回无效动作

        if ((double)rand() / RAND_MAX < epsilon)       // 以 epsilon 概率探索
            return legalActions[rand() % legalActions.size()]; // 随机挑一个合法动作

        auto it = qTable.find(state);                  // 否则走利用：查当前状态的 Q 值
        double bestQ = -1e18;                          // 当前最优 Q，先置极小值
        vector<int> bestActs;                          // 可能有多个并列最优动作

        for (int a : legalActions)                     // 遍历合法动作
        {
            double q = 0.0;                            // 若状态未出现过，默认 Q=0
            if (it != qTable.end())                    // 若状态存在于 Q 表
                q = it->second[a];                     // 读取该动作 Q 值

            if (q > bestQ + 1e-12)                     // 更优动作（带浮点容差）
            {
                bestQ = q;                             // 更新最优 Q
                bestActs.clear();                      // 清空旧候选
                bestActs.push_back(a);                 // 记录当前最优动作
            }
            else if (fabs(q - bestQ) <= 1e-12)         // 与最优值近似相等
            {
                bestActs.push_back(a);                 // 加入并列最优集合
            }
        }

        return bestActs[rand() % bestActs.size()];     // 并列最优中随机挑一个
    }

    // 段落总结：
    // epsilonGreedyAction 平衡“探索未知动作”和“利用当前最优动作”。

    double maxQ(const string &state, const vector<int> &legalActions) const // 计算状态下可行动作的最大 Q
    {
        if (legalActions.empty())                      // 无可行动作
            return 0.0;                                // 最大 Q 记为 0

        auto it = qTable.find(state);                  // 查状态是否在 Q 表中
        if (it == qTable.end())                        // 若未学习过此状态
            return 0.0;                                // 按 0 处理

        double best = -1e18;                           // 最大值初始化为极小
        for (int a : legalActions)                     // 遍历合法动作
            if (it->second[a] > best)                  // 发现更大 Q
                best = it->second[a];                  // 更新最大 Q
        return best;                                   // 返回 max_a Q(s,a)
    }

    // 段落总结：
    // maxQ 用于 TD 目标中的“下一状态最优估计值”。

    void updateQ(const string &state, int action, double reward, const string &nextState,
                 const vector<int> &nextLegal, bool done) // 执行一次 Q-learning 更新
    {
        if (qTable.find(state) == qTable.end())        // 若当前状态首次出现
            qTable[state].fill(0.0);                   // 初始化 9 个动作 Q 值为 0

        if (!done && qTable.find(nextState) == qTable.end()) // 若未终局且下一状态未出现
            qTable[nextState].fill(0.0);               // 初始化下一状态

        double target = reward;                        // TD 目标先等于即时奖励
        if (!done)                                     // 若不是终局
            target += gamma * maxQ(nextState, nextLegal); // 加上折扣后的未来最优价值

        // Q(s,a) <- Q(s,a) + alpha * (target - Q(s,a))
        qTable[state][action] += alpha * (target - qTable[state][action]);
    }

    // 段落总结：
    // updateQ 是核心学习步骤，依据 TD 误差迭代更新动作价值。

public:
    QLearningAgent(double a = 0.12, double g = 0.95, double e = 0.10) // 构造函数：设置超参数
    {
        alpha = a;                                      // 学习率
        gamma = g;                                      // 折扣率
        epsilon = e;                                    // 探索率
    }

    // 段落总结：
    // 构造函数允许外部调整训练风格；默认值偏向稳定收敛且保留少量探索。

    void preTrain(int episodes)                         // 与随机对手进行若干局自我训练
    {
        for (int ep = 0; ep < episodes; ep++)          // 重复训练 episodes 局
        {
            array<int, 9> board;                       // 单局棋盘（一维表示）
            board.fill(0);                             // 初始化为空盘

            // 随机决定是否由对手先手，提升状态覆盖面
            if (rand() % 2 == 0)
            {
                vector<int> acts = getLegalActions(board);     // 获取空位
                int oppAct = acts[rand() % acts.size()];       // 对手随机走一步
                board[oppAct] = -1;                            // 记为对方落子
            }

            while (true)                                // 一局内循环对弈直到终局
            {
                vector<int> legalActions = getLegalActions(board); // 我方合法动作
                if (legalActions.empty())               // 无路可走（平局或满盘）
                    break;

                string state = encodeState(board);      // 编码当前状态
                int action = epsilonGreedyAction(state, legalActions); // 按策略选动作
                if (action < 0)                         // 无有效动作
                    break;

                board[action] = 1;                      // 执行我方落子
                int winner = checkWinnerOnBoard(board); // 判断是否我方直接获胜
                if (winner == 1)
                {
                    updateQ(state, action, 1.0, "", vector<int>(), true); // 终局奖励 +1
                    break;                               // 本局结束
                }

                vector<int> afterAgentActs = getLegalActions(board); // 我方走后剩余空位
                if (afterAgentActs.empty())             // 若已满盘
                {
                    updateQ(state, action, 0.0, "", vector<int>(), true); // 平局奖励 0
                    break;
                }

                int oppAct = afterAgentActs[rand() % afterAgentActs.size()]; // 对手随机应对
                board[oppAct] = -1;                     // 落对手子
                winner = checkWinnerOnBoard(board);     // 判断对手是否获胜
                if (winner == -1)
                {
                    updateQ(state, action, -1.0, "", vector<int>(), true); // 失败奖励 -1
                    break;
                }

                vector<int> nextLegal = getLegalActions(board); // 进入下一轮前可行动作
                if (nextLegal.empty())                  // 满盘平局
                {
                    updateQ(state, action, 0.0, "", vector<int>(), true); // 奖励 0
                    break;
                }

                string nextState = encodeState(board);  // 编码后继状态
                updateQ(state, action, 0.0, nextState, nextLegal, false); // 非终局，奖励 0
            }
        }
    }

    // 段落总结：
    // preTrain 用随机对手快速构建基础 Q 表，在线对局时直接利用学习结果。

    int pickAction(const array<int, 9> &board) const    // 实战决策：根据当前局面选最优动作
    {
        vector<int> legalActions = getLegalActions(board); // 先取合法动作
        if (legalActions.empty())                      // 无合法动作
            return -1;                                 // 返回无效

        string state = encodeState(board);             // 编码当前状态
        auto it = qTable.find(state);                  // 查 Q 表
        if (it == qTable.end())                        // 若该状态没学过
            return legalActions[rand() % legalActions.size()]; // 回退为随机合法动作

        double bestQ = -1e18;                          // 最优 Q 初值
        vector<int> bestActs;                          // 并列最优动作容器
        for (int a : legalActions)                     // 扫描合法动作
        {
            double q = it->second[a];                  // 当前动作 Q 值
            if (q > bestQ + 1e-12)                     // 更优
            {
                bestQ = q;                             // 更新最优值
                bestActs.clear();                      // 清空旧最优集合
                bestActs.push_back(a);                 // 记录新最优动作
            }
            else if (fabs(q - bestQ) <= 1e-12)         // 并列最优
            {
                bestActs.push_back(a);                 // 一并保留
            }
        }
        return bestActs[rand() % bestActs.size()];     // 并列最优随机挑选
    }
};

// 段落总结：
// QLearningAgent 完整封装了：状态编码、动作筛选、胜负判断、训练更新、实战选点。

int main()
{
    int x, y;                                          // 暂存输入/输出坐标

    // 读入平台输入的一整行 JSON 文本
    string str;                                        // JSON 原文
    getline(cin, str);                                 // 从标准输入读取整行
    Json::Reader reader;                               // JSON 解析器
    Json::Value input;                                 // 解析后的 JSON 对象
    reader.parse(str, input);                          // 将字符串解析到 input

    // 段落总结：
    // 这一段把平台发送的局面数据加载为可访问的 JSON 结构。

    // 解析历史回合并恢复当前棋盘状态
    int turnID = input["responses"].size();           // 已经完成的我方响应回合数
    currBotColor = input["requests"][(Json::Value::UInt)0]["x"].asInt() < 0 ? 1 : -1;
    // 首回合若收到 (-1,-1)，说明我们先手（黑方），否则为白方

    for (int i = 0; i < turnID; i++)                  // 回放每个已完成回合
    {
        x = input["requests"][i]["x"].asInt();        // 读取对手第 i 手 x
        y = input["requests"][i]["y"].asInt();        // 读取对手第 i 手 y
        if (x >= 0)                                    // 若不是开局占位值
            ProcStep(x, y, -currBotColor);             // 以“对手颜色”模拟落子

        x = input["responses"][i]["x"].asInt();       // 读取我方第 i 手 x
        y = input["responses"][i]["y"].asInt();       // 读取我方第 i 手 y
        if (x >= 0)                                    // 若该手有效
            ProcStep(x, y, currBotColor);              // 以“我方颜色”模拟落子
    }

    // 段落总结：
    // 通过“requests + responses”双序列回放，棋盘被恢复到当前待决策时刻。

    x = input["requests"][turnID]["x"].asInt();      // 读取本回合对手刚下的 x
    y = input["requests"][turnID]["y"].asInt();      // 读取本回合对手刚下的 y
    if (x >= 0)                                        // 若本回合对手落子有效
        ProcStep(x, y, -currBotColor);                 // 更新棋盘到“我方将要落子”的状态

    // 段落总结：
    // 这一步把“对手刚刚落下的一手”补到棋盘上，确保我方决策基于最新局面。

    int possiblePos[9][2];                             // 存储所有可下位置坐标
    int posCount = 0;                                  // 可下位置数量
    int choice;                                        // 随机回退时的下标

    for (y = 0; y < 3; y++)                            // 遍历棋盘每一行
        for (x = 0; x < 3; x++)                        // 遍历棋盘每一列
            if (gridInfo[x][y] == 0)                   // 空位才可落子
            {
                possiblePos[posCount][0] = x;          // 记录该空位 x
                possiblePos[posCount++][1] = y;        // 记录该空位 y，并计数 +1
            }

    // 段落总结：
    // 先提取全部合法落子点，为 Q-learning 或随机回退做准备。

    int resultX = -2, resultY = -2;                    // 初始化输出坐标（调试友好初值）
    if (posCount > 0)                                  // 若仍有可落子位置
    {
        static bool seeded = false;                    // 保证随机种子只初始化一次
        if (!seeded)
        {
            srand((unsigned int)time(0));              // 用当前时间设随机种子
            seeded = true;                             // 标记已初始化
        }

        QLearningAgent agent;                          // 创建 Q-learning 智能体
        agent.preTrain(15000);                         // 先进行 15000 局自对弈训练

        array<int, 9> board;                           // 构造智能体所需一维棋盘
        for (int yy = 0; yy < 3; yy++)                // 扫描二维棋盘行
        {
            for (int xx = 0; xx < 3; xx++)            // 扫描二维棋盘列
            {
                int v = gridInfo[xx][yy];              // 取原始棋盘格值
                if (v == 0)                            // 空格
                    board[yy * 3 + xx] = 0;            // 映射为空
                else if (v == currBotColor)            // 我方棋子
                    board[yy * 3 + xx] = 1;            // 统一映射为 +1
                else                                   // 对方棋子
                    board[yy * 3 + xx] = -1;           // 统一映射为 -1
            }
        }

        int action = agent.pickAction(board);          // 用训练得到的 Q 表选动作
        if (action >= 0)                               // 若动作有效
        {
            resultX = action % 3;                      // 一维下标转 x
            resultY = action / 3;                      // 一维下标转 y
        }
        else                                           // 若异常未选出动作
        {
            choice = rand() % posCount;                // 从合法位置随机选一个
            resultX = possiblePos[choice][0];          // 取随机点 x
            resultY = possiblePos[choice][1];          // 取随机点 y
        }
    }
    else                                               // 若没有可下位置（理论上是满盘）
    {
        resultX = -1;                                  // 约定返回 -1
        resultY = -1;                                  // 约定返回 -1
    }

    // 段落总结：
    // 决策流程是：可下则训练并选最优，不可下则返回 (-1,-1)，并有随机兜底分支。

    Json::Value ret;                                   // 构造输出 JSON
    ret["response"]["x"] = resultX;                  // 写入决策 x
    ret["response"]["y"] = resultY;                  // 写入决策 y
    Json::FastWriter writer;                           // JSON 快速写出器
    cout << writer.write(ret) << endl;                 // 输出到标准输出
    return 0;                                          // 程序正常结束
}

// 段落总结：
// main 完成：读取输入 -> 回放棋局 -> 训练与决策 -> 输出 JSON。