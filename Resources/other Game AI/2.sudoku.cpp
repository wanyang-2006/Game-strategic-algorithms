#include <stdio.h>

int flag = 0, num, map[10][10]; //定义一个棋盘,num代表未填方块的个数
typedef struct
{
    int x, y;
} P;
P p[81];

int judge(int n, int k)
{
    int a, b;
    for (a = 0; a < 9; a++)
    {
        if (map[p[n].x][a] == k && a != p[n].y) //列完全匹配
            return 0;

        if (map[a][p[n].y] == k && a != p[n].x) //行完全匹配
            return 0;
    }

    int x = (p[n].x / 3) * 3;
    int y = (p[n].y / 3) * 3; //找到那个?所在的九宫格

    for (a = 0; a < 3; a++)
        for (b = 0; b < 3; b++)
        {
            if (map[a + x][b + y] == k && ((a + x) != p[n].x) && (b + y) != p[n].y)
                return 0; //新填入的数与九宫格任意的8个数都不重复
        }

    return 1;
}

void dfs(int n)
{
    int i;
    if (num == n) //因为最初是dfs(0)，也就是说如果所有空被填完，就返回
    {
        flag = 1;
        return;
    }

    for (i = 1; i <= 9; i++) //分别尝试填入1--9
    {
        if (judge(n, i))
        {
            map[p[n].x][p[n].y] = i;
            dfs(n + 1); //在第n空的基础上搜索第n+1空
            if (flag)
                return;
            map[p[n].x][p[n].y] = 0; //不行的话，退回原来的状态
        }
    }
}

int main()
{
    int cas = 0, g, f; // g,f分别代表x,y坐标,cas代表有不同的数独难题，但是，第一次输出是不带回车的
    char ch1[2];       //为什么要多出一个空间？考虑到输入数据中每个数之间是有空格的

    while (scanf("%s", ch1) != EOF) //每次读入一个数
    {
        num = 0;
        flag = 0;

        //这里，由于while循环里面已经有了一次读数，所以对第一次读数必须单独对待
        if (ch1[0] != '?')
        {
            map[0][0] = ch1[0] - '0';
        }
        else
        {
            p[num].x = p[num].y = 0;
            num++;
            map[0][0] = 0;
        }

        for (g = 0; g < 9; g++)
        {
            for (f = 0; f < 9; f++)
            {
                if ((g != 0 || f != 0))
                {
                    scanf("%s", ch1);
                    if (ch1[0] != '?')
                        map[g][f] = ch1[0] - '0'; //将字符串存储的棋盘转换为二维数组存储
                    else
                    {
                        map[g][f] = 0;
                        p[num].x = g;
                        p[num].y = f;
                        num++;
                    }
                }
            }
        }

        if (cas)
            printf("\n");
        cas++;
        dfs(0); //深度优先搜索

        //输出
        for (g = 0; g < 9; g++)
        {
            for (f = 0; f < 9; f++)
            {
                if (f == 0)
                    printf("%d", map[g][f]);
                else
                    printf(" %d", map[g][f]); //对输出数据空格的常规性处理
            }
            printf("\n");
        }
    }

    return 0;
}
