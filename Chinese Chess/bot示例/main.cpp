#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <cassert>
#include <algorithm>
#include <vector>
#include <time.h>
#include <stdexcept>
#include "jsoncpp/json.h"

using std::string;
using std::vector;
using std::map;
using std::cin;
using std::cout;
using std::endl;

class TimeOutException : public std::runtime_error {
public:
    TimeOutException() : std::runtime_error("Time out") {}
};

clock_t startTime;
const int TIME_LIMIT_MS = 900;// 安全及时900ms

inline void checkTime() {
    clock_t now = clock();//转换为毫秒
    int duration_ms = (now - startTime) * 1000 / CLOCKS_PER_SEC; 
    
    if (duration_ms > TIME_LIMIT_MS) {
        throw TimeOutException(); // 超时直接抛出异常，打断搜索
    }
}


// === 全局宏定义与常量 ===
#define BOARDWIDTH 9  // 棋盘宽度（9列）
#define BOARDHEIGHT 10 // 棋盘高度（10行）

// 棋子类型枚举：空、帅/将、相/象、马、车、兵/卒、炮、仕/士
enum stoneType {None = 0, King=1, Bishop=2, Knight=3, Rook=4, Pawn=5, Cannon=6, Assistant=7};
// 阵营类型枚举：黑方、红方、空位置
enum colorType {BLACK=0, RED=1, EMPTY = 2};

int baseLine[2] = {9, 0}; // 黑方和红方的底线y坐标
char stoneSym[] = "*kbnrpca"; // 棋子类型的字符表示（便于打印调试）
// 字符到棋子类型的映射字典
map<char,stoneType> charSym = {{stoneSym[0], None}, {stoneSym[1], King},
                               {stoneSym[2], Bishop}, {stoneSym[3], Knight},
                               {stoneSym[4], Rook}, {stoneSym[5], Pawn},
                               {stoneSym[6], Cannon}, {stoneSym[7], Assistant}};

// === 移动方向偏移量数组 ===
int dx[] = {-1,-1,-1,0,0,1,1,1}; // 8个方向的x偏移
int dy[] = {-1,0,1,-1,1,-1,0,1}; // 8个方向的y偏移
int dx_ob[] = {-1,1,-1,1};       // 斜向移动（仕）的x偏移
int dy_ob[] = {-1,-1,1,1};       // 斜向移动（仕）的y偏移
int dx_strai[] = {-1,0,0,1};     // 直线移动（车、炮、帅）的x偏移
int dy_strai[] = {0,-1,1,0};     // 直线移动（车、炮、帅）的y偏移
int dx_lr[] = {-1,1};            // 左右移动（兵过河后）的x偏移

// 马的走法（日字）
int dx_knight[] = {-2,-2,-1,-1,1,1,2,2};
int dy_knight[] = {-1,1,-2,2,-2,2,-1,1};
// 对应的“马腿”位置（用于判断是否撇马腿）
int dx_knight_foot[] = {-1,-1,0,0,0,0,1,1};
int dy_knight_foot[] = {0,0,-1,1,-1,1,0,0};

// 象的走法（田字）
int dx_bishop[] = {-2,-2,2,2};
int dy_bishop[] = {-2,2,-2,2};
// 对应的“象眼”位置（用于判断是否塞象眼）
int dx_bishop_eye[] = {-1,-1,1,1};
int dy_bishop_eye[] = {-1,1,-1,1};


// === 坐标系转换工具函数 ===
// 将 PGN 格式的列字符 (a-i) 转换为整数 (0-8)
int pgnchar2int(char c) {
    return (int)c - (int)'a';
}
char pgnint2char(int i) {
    return (char)((int)'a' + i);
}
// 将数字字符 ('0'-'9') 转换为整数 (0-9)
int char2int(char c) {
    return (int)c - (int)'0';
}
char int2char(int i) {
    return (char)((int)'0' + i);
}

// === 数据结构 ===

// 表示一步棋的结构体
struct Move {
    int source_x, source_y, target_x, target_y; // 起点和终点坐标
    Move() {source_x = -1; source_y = -1; target_x = -1; target_y = -1;}
    Move(int msx, int msy, int mtx, int mty): source_x(msx), source_y(msy),
                                              target_x(mtx), target_y(mty) {;}
    // 从PGN字符串格式解析移动（例如 "h2", "e2"）
    Move(string msource, string mtarget) {
        source_x = pgnchar2int(msource[0]);
        source_y = char2int(msource[1]);
        target_x = pgnchar2int(mtarget[0]);
        target_y = char2int(mtarget[1]);
    }
    bool operator ==(const Move& other) const {
        return source_x == other.source_x && source_y == other.source_y && target_x == other.target_x
               && target_y == other.target_y;
    }
};

