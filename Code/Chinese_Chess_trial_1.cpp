#include<iostream>
#include<string>
#include<vector>
#include<cmath>
#include<map>
#include<cassert>  //提供断言，检查程序在运行条件中是否为真
#include<algorithm>
#include<vector>
#include<time.h>
#include"jsoncpp/json.h"

using std::string;
using std::vector;
using std::map;
using std::cout;
using std::cin;
using std::cout;
using std::endl;

#define BOARDWIDTH 9
#define BOARDHEIGHT 10

enum stoneType{
    None = 0, //空位
    King = 1, //将 帅
    Bishop = 2, //象 相
    Knight = 3, //马
    Rook = 4, //车
    Pawn = 5, //兵 卒
    Cannon = 6, //炮
    Assistant = 7 //士
};

enum colorType{
    BLACK = 0, // 黑方
    RED = 1,   // 红方
    EMPTY = 2  // 空（无棋子）
};

int baseline[2] = {9,0}; // 各自阵营的底线y坐标（黑方9，红方0）

//stoneSym 和 charSym建立一一映射，与棋子种类对应
//用结构体是否更加简洁?
char stoneSym[] = "*kbnrpca"; // 棋子类型的字符表示，按stoneType枚举的顺序排列
map<char,stoneType> charSym = { // 将字符映射到对应棋子类型的字典
    {stoneSym[0],None},
    {stoneSym[1],King},
    {stoneSym[2],Bishop},
    {stoneSym[3],Bishop},
    {stoneSym[4],Rook},
    {stoneSym[5],Pawn},
    {stoneSym[6],Cannon},
    {stoneSym[7],Assistant}
};

//不同棋子的走动方法（位移增量）
int dx[] = {-1,-1,-1,0,0,1,1,1}; // 8个方向的x轴位移
int dy[] = {-1,0,1,-1,1,-1,0,1}; // 8个方向的y轴位移

int dx_ob[] = {-1,1,-1,1};   //斜走（士的走法）x轴位移
int dy_ob[] = {-1,-1,1,1};   //斜走（士的走法）y轴位移

int dx_lr[] = {-1,1};        //左右横走（过河卒/兵）x轴位移

int dx_strai[] = {-1,0,0,1}; //直走（车、炮、将帅、兵）x轴位移
int dy_strai[] = {0,-1,1,0}; //直走（车、炮、将帅、兵）y轴位移

int dx_knight[] = {-2,-2,-1,-1,1,1,2,2};  //马的移动方法（日字）x轴位移
int dy_knight[] = {-1,1,-2,2,-2,2,-1,1};  //马的移动方法（日字）y轴位移

int dx_knight_foot[] = {-1,-1,0,0,0,0,1,1};  //马的直走移动，判断是否“蹩马腿”x轴位移
int dy_knight_foot[] = {0,0,-1,1,-1,1,0,0};  //马的直走移动，判断是否“蹩马腿”y轴位移

int dx_bishop[] = {-2,-2,2,2};  //象的移动方法（田字）x轴位移
int dy_bishop[] = {-2,2,-2,2};  //象的移动方法（田字）y轴位移

int dx_bishop_eye[] = {-1,-1,1,1};  //象的斜向移动，判断是否“堵象眼”x轴位移
int dy_bishop_eye[] = {-1,1,-1,1};  //象的斜向移动，判断是否“堵象眼”y轴位移

// 将输入的小写字母坐标（'a'-'i'）专为对应的x轴整数下标（0-8）
int pgnchar2int(char c){
    return (int)c - (int)'a';
}
// 将x轴的整数下标（0-8）转回小写字母坐标（'a'-'i'）
char pgnint2char(int i){
    return (char)((int)'a' + i);
}

// 将输入的字符数字（'0'-'9'）转为对应的y轴整数下标（0-9）
int char2int(char c){
    return (int)c - (int)'0';
}
// 将y轴的整数下标（0-9）转回字符数字（'0'-'9'）
char int2char(int i){
    return (char)((int)'0' + i);
}

