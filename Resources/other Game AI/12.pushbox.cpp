/*
   题意抽象：
   在网格中有人和箱子两个实体，目标是把箱子推到终点，代价定义为推箱子次数。
   该程序把状态定义为 (人x, 人y, 箱x, 箱y)，并用优先队列按最小推箱次数扩展状态。
*/

#include <iostream>  // 提供基础输入输出（尽管本程序主要使用 scanf/printf）
#include <queue>     // 提供 priority_queue 容器
#include <cstdio>    // 提供 scanf/printf
#include <cstring>   // 提供 memset

#define N 8          // 地图最大边长（题目数据规模固定为 8x8 以内）

using namespace std; // 使用标准命名空间，简化书写

int arr[N][N];                 // 地图：0=空地，1=墙；读入时 2/3/4 分别表示箱子/目标/人
bool flag[N][N][N][N];         // 访问标记：flag[人x][人y][箱x][箱y] 是否已访问
int n, m;                      // 当前测试用例的地图行数与列数

struct Node                   // 搜索结点，表示一个完整状态
{
  int x;                      // 人的行坐标
  int y;                      // 人的列坐标
  int bx;                     // 箱子的行坐标
  int by;                     // 箱子的列坐标
  int t;                      // 从起点到当前状态的推箱次数


  bool operator < (const Node& b){
    return this->t > b.t;  //priority_queue 会根据类/struct中的operator< 来判断大小关系
  }
};

int sx, sy, bx, by, ex, ey;   // 人起点(sx,sy)、箱起点(bx,by)、箱终点(ex,ey)
int dx[] = {1, -1, 0, 0};     // 四个方向的行增量：下、上、左、右
int dy[] = {0, 0, -1, 1};     // 四个方向的列增量：下、上、左、右

void BFS()                    // 按推箱次数最少搜索可达状态
{
  int i;                      // 方向循环变量
  int a, b;                   // 计算箱子被推动后的目标位置
  priority_queue<Node> Q;     // 优先队列：总是先处理推箱次数更少的状态
  Node curn, nxtn;            // 当前状态与下一状态

  memset(flag, false, sizeof(flag)); // 每个测试用例都要清空访问标记

  flag[sx][sy][bx][by] = true; // 初始状态标记为已访问

  curn.bx = bx;               // 初始化当前状态中的箱子行坐标
  curn.by = by;               // 初始化当前状态中的箱子列坐标
  curn.t = 0;                 // 初始推箱次数为 0
  curn.x = sx;                // 初始化当前状态中的人行坐标
  curn.y = sy;                // 初始化当前状态中的人列坐标

  Q.push(curn);               // 初始状态入队

  while (!Q.empty())          // 队列不空，持续扩展状态
  {
    curn = Q.top();           // 取出当前推箱次数最少的状态
    Q.pop();                  // 弹出队首状态

    if (curn.bx == ex && curn.by == ey) // 若箱子已到终点
    {
      printf("%d\n", curn.t); // 输出最少推箱次数
      return;                 // 找到最优解后结束
    }

    for (i = 0; i < 4; i++)   // 枚举人的四个移动方向
    {
      nxtn.x = curn.x + dx[i]; // 人下一步行坐标
      nxtn.y = curn.y + dy[i]; // 人下一步列坐标
      nxtn.t = curn.t;         // 默认这一步不推箱子，代价先不变

      if (nxtn.x >= 0 && nxtn.x < n && nxtn.y >= 0 && nxtn.y < m && arr[nxtn.x][nxtn.y] != 1)
      {
        if (curn.bx == nxtn.x && curn.by == nxtn.y) // 人下一步正好走到箱子所在格
        {
          a = nxtn.x + dx[i];  // 箱子将被推动到的行坐标（沿同方向再走一格）
          b = nxtn.y + dy[i];  // 箱子将被推动到的列坐标

          if (a >= 0 && b >= 0 && a < n && b < m && arr[a][b] != 1 && !flag[nxtn.x][nxtn.y][a][b])
          {
            nxtn.t++;          // 发生一次推箱，代价 +1
            nxtn.bx = a;       // 更新箱子新位置
            nxtn.by = b;       // 更新箱子新位置
            flag[nxtn.x][nxtn.y][nxtn.bx][nxtn.by] = true; // 标记新状态已访问
            Q.push(nxtn);      // 新状态入队等待后续扩展
          }
        }
        else                   // 人移动到空地，不推箱子
        {
          nxtn.bx = curn.bx;   // 箱子位置保持不变
          nxtn.by = curn.by;   // 箱子位置保持不变

          if (!flag[nxtn.x][nxtn.y][nxtn.bx][nxtn.by]) // 若该状态未访问
          {
            flag[nxtn.x][nxtn.y][nxtn.bx][nxtn.by] = true; // 标记已访问
            Q.push(nxtn);      // 入队继续搜索
          }
        }
      }
    }
  }

  printf("-1\n");            // 队列耗尽仍未到达终点，说明无解
}

int main()                    // 程序入口
{
  int Case;                   // 测试用例数量
  int i, j;                   // 地图遍历下标

  scanf("%d", &Case);         // 读入测试用例数

  while (Case--)              // 逐个处理每个测试用例
  {
    scanf("%d%d", &n, &m);    // 读入地图尺寸

    for (i = 0; i < n; i++)   // 读入整张地图
      for (j = 0; j < m; j++)
      {
        scanf("%d", &arr[i][j]);
      }

    for (i = 0; i < n; i++)   // 扫描地图，提取人/箱子/目标的初始坐标
      for (j = 0; j < m; j++)
      {
        if (arr[i][j] == 2)   // 箱子初始位置
        {
          bx = i;
          by = j;
          arr[i][j] = 0;      // 回填为空地，便于统一判障碍
        }
        else if (arr[i][j] == 3) // 目标位置
        {
          ex = i;
          ey = j;
          arr[i][j] = 0;      // 回填为空地
        }
        else if (arr[i][j] == 4) // 人的初始位置
        {
          sx = i;
          sy = j;
          arr[i][j] = 0;      // 回填为空地
        }
      }

    BFS();                    // 运行搜索并输出当前测试用例答案
  }

  return 0;                   // 正常结束
}