// 表示棋盘上一个格子的结构体
struct Grid {
    stoneType type;  // 棋子类型
    colorType color; // 棋子阵营
    Grid () : type(None), color(EMPTY) {;}
    Grid (stoneType mtype, colorType mcolor):
            type(mtype), color(mcolor) {;}
    Grid& operator = (const Grid& other) {
        if (this != &other) {
            type = other.type;
            color = other.color;
        }
        return *this;
    }
    bool operator == (const Grid& other) const {
        return type == other.type && color == other.color;
    }
};

// 象棋核心逻辑类
class Chess {
private:
    std::vector<std::vector<Grid>> state; // 记录历史所有回合的棋盘状态
    int currTurnId;                       // 当前回合数
    colorType currColor;                  // 当前走棋方（RED 或 BLACK）
    vector<int> continuePeaceTurn;        // 连续未吃子回合数（用于判断60回合自然限着和棋）
    vector<int> lastMoveEaten;            // 记录历史每一步是否吃子
public:
    Chess();
    void resetBoard(); // 初始化开局棋盘
    // 生成当前局面的所有伪合法走法（mustDefend为true时会过滤掉送将的走法）
    void generateMoves(vector<Move> &legalMoves, bool mustDefend=true);
    // 生成走法并过滤掉会导致局面三次重复的走法（防长将/长捉）
    void generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend=true);
    bool repeatAfterMove(const Move& move); // 判断走这步棋后是否导致局面重复
    static bool inBoard(int mx, int my); // 坐标是否在棋盘内
    static bool inKingArea(int mx, int my, colorType mcolor); // 坐标是否在九宫格内
    static bool inColorArea(int mx, int my, colorType mcolor); // 坐标是否在本方半场（未过河）
    colorType oppColor(); // 获取对手颜色
    colorType oppColor(colorType mcolor);
    bool isMoveValid(const Move& move, bool mustDefend=true);
    bool isLegalMove(const Move& move, bool mustDefend=true);
    bool isLegalMoveWithForbidden(const Move& move, bool mustDefend=true);
    bool isMoveValidWithForbidden(const Move& move, bool mustDefend=true);
    bool makeMoveAssumeLegal(const Move& move); // 执行一步棋，并推进游戏状态
    bool attacked(colorType color, int mx, int my); // 判断(mx, my)是否被 color 方攻击
    bool betweenIsEmpty(int sx, int sy, int ex, int ey); // 判断两点之间是否为空
    int betweenNotEmptyNum(int sx, int sy, int ex, int ey); // 计算两点之间有几个棋子（用于炮吃子判断）
    static bool inSameLine(int sx, int sy, int ex, int ey); 
    static bool inSameStraightLine(int sx, int sy, int ex, int ey); // 是否在同一水平/垂直线上
    static bool inSameObiqueLine(int sx, int sy, int ex, int ey); // 是否在同一斜线上
    bool isMyKingAttackedAfterMove(const Move& move); // 走这步后自己是否被将
    bool isOppKingAttackedAfterMove(const Move& move); // 走这步后是否将了对手
    bool currKingAttacked(const Move& move);
    bool winAfterMove(const Move& move); // 走这步后对手是否绝杀无解
    void undoMove(); // 悔棋/撤销一步，用于在搜索时回溯状态
    bool exceedMaxPeaceState(); // 是否达到和棋步数条件
    
    colorType getCurrColor() const {
        return currColor;
    }

    // 一维数组和二维坐标的互相转换
    static int xy2pos(int mx, int my);
    static int pos2x(int mpos);
    static int pos2y(int mpos);
    void printBoard(); // 打印棋盘
};

// Botzone 平台 IO 接口
void getInputBotzone(Chess& chess);
void giveOutputBotzone(Chess& chess);

int main() {
    Chess chess;
    getInputBotzone(chess); // 读取 Botzone 传入的对局历史记录
    giveOutputBotzone(chess); // 生成走法并输出给 Botzone
    return 0;
}

