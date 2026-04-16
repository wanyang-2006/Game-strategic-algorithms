//该bot为botzone示例bot，随机下棋，保证合法，未使用任何评估算法

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <cassert>
#include <algorithm>
#include "jsoncpp/json.h"

using std::string;
using std::vector;
using std::map;
using std::cin;
using std::cout;
using std::endl;

// 9路 10道
#define BOARDWIDTH 9
#define BOARDHEIGHT 10

// 棋子类型
// None=空, King=将/帅, Bishop=象/相, Knight=马, Rook=车, Pawn=兵/卒, Cannon=炮, Assistant=士/仕
enum stoneType {None = 0, King=1, Bishop=2, Knight=3, Rook=4, Pawn=5, Cannon=6, Assistant=7};

// 枚举：棋子颜色（BLACK=黑方, RED=红方, EMPTY=空位置）
enum colorType {BLACK=0, RED=1, EMPTY = 2};

// 九宫格的底线Y坐标（黑方底线为9，红方底线为0）
int baseLine[2] = {9, 0};

// 棋子对应的字符表示（用于打印或调试）
char stoneSym[] = "*kbnrpca";
map<char,stoneType> charSym = {{stoneSym[0], None}, {stoneSym[1], King},
                               {stoneSym[2], Bishop}, {stoneSym[3], Knight},
                               {stoneSym[4], Rook}, {stoneSym[5], Pawn},
                               {stoneSym[6], Cannon}, {stoneSym[7], Assistant}};

// ==========================================
// 棋子移动方向常量数组
// ==========================================
// 米字型方向（周围8个点）
int dx[] = {-1,-1,-1,0,0,1,1,1};
int dy[] = {-1,0,1,-1,1,-1,0,1};
// 士/仕的移动方向（斜向4个点）
int dx_ob[] = {-1,1,-1,1};
int dy_ob[] = {-1,-1,1,1};
// 车/炮/将的直线移动方向（上下左右）
int dx_strai[] = {-1,0,0,1};
int dy_strai[] = {0,-1,1,0};
// 兵/卒过河后的左右移动
int dx_lr[] = {-1,1};

// 马的移动方向（走日字，共8个落点）
int dx_knight[] = {-2,-2,-1,-1,1,1,2,2};
int dy_knight[] = {-1,1,-2,2,-2,2,-1,1};
// 马腿的坐标增量（用于检测“蹩马腿”）
int dx_knight_foot[] = {-1,-1,0,0,0,0,1,1};
int dy_knight_foot[] = {0,0,-1,1,-1,1,0,0};

// 象/相的移动方向（走田字，共4个落点）
int dx_bishop[] = {-2,-2,2,2};
int dy_bishop[] = {-2,2,-2,2};
// 象眼的坐标增量（用于检测“塞象眼”）
int dx_bishop_eye[] = {-1,-1,1,1};
int dy_bishop_eye[] = {-1,1,-1,1};

// ==========================================
// 坐标系转换工具函数
// ==========================================
// 将 PGN 字母(a-i)转为 0-8 整数
int pgnchar2int(char c) { return (int)c - (int)'a'; }
// 将 0-8 整数转为 PGN 字母(a-i)
char pgnint2char(int i) { return (char)((int)'a' + i); }
// 将字符数字('0'-'9')转为整数 0-9
int char2int(char c) { return (int)c - (int)'0'; }
// 将整数 0-9 转为字符数字
char int2char(int i) { return (char)((int)'0' + i); }

// 结构体：表示一步棋
struct Move {
    int source_x, source_y, target_x, target_y; // 起点坐标与终点坐标
    Move() {source_x = -1; source_y = -1; target_x = -1; target_y = -1;}
    Move(int msx, int msy, int mtx, int mty): source_x(msx), source_y(msy),
                                                                target_x(mtx), target_y(mty) {;}
    // 从平台返回的字符串形式(如"a0", "a1")构造
    Move(string msource, string mtarget) {
        source_x = pgnchar2int(msource[0]);
        source_y = char2int(msource[1]);
        target_x = pgnchar2int(mtarget[0]);
        target_y = char2int(mtarget[1]);
    }
    // 判断两步棋是否完全相同
    bool operator ==(const Move& other) const {
        return source_x == other.source_x && source_y == other.source_y && target_x == other.target_x
               && target_y == other.target_y;
    }
};

