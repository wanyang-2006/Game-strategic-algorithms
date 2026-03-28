#include <iostream> // 引入输入输出库，用于调试或扩展（此程序使用 C 风格 IO）
#include <queue> // 使用 STL 的队列容器来实现 BFS
using namespace std; // 使用 std 命名空间，便于书写

// 定义用于 BFS 的节点结构体：包含坐标、已连的长度、方向标记和是否经过空点标志
struct Node {
    char x, y;    // x,y：当前点的坐标（0..14）
    char time;    // time：从起点到当前连通的长度（步数）
    char dirc;    // dirc：当前连通方向（-1 表示未定方向），0..3 分别对应四个方向
    bool no_stone; // no_stone：表示在这条连线上是否已经跨过一次空位（true 表示已跨过）
};

char map[15][15]; // 二维棋盘数组，'.' 表示空，'W' 白子，'B' 黑子
char c; // 当前待检测的棋子颜色（'W' 或 'B'）——程序通过棋子数推断当前落子方

// 定义四个方向向量：右、下、左下、右下（覆盖横、竖、两条斜线的其中一组）
int dir[4][2] = {{0,1}, {1,0}, {1,-1}, {1,1}};

// BFS 搜索函数：从 (x,y) 出发，判断是否存在恰好五子连成一线的情况
// 注意：本算法允许在连线中使用一次空位（'.'）作为延伸，但不能超过一次
bool bfs(int x, int y) {
    queue<Node> Q;       // 用队列实现广度优先搜索
    Node first, next;    // first 为当前出队节点，next 为要入队的下一个节点
    int i;

    // 初始化起点为 (x,y)
    first.x = x;         // 起点横坐标
    first.y = y;         // 起点纵坐标
    first.no_stone = false; // 起点不是通过空点来的，因此 no_stone 为 false
    first.time = 1;      // 起点计为第 1 格
    first.dirc = -1;     // 起点方向未定，用 -1 表示
    Q.push(first);       // 将起点加入队列

    while (!Q.empty()) {
        first = Q.front();
        Q.pop();

        // 如果当前连通长度为 5，并且在这条路径上没有使用过空位（即恰好五个自己的棋子或允许一次空但未使用）
        if (first.time == 5 && first.no_stone) {
            return true; // 找到满足条件的五子连，返回胜利
        }

        // 尝试四个方向扩展
        for (i = 0; i < 4; i++) {
            // 只有在方向未定或保持当前方向时才继续（保证连成一直线）
            if (first.dirc == -1 || first.dirc == i) {
                next.x = first.x + dir[i][0];
                next.y = first.y + dir[i][1];

                // 检查边界
                if (next.x >= 0 && next.x < 15 && next.y >= 0 && next.y < 15) {
                    next.dirc = i;                // 记录方向
                    next.time = first.time + 1;   // 连通长度加 1

                    // 如果下一个位置是同色棋子，则继承 no_stone 状态并入队继续扩展
                    if (map[next.x][next.y] == c) {
                        next.no_stone = first.no_stone;
                        Q.push(next);
                    }
                    // 否则如果是空位，并且之前没有使用过空位，则把这个空位视作一次允许的“跳过”并继续
                    else if (map[next.x][next.y] == '.' && !first.no_stone) {
                        next.no_stone = true; // 标记已使用空位一次
                        Q.push(next);
                    }
                    // 对手的棋子或已使用过空位的情况下遇到空位则不继续
                }
            }
        }
    }
    return false; // BFS 结束未找到符合条件的五子连，返回 false
}

int main() {
    int T;                 // 测试用例数
    scanf("%d", &T);      // 读取测试用例数量

    while (T--) {
        int i, j;
        int white_num = 0, black_num = 0; // 统计当前棋盘上白子和黑子的数量
        getchar(); // 读取并丢弃上一行的换行符

        // 读取 15 行棋盘，每行 15 个字符
        for (i = 0; i < 15; i++) {
            gets(map[i]); // 将第 i 行读入 map[i]（注意：gets 不安全，但保持原代码风格）
        }

        // 统计棋子数量用于判断轮到谁下（黑子先下，若黑子比白子多，则轮到白）
        for (i = 0; i < 15; i++) {
            for (j = 0; j < 15; j++) {
                if (map[i][j] == 'W') {
                    white_num++;
                } else if (map[i][j] == 'B') {
                    black_num++;
                }
            }
        }

        // 如果黑子比白子多，说明当前轮到白子落子；否则轮到黑子
        if (black_num > white_num) {
            c = 'W';
        } else {
            c = 'B';
        }

        bool flag = true; // flag 用于表示是否没有找到五子连（true 表示未找到）

        // 遍历棋盘，找任意一个与 c 相同的棋子作为起点进行 BFS 检测
        // 只要找到一个满足条件的连珠，就可以提前结束
        for (i = 0; i < 15 && flag; i++) {
            for (j = 0; j < 15 && flag; j++) {
                if (map[i][j] == c) {
                    flag = !bfs(i, j); // 如果 bfs 返回 true（找到五子连），则 flag 置为 false
                }
            }
        }

        // 输出结果：找到返回 YES，否则 NO
        if (flag) printf("NO\\n");
        else printf("YES\\n");
    }

    return 0; // 程序正常结束
}