// 输出给 Botzone 平台
void giveOutputBotzone(Chess& chess) {
    vector<Move> retMoves;
    chess.generateMovesWithForbidden(retMoves); // 生成所有合法且不禁手的走法
    Json::Value ret;

    if (retMoves.empty()) {
        // 如果没有合法走法，认输
        ret["response"]["source"] = string("-1");
        ret["response"]["target"] = string("-1");
    } else {
        // AI 核心：随机选择一步合法走法（注：若要实现智能AI，需将此处替换为极小化极大搜索 Minimax 算法）
        srand(time(nullptr));
        int choice = rand() % retMoves.size();
        auto selMove = retMoves[choice];
        ret["response"]["source"] = string(1, pgnint2char(selMove.source_x)) + string(1, int2char(selMove.source_y));
        ret["response"]["target"] = string(1, pgnint2char(selMove.target_x)) + string(1, int2char(selMove.target_y));
    }
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;
}

// 从 Botzone 读取 JSON 格式输入
void getInputBotzone(Chess& chess) {
    string str;
    getline(cin, str);
    Json::Reader reader;
    Json::Value input;
    reader.parse(str, input);
    int turnID = input["responses"].size();
    
    // 第一回合收到的起点是"-1", 说明我是红方
    colorType currBotColor =
            input["requests"][(Json::Value::UInt) 0]["source"].asString() == "-1" ?  RED : BLACK;
    string curSource, curTarget;
    
    // 重演历史记录，将棋盘推进到当前局面
    for (int i = 0; i < turnID; i++)
    {
        // 根据这些输入输出逐渐恢复状态到当前回合
        curSource = input["requests"][i]["source"].asString();
        curTarget = input["requests"][i]["target"].asString();
        if (curSource != "-1") {
            Move curMove(curSource, curTarget);
            if (!chess.isMoveValidWithForbidden(curMove)) {
                throw std::runtime_error("input is not valid!");
            }
            chess.makeMoveAssumeLegal(curMove);
        }

        curSource = input["responses"][i]["source"].asString();
        curTarget = input["responses"][i]["target"].asString();
        Move curMove(curSource, curTarget);
        if (!chess.isMoveValidWithForbidden(curMove)) {
            throw std::runtime_error("input is not valid!");
        }
        chess.makeMoveAssumeLegal(curMove);
    }

    curSource = input["requests"][turnID]["source"].asString();
    curTarget = input["requests"][turnID]["target"].asString();
    if (curSource != "-1") {
        Move curMove(curSource, curTarget);
        if (!chess.isMoveValidWithForbidden(curMove)) {
            throw std::runtime_error("input is not valid!");
        }
        chess.makeMoveAssumeLegal(curMove);
    }
}

// 初始化中国象棋标准开局阵型
void Chess::resetBoard() {
    vector<Grid> currState;
    Grid boardInfo[BOARDWIDTH][BOARDHEIGHT];
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            boardInfo[x][y] = Grid(None, EMPTY);
        }
    }

    // 摆放红方棋子 (底线在 y=0)
    boardInfo[0][0] = Grid(Rook, RED);
    boardInfo[1][0] = Grid(Knight, RED);
    boardInfo[2][0] = Grid(Bishop, RED);
    boardInfo[3][0] = Grid(Assistant, RED);
    boardInfo[4][0] = Grid(King, RED);
    boardInfo[5][0] = Grid(Assistant, RED);
    boardInfo[6][0] = Grid(Bishop, RED);
    boardInfo[7][0] = Grid(Knight, RED);
    boardInfo[8][0] = Grid(Rook, RED);
    boardInfo[1][2] = Grid(Cannon, RED);
    boardInfo[7][2] = Grid(Cannon, RED);
    boardInfo[0][3] = Grid(Pawn, RED);
    boardInfo[2][3] = Grid(Pawn, RED);
    boardInfo[4][3] = Grid(Pawn, RED);
    boardInfo[6][3] = Grid(Pawn, RED);
    boardInfo[8][3] = Grid(Pawn, RED);

    // 摆放黑方棋子 (底线在 y=9)
    boardInfo[0][9] = Grid(Rook, BLACK);
    boardInfo[1][9] = Grid(Knight, BLACK);
    boardInfo[2][9] = Grid(Bishop, BLACK);
    boardInfo[3][9] = Grid(Assistant, BLACK);
    boardInfo[4][9] = Grid(King, BLACK);
    boardInfo[5][9] = Grid(Assistant, BLACK);
    boardInfo[6][9] = Grid(Bishop, BLACK);
    boardInfo[7][9] = Grid(Knight, BLACK);
    boardInfo[8][9] = Grid(Rook, BLACK);
    boardInfo[1][7] = Grid(Cannon, BLACK);
    boardInfo[7][7] = Grid(Cannon, BLACK);
    boardInfo[0][6] = Grid(Pawn, BLACK);
    boardInfo[2][6] = Grid(Pawn, BLACK);
    boardInfo[4][6] = Grid(Pawn, BLACK);
    boardInfo[6][6] = Grid(Pawn, BLACK);
    boardInfo[8][6] = Grid(Pawn, BLACK);

    for (int mpos = 0; mpos < BOARDWIDTH * BOARDHEIGHT; mpos++) {
        currState.push_back(boardInfo[pos2x(mpos)][pos2y(mpos)]);
    }
    state.clear();
    state.push_back(currState);
}