// 结构体：表示棋盘上的一个格子
struct Grid {
    stoneType type;  // 棋子类型
    colorType color; // 棋子颜色（属于哪一方，或者为空）
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

// 核心类：中国象棋规则与状态引擎
class Chess {
private:
    // 状态列表：使用三维数组保存历史上每一步完整的棋盘状态，方便悔棋和判断重复局面
    std::vector<std::vector<Grid>> state;
    int currTurnId;                  // 当前回合数
    colorType currColor;             // 当前轮到哪一方走棋
    vector<int> continuePeaceTurn;   // 连续未吃子的回合数（用于实现自然限着60步判和）
    vector<int> lastMoveEaten;       // 记录历史每一步是否发生了吃子

public:
    Chess();
    void resetBoard();
    
    // 生成所有伪合法步（mustDefend为true时会剔除送将的步）
    void generateMoves(vector<Move> &legalMoves, bool mustDefend=true);
    // 生成合法步，且过滤掉会导致三次重复局面的违规步
    void generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend=true);
    bool repeatAfterMove(const Move& move); // 检测走这步后是否会造成长将/长捉等重复局面
    
    static bool inBoard(int mx, int my);
    static bool inKingArea(int mx, int my, colorType mcolor); // 是否在九宫格内
    static bool inColorArea(int mx, int my, colorType mcolor); // 是否未过河
    
    colorType oppColor();
    colorType oppColor(colorType mcolor);
    
    bool isMoveValid(const Move& move, bool mustDefend=true);
    bool isLegalMove(const Move& move, bool mustDefend=true);
    bool isLegalMoveWithForbidden(const Move& move, bool mustDefend=true);
    bool isMoveValidWithForbidden(const Move& move, bool mustDefend=true);
    
    // 在棋盘上执行一步棋，并记录历史状态
    bool makeMoveAssumeLegal(const Move& move);
    // 悔棋，撤销上一步操作
    void undoMove();
    
    // 判断给定坐标的格子是否正在被指定方攻击
    bool attacked(colorType color, int mx, int my);
    
    // 路径检查相关
    bool betweenIsEmpty(int sx, int sy, int ex, int ey);
    int betweenNotEmptyNum(int sx, int sy, int ex, int ey);
    static bool inSameLine(int sx, int sy, int ex, int ey);
    static bool inSameStraightLine(int sx, int sy, int ex, int ey);
    static bool inSameObiqueLine(int sx, int sy, int ex, int ey);
    
    // 将军判断相关
    bool isMyKingAttackedAfterMove(const Move& move);  // 走完这步后己方老将是否被将军（防送将）
    bool isOppKingAttackedAfterMove(const Move& move); // 走完这步后对方老将是否被将军
    bool currKingAttacked(const Move& move);
    bool winAfterMove(const Move& move); // 判断是否绝杀
    
    bool exceedMaxPeaceState(); // 是否达到连续不吃子判和条件
    
    // 1D坐标与2D坐标互相转换
    static int xy2pos(int mx, int my);
    static int pos2x(int mpos);
    static int pos2y(int mpos);
    


    //原获取信息接口是private的，需要获取信息添加public的接口
    void printBoard();
    // 获取当前棋盘上指定坐标格信息
    Grid getGrid(int x, int y) const {
        return state[currTurnId][xy2pos(x, y)];
    }
    // 获取当前走棋方颜色
    colorType getCurrColor() const {
        return currColor;
    }
    //


};

void getInputBotzone(Chess& chess);
void giveOutputBotzone(Chess& chess);

int main() {
    Chess chess;
    getInputBotzone(chess);  // 获取平台输入并恢复局面
    giveOutputBotzone(chess); // 生成AI决策并输出
    return 0;
}

// 输出 AI 的决策给平台
void giveOutputBotzone(Chess& chess) {
    vector<Move> retMoves;
    // 获取当前局面所有合法的走步
    chess.generateMovesWithForbidden(retMoves);
    Json::Value ret;

    if (retMoves.empty()) {
        // 无路可走（被将死或困毙），认输抛出 -1
        ret["response"]["source"] = string("-1");
        ret["response"]["target"] = string("-1");
    } else {
        // =====================================================================
        // 下面部分为随机示例，优化算法更改下方即可
        // =====================================================================
        srand(time(nullptr));
        int choice = rand() % retMoves.size();
        auto selMove = retMoves[choice]; 
        
        // 将内部的数字坐标转换为 PGN 字符串格式
        ret["response"]["source"] = string(1, pgnint2char(selMove.source_x)) + string(1, int2char(selMove.source_y));
        ret["response"]["target"] = string(1, pgnint2char(selMove.target_x)) + string(1, int2char(selMove.target_y));
    }
    // 转为 JSON 格式并打印到标准输出
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;
}

