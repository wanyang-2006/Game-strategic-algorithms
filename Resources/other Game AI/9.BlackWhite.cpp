//假设双方都采用最优策略，计算最终比赛结果，并输出 White、Black 或 Draw。

#include <fstream>   // 提供文件输入输出流
#include <cstring>   // 提供 strcmp
#include <cassert>   // 提供 assert

using namespace std; // 使用标准命名空间，简化书写

ifstream filein("stone.in");   // 输入文件：棋盘与当前执子方
ofstream fileout("stone.out"); // 输出文件：最终结果

int grid[8][8];                 // 棋盘：1 表示白子，-1 表示黑子，0 表示空位
int num;                        // 当前空位总数(题目约束不超过 10)
int emptyPos[10][2];            // 记录每个空位坐标
int enable[10];                 // 记录该空位是否尚未被填过(用于递归回溯)
const int path[8][2] = {        // 八个方向：右、左、下、上、右下、右上、左下、左上
    {0, 1},
    {0, -1},
    {1, 0},
    {-1, 0},
    {1, 1},
    {1, -1},
    {-1, 1},
    {-1, -1}
};

int black();                    // 前向声明：white() 中会调用 black()

int cal()                       // 统计终局棋子数量并返回结果
{
    int i, j;                   // 循环变量
    int whiteCount = 0;         // 白子计数
    int blackCount = 0;         // 黑子计数

    for (i = 0; i < 8; i++)     // 遍历每一行
        for (j = 0; j < 8; j++) // 遍历每一列
        {
            if (grid[i][j] == 1) whiteCount++;         // 白子加一
            else if (grid[i][j] == -1) blackCount++;   // 黑子加一
            else assert(grid[i][j] == 0);              // 非白非黑就必须是空位
        }

    if (whiteCount > blackCount) return 1;             // 白胜
    if (whiteCount == blackCount) return 0;            // 平局
    return -1;                                         // 黑胜
}

int white()                    // 白方回合：选择使结果最大的落子
{
    int i, j, k;               // 通用循环变量
    int u, v;                  // 当前尝试落子的坐标
    int loop;                  // 方向或数组遍历变量
    int temp1, temp2;          // 沿方向移动的临时坐标
    int res = -1;              // 白方要最大化：初始设为最差值
    int tempRes;               // 某一步递归返回值
    int rem[32][2];            // 本次落子会翻转的棋子坐标(最多不超过 32)
    int remNum;                // 翻转总数
    int hasValidMove = 0;      // 是否存在合法落子

    for (i = 0; i < num; i++)  // 枚举每个空位
        if (enable[i])         // 只处理当前仍可用空位
        {
            enable[i] = 0;     // 标记该空位本层已占用
            u = emptyPos[i][0];// 取出行坐标
            v = emptyPos[i][1];// 取出列坐标
            grid[u][v] = 1;    // 白方先把子落下
            remNum = 0;        // 清空翻转列表计数

            for (loop = 0; loop < 8; loop++) // 检查 8 个方向是否能夹击
            {
                for (j = u, k = v;          // 从新落子点出发
                     j + path[loop][0] >= 0 && j + path[loop][0] < 8 &&
                     k + path[loop][1] >= 0 && k + path[loop][1] < 8 &&
                     grid[j + path[loop][0]][k + path[loop][1]] != 0;)
                {
                    j += path[loop][0];     // 向该方向前进一格
                    k += path[loop][1];     // 向该方向前进一格

                    if (grid[j][k] == grid[u][v]) // 遇到同色终点，可能形成夹击
                    {
                        for (temp1 = u + path[loop][0], temp2 = v + path[loop][1];
                             temp1 != j || temp2 != k;
                             temp1 += path[loop][0], temp2 += path[loop][1])
                        {
                            rem[remNum][0] = temp1; // 记录要翻转的行坐标
                            rem[remNum][1] = temp2; // 记录要翻转的列坐标
                            remNum++;               // 翻转列表数量加一
                        }
                        break;                      // 当前方向处理完毕
                    }
                }
            }

            if (remNum > 0)                    // 有可翻转棋子才是合法落子
            {
                for (loop = 0; loop < remNum; loop++)
                {
                    j = rem[loop][0];          // 取要翻转位置行
                    k = rem[loop][1];          // 取要翻转位置列
                    grid[j][k] = -grid[j][k];  // 翻转颜色
                }

                tempRes = black();             // 轮到黑方递归搜索
                hasValidMove = 1;              // 标记至少存在一步合法棋
                if (tempRes > res) res = tempRes; // 白方取更大值(极大化)

                for (loop = 0; loop < remNum; loop++)
                {
                    j = rem[loop][0];          // 恢复行
                    k = rem[loop][1];          // 恢复列
                    grid[j][k] = -grid[j][k];  // 回溯：恢复翻转前状态
                }
            }

            grid[u][v] = 0;                    // 回溯：撤销本次落子
            enable[i] = 1;                     // 回溯：恢复空位可用标记
        }

    if (!hasValidMove) return cal();           // 若无合法落子，则按当前盘面计分
    return res;                                // 返回白方该层最优值
}