// 构造函数
Chess::Chess() {
    resetBoard();
    continuePeaceTurn.clear();
    lastMoveEaten.clear();
    continuePeaceTurn.push_back(0);
    lastMoveEaten.push_back(false);
    currColor = RED; // 红方先走
    currTurnId = 0;
}

// 核心函数：根据当前局面生成当前方所有的合法移动
void Chess::generateMoves(vector<Move> &legalMoves, bool mustDefend) {
    legalMoves.clear();
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == currColor) {
                // 根据不同兵种进行规则判断
                switch (curGrid.type) {
                    case King: { // 帅/将
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            if (!inKingArea(tx, ty, curGrid.color)) { // 必须在九宫格内
                                continue;
                            }
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            if (tGrid.color == currColor) // 目标不能是自己人
                                continue;
                            if (mustDefend) { // 判断走完这步是否会处于被将军状态（送将）
                                if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            } else {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                        break;
                    }
                    case Assistant: { // 仕/士
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_ob[dir], ty = y + dy_ob[dir];
                            if (!inKingArea(tx, ty, curGrid.color)) { // 必须在九宫内且走斜线
                                continue;
                            }
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            if (tGrid.color == currColor) {
                                continue;
                            }
                            if (mustDefend) {
                                if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            } else {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                        break;
                    }
                    case Pawn: { // 兵/卒
                        int forwardOne = (curGrid.color == RED ? 1 : -1); // 兵的前进方向
                        bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5)); // 是否过河
                        int tx = x, ty = y + forwardOne;
                        
                        // 1. 向前走
                        if (!inBoard(tx, ty)) {
                            continue;
                        }
                        auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                        if (tGrid.color == currColor) {
                            continue;
                        }
                        if (mustDefend) {
                            if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        } else {
                            legalMoves.emplace_back(x, y, tx, ty);
                        }
                        
                        // 2. 如果过河了，还可以向左右走
                        if (crossMidLine) {
                            for (int dir = 0; dir < 2; dir++) {
                                int ox = x + dx_lr[dir], oy = y;
                                if (!inBoard(ox, oy)) {
                                    continue;
                                }
                                auto& oGrid = state[currTurnId][xy2pos(ox, oy)];
                                if (oGrid.color == currColor) {
                                    continue;
                                }
                                if (mustDefend) {
                                    if (!isMyKingAttackedAfterMove(Move(x,y,ox,oy))) {
                                        legalMoves.emplace_back(x, y, ox, oy);
                                    }
                                } else {
                                    legalMoves.emplace_back(x, y, ox, oy);
                                }
                            }
                        }
                        break;
                    }
                    case Knight: { // 马
                        for (int dir = 0; dir < 8; dir++) { // 8个“日”字对角
                            int tx = x + dx_knight[dir], ty = y + dy_knight[dir];
                            if (!inBoard(tx, ty)) {
                                continue;
                            }
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            int fx = x + dx_knight_foot[dir], fy = y + dy_knight_foot[dir];
                            auto& fGrid = state[currTurnId][xy2pos(fx, fy)];
                            
                            // 规则：目标位置不能是己方棋子，且“马腿”位置不能为空（不能撇马腿）
                            if (tGrid.color == currColor || fGrid.color != EMPTY) {
                                continue;
                            }
                            if (mustDefend) {
                                if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            } else {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                        break;
                    }
                    case Bishop: { // 相/象
                        for (int dir = 0; dir < 4; dir++) { // 4个“田”字对角
                            int tx = x + dx_bishop[dir], ty = y + dy_bishop[dir];
                            // 象不能过河
                            if (!inBoard(tx, ty) || !inColorArea(tx, ty, currColor)) {
                                continue;
                            }
                            int ex = x + dx_bishop_eye[dir], ey = y + dy_bishop_eye[dir];
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            auto& eGrid = state[currTurnId][xy2pos(ex, ey)];
                            
                            // 规则：目标位置不能是己方棋子，且“象眼”位置不能为空（不能塞象眼）
                            if (tGrid.color == currColor || eGrid.color != EMPTY) {
                                continue;
                            }
                            if (mustDefend) {
                                if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            } else {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                        break;
                    }
                    case Rook: { // 车
                        for (int dir = 0; dir < 4; dir++) { // 上下左右
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            // 射线扫描：一直走直到碰到边界或棋子
                            while (inBoard(tx, ty)) {
                                auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                                if (tGrid.color == currColor) { // 遇到己方棋子，路线受阻
                                    break;
                                } else if (tGrid.color == oppColor()) { // 遇到敌方棋子，可以吃掉，但不能再继续走了
                                    if (mustDefend) {
                                        if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                            legalMoves.emplace_back(x, y, tx, ty);
                                        }
                                    } else {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                    break;
                                } else { // 遇到空格，可以走
                                    assert(tGrid.color == EMPTY);
                                    if (mustDefend) {
                                        if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                            legalMoves.emplace_back(x, y, tx, ty);
                                        }
                                    } else {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                }
                                tx += dx_strai[dir];
                                ty += dy_strai[dir];
                            }
                        }
                        break;
                    }
                    case Cannon: { // 炮
                        for (int dir = 0; dir < 4; dir++) { // 上下左右
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            while (inBoard(tx, ty)) {
                                int betweenNum = betweenNotEmptyNum(x, y, tx, ty); // 获取两点之间的棋子数
                                if (betweenNum > 1) { // 如果中间隔了2个及以上，无法跨越
                                    break;
                                } else if (betweenNum == 1) { // 中间隔了1个（作为炮架）
                                    auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                                    // 必须是吃子，目标不能是己方或空
                                    if (tGrid.color == currColor || tGrid.color == EMPTY) {
                                        tx += dx_strai[dir];
                                        ty += dy_strai[dir];
                                        continue;
                                    }
                                    if (mustDefend) {
                                        if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                            legalMoves.emplace_back(x, y, tx, ty);
                                        }
                                    } else {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                } else if (betweenNum == 0) { // 中间没有棋子，视为移动（不能吃子）
                                    auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                                    if (tGrid.color != EMPTY) { // 遇到棋子且中间没有炮架，无法吃，跳过
                                        tx += dx_strai[dir];
                                        ty += dy_strai[dir];
                                        continue;
                                    }
                                    if (mustDefend) {
                                        if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                            legalMoves.emplace_back(x, y, tx, ty);
                                        }
                                    } else {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                }
                                tx += dx_strai[dir];
                                ty += dy_strai[dir];
                            }
                        }
                        break;
                    }
                    case None:
                        throw std::runtime_error("color is not empty but type is none");
                }
            }
        }
    }
}

// === 辅助逻辑和检查 ===
bool Chess::inBoard(int mx, int my) {
    return mx >= 0 && mx < BOARDWIDTH && my >= 0 && my < BOARDHEIGHT;
}

colorType Chess::oppColor() {
    return currColor == BLACK ? RED : BLACK;
}

bool Chess::isLegalMove(const Move &move, bool mustDefend) {
    vector<Move> currLegalMoves;
    generateMoves(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

// 模拟执行一步棋，更新所有的游戏状态信息
bool Chess::makeMoveAssumeLegal(const Move &move) {
    int nextPeaceTrun =  continuePeaceTurn[currTurnId] + 1;
    vector<Grid> copyState = state[currTurnId];
    state.push_back(copyState);
    currTurnId++;

    auto& sourceGrid = state[currTurnId][xy2pos(move.source_x, move.source_y)];
    auto& targetGrid = state[currTurnId][xy2pos(move.target_x, move.target_y)];
    if (targetGrid.color == oppColor()) { // 判断是否吃子
        nextPeaceTrun = 0; // 吃子重置连续和棋步数
        lastMoveEaten.push_back(true);
    } else {
        lastMoveEaten.push_back(false);
    }
    
    // 覆盖目标位置
    targetGrid.type = sourceGrid.type;
    targetGrid.color = sourceGrid.color;
    assert(sourceGrid.color == currColor);

    // 清空起始位置
    sourceGrid.color = EMPTY;
    sourceGrid.type = None;

    continuePeaceTurn.push_back(nextPeaceTrun);
    currColor = oppColor(); // 交换回合方
    return true;
}

// 检查某个特定点 (mx, my) 是否被特定颜色 color 攻击
// 常用于判断帅/将是否处于被将军状态
bool Chess::attacked(colorType color, int mx, int my) {
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color != color) {
                continue;
            }
            int dex = mx - x, dey = my - y;
            switch (curGrid.type) {
                case King: { // 面对面（飞将）
                    if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my)) {
                        return true;
                    }
                    break;
                }
                case Assistant: {
                    if (inKingArea(mx, my, oppColor(color))) {
                        if (abs(dex) == 1 && abs(dey) == 1) {
                            return true;
                        }
                    }
                    break;
                }
                case Knight: { // 马
                    if (abs(dex) == 2 && abs(dey) == 1) {
                        int fx = x + (dex > 0 ? 1 : -1), fy = y;
                        auto& fGrid = state[currTurnId][xy2pos(fx, fy)];
                        if (fGrid.color == EMPTY) {
                            return true;
                        }
                    } else if (abs(dex) == 1 && abs(dey) == 2) {
                        int fx = x, fy = y + (dey > 0 ? 1 : -1);
                        auto& fGrid = state[currTurnId][xy2pos(fx, fy)];
                        if (fGrid.color == EMPTY) {
                            return true;
                        }
                    }
                    break;
                }
                case Bishop: { // 象
                    if (abs(dex) == 2 && abs(dey) == 2) {
                        int ex = x + dex / 2, ey = y + dey / 2;
                        auto& eGrid = state[currTurnId][xy2pos(ex, ey)];
                        if (eGrid.color == EMPTY) {
                            return true;
                        }
                    }
                    break;
                }
                case Rook: { // 车
                    if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my)) {
                        return true;
                    }
                    break;
                }
                case Pawn: { // 兵
                    int forwardOne = (curGrid.color == RED ? 1 : -1);
                    bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5));
                    if (dex == 0 && dey == forwardOne) {
                        return true;
                    }
                    if (crossMidLine && (abs(dex) == 1 && dey == 0)) {
                        return true;
                    }
                    break;
                }
                case Cannon: { // 炮
                    if (inSameStraightLine(x, y, mx, my) && betweenNotEmptyNum(x,y,mx,my) == 1) {
                        return true;
                    }
                    break;
                }
                case None: {
                    throw std::runtime_error("color is not empty but type is none");
                    break;
                }
            }
        }
    }
    return false;
}

