#include <iostream> // C++ 输入输出库
#include <queue>    // 使用 STL 中的 queue 容器实现 BFS
#include <cstdio>   // 使用 scanf/puts 等 C 风格 IO
using namespace std;

const int N = 1001; // 棋盘尺寸上限（为安全分配较大数组）

bool flag; // 最终是否找到符合条件的连通路径（YES/NO）

int n, m, sx, sy, ex, ey; // 棋盘行数n，列数m，查询起点(sx,sy)和终点(ex,ey)

int hash_[N][N], map[N][N]; // map[][]: 棋盘格子上的值（0 表示空），hash[][]: 到达某格子的最少转弯次数上限/记录
// 说明：hash[][] 用于剪枝和记录到达某格子时用到的最少拐点数（turn）。初始化为较大数，只有当
// 当前路径的转弯数 t.turn 小于等于 hash[x][y] 时才入队并更新 hash[x][y]=t.turn。

// 四个方向：下、右、上、左（与原程序中一致）
int dir[4][2] = {{1,0},{0,1},{-1,0},{0,-1}};

struct node {
  int x, y;    // 坐标
  int turn;    // 从起点到该点已经发生的方向改变（转弯）次数
  int d;       // 当前进入该点时的方向（0..3）
} start;

queue<node> q; // BFS 队列，存储待扩展的节点

// 判断节点是否在棋盘内
inline bool in(const node &p) {
  if (p.x < 0 || p.y < 0 || p.x >= n || p.y >= m) return false;
  return true;
}

// 广度优先搜索（BFS），状态包含位置 + 当前方向 + 转弯次数
// 该 BFS 允许最多两次转弯（程序中通过判断 now.turn<=2 达成），并通过 hash[][] 剪枝
void bfs() {
  node now, t;
  while (!q.empty()) {
    now = q.front(); // 取队首
    q.pop();         // 出队

    // 若到达目标且转弯次数不超过 2，则成功
    if (now.x == ex && now.y == ey && now.turn <= 2) {
      flag = true;
      return;
    }

    // 向四个方向尝试前进一步
    for (int i = 0; i < 4; i++) {
      t.x = now.x + dir[i][0]; // 新坐标 x
      t.y = now.y + dir[i][1]; // 新坐标 y

      // 如果方向不变，则转弯数不变；否则转弯数 +1，并记录新方向
      if (now.d == i) {
        t.turn = now.turn;
        t.d = now.d;
      } else {
        t.turn = now.turn + 1;
        t.d = i;
      }

      // 入队条件解释：
      // 1) t 在棋盘内；
      // 2) 要么该格子为空（map==0），要么就是目标格（可以到目标并覆盖非空格子）；
      // 3) 当前到达该格子的转弯次数 t.turn 要优于（<=）之前记录的 hash 值（即可改善或匹配最优）
      if (in(t) && (map[t.x][t.y] == 0 || (t.x == ex && t.y == ey)) && hash_[t.x][t.y] >= t.turn) {
        // 更新到该格的最少转弯记录为当前的转弯数（用于后续剪枝）
        hash_[t.x][t.y] = t.turn;
        q.push(t); // 满足条件则入队继续扩展
      }
    }
  }
}

int main() {
  int i, j, t;
  // 多组输入，直到 n==0 且 m==0 时结束
  while (scanf("%d%d", &n, &m), (n || m)) {
    // 读取棋盘每个格子的值，0 表示空格，非 0 表示带有图案/颜色
    for (i = 0; i < n; i++)
      for (j = 0; j < m; j++)
        scanf("%d", &map[i][j]);

    // 读取要查询的对数（询问次数）
    scanf("%d", &t);

    while (t--) {
      // 读取起点和终点坐标（1-based）
      scanf("%d%d%d%d", &sx, &sy, &ex, &ey);

      // 转为 0-based 索引以匹配数组
      sx--, sy--, ex--, ey--;

      // 初始判定：起点与终点必须为相同颜色且非空，并且不能为同一格
      if ((map[sx][sy] != map[ex][ey]) || map[sx][sy] == 0 || map[ex][ey] == 0 || (sx == ex && sy == ey)) {
        puts("NO");
        continue; // 不满足基本条件直接输出 NO
      }

      // 将 hash[][] 初始化为较大值（这里用 11 表示一个比允许拐点上限大的数），用于记录最少转弯次数
      for (i = 0; i < n; i++)
        for (j = 0; j < m; j++)
          hash_[i][j] = 11;

      // 从起点向四个方向各自作为初始状态入队（起点可以从任意方向出发）
      for (i = 0; i < 4; i++) {
        start.x = sx;
        start.y = sy;
        start.turn = 0; // 起点转弯次数为 0
        start.d = i;    // 设置起始方向
        q.push(start);
      }

      flag = false;          // 重置结果标志
      hash_[sx][sy] = 0;     // 起点到起点的转弯数为 0

      bfs(); // 执行 BFS 搜索，检查是否可以连通且转弯数 <= 2

      // 输出结果：flag 为 true 则 YES，否则 NO
      puts(flag ? "YES" : "NO");
    }
  }

  return 0;
}
