#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <cassert>
#include <algorithm>
#include <vector>
#include <time.h>
#include "jsoncpp/json.h"


using std::string;
using std::vector;
using std::map;
using std::cin;
using std::cout;
using std::endl;

#define BOARDWIDTH 9 // 棋盘宽度
#define BOARDHEIGHT 10 // 棋盘高度
enum stoneType {None = 0, King=1, Bishop=2, Knight=3, Rook=4, Pawn=5, Cannon=6, Assistant=7}; // 棋子类型枚举
enum colorType {BLACK=0, RED=1, EMPTY = 2}; // 阵营颜色枚举
int baseLine[2] = {9, 0}; // 红黑双方的底线Y坐标
char stoneSym[] = "*kbnrpca"; // 棋子类型对应的字符表示
map<char,stoneType> charSym = {{stoneSym[0], None}, {stoneSym[1], King},
                               {stoneSym[2], Bishop}, {stoneSym[3], Knight},
                               {stoneSym[4], Rook}, {stoneSym[5], Pawn},
                               {stoneSym[6], Cannon}, {stoneSym[7], Assistant}}; // 字符到棋子类型的映射枚举

int dx[] = {-1,-1,-1,0,0,1,1,1}; // 8个方向的X轴偏移（通用）
int dy[] = {-1,0,1,-1,1,-1,0,1}; // 8个方向的Y轴偏移（通用）
int dx_ob[] = {-1,1,-1,1}; // 士（仕）的斜向X轴偏移（4个对角方向）
int dy_ob[] = {-1,-1,1,1}; // 士（仕）的斜向Y轴偏移（4个对角方向）
int dx_strai[] = {-1,0,0,1}; // 直线的X轴偏移（上下左右）
int dy_strai[] = {0,-1,1,0}; // 直线的Y轴偏移（上下左右）
int dx_lr[] = {-1,1}; // 兵（卒）过河后的左右X轴偏移
int dx_knight[] = {-2,-2,-1,-1,1,1,2,2}; // 马走“日”目的地的X轴偏移
int dy_knight[] = {-1,1,-2,2,-2,2,-1,1}; // 马走“日”目的地的Y轴偏移
int dx_knight_foot[] = {-1,-1,0,0,0,0,1,1}; // 马走“日”对应的蹩马腿检查点X轴偏移
int dy_knight_foot[] = {0,0,-1,1,-1,1,0,0}; // 马走“日”对应的蹩马腿检查点Y轴偏移
int dx_bishop[] = {-2,-2,2,2}; // 象（相）走“田”目的地的X轴偏移
int dy_bishop[] = {-2,2,-2,2}; // 象（相）走“田”目的地的Y轴偏移
int dx_bishop_eye[] = {-1,-1,1,1}; // 象（相）走“田”对应的塞象眼检查点X轴偏移
int dy_bishop_eye[] = {-1,1,-1,1}; // 象（相）走“田”对应的塞象眼检查点Y轴偏移


// PGN字符横坐标转数值
int pgnchar2int(char c) {
    return (int)c - (int)'a';
}

// 数值转PGN字符横坐标
char pgnint2char(int i) {
    return (char)((int)'a' + i);
}

// 字符数字转整型数值
int char2int(char c) {
    return (int)c - (int)'0';
}

// 整型数值转字符数字
char int2char(int i) {
    return (char)((int)'0' + i);
}

struct Move {
    int source_x, source_y, target_x, target_y;
    Move() {source_x = -1; source_y = -1; target_x = -1; target_y = -1;}
    Move(int msx, int msy, int mtx, int mty): source_x(msx), source_y(msy),
                                                                target_x(mtx), target_y(mty) {;}
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
}; // 描述一步行棋（起点和终点坐标）的数据结构

struct Grid {
    stoneType type;
    colorType color;
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
}; // 描述棋盘上单个格子（当前包含的棋子类型和颜色）的数据结构