// 检查两点是否处在直线上并且中间为空
bool Chess::betweenIsEmpty(int sx, int sy, int ex, int ey) {
    assert(inSameStraightLine(sx, sy, ex, ey));
    int dex = ex > sx ? 1 : (ex == sx ? 0 : -1);
    int dey = ey > sy ? 1 : (ey == sy ? 0 : -1);
    int tx = sx + dex, ty = sy + dey;
    while (inBoard(tx, ty) && !(tx == ex && ty == ey)) {
        auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
        if (tGrid.color != EMPTY)
            return false;
        tx += dex; ty += dey;
    }
    return true;
}

bool Chess::inSameLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx, dey = ey - sy;
    return dex == dey || dex == -dey || dex == 0 || dey == 0;
}

bool Chess::inSameStraightLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx, dey = ey - sy;
    return dex == 0 || dey == 0;
}

bool Chess::inSameObiqueLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx, dey = ey - sy;
    return dex == dey || dex == -dey;
}

// 假设走了这步棋后，己方的将是否被攻击（用于排除“送将”违规步）
bool Chess::isMyKingAttackedAfterMove(const Move &move) {
    colorType originColor = currColor;
    makeMoveAssumeLegal(move);
    int kingX = -1, kingY = -1;
    // 寻找己方将的位置
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == originColor && curGrid.type == King) {
                kingX = x; kingY = y;
                break;
            }
        }
    }
    bool myKingStillAttacked = attacked(oppColor(originColor), kingX, kingY);
    undoMove(); // 检查完后回溯状态
    assert(currColor == originColor);
    return myKingStillAttacked;
}

