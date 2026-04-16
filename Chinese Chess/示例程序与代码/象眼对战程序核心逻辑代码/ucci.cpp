#include "ucci.h"  

//将GUI发送来的字符串，如“position startpos moves h2e2”翻译为内部结构体 UcciCommStruct
static PipeStruct pipeStd; // 封装了标准输入输出管道的结构体

const int MAX_MOVE_NUM = 1024; // 局面最大允许的步数
static char szFen[LINE_INPUT_MAX_CHAR]; // 用于暂存 FEN 串的缓冲区
static uint32_t dwCoordList[MAX_MOVE_NUM]; // 用于暂存着法坐标的数组

// ParsePos: 专门用来解析 position 指令（设置局面）
static bool ParsePos(UcciCommStruct &UcciComm, char *lp) {
  int i;
  // 判断字符串是以 "fen " 开头还是 "startpos"（初始局面）
  if (StrEqvSkip(lp, "fen ")) {
    strcpy(szFen, lp);
    UcciComm.szFenStr = szFen; // 记录 FEN 串指针
  } else if (StrEqv(lp, "startpos")) {
    // startpos 直接替换为标准初始局面的 FEN 串
    UcciComm.szFenStr = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w";
  } else {
    return false; // 格式不对，解析失败
  }
  
  // 查找后面有没有附带 "moves" 关键字（代表在 FEN 基础上走过的历史步数）
  UcciComm.nMoveNum = 0;
  if (StrScanSkip(lp, " moves ")) {
    *(lp - strlen(" moves ")) = '\0'; // 截断字符串，分离 FEN 和 moves
    // 每一个着法坐标占 5 个字符（例如 "h2e2 "），计算总步数
    UcciComm.nMoveNum = MIN((int) (strlen(lp) + 1) / 5, MAX_MOVE_NUM); 
    
    for (i = 0; i < UcciComm.nMoveNum; i ++) {
      // 巧妙的优化：把4个字母的着法（如'h','2','e','2'）直接强转为一个 32位整数 存储，处理极快
      dwCoordList[i] = *(uint32_t *) lp; 
      lp += sizeof(uint32_t) + 1; // 指针向后移动 5 个字节
    }
    UcciComm.lpdwMovesCoord = dwCoordList; // 将解析好的着法数组挂载到结构体上
  }
  return true;
}

// IdleLine: 引擎空闲时的主力解析器
UcciCommEnum IdleLine(UcciCommStruct &UcciComm, bool bDebug) {
  char szLineStr[LINE_INPUT_MAX_CHAR];
  char *lp;

  // 阻塞读取一行输入，如果没有输入则执行 Idle() 让出 CPU 切片
  while (!pipeStd.LineInput(szLineStr)) {
    Idle();
  }
  lp = szLineStr;
  
  // 接下来是一长串的 if-else 匹配，识别 GUI 发了什么命令
  if (StrEqv(lp, "isready")) {
    return UCCI_COMM_ISREADY;
  } else if (StrEqvSkip(lp, "setoption ")) {
    // 内部又是一大串匹配，提取具体的 option 和值...
    // (此处省略)
    return UCCI_COMM_SETOPTION;
  } else if (StrEqvSkip(lp, "position ")) {
    // 如果是 position 指令，交给刚才看的 ParsePos 去干活
    return ParsePos(UcciComm, lp) ? UCCI_COMM_POSITION : UCCI_COMM_UNKNOWN;
  } else if (StrEqvSkip(lp, "go ")) {
    // 解析 go 指令，判断是限定深度(depth)、限定时间(time)还是后台思考(ponder)
    // (此处省略)
    return UCCI_COMM_GO;
  } 
  // ... (省略 stop, quit 等解析)
  else {
    return UCCI_COMM_UNKNOWN; // 无法识别的指令
  }
}