class Chess {
private:
    std::vector<std::vector<Grid>> state;
    int currTurnId;
    colorType currColor;
    vector<int> continuePeaceTurn;
    vector<int> lastMoveEaten;
    //新增属性
    vector<Grid> curstones;    //存储我方的棋子 一个容器内存储各种不同的棋子
    vector<bool> isbeattacked; //存储我方的棋子是否会被吃，和curstone中的索引顺序一致
public:
    Chess(); // 构造函数，初始化棋盘
    void resetBoard(); // 重置棋盘为初始状态
    void generateMoves(vector<Move> &legalMoves, bool mustDefend=true); // 生成常规合法移动步
    void generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend=true); // 生成包含禁手规则的合法移动步
    bool repeatAfterMove(const Move& move); // 判断移动后是否出现长将/长捉等重复局面
    static bool inBoard(int mx, int my); // 判断坐标是否在棋盘内
    static bool inKingArea(int mx, int my, colorType mcolor); // 判断是否在九宫格内
    static bool inColorArea(int mx, int my, colorType mcolor); // 判断是否在己方半场
    colorType oppColor(); // 获取对手颜色
    colorType oppColor(colorType mcolor); // 获取指定颜色的对手颜色
    bool isMoveValid(const Move& move, bool mustDefend=true); // 验证移动是否有效
    bool isLegalMove(const Move& move, bool mustDefend=true); // 判断是否为合法移动
    bool isLegalMoveWithForbidden(const Move& move, bool mustDefend=true); // 判断包含禁手规则下是否合法
    bool isMoveValidWithForbidden(const Move& move, bool mustDefend=true); // 验证包含禁手规则下的移动
    bool makeMoveAssumeLegal(const Move& move); // 假设合法并执行移动，更新状态
    bool attacked(colorType color, int mx, int my); // 检查己方将/帅是否被某方颜色攻击
    bool betweenIsEmpty(int sx, int sy, int ex, int ey); // 判断两点之间是否没有棋子
    int betweenNotEmptyNum(int sx, int sy, int ex, int ey); // 计算两点之间非空棋子的数量
    static bool inSameLine(int sx, int sy, int ex, int ey); // 判断两点是否在同一直线或对角线
    static bool inSameStraightLine(int sx, int sy, int ex, int ey); // 判断两点是否在同行或同列
    static bool inSameObiqueLine(int sx, int sy, int ex, int ey); // 判断两点是否在同斜线
    bool isMyKingAttackedAfterMove(const Move& move); // 判断移动后己方主帅是否被将军
    bool isOppKingAttackedAfterMove(const Move& move); // 判断移动后是否将对手的军
    bool currKingAttacked(const Move& move); // 判断当前主帅是否正在被攻击
    bool winAfterMove(const Move& move); // 判断该移动是否能直接获胜
    void undoMove(); // 撤销上一步移动
    bool exceedMaxPeaceState(); // 判断是否超过最大求和/平局步数
    static int xy2pos(int mx, int my); // 二维坐标转一维索引
    static int pos2x(int mpos); // 一维索引转X坐标
    static int pos2y(int mpos); // 一维索引转Y坐标
    void printBoard(); // 打印当前棋盘


    //新增函数 
    void getcurstones(); //获取我方的棋子，使用currColor
    bool isattacked(colorType color,int mx,int my); 
    bool iscurstonebeattacked(); //判断我当前的棋子(存储在curstones)是否可能被吃
};

void getInputBotzone(Chess& chess); // 处理Botzone平台输入
void giveOutputBotzone(Chess& chess); // 向Botzone平台输出落子

int main() {
    Chess chess;
    getInputBotzone(chess);
    giveOutputBotzone(chess);
    return 0;
}