bool Chess::isMoveValid(const Move& move, bool mustDefend) {
    return isLegalMove(move, mustDefend);
}

// 绝杀检测：走完这步后对手是否再无合法招法
bool Chess::winAfterMove(const Move &move) {
    makeMoveAssumeLegal(move);
    vector<Move> oppLegalMoves;
    generateMoves(oppLegalMoves); // 生成对手的合法移动
    undoMove();
    return oppLegalMoves.empty(); // 如果为空，表示绝杀
}

// 当前将/帅是否正在被攻击（被将军）
bool Chess::currKingAttacked(const Move &move) {
    int kingX = -1, kingY = -1;
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == currColor && curGrid.type == King) {
                kingX = x; kingY = y;
            }
        }
    }
    return attacked(oppColor(), kingX, kingY);
}

bool Chess::isOppKingAttackedAfterMove(const Move &move) {
    colorType originColor = currColor;
    makeMoveAssumeLegal(move);
    int kingX = -1, kingY = -1;
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == oppColor(originColor) && curGrid.type == King) {
                kingX = x; kingY = y;
                break;
            }
        }
    }
    bool oppKingBeingAttacked = attacked(originColor, kingX, kingY);
    undoMove();
    assert(currColor == originColor);
    return oppKingBeingAttacked;
}