// 处理来自平台的输入，恢复棋盘状态
void getInputBotzone(Chess& chess) {
    string str;
    getline(cin, str); // 从标准输入读取 JSON 字符串
    Json::Reader reader;
    Json::Value input;
    reader.parse(str, input);
    
    // 计算历史回合数
    int turnID = input["responses"].size();
    
    // 如果第一回合收到的起点是"-1", 说明我是红方先手
    colorType currBotColor =
            input["requests"][(Json::Value::UInt) 0]["source"].asString() == "-1" ?  RED : BLACK;
            
    string curSource, curTarget;
    
    // 遍历历史回合记录
    for (int i = 0; i < turnID; i++)
    {
        // 恢复对手的历史走步
        curSource = input["requests"][i]["source"].asString();
        curTarget = input["requests"][i]["target"].asString();
        if (curSource != "-1") {
            Move curMove(curSource, curTarget);
            if (!chess.isMoveValidWithForbidden(curMove)) {
                throw std::runtime_error("input is not valid!");
            }
            chess.makeMoveAssumeLegal(curMove); // 在引擎内部走子
        }

        // 恢复己方的历史走步
        curSource = input["responses"][i]["source"].asString();
        curTarget = input["responses"][i]["target"].asString();
        Move curMove(curSource, curTarget);
        if (!chess.isMoveValidWithForbidden(curMove)) {
            throw std::runtime_error("input is not valid!");
        }
        chess.makeMoveAssumeLegal(curMove);
    }

    // 接收对手刚刚走的一步
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

// 初始化棋盘（标准中国象棋开局站位）
void Chess::resetBoard() {
    vector<Grid> currState;
    Grid boardInfo[BOARDWIDTH][BOARDHEIGHT];
    // 全部初始化为空
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            boardInfo[x][y] = Grid(None, EMPTY);
        }
    }

    // 摆放红方
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

    // 摆放黑方
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

    // 序列化放入历史状态第一层
    for (int mpos = 0; mpos < BOARDWIDTH * BOARDHEIGHT; mpos++) {
        currState.push_back(boardInfo[pos2x(mpos)][pos2y(mpos)]);
    }
    state.clear();
    state.push_back(currState);
}

// 初始化状态
Chess::Chess() {
    resetBoard();
    continuePeaceTurn.clear();
    lastMoveEaten.clear();
    continuePeaceTurn.push_back(0);
    lastMoveEaten.push_back(false);
    currColor = RED; // 红先
    currTurnId = 0;
}

