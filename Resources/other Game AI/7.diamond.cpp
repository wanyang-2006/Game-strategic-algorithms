/*
问题描述：
独立钻石跳棋问题。在下图中，33 个方格顶点摆放着 32 枚棋子，仅中央的顶点空着未摆放棋子。
下棋规则是：任一棋子可以沿水平方向或垂直方向跳过与其相邻的棋子，进入空着的顶点，并吃掉被跳过的棋子。
目标：设计一个算法，找出一种下棋方法，使得最终棋盘上只剩下一个棋子且位于棋盘中央。

0------------------------->x
| 0 1 2 3 4 5 6
|     * * *        0                                                   3
|     * * *        1                                                    |
| * * * * * * *    2                                                    |
| * * * . * * *    3      方向规定：2 <----- * -----> 0
| * * * * * * *    4                                                    |
|     * * *        5                                                    |
|     * * *        6                                                    1
y
*/
/*************************************************************************/

#include <iostream> // 控制台输出
#include <fstream>  // 文件输出
using namespace std; // 使用标准命名空间，避免频繁写 std::

struct step // 记录一步移动的信息
{
  int sx, sy; // 起点坐标 (x, y)
  int tx, ty; // 终点坐标 (x, y)
  int dir;    // 本步移动方向：0 右，1 下，2 左，3 上
};

struct step mystack[100], last_step; // 回溯栈与回退时保存的最后一步
char diamond[7][7];                  // 7x7 棋盘：'*' 有子，'.' 空位
int Left_diamond = 32;               // 当前剩余棋子数，初始为 32
int x, y, nx, ny, ndir, top;         // 遍历坐标与回溯起点：nx/ny/ndir 用于断点续搜，top 为栈顶
int flag = 1;                        // 是否成功找到解：1 成功，0 失败

/*****************************初始化棋盘*****************************/
void Init_diamond()
{
  for(int i = 0; i < 7; i++) // 遍历每一行
  {
    for(int j = 0; j < 7; j++) // 遍历每一列
    {
      // 十字棋盘外的四角区域不参与游戏，保持原值不处理
      if((i < 2 || i > 4) && (j < 2 || j > 4));
      else
      {
        diamond[i][j] = '*'; // 十字区域初始化为有棋子
      }
    }
  }
  diamond[3][3] = '.'; // 中央位置初始为空
}
/*********************************************************************/

/*******************************移动棋子******************************/
int Move_diamond(int y, int x, int dir)
{
  if(diamond[y][x] != '*') // 起点没有棋子则不能移动
  {
    return 0;
  }

  struct step temp; // 暂存本次移动记录，成功后压栈

  switch(dir) // 根据方向尝试跳跃
  {
    case 0: // 向右：从 (x,y) 跳到 (x+2,y)
      // 非法条件：越界 / 中间无棋子 / 终点非空
      if(x + 2 > 6 || diamond[y][x + 1] != '*' || diamond[y][x + 2] != '.')
      {
        return 0;
      }
      diamond[y][x] = '.';         // 起点变空
      diamond[y][x + 1] = '.';     // 被跨越棋子被吃掉
      diamond[y][x + 2] = '*';     // 终点放置棋子
      temp.sx = x;                 // 记录起点 x
      temp.sy = y;                 // 记录起点 y
      temp.tx = x + 2;             // 记录终点 x
      temp.ty = y;                 // 记录终点 y
      temp.dir = dir;              // 记录方向
      mystack[top++] = temp;       // 成功移动后压栈，top 后移
      return 1;                    // 返回成功
      break;

    case 1: // 向下：从 (x,y) 跳到 (x,y+2)
      if(y + 2 > 6 || diamond[y + 1][x] != '*' || diamond[y + 2][x] != '.')
      {
        return 0;
      }
      diamond[y][x] = '.';
      diamond[y + 1][x] = '.';
      diamond[y + 2][x] = '*';
      temp.sx = x;
      temp.sy = y;
      temp.tx = x;
      temp.ty = y + 2;
      temp.dir = dir;
      mystack[top++] = temp;
      return 1;
      break;

    case 2: // 向左：从 (x,y) 跳到 (x-2,y)
      if(x - 2 < 0 || diamond[y][x - 1] != '*' || diamond[y][x - 2] != '.')
      {
        return 0;
      }
      diamond[y][x] = '.';
      diamond[y][x - 1] = '.';
      diamond[y][x - 2] = '*';
      temp.sx = x;
      temp.sy = y;
      temp.tx = x - 2;
      temp.ty = y;
      temp.dir = dir;
      mystack[top++] = temp;
      return 1;
      break;

    case 3: // 向上：从 (x,y) 跳到 (x,y-2)
      if(y - 2 < 0 || diamond[y - 1][x] != '*' || diamond[y - 2][x] != '.')
      {
        return 0;
      }
      diamond[y][x] = '.';
      diamond[y - 1][x] = '.';
      diamond[y - 2][x] = '*';
      temp.sx = x;
      temp.sy = y;
      temp.tx = x;
      temp.ty = y - 2;
      temp.dir = dir;
      mystack[top++] = temp;
      return 1;
      break;

    default: // 非法方向
      break;
  }

  return 0; // 兜底返回失败
}

