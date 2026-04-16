#include <stdio.h>
#include "../base/base2.h"
#include "../base/parse.h"
#include "ucci.h"
#include "pregen.h"
#include "position.h"
#include "hash.h"
#include "search.h"

const int INTERRUPT_COUNT = 4096; // 搜索若干结点后调用一次中断，用来检查是否有新指令输入或是否超时

inline void PrintLn(const char *sz) {
  printf("%s\n", sz);
  fflush(stdout); // 引擎与GUI通信通常通过标准输入输出，fflush确保信息立刻发送给GUI，不被缓冲区滞留
}

int main(void) {
  int i;
  bool bPonderTime;        // 标记是否使用后台思考(Ponder)的时间分配策略
  UcciCommStruct UcciComm; // 这是一个非常重要的联合体，用来存放解析后的UCCI指令信息
  PositionStruct posProbe; // 用于 "probe" 指令（探测某个局面状态）的临时局面对象

  // BootLine() 会阻塞等待第一条指令。按UCCI协议，GUI发出的第一条指令必须是 "ucci"
  if (BootLine() != UCCI_COMM_UCCI) {
    return 0; // 如果不是 "ucci" 协议，程序直接退出
  }
  
  // 确定开局库文件 BOOK.DAT 的路径
  LocatePath(Search.szBookFile, "BOOK.DAT"); 
  bPonderTime = false;
  
  // 初始化预生成数组（非常关键！计算各种棋子的合法走法、位行位列等掩码）
  PreGenInit(); 
  
  // 初始化置换表（Hash Table），参数24表示 2^24 字节，即 16MB 的内存
  NewHash(24); 
  
  // 从默认的初始FEN串（cszStartFen）设置初始局面
  Search.pos.FromFen(cszStartFen); 
  Search.pos.nDistance = 0; // 当前搜索树的根节点深度设为0
  Search.pos.PreEvaluate(); // 对初始局面进行预先评估（计算双方物质和位置基础分）
  
  // 初始化搜索参数
  Search.nBanMoves = 0;     // 禁着数初始化为0
  Search.bQuit = Search.bBatch = Search.bDebug = false; // 退出、批处理、调试标志位初始化
  Search.bUseHash = Search.bUseBook = Search.bNullMove = Search.bKnowledge = true; // 默认开启置换表、开局库、空步裁剪、知识评估
  Search.bIdle = false;     // 引擎是否处于空闲状态
  Search.nCountMask = INTERRUPT_COUNT - 1; // 用于位运算的掩码，决定多久检查一次中断
  Search.nRandomMask = 0;   // 随机性掩码
  Search.rc4Random.InitRand(); // 初始化 RC4 随机数生成器

  // 接下来，引擎向 GUI 连续发送 "id" 和 "option" 指令，告知 GUI 引擎的名称、版权以及支持的高级选项
  PrintLn("id name ElephantEye");
  PrintLn("id version 3.31");
  // ... (省略部分版权输出)
  PrintLn("option usemillisec type check default true"); // 告知GUI支持毫秒级时间
  PrintLn("option promotion type check default false");  // 是否支持兵升变规则
  // ... (省略部分 option 输出)
  printf("option bookfiles type string default %s\n", Search.szBookFile);
  fflush(stdout);
  // ... (省略其他 option 输出)
  
  // 引擎初始化完成，发送 "ucciok" 告诉 GUI 可以开始发送实质性指令了
  PrintLn("ucciok");

  // 【核心主循环】：引擎进入待机状态，不断接收和处理 GUI 发来的指令
  while (!Search.bQuit) {
    // IdleLine() 负责在引擎空闲时读取标准输入，并解析为 UcciComm 枚举类型
    switch (IdleLine(UcciComm, Search.bDebug)) {
    case UCCI_COMM_ISREADY:
      PrintLn("readyok"); // 收到 isready，回复 readyok，用于GUI同步状态
      break;
    case UCCI_COMM_STOP:
      PrintLn("nobestmove"); // 空闲时收到 stop，说明没在思考，直接回复无最佳着法
      break;
    case UCCI_COMM_POSITION:
      // 收到设置局面的指令。根据 UcciComm 中解析出的 FEN 串和历史着法构建局面
      BuildPos(Search.pos, UcciComm);
      Search.pos.nDistance = 0;  // 搜索树深度清零
      Search.pos.PreEvaluate();  // 重新对新局面进行预评估
      Search.nBanMoves = 0;      // 清空禁着
      break;
    case UCCI_COMM_BANMOVES:
      // 收到禁着指令，将其存入 Search.wmvBanList 数组中
      Search.nBanMoves = UcciComm.nBanMoveNum;
      for (i = 0; i < UcciComm.nBanMoveNum; i ++) {
        Search.wmvBanList[i] = COORD_MOVE(UcciComm.lpdwBanMovesCoord[i]);
      }
      break;
    case UCCI_COMM_SETOPTION:
      // 解析 setoption 指令，根据用户在 GUI 的设置动态调整引擎内部变量
      switch (UcciComm.Option) {
        // ... (省略选项的具体赋值逻辑，如设置 Search.bUseHash = UcciComm.bCheck 等)
      }
      break;
    case UCCI_COMM_GO:
      // 收到 go 指令，引擎准备开始搜索！
      Search.bPonder = UcciComm.bPonder; // 是否是后台思考（对方走棋时我们偷偷算）
      Search.bDraw = UcciComm.bDraw;     // 是否考虑提和规则
      
      // 判断思考模式（固定深度、固定节点数、还是按时间限制）
      switch (UcciComm.Go) {
      case UCCI_GO_DEPTH:
        Search.nGoMode = GO_MODE_INFINITY; 
        Search.nNodes = 0;
        SearchMain(UcciComm.nDepth); // 启动主搜索函数，限制最大深度
        break;
      // ... (省略节点数和时间的模式处理)
      case UCCI_GO_TIME_MOVESTOGO:
      case UCCI_GO_TIME_INCREMENT:
        Search.nGoMode = GO_MODE_TIMER; // 时间模式
        // 根据加秒制或时段制，计算出每步棋的“适当时限(nProperTimer)”和“最大时限(nMaxTimer)”
        // ...
        SearchMain(UCCI_MAX_DEPTH); // 启动主搜索函数，深度设为最大，靠时间中断来停止
        break;
      }
      break;
    case UCCI_COMM_QUIT:
      Search.bQuit = true; // 收到 quit，退出循环，结束程序
      break;
    default:
      break;
    }
  }
  
  DelHash(); // 释放置换表内存
  PrintLn("bye"); // 礼貌退出
  return 0;
}