// 基于规则生成当前方所有的合法走法
void Chess::generateMoves(vector<Move> &legalMoves, bool mustDefend) {
    legalMoves.clear();
    // 遍历整个棋盘寻找己方的棋子
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == currColor) {
                switch (curGrid.type) {
                    case King: { // 将/帅
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            if (!inKingArea(tx, ty, curGrid.color)) { // 必须在九宫格内
                                continue;
                            }
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            if (tGrid.color == currColor) // 目标格子不能是自己的棋子
                                continue;
                            if (mustDefend) {
                                // 必须防守：如果走完这一步老将还被攻击（送将），则舍弃
                                if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            } else {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                        break;
                    }
                    case Assistant: { // 士/仕
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_ob[dir], ty = y + dy_ob[dir];
                            if (!inKingArea(tx, ty, curGrid.color)) { // 必须在九宫格内斜走
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
                        int forwardOne = (curGrid.color == RED ? 1 : -1); // 兵只能前进
                        bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5)); // 是否已过河
                        int tx = x, ty = y + forwardOne;
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
                        if (crossMidLine) { // 如果过河了，可以左右平移
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
                        for (int dir = 0; dir < 8; dir++) {
                            int tx = x + dx_knight[dir], ty = y + dy_knight[dir];
                            if (!inBoard(tx, ty)) {
                                continue;
                            }
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            // 蹩马腿检查，获取当前方向的马腿坐标
                            int fx = x + dx_knight_foot[dir], fy = y + dy_knight_foot[dir];
                            auto& fGrid = state[currTurnId][xy2pos(fx, fy)];
                            
                            // 如果目标点是自己人，或者马腿位置有棋子(不为空)，则不可走
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
                    case Bishop: { // 象/相
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_bishop[dir], ty = y + dy_bishop[dir];
                            // 象不能过河
                            if (!inBoard(tx, ty) || !inColorArea(tx, ty, currColor)) {
                                continue;
                            }
                            // 检查象眼是否被塞住
                            int ex = x + dx_bishop_eye[dir], ey = y + dy_bishop_eye[dir];
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            auto& eGrid = state[currTurnId][xy2pos(ex, ey)];
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
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            while (inBoard(tx, ty)) {
                                auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                                if (tGrid.color == currColor) {
                                    break; // 遇到自己人，阻断路线
                                } else if (tGrid.color == oppColor()) {
                                    // 遇到敌人，可以吃掉，停住
                                    if (mustDefend) {
                                        if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                            legalMoves.emplace_back(x, y, tx, ty);
                                        }
                                    } else {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                    break;
                                } else {
                                    // 空地，可以走
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
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            while (inBoard(tx, ty)) {
                                // 统计起点和目标点之间有多少个棋子
                                int betweenNum = betweenNotEmptyNum(x, y, tx, ty);
                                if (betweenNum > 1) {
                                    break; // 隔了两个及以上的棋子，不能走，阻断
                                } else if (betweenNum == 1) {
                                    // 中间有1个炮架
                                    auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                                    // 炮吃子必须目标是有敌军的格子
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
                                } else if (betweenNum == 0) {
                                    // 中间无棋子，普通移动
                                    auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                                    if (tGrid.color != EMPTY) {
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

// 判断坐标是否越界
bool Chess::inBoard(int mx, int my) {
    return mx >= 0 && mx < BOARDWIDTH && my >= 0 && my < BOARDHEIGHT;
}

// 获取对手的颜色
colorType Chess::oppColor() {
    return currColor == BLACK ? RED : BLACK;
}

// 检查某个 Move 是否在合法步列表内
bool Chess::isLegalMove(const Move &move, bool mustDefend) {
    vector<Move> currLegalMoves;
    generateMoves(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

// 执行一步棋（假设这步棋合法）
bool Chess::makeMoveAssumeLegal(const Move &move) {
    int nextPeaceTrun =  continuePeaceTurn[currTurnId] + 1;
    // 复制当前状态作为新的一层状态
    vector<Grid> copyState = state[currTurnId];
    state.push_back(copyState);
    currTurnId++;

    auto& sourceGrid = state[currTurnId][xy2pos(move.source_x, move.source_y)];
    auto& targetGrid = state[currTurnId][xy2pos(move.target_x, move.target_y)];
    
    // 如果吃子，重置自然限着回合数
    if (targetGrid.color == oppColor()) {
        nextPeaceTrun = 0;
        lastMoveEaten.push_back(true);
    } else {
        lastMoveEaten.push_back(false);
    }
    
    // 更新目标格状态（移动过去）
    targetGrid.type = sourceGrid.type;
    targetGrid.color = sourceGrid.color;
    assert(sourceGrid.color == currColor);

    // 清空起始格状态
    sourceGrid.color = EMPTY;
    sourceGrid.type = None;

    continuePeaceTurn.push_back(nextPeaceTrun);
    // 切换持回合方
    currColor = oppColor();
    return true;
}

// 检查坐标为 (mx, my) 的格子是否受到 color 方的攻击
bool Chess::attacked(colorType color, int mx, int my) {
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color != color) {
                continue;
            }
            int dex = mx - x, dey = my - y;
            switch (curGrid.type) {
                case King: { // 检查白脸将（老将对面吃老将）
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
                case Knight: { // 马的攻击检测
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
                case Bishop: { // 象的攻击检测
                    if (abs(dex) == 2 && abs(dey) == 2) {
                        int ex = x + dex / 2, ey = y + dey / 2;
                        auto& eGrid = state[currTurnId][xy2pos(ex, ey)];
                        if (eGrid.color == EMPTY) {
                            return true;
                        }
                    }
                    break;
                }
                case Rook: { // 车的攻击检测
                    if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my)) {
                        return true;
                    }
                    break;
                }
                case Pawn: { // 兵的攻击检测
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
                case Cannon: { // 炮的攻击检测（必须隔一个棋子）
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

// 检查两点连线中间是否为空（不含起止点）
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

// 检查是否在同行/同列/同斜线
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

// 测试某一步棋走完之后，己方老将是否依然处于被攻击状态
bool Chess::isMyKingAttackedAfterMove(const Move &move) {
    colorType originColor = currColor;
    // 假设走这一步
    makeMoveAssumeLegal(move);
    int kingX = -1, kingY = -1;
    // 寻找己方老将
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == originColor && curGrid.type == King) {
                kingX = x; kingY = y;
                break;
            }
        }
    }
    // 检测是否被攻击
    bool myKingStillAttacked = attacked(oppColor(originColor), kingX, kingY);
    // 必须撤销状态
    undoMove();
    assert(currColor == originColor);
    return myKingStillAttacked;
}

bool Chess::isMoveValid(const Move& move, bool mustDefend) {
    return isLegalMove(move, mustDefend);
}

// 检查走完这一步是否能赢
bool Chess::winAfterMove(const Move &move) {
    makeMoveAssumeLegal(move);
    vector<Move> oppLegalMoves;
    generateMoves(oppLegalMoves);
    undoMove();
    return oppLegalMoves.empty(); // 对手无步可走我方胜利
}

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

// 超过60步无吃子判和
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

bool Chess::inKingArea(int mx, int my, colorType mcolor) {
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED) {
        return mx >= 3 && mx <= 5 && my >= 0 && my <= 2;
    } else {
        return mx >= 3 && mx <= 5 && my >= 7 && my <= 9;
    }
}

bool Chess::inColorArea(int mx, int my, colorType mcolor) {
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED) {
        return my <= 4;
    } else {
        return my >= 5;
    }
}

// 统计起点和终点连线中间的棋子数量（用于炮走棋检测）
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

// 撤销上一步操作
void Chess::undoMove() {
    if (currTurnId == 0) {
        return;
    }
    state.pop_back(); // 将棋盘状态回退一层
    currTurnId--;
    currColor = oppColor();
    continuePeaceTurn.pop_back();
    lastMoveEaten.pop_back();
}

// 生成遵守“禁止长将/长捉等重复局面”的合法步
void Chess::generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend) {
    vector<Move> firstMoves;
    generateMoves(firstMoves, mustDefend);
    legalMoves.clear();
    for (const auto& move : firstMoves) {
        // 如果此步没有导致重复死循环，则加入合法列表
        if (!repeatAfterMove(move)) {
            legalMoves.push_back(move);
        }
    }
}

// 检查某个步是否会导致三次重复局面（长将、长捉通常在此处拦截）
bool Chess::repeatAfterMove(const Move& move) {
    makeMoveAssumeLegal(move);
    int tmp_id = currTurnId;
    // 如果有吃子发生，局面不可逆转，绝不可能重复，直接返回 false
    if (lastMoveEaten[tmp_id]) {
        undoMove();
        return false;
    }
    
    // 向前追溯直到上次吃子或者开局
    while (true) {
        if (tmp_id <= 1) {
            break;
        }
        if (lastMoveEaten[tmp_id-2]) {
            break;
        }
        tmp_id -= 2; // 只检查该方走棋的局合
    }
    
    // 统计盘面完全一样的次数
    int repeatTime = 0;
    for (int id = currTurnId; id >= tmp_id; id -= 2) {
        if (state[id] == state[currTurnId]) {
            repeatTime++;
        }
    }
    undoMove();
    return repeatTime >= 3; // 达到或超过3次，属于违规重复局面
}

bool Chess::isLegalMoveWithForbidden(const Move& move, bool mustDefend) {
    vector<Move> currLegalMoves;
    generateMovesWithForbidden(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

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



//以下为决策实现的算法

//1.原代码已经实现了合法步的生成，所以只需要调用void generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend=true) 获取合法所有步
//2.在搜索算法中模拟下一步棋调用bool makeMoveAssumeLegal(const Move& move)
//3.在搜索算法中撤销模拟的棋步调用void undoMove()


// 评估函数
int evaluateBoard(const Chess& chess)
{

}


// alpha-beta 搜索
int alphaBetaSearch()
{

}