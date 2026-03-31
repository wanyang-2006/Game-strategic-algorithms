//    Input:一局胜负未定的棋。

//   Output:如果先手X能够在接下来的一步棋中走出必胜手，则输出必胜手的位置，如果不行的话，输出#####
 
 
 /*
        Highlights:
                            (1)和以前的黑白棋AI一样，极大极小函数的对偶演绎地非常漂亮
                            (2)定义chess=-4，这里是有其用意的，后面有利用四个回车将其补齐成chess=0
                            (3)判断一盘棋是否结束，还是沿用模拟算法的那四个条件,也就是行，列，对角线和反对角线
                            (4)我们将搜索的深度的最大值定义为12层，如果12层仍然未搜索出结果（也就是说只有1/4的地方有落子），我们就判定为搜索不力而无法找到必胜手
                            (5)将inf设置到100000000实在是有些小题大做，极大值为1，极小值为-1，平局为0，就已经足矣了
 */

#include <iostream>   // 使用输入输出流
#include <algorithm>  // 提供 max/min
#include <cstring>    // 提供 memset
#include <cstdio>     // 提供 scanf/printf
using namespace std;  // 省略 std:: 前缀

#define inf 100000000  // 作为“正无穷”估值，用于极值初始化

// state 为 4x4 棋盘（多开一维方便读入），chess 为当前非空字符计数，xi/xj 记录最佳落点
int state[5][5], chess, xi, xj;
char ch;  // 用于逐字符读取输入流

// 声明极大层与极小层递归函数（Minimax + Alpha-Beta 风格剪枝）
int minfind(int, int, int);
int maxfind(int, int, int);

// 判断最近一次落子点 (x,y) 后，棋局是否已经分出胜负
bool over(int x, int y)
{
    bool flag = false;      // 结束标记
    int row[5], col[5];     // 行列计分数组：x 记 +1，o 记 -1
    memset(row, 0, sizeof(row));  // 行计分清零
    memset(col, 0, sizeof(col));  // 列计分清零

    // 统计每一行和每一列的净和
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            if (state[i][j] == 'x')
            {
                row[i]++;  // x 在该行贡献 +1
                col[j]++;  // x 在该列贡献 +1
            }
            if (state[i][j] == 'o')
            {
                row[i]--;  // o 在该行贡献 -1
                col[j]--;  // o 在该列贡献 -1
            }
        }

    // 只检查与落点相关的那一行/列是否达到绝对值 4（四连）
    if (row[x] == -4 || row[x] == 4 || col[y] == -4 || col[y] == 4)
        flag = true;

    int tot1 = 0, tot2 = 0;  // 主对角线与副对角线净和

    // 统计两条对角线的净和
    for (int i = 0; i < 4; i++)
    {
        if (state[i][i] == 'x') tot1++;
        if (state[i][3 - i] == 'x') tot2++;
        if (state[i][i] == 'o') tot1--;
        if (state[i][3 - i] == 'o') tot2--;
    }

    // 只有落点确实在对应对角线上时，才用对应对角线判胜
    if ((tot1 == 4 || tot1 == -4) && x == y) flag = true;
    if ((tot2 == 4 || tot2 == -4) && x == 3 - y) flag = true;

    return flag;  // 返回是否结束
}

// 极大层：轮到 x 走，目标是最大化局面值
int maxfind(int x, int y, int mini)
{
    int tmp, maxi = -inf;       // maxi 初始化为负无穷
    if (over(x, y)) return maxi;  // 若上一步已终局，当前层视为最差
    if (chess == 16) return 0;    // 棋盘满了，平局

    // 枚举所有空位作为 x 的下一步
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (state[i][j] == '.')
            {
                state[i][j] = 'x';          // 试走
                chess++;                    // 非空计数加一
                tmp = minfind(i, j, maxi);  // 进入对手极小层
                chess--;                    // 回溯计数
                state[i][j] = '.';          // 回溯棋盘
                maxi = max(maxi, tmp);      // 更新当前可达最优值
                if (maxi >= mini) return maxi;  // 剪枝：已不可能更优
            }

    return maxi;  // 返回极大层结果
}

// 极小层：轮到 o 走，目标是最小化局面值
int minfind(int x, int y, int maxi)
{
    int tmp, mini = inf;        // mini 初始化为正无穷
    if (over(x, y)) return mini;  // 若上一步已终局，当前层视为最差（对极小层）
    if (chess == 16) return 0;    // 满盘平局

    // 枚举所有空位作为 o 的下一步
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            if (state[i][j] == '.')
            {
                state[i][j] = 'o';          // 试走
                chess++;                    // 非空计数加一
                tmp = maxfind(i, j, mini);  // 进入 x 的极大层
                chess--;                    // 回溯计数
                state[i][j] = '.';          // 回溯棋盘
                mini = min(mini, tmp);      // 更新当前可达最差值（对 x 来说）
                if (mini <= maxi) return mini;  // 剪枝：已不可能更差
            }
        }

    return mini;  // 返回极小层结果
}

// 在根节点尝试 x 的所有走法，寻找是否存在必胜手
bool tryit()
{
    int tmp, maxi = -inf;  // 根层也按极大层处理

    // 枚举每个可落子点
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (state[i][j] == '.')
            {
                state[i][j] = 'x';       // 假设 x 下在 (i,j)
                chess++;                 // 更新棋子数
                tmp = minfind(i, j, maxi);  // 让对手应对，得到该步价值
                chess--;                 // 回溯棋子数
                state[i][j] = '.';       // 回溯棋盘

                if (tmp >= maxi)
                {
                    maxi = tmp;  // 记录更优价值
                    xi = i;      // 记录最优行
                    xj = j;      // 记录最优列
                }

                if (maxi == inf) return true;  // 出现必胜估值，提前返回
            }

    return false;  // 没有找到必胜手
}

int main()
{
    // 连续读取多个测试局面，直到读到 '$'
    while (scanf("%c", &ch))
    {
        int flag = 0;          // 本局是否已直接输出结果
        if (ch == '$') break;  // 结束输入

        chess = -4;  // 抵消后续读取 4 行行末换行符带来的计数影响

        // 读取 4 行，每行 5 个字符（4 个棋位 + 1 个换行）
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 5; j++)
            {
                scanf("%c", &state[i][j]);
                chess += state[i][j] != '.';  // 非 '.' 字符（含换行）都会使计数增加
            }

        // 早停剪枝：棋子过少时直接认为暂不可判必胜
        if (chess <= 4)
        {
            printf("#####\n");  // 输出无必胜手标记
            flag = 1;           // 标记本局已输出
            continue;           // 读下一局
        }

        if (tryit())
            printf("(%d,%d)\n", xi, xj);  // 输出必胜坐标
        else if (!flag)
            printf("#####\n");            // 没有必胜手
    }

    return 0;  // 正常结束
}