// 向Botzone平台输出落子的函数定义
void giveOutputBotzone(Chess& chess) {
    vector<Move> retMoves;
    chess.generateMovesWithForbidden(retMoves);
    Json::Value ret;

    
    if (retMoves.empty()) {
        ret["response"]["source"] = string("-1");
        ret["response"]["target"] = string("-1");
    } else {
        srand(time(nullptr));
        int choice = rand() % retMoves.size();
        auto selMove = retMoves[choice];
        ret["response"]["source"] = string(1, pgnint2char(selMove.source_x)) + string(1, int2char(selMove.source_y));
        ret["response"]["target"] = string(1, pgnint2char(selMove.target_x)) + string(1, int2char(selMove.target_y));
    }
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;
}

// 解析Botzone平台输入以更新棋局状态的函数定义
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

// 重置棋盘状态为初始状态的函数定义
void Chess::resetBoard() {
    vector<Grid> currState;
    Grid boardInfo[BOARDWIDTH][BOARDHEIGHT];
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            boardInfo[x][y] = Grid(None, EMPTY);
        }
    }

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

// Chess类构造函数的定义
Chess::Chess() {
    resetBoard();
    continuePeaceTurn.clear();
    lastMoveEaten.clear();
    continuePeaceTurn.push_back(0);
    lastMoveEaten.push_back(false);
    currColor = RED;
    currTurnId = 0;
}