// 表示棋子在棋盘上的一次移动
struct Move
{
    int source_x,source_y,target_x,target_y; // 起点x坐标，起点y坐标，终点x坐标，终点y坐标
    Move() {source_x = -1;
            source_y = -1;
            target_x = -1;
            target_y = -1;
    }
    Move(int msx,int msy,int mtx,int mty):source_x(msx),source_y(msy),target_x(mtx),target_y(mty){};
    //棋盘的任意一点的坐标，由一个数字和一个字母共同确定
    //x方向为行方向，从左到右，abcdefghi
    //y方向为列方向，从下到上，0123456789
    Move(string msource,string mtarget){
        source_x = pgnchar2int(msource[0]);
        source_y = char2int(msource[1]);
        target_x = pgnchar2int(mtarget[0]);
        target_y = char2int(mtarget[1]);
    }

    // 重载比较运算符，用于判断两步棋是否一致
    bool operator ==(const Move& other)const{
        return source_x == other.source_x && source_y == other.source_y && target_x == other.target_x && target_y == other.target_y;
    }
};

//网格上的一点的x y确定后，确定棋子类型
// 表示棋盘上的每一个交叉点（网格中的一点）
struct Grid{
    stoneType type; // 棋子类型（如将、车等）
    colorType color; // 该点上棋子的颜色（红、黑、空）
    Grid() : type(None),color(EMPTY){};
    Grid(stoneType mtype,colorType mcolor):type(mtype),color(mcolor){};
    
    // 重载赋值运算符，用于更新棋盘网格状态
    Grid& operator = (const Grid& other){
        if(this != &other){
            type = other.type;
            color = other.color;
        }

        return *this;
    }
    bool operator == (const Grid& other)const{
        return type == other.type && color == other.color;
    }
};
//由chess传回相应方法的参数，再由当前待走的棋子作决定，判断：是否 合法/吃子，实质上是移动是否合法
class Chess{
private:
    std::vector<std::vector<Grid>> state; // 记录棋局对弈的过程状态，每走一步增加一个局面，相当于 快照
    int currTurnId;                       // 当前的回合数（走步的层数）
    colorType currColor;                  // 当前该走棋的一方颜色（红或黑）
    vector<int> continuePeaceTurn;        // 记录连续未吃子进行的回合数（用于判断自然限着和局策略）
    vector<int> lastMoveEaten;            // 记录历史每一步是否发生了吃子行为
public:
    Chess();
    void resetBoard();  //初始化
    void generateMoves(vector<Move>& legalMoves,bool mustDefend = true);                      //总移动逻辑，若将/帅被将，必须防御
    void generateMovesWithForbidden(vector<Move>& legalMoves,bool mustDefend = true);         //着法生成器，保留所有可能下法，将下法以Move的数据结构存储
    bool repeatAfterMove(const Move& move);
    static bool inBoard(int mx,int my); //单例 / 类的static方法
    static bool inKingArea(int mx,int my,colorType mcolor);
    static bool inColorArea(int mx,int my,colorType mcolor);
    colorType oppColor();
    colorType oppColor(colorType mcolor);
    bool isMoveValid(const Move& move,bool mustDefend = true);
    bool isLegalMove(const Move& move,bool mustDefend = true);
    bool isLegalMoveWithForbidden(const Move& move,bool mustDefend = true);
    bool isMoveValidWithForbidden(const Move& move,bool mustDefend = true);
    bool makeMoveAssumeLegal(const Move& move);
    bool attacked(colorType color,int mx,int my);
    bool betweenIsEmpty(int sx,int sy,int ex,int ey);
    int betweenNotEmptyNum(int sx,int sy,int ex,int ey);
    static bool inSameLine(int sx,int sy,int ex,int ey);
    static bool inSameStraightLine(int sx,int sy,int ex,int ey);
    static bool inSameObiqueLine(int sx,int sy,int ex,int ey);
    bool isMyKingAttackedAfterMove(const Move& move);
    bool isOppKingAttackedAfterMove(const Move& move);
    bool currKingAttacked(const Move& move);
    bool winAfterMove(const Move& move);
    void undoMove();
    bool exceedMaxPeaceState();
    static int xy2pos(int mx,int my);
    static int pos2x(int mpos);
    static int pos2y(int mpos);
    void printBoard();

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

};

void getInputBotzone(Chess& chess);
void giveOutputBotzone(Chess& chess);

// 主入口函数，负责调用Botzone交互逻辑
int main(){
    Chess chess;
    getInputBotzone(chess);
    giveOutputBotzone(chess);

    return 0;
}

