//是男人就下100层——求最短下落时间，递归与分治
#include <stdio.h>
#include<memory.h> 
#include<stdlib.h>             //为了开启 Mycompare和memset
#define MAX_N 1000 
#define INFINITE 1000000 //很大的数，展示不可能性
int t,n, x, y, max; // t为时间,n为板子的数目,max为每次可以跳的最大高度
struct Platform 
{
    int lx, rx, h;
}; //关于平板的结构体
Platform aPlatform[MAX_N + 10];
int aLeftMinTime[MAX_N + 10];
int aRightMinTime[MAX_N + 10];
int MyCompare(const void *e1, const void *e2)
{
    Platform *p1, *p2;
    p1 = (Platform *)e1;
    p2 = (Platform *)e2;
    return p2->h - p1->h; //作为后面用memset的铺垫，从大到小排序
}
int MinTime(int L, bool bLeft) //递归函数
{
    int y = aPlatform[L].h;
    int x, i;
    if (bLeft) //判断上一次掉落是左边落下的还是右边落下的
        x = aPlatform[L].lx;
    else x = aPlatform[L].rx;
    for (i = L + 1; i <= n; i++)
    {
        if (aPlatform[i].lx <= x && aPlatform[i].rx >= x) break;
    } //寻找一个适合降落的点，至上而下寻找
    if (i <= n) 
    {
        if (y - aPlatform[i].h > max)  return INFINITE;
    }else 
    {
        if (y > max)  return INFINITE;
        else  return y; //这里处理最后一次跳落
    }
    //每次过程都有一个竖过程和一个横过程
    int nLeftTime = y - aPlatform[i].h + x - aPlatform[i].lx;
    int nRightTime = y - aPlatform[i].h + aPlatform[i].rx - x;
    //利用这两个数组记录子结果，节约了不少时间
    if (aLeftMinTime[i] == -1) aLeftMinTime[i] = MinTime(i, true);
    if (aRightMinTime[i] == -1) aRightMinTime[i] = MinTime(i, false);
    //这里的左时间和右时间应该是基于第一次的选择
    nLeftTime += aLeftMinTime[i];
    nRightTime += aRightMinTime[i];
    if (nLeftTime < nRightTime) return nLeftTime;
    return nRightTime;
}
int main()
{
    scanf("%d", &t);
    for (int i = 0; i < n; i++)
    {
        memset(aLeftMinTime, -1, sizeof(aLeftMinTime));
        memset(aRightMinTime, -1, sizeof(aRightMinTime));
        scanf("%d%d%d%d", &n, &x, &y, &max);
        //比较有创意的第0个板子
        aPlatform[0].lx = x;
        aPlatform[0].rx = x;
        aPlatform[0].h = y;
        for (int j = 1; j <= n; j++)scanf("%d%d%d", &aPlatform[j].lx, &aPlatform[j].rx, &aPlatform[j].h);
        //用自定义的排序方式进行快排
        qsort(aPlatform, n + 1, sizeof(Platform), MyCompare);
        //输出，最初从左边跳和右边跳是一回事
        printf("%d\n", MinTime(0, true));
    }
    return 0;
}