// 是否超过无吃子回合数（自然限着规则通常为60回合）
bool Chess::exceedMaxPeaceState() {
    return continuePeaceTurn[currTurnId] >= 60 - 1;
}

int Chess::xy2pos(int mx, int my) {
    return my * BOARDWIDTH + mx;
}
int Chess::pos2x(int mpos) {
    return mpos % BOARDWIDTH;
}
int Chess::pos2y(int mpos) {
    return mpos / BOARDWIDTH;
}

// 是否在九宫格
bool Chess::inKingArea(int mx, int my, colorType mcolor) {
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED) {
        return mx >= 3 && mx <= 5 && my >= 0 && my <= 2;
    } else {
        return mx >= 3 && mx <= 5 && my >= 7 && my <= 9;
    }
}

// 是否在己方阵地（不能过河）
bool Chess::inColorArea(int mx, int my, colorType mcolor) {
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED) {
        return my <= 4;
    } else {
        return my >= 5;
    }
}

// 获取起点和终点之间的棋子数
int Chess::betweenNotEmptyNum(int sx, int sy, int ex, int ey) {
    assert(inSameStraightLine(sx, sy, ex, ey));
    int dex = ex > sx ? 1 : (ex == sx ? 0 : -1);
    int dey = ey > sy ? 1 : (ey == sy ? 0 : -1);
    int tx = sx + dex, ty = sy + dey;
    int retNum = 0;
    while (inBoard(tx, ty) && !(tx == ex && ty == ey)) {
        auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
        if (tGrid.color != EMPTY) {
            retNum++;
        }
        tx += dex; ty += dey;
    }
    return retNum;
}

colorType Chess::oppColor(colorType mcolor) {
    return mcolor == RED ? BLACK : RED;
}

// 回溯/撤销一步棋（供历史模拟和搜索引擎使用）
void Chess::undoMove() {
    if (currTurnId == 0) {
        return;
    }
    state.pop_back();
    currTurnId--;
    currColor = oppColor();
    continuePeaceTurn.pop_back();
    lastMoveEaten.pop_back();
}

// 生成排除了导致局面重复的合法招法（长将长捉禁手防御简化版）
void Chess::generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend) {
    vector<Move> firstMoves;
    generateMoves(firstMoves, mustDefend);
    legalMoves.clear();
    for (const auto& move : firstMoves) {
        if (!repeatAfterMove(move)) {
            legalMoves.push_back(move);
        }
    }
}

// 检查走完这步是否会导致三次重复局面
bool Chess::repeatAfterMove(const Move& move) {
    makeMoveAssumeLegal(move);
    int tmp_id = currTurnId;
    // 只要有吃子就不算重复状态，可以剪枝
    if (lastMoveEaten[tmp_id]) {
        undoMove();
        return false;
    }
    // 往前回溯找到上一次吃子的时候（或者开局）
    while (true) {
        if (tmp_id <= 1) {
            break;
        }
        if (lastMoveEaten[tmp_id-2]) {
            break;
        }
        tmp_id -= 2;
    }
    int repeatTime = 0;
    // 对比历史记录中是否有相同盘面，到达3次判重复
    for (int id = currTurnId; id >= tmp_id; id -= 2) {
        if (state[id] == state[currTurnId]) {
            repeatTime++;
        }
    }
    undoMove();
    return repeatTime >= 3;
}