/*********************************************************************/

/*******************************主函数********************************/
void main()
{
  ofstream answer("answer.txt"); // 打开输出文件，用于保存求解过程
  Init_diamond();                 // 初始化棋盘状态
  top = nx = ny = ndir = 0;       // 回溯栈清空；从 (0,0) 且方向 0 开始搜索

  while(1) // 反复进行“前进搜索 + 必要时回溯”
  {
    // 终止条件：只剩 1 个棋子，且位于中心
    if(Left_diamond == 1 && diamond[3][3] == '*')
    {
      break;
    }

    // 从上一次中断位置 (ny,nx,ndir) 继续穷举下一步
    for(y = ny; y < 7; y++, nx = 0)
    {
      for(x = nx; x < 7; x++, ndir = 0)
      {
        for(int dir = ndir; dir < 4; dir++)
        {
          if(Move_diamond(y, x, dir)) // 找到一条可走的边就先走下去（深度优先）
          {
            Left_diamond--;          // 每走一步都会吃掉一个棋子，剩余数减 1
            nx = ny = ndir = 0;      // 下一层从头扫描所有位置与方向
            goto nextstep;           // 跳出三层循环，进入下一轮 while
          }
        }
      }
    }

  nextstep:
    if(y == 7) // 若三层循环自然结束，说明当前局面无后继可走，必须回溯
    {
      top--; // 弹出最近一步，准备撤销

      if(top >= 0) // 仍有历史步可回退
      {
        last_step = mystack[top]; // 取出最近一步

        // 撤销最近一步：把被吃掉的中间棋子恢复
        diamond[(last_step.sy + last_step.ty) / 2][(last_step.sx + last_step.tx) / 2] = '*';
        // 撤销最近一步：把起点恢复为有子
        diamond[last_step.sy][last_step.sx] = '*';
        // 撤销最近一步：把终点恢复为空
        diamond[last_step.ty][last_step.tx] = '.';

        nx = last_step.sx;      // 从这一步的起点重新尝试
        ny = last_step.sy;
        ndir = last_step.dir + 1; // 方向从“上次方向 + 1”开始，避免重复
        Left_diamond++;          // 撤销一步后，棋子数加回 1
      }
      else // top < 0，说明所有分支都尝试完仍无解
      {
        answer << "Can't find any answer, I am sorry." << endl; // 写文件
        cout << "Can't find any answer, I am sorry." << endl;   // 打印控制台
        flag = 0;                                                 // 标记失败
        break;                                                    // 退出主循环
      }
    }
  }

  Init_diamond(); // 重新初始化棋盘，用于按解路径演示每一步

  answer << "The initialization diamond states:" << endl; // 输出初始棋盘
  for(int i = 0; i < 7; i++)
  {
    for(int j = 0; j < 7; j++)
    {
      answer << diamond[i][j] << ' '; // 输出单元格状态
    }
    answer << endl; // 换行到下一行棋盘
  }
  answer << endl << endl; // 空两行，便于阅读

  // 按栈中记录的解路径逐步回放并输出每一步后的棋盘
  for(int n = 0; n < top; n++)
  {
    answer << "step " << n + 1 << ": Move diamond (" << mystack[n].sy + 1 << ","
           << mystack[n].sx + 1 << ") ---> (" << mystack[n].ty + 1 << "," << mystack[n].tx + 1 << ")" << endl;

    diamond[mystack[n].sy][mystack[n].sx] = '.'; // 起点置空
    diamond[(mystack[n].sy + mystack[n].ty) / 2][(mystack[n].sx + mystack[n].tx) / 2] = '.'; // 中间被吃掉
    diamond[mystack[n].ty][mystack[n].tx] = '*'; // 终点落子

    answer << "Left diamonds : " << top - n << endl; // 输出当前剩余棋子（按原程序口径）
    for(int k = 0; k < 7; k++)
    {
      for(int j = 0; j < 7; j++)
      {
        answer << diamond[k][j] << ' '; // 输出当前棋盘
      }
      answer << endl;
    }
    answer << endl << endl; // 每步之间空行
  }

  if(flag) // 有解时提示用户查看结果文件
  {
    cout << "The answer has been exported to file \"answer.txt\" in the same directory.Welcome to see!" << endl;
  }
}
/*********************************************************************/