int black()                    // 黑方回合：选择使结果最小的落子
{
    int i, j, k;               // 通用循环变量
    int u, v;                  // 当前尝试落子的坐标
    int loop;                  // 方向或数组遍历变量
    int temp1, temp2;          // 沿方向移动的临时坐标
    int res = 1;               // 黑方要最小化：初始设为最差值
    int tempRes;               // 某一步递归返回值
    int rem[32][2];            // 本次落子会翻转的棋子坐标
    int remNum;                // 翻转总数
    int hasValidMove = 0;      // 是否存在合法落子

    for (i = 0; i < num; i++)  // 枚举每个空位
        if (enable[i])         // 只处理当前仍可用空位
        {
            enable[i] = 0;     // 标记该空位本层已占用
            u = emptyPos[i][0];// 取出行坐标
            v = emptyPos[i][1];// 取出列坐标
            grid[u][v] = -1;   // 黑方先把子落下
            remNum = 0;        // 清空翻转列表计数

            for (loop = 0; loop < 8; loop++) // 检查 8 个方向是否能夹击
            {
                for (j = u, k = v;          // 从新落子点出发
                     j + path[loop][0] >= 0 && j + path[loop][0] < 8 &&
                     k + path[loop][1] >= 0 && k + path[loop][1] < 8 &&
                     grid[j + path[loop][0]][k + path[loop][1]] != 0;)
                {
                    j += path[loop][0];     // 向该方向前进一格
                    k += path[loop][1];     // 向该方向前进一格

                    if (grid[j][k] == grid[u][v]) // 遇到同色终点，可能形成夹击
                    {
                        for (temp1 = u + path[loop][0], temp2 = v + path[loop][1];
                             temp1 != j || temp2 != k;
                             temp1 += path[loop][0], temp2 += path[loop][1])
                        {
                            rem[remNum][0] = temp1; // 记录要翻转的行坐标
                            rem[remNum][1] = temp2; // 记录要翻转的列坐标
                            remNum++;               // 翻转列表数量加一
                        }
                        break;                      // 当前方向处理完毕
                    }
                }
            }

            if (remNum > 0)                    // 有可翻转棋子才是合法落子
            {
                for (loop = 0; loop < remNum; loop++)
                {
                    j = rem[loop][0];          // 取要翻转位置行
                    k = rem[loop][1];          // 取要翻转位置列
                    grid[j][k] = -grid[j][k];  // 翻转颜色
                }

                tempRes = white();             // 轮到白方递归搜索
                hasValidMove = 1;              // 标记至少存在一步合法棋
                if (tempRes < res) res = tempRes; // 黑方取更小值(极小化)

                for (loop = 0; loop < remNum; loop++)
                {
                    j = rem[loop][0];          // 恢复行
                    k = rem[loop][1];          // 恢复列
                    grid[j][k] = -grid[j][k];  // 回溯：恢复翻转前状态
                }
            }

            grid[u][v] = 0;                    // 回溯：撤销本次落子
            enable[i] = 1;                     // 回溯：恢复空位可用标记
        }

    if (!hasValidMove) return cal();           // 若无合法落子，则按当前盘面计分
    return res;                                // 返回黑方该层最优值
}

int main()                                     // 主函数：负责输入、求解与输出
{
    char ch;                                   // 读取单个棋盘字符
    char str[100];                             // 读取字符串(首行、终止标记、执子方)
    int i, j;                                  // 循环变量

    for (;;)                                   // 反复读取多组测试数据
    {
        filein >> str;                         // 读取一行字符串或结束标记
        if (strcmp(str, "EndOfInput") == 0) break; // 遇到结束标记则退出

        for (i = 0; i < 8; i++)                // 先解析第 0 行(已在 str 中)
        {
            if (str[i] == 'w') grid[0][i] = 1;    // 白子
            else if (str[i] == 'b') grid[0][i] = -1; // 黑子
            else if (str[i] == 'e') grid[0][i] = 0;  // 空位
        }

        for (i = 1; i < 8; i++)                // 再读取第 1~7 行
            for (j = 0; j < 8; j++)
            {
                filein >> ch;                  // 逐字符读取棋盘元素
                if (ch == 'w') grid[i][j] = 1; // 白子
                else if (ch == 'b') grid[i][j] = -1; // 黑子
                else if (ch == 'e') grid[i][j] = 0;  // 空位
            }

        num = 0;                                // 重新统计空位总数
        for (i = 0; i < 8; i++)                 // 遍历棋盘
            for (j = 0; j < 8; j++)
                if (grid[i][j] == 0)            // 如果是空位
                {
                    emptyPos[num][0] = i;       // 记录行坐标
                    emptyPos[num][1] = j;       // 记录列坐标
                    enable[num] = 1;            // 标记该空位可用
                    num++;                      // 空位计数加一
                }

        filein >> str;                          // 读取当前执子方(WHITE 或 BLACK)

        if (strcmp(str, "WHITE") == 0)         // 若白方先手
        {
            i = white();                        // 计算最优博弈结果
            if (i == 1) fileout << "White" << endl; // 输出白胜
            else if (i == 0) fileout << "Draw" << endl; // 输出平局
            else if (i == -1) fileout << "Black" << endl; // 输出黑胜
            else assert(0);                     // 理论上不会到这里
        }
        else if (strcmp(str, "BLACK") == 0)    // 若黑方先手
        {
            i = black();                        // 计算最优博弈结果
            if (i == 1) fileout << "White" << endl; // 输出白胜
            else if (i == 0) fileout << "Draw" << endl; // 输出平局
            else if (i == -1) fileout << "Black" << endl; // 输出黑胜
            else assert(0);                     // 理论上不会到这里
        }
        else assert(0);                         // 执子方标记非法
    }

    return 0;                                   // 程序正常结束
}