// 生成当前所有伪合法或合法步的函数定义
void Chess::generateMoves(vector<Move> &legalMoves, bool mustDefend) {
    legalMoves.clear();
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == currColor) {
                switch (curGrid.type) {
                    case King: {
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            if (!inKingArea(tx, ty, curGrid.color)) {
                                continue;
                            }
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            if (tGrid.color == currColor)
                                continue;
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
                    case Assistant: {
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_ob[dir], ty = y + dy_ob[dir];
                            if (!inKingArea(tx, ty, curGrid.color)) {
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
                    case Pawn: {
                        int forwardOne = (curGrid.color == RED ? 1 : -1);
                        bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5));
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
                    case Knight: {
                        for (int dir = 0; dir < 8; dir++) {
                            int tx = x + dx_knight[dir], ty = y + dy_knight[dir];
                            if (!inBoard(tx, ty)) {
                                continue;
                            }
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            int fx = x + dx_knight_foot[dir], fy = y + dy_knight_foot[dir];
                            auto& fGrid = state[currTurnId][xy2pos(fx, fy)];
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
                    case Bishop: {
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_bishop[dir], ty = y + dy_bishop[dir];
                            if (!inBoard(tx, ty) || !inColorArea(tx, ty, currColor)) {
                                continue;
                            }
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
                    case Rook: {
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            while (inBoard(tx, ty)) {
                                auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                                if (tGrid.color == currColor) {
                                    break;
                                } else if (tGrid.color == oppColor()) {
                                    if (mustDefend) {
                                        if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                            legalMoves.emplace_back(x, y, tx, ty);
                                        }
                                    } else {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                    break;
                                } else {
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
                    case Cannon: {
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            while (inBoard(tx, ty)) {
                                int betweenNum = betweenNotEmptyNum(x, y, tx, ty);
                                if (betweenNum > 1) {
                                    break;
                                } else if (betweenNum == 1) {
                                    auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
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

// 判断给定坐标是否位于棋盘内的函数定义
bool Chess::inBoard(int mx, int my) {
    return mx >= 0 && mx < BOARDWIDTH && my >= 0 && my < BOARDHEIGHT;
}

// 返回当前行动方对手颜色的函数定义
colorType Chess::oppColor() {
    return currColor == BLACK ? RED : BLACK;
}

// 判断给定移动是否为合法步的函数定义
bool Chess::isLegalMove(const Move &move, bool mustDefend) {
    vector<Move> currLegalMoves;
    generateMoves(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

// 在假设移动合法的前提下执行该移动，更新棋盘状态的函数定义
bool Chess::makeMoveAssumeLegal(const Move &move) {
    int nextPeaceTrun =  continuePeaceTurn[currTurnId] + 1;
    vector<Grid> copyState = state[currTurnId];
    state.push_back(copyState);
    currTurnId++;

    auto& sourceGrid = state[currTurnId][xy2pos(move.source_x, move.source_y)];
    auto& targetGrid = state[currTurnId][xy2pos(move.target_x, move.target_y)];
    if (targetGrid.color == oppColor()) {
        nextPeaceTrun = 0;
        lastMoveEaten.push_back(true);
    } else {
        lastMoveEaten.push_back(false);
    }
    targetGrid.type = sourceGrid.type;
    targetGrid.color = sourceGrid.color;
    assert(sourceGrid.color == currColor);

    sourceGrid.color = EMPTY;
    sourceGrid.type = None;

    continuePeaceTurn.push_back(nextPeaceTrun);
    currColor = oppColor();
    return true;
}

// 判断将/帅在指定位置是否受特定颜色棋子攻击的函数定义
bool Chess::attacked(colorType color, int mx, int my) {
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color != color) {
                continue;
            }
            int dex = mx - x, dey = my - y;
            switch (curGrid.type) {
                case King: {
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
                case Knight: {
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
                case Bishop: {
                    if (abs(dex) == 2 && abs(dey) == 2) {
                        int ex = x + dex / 2, ey = y + dey / 2;
                        auto& eGrid = state[currTurnId][xy2pos(ex, ey)];
                        if (eGrid.color == EMPTY) {
                            return true;
                        }
                    }
                    break;
                }
                case Rook: {
                    if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my)) {
                        return true;
                    }
                    break;
                }
                case Pawn: {
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
                case Cannon: {
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

// 判断两坐标点之间是否无任何棋子的函数定义
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

// 判断两坐标点是否处于同一横线、竖线或对角线的函数定义
bool Chess::inSameLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx, dey = ey - sy;
    return dex == dey || dex == -dey || dex == 0 || dey == 0;
}

// 判断两坐标点是否处于同一横线或竖线的函数定义
bool Chess::inSameStraightLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx, dey = ey - sy;
    return dex == 0 || dey == 0;
}

// 判断两坐标点是否处于同一对角线的函数定义
bool Chess::inSameObiqueLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx, dey = ey - sy;
    return dex == dey || dex == -dey;
}

// 判断执行某步移动后，己方主帅是否处于被攻击状态的函数定义
bool Chess::isMyKingAttackedAfterMove(const Move &move) {
    colorType originColor = currColor;
    makeMoveAssumeLegal(move);
    int kingX = -1, kingY = -1;
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
    undoMove();
    assert(currColor == originColor);
    return myKingStillAttacked;
}

// 综合验证移动是否有效的函数定义
bool Chess::isMoveValid(const Move& move, bool mustDefend) {
    return isLegalMove(move, mustDefend);
}

// 评估某步移动之后是否能造成对方无路可走(获胜)的函数定义
bool Chess::winAfterMove(const Move &move) {
    makeMoveAssumeLegal(move);
    vector<Move> oppLegalMoves;
    generateMoves(oppLegalMoves);
    undoMove();
    return oppLegalMoves.empty();
}

// 判断当前行动方主帅是否正处于被攻击状态的函数定义
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

// 评估执行某步移动后是否对敌方主帅造成攻击(将军)的函数定义
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

// 判断对局是否达到自然限着回合数导致平局的函数定义
bool Chess::exceedMaxPeaceState() {
    return continuePeaceTurn[currTurnId] >= 60 - 1;
}

// 将二维坐标映射到一维数组索引的函数定义
int Chess::xy2pos(int mx, int my) {
    return my * BOARDWIDTH + mx;
}

// 将一维数组索引解码为X轴坐标的函数定义
int Chess::pos2x(int mpos) {
    return mpos % BOARDWIDTH;
}

// 将一维数组索引解码为Y轴坐标的函数定义
int Chess::pos2y(int mpos) {
    return mpos / BOARDWIDTH;
}

// 判断给定坐标是否位于指定方九宫格内的函数定义
bool Chess::inKingArea(int mx, int my, colorType mcolor) {
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED) {
        return mx >= 3 && mx <= 5 && my >= 0 && my <= 2;
    } else {
        return mx >= 3 && mx <= 5 && my >= 7 && my <= 9;
    }
}

// 判断给定坐标是否位于指定方半场内的函数定义
bool Chess::inColorArea(int mx, int my, colorType mcolor) {
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED) {
        return my <= 4;
    } else {
        return my >= 5;
    }
}

// 统计同行或同列两坐标点之间障碍棋子数量的函数定义
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

// 返回给定颜色相对对手颜色的函数定义
colorType Chess::oppColor(colorType mcolor) {
    return mcolor == RED ? BLACK : RED;
}

// 撤销落子并恢复系统至上一步状态的函数定义
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

// 生成满足禁手规则的合法移动步集合的函数定义
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

// 评估某步移动后是否会导致达到三次重复局面的函数定义
bool Chess::repeatAfterMove(const Move& move) {
    makeMoveAssumeLegal(move);
    int tmp_id = currTurnId;
    if (lastMoveEaten[tmp_id]) {
        undoMove();
        return false;
    }
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
    for (int id = currTurnId; id >= tmp_id; id -= 2) {
        if (state[id] == state[currTurnId]) {
            repeatTime++;
        }
    }
    undoMove();
    return repeatTime >= 3;
}

// 综合判断给定移动是否遵守禁手规则而合法的函数定义
bool Chess::isLegalMoveWithForbidden(const Move& move, bool mustDefend) {
    vector<Move> currLegalMoves;
    generateMovesWithForbidden(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

// 将当前棋盘通过控制台可视化的函数定义
void Chess::printBoard() {
    std::cout << std::endl << "Board" << std::endl;
    for (int y = 9; y >= 0; y--) {
        for (int x = 0; x <= 8; x++) {
            std::cout << stoneSym[state[currTurnId][xy2pos(x,y)].type] << " ";
        }
        std::cout << std::endl;
    }
}

// 验证在禁手规则限制下某移动是否有效的函数定义
bool Chess::isMoveValidWithForbidden(const Move& move, bool mustDefend) {
    return isLegalMoveWithForbidden(move, mustDefend);
}


//获取我方的各个棋子
void Chess::getcurstones(){
    int Color = this->currColor;
    //注意：y行x列,y(9->0),x(a->i),tmp(state[currTurnId][..],被展平为一维的9*10个格子)
    for(int y = 9;y >= 0;y --){
        for(int x = 0;x <= 8;x ++){
            if(state[currTurnId][xy2pos(x,y)].color == Color){
                this->curstones.push_back(state[currTurnId][xy2pos(x,y)]);
                if(state[currTurnId][xy2pos(x,y)].type == King){
                    isbeattacked.push_back(attacked(oppColor(this->currColor),x,y));
                }else{
                    isbeattacked.push_back(isattacked(oppColor(this->currColor),x,y));
                }
            }
        }
    }
}

//由attacked接口 ： 非将帅，这一点是否受特定棋子攻击?
//我方的各个棋子是否会被攻击
//color为对方的color，我方的点为(mx,my)
bool Chess::isattacked(colorType color,int mx,int my){
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color != color) {
                continue;
            }
            int dex = mx - x, dey = my - y; //当前敌方棋子的坐标为(x,y) 
            switch (curGrid.type) {
                case King: {
                    //King不能超出九宫格
                    if((abs(dex) == 1 && abs(dey) == 0 && abs(x - 4) <= 1) || (abs(dex) == 0 && abs(dey) == 1 && abs(x - 4) <= 1)){
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
                case Knight: {
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
                case Bishop: {
                    if (abs(dex) == 2 && abs(dey) == 2) {
                        int ex = x + dex / 2, ey = y + dey / 2;
                        auto& eGrid = state[currTurnId][xy2pos(ex, ey)];
                        if (eGrid.color == EMPTY) {
                            return true;
                        }
                    }
                    break;
                }
                case Rook: {
                    if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my)) {
                        return true;
                    }
                    break;
                }
                case Pawn: {
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
                case Cannon: {
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