bool Chess::isLegalMoveWithForbidden(const Move& move, bool mustDefend) {
    vector<Move> currLegalMoves;
    generateMovesWithForbidden(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

// 命令行打印当前棋盘
void Chess::printBoard() {
    std::cout << std::endl << "Board" << std::endl;
    for (int y = 9; y >= 0; y--) {
        for (int x = 0; x <= 8; x++) {
            std::cout << stoneSym[state[currTurnId][xy2pos(x,y)].type] << " ";
        }
        std::cout << std::endl;
    }
}

bool Chess::isMoveValidWithForbidden(const Move& move, bool mustDefend) {
    return isLegalMoveWithForbidden(move, mustDefend);
}

//???????????????

//1.????????????????????????????????????void generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend=true) ???????????
//2.???????????????????????bool makeMoveAssumeLegal(const Move& move)
//3.???????????????????????void undoMove()




void giveOutputTime(Chess& chess) {
    // 记录开始时间
    startTime = clock(); 

    vector<Move> retMoves;
    chess.generateMovesWithForbidden(retMoves);
    Json::Value ret;

    if (retMoves.empty()) {
        ret["response"]["source"] = string("-1");
        ret["response"]["target"] = string("-1");
    } else {
        bool isRedTurn = (chess.getCurrColor() == RED);
        Move globalBestMove = retMoves[0]; // 最佳走法

        // 加深搜索
        try {
            for (int depth = 1; depth <= 30; ++depth) {
                Move currentDepthBestMove = retMoves[0];
                int bestScore = isRedTurn ? -1 : 100;

                sortMoves(chess, retMoves); // 顶层排序

                for (const Move& move : retMoves) {
                    chess.makeMoveAssumeLegal(move);
                    int score = alphaBetaSearch(chess, depth - 1, -1, 100, !isRedTurn); 
                    chess.undoMove();

                    if (isRedTurn) {
                        if (score > bestScore) {
                            bestScore = score;
                            currentDepthBestMove = move;
                        }
                    } else {
                        if (score < bestScore) {
                            bestScore = score;
                            currentDepthBestMove = move;
                        }
                    }
                }
                
                // 搜索完这一层更新最佳走法
                globalBestMove = currentDepthBestMove; 
                if (bestScore > 90 || bestScore < -90) break; // kill提前结束
            }
        } catch (const TimeOutException& e) {
            // 超时，跳出循环，使用上一层得到的 globalBestMove
        }

        ret["response"]["source"] = string(1, pgnint2char(globalBestMove.source_x)) + string(1, int2char(globalBestMove.source_y));
        ret["response"]["target"] = string(1, pgnint2char(globalBestMove.target_x)) + string(1, int2char(globalBestMove.target_y));
    }
    
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;
}//分数中具体数值待填，暂时用-1和100占位


// 对走法进行启发式排序
void sortMoves(const Chess& chess, vector<Move>& moves)
{

}


// 评估，给出分数用于排序
int getMovePriority(const Chess& chess, const Move& move) 
{

}

// alpha-beta 搜索
int alphaBetaSearch(Chess& chess, int depth, int alpha, int beta, bool isRedTurn)
{
    // 每次进入新节点，检查时间
    checkTime(); 

    if (depth == 0) {
        return evaluateBoard(chess);
    }

    vector<Move> legalMoves;
    chess.generateMovesWithForbidden(legalMoves);
    
    if (legalMoves.empty()) {
        return isRedTurn ? -1 : 100; // 越快将死对手得分越高
    }
    if (chess.exceedMaxPeaceState()) {
        return 0;
    }
    // 对走法进行启发式排序
    sortMoves(chess, legalMoves);

    if (isRedTurn) {
        int maxEval = -1;
        for (const Move& move : legalMoves) {
            chess.makeMoveAssumeLegal(move);
            int eval = alphaBetaSearch(chess, depth - 1, alpha, beta, false);
            chess.undoMove();
            
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break; // Beta 剪枝
        }
        return maxEval;
    } else {
        int minEval = 100;
        for (const Move& move : legalMoves) {
            chess.makeMoveAssumeLegal(move);
            int eval = alphaBetaSearch(chess, depth - 1, alpha, beta, true);
            chess.undoMove();
            
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break; // Alpha 剪枝
        }
        return minEval;
    }
}//分数中具体数值待填，暂时用-1和100占位


int evaluateBoard(const Chess& chess)
{

}


//考虑在开局中局残局时各个棋子处境与价值可能完全不同
//开局可能可以公式化开局，后续必须考虑哪些棋子需要考虑其受威胁或者进攻属性，在不需要时也许可以跳过这些棋子减小性能开销
