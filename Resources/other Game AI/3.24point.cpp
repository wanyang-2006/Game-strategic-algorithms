//输入4个整数 1 - 13 后，计算24
#include <stdio.h>   // 提供 scanf / puts 等输入输出函数
#include <stdlib.h>  // 保留原程序头文件（本程序中可不必严格依赖）
#include <string.h>  // 提供 strlen，用于识别字符串 "10"

int num[4];  // 存放四张牌转换后的数值

// 在两个数 a、b 上执行第 n 种运算：
// n=0 -> 加法，n=1 -> 乘法，n=2 -> 绝对差，n=3 -> 可整除时做除法
// 返回 -1 表示该运算在当前约束下无效（例如除不尽）
int Cal(int a, int b, int n)
{
    switch (n)
    {
        case 0:
            return a + b;  // 加法始终有效
        case 1:
            return a * b;  // 乘法始终有效
        case 2:
            if (a > b)
            {
                return a - b;  // 只保留非负结果（等价于取绝对差）
            }
            else
            {
                return b - a;
            }
        case 3:
            if (a >= b && b != 0 && a % b == 0)
            {
                return a / b;  // a 能整除 b
            }
            else if (a < b && a != 0 && b % a == 0)
            {
                return b / a;  // b 能整除 a
            }
            else
            {
                return -1;  // 不能整除时视为无效分支
            }
        default:
            return -1;  // 防御式返回，避免非法 n
    }
}

// 已经得到一个中间值 as（由第1、2张牌计算而来），
// 再依次和剩余两张牌组合，判断能否得到 24。
// 参数 a、b 是已使用的两张牌下标。
bool CalThree(int a, int b, int as)
{
    int i, j, temp;     // i/j 枚举运算符，temp 保存中间结果
    int k = 0, re[2];   // re 存放剩余两张牌的下标

    for (i = 0; i < 4; i++)
    {
        if ((i == a) || (i == b))
        {
            continue;  // 跳过已经用过的两张牌
        }
        re[k++] = i;   // 收集剩余牌的下标
    }

    // 情况1：as 先与 re[0] 运算，再与 re[1] 运算
    for (i = 0; i < 4; i++)
    {
        temp = Cal(as, num[re[0]], i);  // 枚举 as ? re[0]
        if (temp == -1)
        {
            continue;  // 当前运算无效，剪枝
        }

        for (j = 0; j < 4; j++)
        {
            if (Cal(temp, num[re[1]], j) == 24)  // 再与最后一张牌组合
            {
                return true;  // 找到可行表达式
            }
        }
    }

    // 情况2：as 先与 re[1] 运算，再与 re[0] 运算
    // 这一步用于覆盖不同结合顺序，避免漏解
    for (i = 0; i < 4; i++)
    {
        temp = Cal(as, num[re[1]], i);
        if (temp == -1)
        {
            continue;
        }

        for (j = 0; j < 4; j++)
        {
            if (Cal(temp, num[re[0]], j) == 24)
            {
                return true;
            }
        }
    }

    return false;  // 两种顺序都无法得到 24
}

// 已经得到中间值 as（来自两张牌），
// 剩余两张牌先彼此运算成 temp，再与 as 运算。
// 这对应形如 (a ? b) ? (c ? d) 的结构。
bool CalTwo(int a, int b, int as)
{
    int i, j, k = 0, re[2];  // re 存放剩余两张牌
    int temp;                // temp 为剩余两牌运算结果

    for (i = 0; i < 4; i++)
    {
        if (i == a || i == b)
        {
            continue;
        }
        re[k++] = i;
    }

    for (i = 0; i < 4; i++)
    {
        temp = Cal(num[re[0]], num[re[1]], i);  // 先算剩余两牌
        if (temp == -1)
        {
            continue;
        }

        for (j = 0; j < 4; j++)
        {
            if (Cal(as, temp, j) == 24)  // 再与 as 组合
            {
                return true;
            }
        }
    }

    return false;
}

// 固定第一张牌下标 n，枚举第二张牌及运算，
// 然后分别交给 CalThree / CalTwo 覆盖不同括号结构。
bool CalFour(int n)
{
    int i, j, temp;  // i 为第二张牌下标，j 为运算符
    bool flag;       // 记录子过程是否找到解

    for (i = 0; i < 4; i++)
    {
        if (i == n)
        {
            continue;  // 第二张牌不能与第一张重复
        }

        for (j = 0; j < 4; j++)
        {
            temp = Cal(num[n], num[i], j);  // 先算两张牌
            if (temp == -1)
            {
                continue;
            }

            flag = CalThree(n, i, temp);  // 第三张牌与前面结果混合
            if (flag)
            {
                return true;
            }

            flag = CalTwo(n, i, temp);    // 后两张牌先独立运算
            if (flag)
            {
                return true;
            }
        }
    }

    return false;  // 以 n 作为首牌时无解
}

int main()
{
    int i;                   // 循环变量
    char ch[10][5];          // 输入牌面字符串，长度留冗余
    bool ans;                // 当前4张牌是否能算出24

    while (scanf("%s%s%s%s", ch[0], ch[1], ch[2], ch[3]) != EOF)
    {
        // 将输入牌面转换为整数值
        for (i = 0; i < 4; i++)
        {
            if (ch[i][0] == 'A')
            {
                num[i] = 1;
            }
            else if (ch[i][0] == 'J')
            {
                num[i] = 11;
            }
            else if (ch[i][0] == 'Q')
            {
                num[i] = 12;
            }
            else if (ch[i][0] == 'K')
            {
                num[i] = 13;
            }
            else if (ch[i][0] == '1' && strlen(ch[i]) == 2)
            {
                num[i] = 10;  // 识别 "10"
            }
            else
            {
                num[i] = ch[i][0] - '0';  // '2'~'9' 转数字
            }
        }

        ans = false;  // 默认无解

        // 枚举首张牌下标，尝试所有组合
        for (i = 0; i < 4; i++)
        {
            ans = CalFour(i);
            if (ans)
            {
                break;  // 找到解立即停止（剪枝）
            }
        }

        if (ans)
        {
            puts("Yes");
        }
        else
        {
            puts("No");
        }
    }

    return 0;  // 程序正常结束
}
