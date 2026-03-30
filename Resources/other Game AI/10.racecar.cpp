/*
  程序思路概述：
  1) 使用线性同余法生成伪随机数。
  2) 对每一行随机尝试放置战车（rook），并把该战车可攻击到的位置标记为不可再放。
  3) 重复大量次随机实验，取出现过的最大放置数量作为答案近似值。
*/

#include <iostream>    // 输入输出
#include <fstream>     // 保留原始头文件（本程序未使用）
#include <time.h>      // time() 用于随机种子
#include <stdlib.h>    // 标准库（本程序几乎未显式使用）
using namespace std;
// 线性同余随机数参数：new_seed = multiplier * old_seed + adder
const unsigned long multiplier = 1194211693L;
const unsigned long adder = 12345L;

// 随机数类：封装种子与取随机数接口
class CRandomNum
{
private:
    unsigned long nSeed;  // 当前随机种子

public:
    CRandomNum(unsigned long s = 0);         // 构造函数：允许外部传种子
    unsigned short Random(unsigned long n);  // 返回 [0, n-1] 范围随机数
};

// 构造函数：若未提供种子，则用当前时间作为初始种子
CRandomNum::CRandomNum(unsigned long s)
{
    if (s == 0)          // 调用者未指定种子
        nSeed = time(0); // 用当前时间秒值做种
    else
        nSeed = s;       // 使用外部给定种子
}

// 产生一个 [0, n-1] 的伪随机整数
unsigned short CRandomNum::Random(unsigned long n)
{
    nSeed = multiplier * nSeed + adder;          // 线性同余推进种子
    return (unsigned short)((nSeed >> 16) % n);  // 取高位并对 n 取模
}

// 在 (x, y) 尝试放置战车：
// b 数组中 1 表示该格被占用或被攻击，0 表示当前可放置。
// a 数组中 'X' 表示障碍物，会阻断攻击线。
bool CheckPlaceChe(int **b, char **a, int x, int y, int n)
{
    if (b[x][y] == 1)  // 当前位置已不可放置
        return false;  // 放置失败
    else
    {
        b[x][y] = 1;  // 成功放置战车，占用当前位置

        // 向左扫描：直到边界或遇到障碍物为止
        if ((y >= 1) && (y <= n))
        {
            for (int i = y - 1; i >= 0; i--)
            {
                if ((b[x][i] == 1) && (a[x][i] == 'X'))  // 障碍物阻挡
                    break;
                else
                    b[x][i] = 1;  // 标记为受攻击位置
            }
        }

        // 向右扫描：直到边界或遇到障碍物为止
        if ((y <= n - 1) && (y >= 0))
        {
            for (int i = y + 1; i < n; i++)
            {
                if ((b[x][i] == 1) && (a[x][i] == 'X'))  // 障碍物阻挡
                    break;
                else
                    b[x][i] = 1;  // 标记为受攻击位置
            }
        }

        // 向上扫描：直到边界或遇到障碍物为止
        if ((x >= 1) && (x <= n))
        {
            for (int j = x - 1; j >= 0; j--)
            {
                if ((b[j][y] == 1) && (a[j][y] == 'X'))  // 障碍物阻挡
                    break;
                else
                    b[j][y] = 1;  // 标记为受攻击位置
            }
        }

        // 向下扫描：直到边界或遇到障碍物为止
        if ((x >= 0) && (x <= n - 1))
        {
            for (int j = x - 1; j < n; j++)
            {
                if ((b[j][y] == 1) && (a[j][y] == 'X'))  // 障碍物阻挡
                    break;
                else
                    b[j][y] = 1;  // 标记为受攻击位置
            }
        }

        return true;  // 当前格放置成功
    }
}

// 单次随机实验：按“逐行”处理，估计可放置战车数量
int MaxChes(int **b, char **a, int *x, int n)
{
    int max1 = 0;          // 本次实验放置到的战车数量
    static CRandomNum rnd; // 静态随机数对象，跨调用保留种子状态

    for (int i = 0; i < n; i++)  // 逐行尝试
    {
        bool flag = false;  // flag=true 表示该行还存在可尝试位置

        do
        {
            for (int j = 0; j < n; j++)  // 扫描整行，判断是否还有 0
            {
                if (b[i][j] == 0)
                {
                    flag = true;  // 找到至少一个可用格
                    break;
                }
                else
                {
                    flag = false; // 当前格不可用，继续看下一个格
                    continue;
                }
            }

            // 若该行仍有可放置位置：随机选一个列坐标再尝试放置
            if (flag)
            {
                x[i] = rnd.Random(n);          // 随机列号
                if (CheckPlaceChe(b, a, i, x[i], n))
                    max1++;                    // 放置成功则计数 +1
            }
        }
        while (flag); // 当该行再无可尝试位置时结束本行
    }

    return max1; // 返回本轮实验得到的放置数量
}

int main()
{
    int i, j, n;    // 循环变量与棋盘规模
    cin >> n;       // 输入棋盘边长 n

    // a: 原始棋盘，保存字符（如 'X' 表示障碍）
    char **a = new char *[n];
    for (i = 0; i < n; i++)
        a[i] = new char[n];

    // 读入 n*n 的棋盘字符
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            cin >> a[i][j];

    // b: 状态棋盘，1 表示不可放（障碍或攻击覆盖），0 表示可放
    int **b = new int *[n];
    for (i = 0; i < n; i++)
        b[i] = new int[n];

    int max = 0;    // 所有随机实验中的最优值

    // x[i] 记录第 i 行当前随机到的列坐标
    int *x = new int[n];
    for (i = 0; i < n; i++)
        x[i] = 0;

    int t = 0;      // 实验计数器

    // 重复做 15000 次随机实验，提升命中较优解的概率
    while (t < 15000)
    {
        // 每轮实验先根据 a 重置 b
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                if (a[i][j] == 'X')
                    b[i][j] = 1;  // 障碍格初始为不可放
                else
                    b[i][j] = 0;  // 空地初始为可放
            }
        }

        int max2 = MaxChes(b, a, x, n); // 做一轮随机放置
        if (max < max2)
            max = max2; // 刷新历史最优值

        t++;            // 进入下一轮实验
    }

    cout << max;        // 输出估计得到的最大可放置数

    // 释放动态内存
    delete[] x;
    for (i = 0; i < n; i++)
        delete[] b[i];
    delete[] b;

    return 0;           // 程序正常结束
}