// 基于当前棋局状态评估计算下一步棋并按Botzone要求输出Json结果
void giveOutputBotzone(Chess& chess){
    vector<Move> retMoves;
    chess.generateMovesWithForbidden(retMoves);
    Json::Value ret;
    
    if(retMoves.empty()){
        ret["response"]["source"] = string("-1");
        ret["response"]["target"] = string("-1");
    }else{
        //在这里，修改下子逻辑
        srand(time(nullptr));
        int choice = rand() % retMoves.size();
        auto selMove = retMoves[choice];
        ret["response"]["source"] = string(1, pgnint2char(selMove.source_x)) + string(1, int2char(selMove.source_y));
        ret["response"]["target"] = string(1, pgnint2char(selMove.target_x)) + string(1, int2char(selMove.target_y));
    }
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;
    
}

//从botzone平台中提取棋局数据，同步历史状态至当前最新局面
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
// 初始化/重置棋盘结构，为每个网格填充初始棋面阵容
void Chess::resetBoard(){
    vector<Grid> currState;
    Grid boardInfo[BOARDWIDTH][BOARDHEIGHT];
    for(int x = 0;x < BOARDWIDTH;x ++){
        for(int y = 0;y < BOARDHEIGHT;y ++){
            boardInfo[x][y] = Grid(None,EMPTY);
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

    for(int mpos = 0;mpos < BOARDWIDTH * BOARDHEIGHT;mpos ++){
        currState.push_back(boardInfo[pos2x(mpos)][pos2y(mpos)]);
    }

    state.clear();
    state.push_back(currState);
}

// Chess类型实例的构造函数，调取初始化并设置红先黑后记录
Chess::Chess(){
    resetBoard();
    continuePeaceTurn.clear();
    lastMoveEaten.clear();
    continuePeaceTurn.push_back(0);
    lastMoveEaten.push_back(false);
    currColor = RED;
    currTurnId = 0;
}

// 收集目前棋盘上己方所有合理且不违规的下子移动操作至legalMoves中
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


// 检查指定的坐标是否未超出棋盘边界内的有效范围
bool Chess::inBoard(int mx, int my) {
    return mx >= 0 && mx < BOARDWIDTH && my >= 0 && my < BOARDHEIGHT;
}

// 返回当前走棋方的敌对一方的颜色状态
colorType Chess::oppColor() {
    return currColor == BLACK ? RED : BLACK;
}

// 检查给定的移动走法(move)是否在所有生成的合法移动集合中
bool Chess::isLegalMove(const Move &move, bool mustDefend) {
    vector<Move> currLegalMoves;
    generateMoves(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

// 模拟执行一步假设合法的移动，并推进回合、切换走棋颜色，记录吃子状态等（不进行移动合法性校验）
bool Chess::makeMoveAssumeLegal(const Move &move) {
    int nextPeaceTrun =  continuePeaceTurn[currTurnId] + 1;
    vector<Grid> copyState = state[currTurnId];  //state - 当前的棋局状态。这里提前做拷贝
    state.push_back(copyState);
    currTurnId++;

    //Grid:
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

// 判断指定位置(mx, my)是否被特定阵营(color)的火力直接攻击或“将军”
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


// 在水平或垂直方向上，检测自起始坐标两点之间是否为空置没有棋子的状态
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


// 判断两点是否在共线(水平、垂直、或者绝对对角线)
bool Chess::inSameLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx, dey = ey - sy;
    return dex == dey || dex == -dey || dex == 0 || dey == 0;
}

// 判断两点是否处于完全水平或垂直的共同直线上
bool Chess::inSameStraightLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx, dey = ey - sy;
    return dex == 0 || dey == 0;
}

// 判断两点是否在纯斜线/对角线之上(斜率正负1)
bool Chess::inSameObiqueLine(int sx, int sy, int ex, int ey) {
    int dex = ex - sx, dey = ey - sy;
    return dex == dey || dex == -dey;
}


// 假设执行该举之后，判断我方主帅是否依然暴漏在被敌方吃掉的威胁中（检查“送将”）
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


// 二次封装对合法移动的校验(必须防御为主)
bool Chess::isMoveValid(const Move& move, bool mustDefend) {
    return isLegalMove(move, mustDefend);
}

// 模拟这走法后，如果对方没有合法着法能进行活动，判定当前方获胜
bool Chess::winAfterMove(const Move &move) {
    makeMoveAssumeLegal(move);
    vector<Move> oppLegalMoves;
    generateMoves(oppLegalMoves);
    undoMove();
    return oppLegalMoves.empty();
}

// 判断在走步之前当前方的老帅是否已经被对方攻击处于被“将死”的状态
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


// 预演着法后，判断对方主帅是否被直接致死/面临将军状态
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

// 检查连续未发生吃子的局数是否达到和局规定（通常是60回合）
bool Chess::exceedMaxPeaceState() {
    return continuePeaceTurn[currTurnId] >= 60 - 1;
}

// 辅助函数：将表示x和y二维坐标的位置打平映射为一维数组索引(从0到89)
int Chess::xy2pos(int mx, int my) {
    return my * BOARDWIDTH + mx;
}

// 辅助函数：从1维数组索引解码出棋盘X轴二维坐标
int Chess::pos2x(int mpos) {
    return mpos % BOARDWIDTH;
}

// 辅助函数：从1维数组索引解码出棋盘Y轴二维坐标
int Chess::pos2y(int mpos) {
    return mpos / BOARDWIDTH;
}

// 检查给定的坐标位置是否属于特定颜色阵容的“九宫格”范围
bool Chess::inKingArea(int mx, int my, colorType mcolor) {
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED) {
        return mx >= 3 && mx <= 5 && my >= 0 && my <= 2;
    } else {
        return mx >= 3 && mx <= 5 && my >= 7 && my <= 9;
    }
}

// 检查指定坐标位置是否仍在给定的特定颜色所在棋盘半区（兵未过河前、士象等不得越界）
bool Chess::inColorArea(int mx, int my, colorType mcolor) {
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED) {
        return my <= 4;
    } else {
        return my >= 5;
    }
}


// 如果在同一直线上，统计起始两点之间中间存在多少个棋子（用于判断炮的跳吃）
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

// 提供特定颜色的敌方颜色
colorType Chess::oppColor(colorType mcolor) {
    return mcolor == RED ? BLACK : RED;
}

// 悔棋操作撤销历史一次行进步伐，恢复棋盘快照至前一回合
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

// 附带“禁止长将或多次重复禁着”规则判断的所有合法动作全集生成器
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

// 查重：预判该步是否导致同一局面出现三次（从而违反规则）
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

// 在含有禁着检测的基础上判断特定单着(move)是否合法合规
bool Chess::isLegalMoveWithForbidden(const Move& move, bool mustDefend) {
    vector<Move> currLegalMoves;
    generateMovesWithForbidden(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

// 可视化输出当前整个棋盘情况，终端调试打印所用
void Chess::printBoard() {
    std::cout << std::endl << "Board" << std::endl;
    for (int y = 9; y >= 0; y--) {
        for (int x = 0; x <= 8; x++) {
            std::cout << stoneSym[state[currTurnId][xy2pos(x,y)].type] << " ";
        }
        std::cout << std::endl;
    }
}

// 二次封装结合完整禁着规则约束（如限制长打）验证给定的单个指令能否生效
bool Chess::isMoveValidWithForbidden(const Move& move, bool mustDefend) {
    return isLegalMoveWithForbidden(move, mustDefend);
}
/*
    一定要想出尽可能多的创新点，不要被轻易地被别人所赶上！不要再重复无能和平庸的方法！
*/
/*
    每个回合，Bot收到的request是一个JSON对象，表示对方走子
    "source":"a1",
    "target":"a2"

    好处在于：不需要提前判断是哪个棋子，先提交Json下子请求再进行后续操作
*/
/*
    0.理解示例程序
    1.搞懂交互语法
    2.MiniMax 搜素
    2.5 启发式搜素
    3.α-β剪枝 
    4.使用转置表/哈希表 调用棋谱

*/

//决策实现的算法仅需调用下方三个接口

/*
1.原代码已经实现了合法步的生成，所以只需要调用void generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend=true) 获取合法所有步
2.在搜索算法中模拟下一步棋调用bool makeMoveAssumeLegal(const Move& move)
3.在搜索算法中撤销模拟的棋步调用void undoMove()
*/