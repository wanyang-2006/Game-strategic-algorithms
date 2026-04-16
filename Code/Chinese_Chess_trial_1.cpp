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
    BLACK = 0,
    RED = 1,
    EMPTY = 2
};

int baseline[2] = {9,0}; //?

//stoneSym 和 charSym建立一一映射，与棋子种类对应
//用结构体是否更加简洁?
char stoneSym[] = "*kbnrpca";
map<char,stoneType> charSym = {
    {stoneSym[0],None},
    {stoneSym[1],King},
    {stoneSym[2],Bishop},
    {stoneSym[3],Bishop},
    {stoneSym[4],Rook},
    {stoneSym[5],Pawn},
    {stoneSym[6],Cannon},
    {stoneSym[7],Assistant}
};

//不同棋子的走动方法
int dx[] = {-1,-1,-1,0,0,1,1,1};
int dy[] = {-1,0,1,-1,1,-1,0,1};

int dx_ob[] = {-1,1,-1,1};   //斜走
int dy_ob[] = {-1,-1,1,1};

int dx_strai[] = {-1,0,0,1}; //直走
int dy_strai[] = {0,-1,1,0};

int dx_knight[] = {-2,-2,-1,-1,1,1,2,2};  //马的移动方法
int dy_knight[] = {-1,1,-2,2,-2,2,-1,1};

int dx_knight_foot[] = {-1,-1,0,0,0,0,1,1};  //马的直走移动，判断是否“蹩马腿”
int dy_knight_foot[] = {0,0,-1,1,-1,1,0,0};

int dx_bishop[] = {-2,-2,2,2};  //象的移动方法
int dy_bishop[] = {-2,2,-2,2};

int dx_bishop_eye[] = {-1,-1,1,1};  //象的斜向移动，判断是否“堵象眼”
int dy_bishop_eye[] = {-1,1,-1,1}; 

//将输入的字符x <==> 'x' - 'a',将棋子标号和对应字母互相转换
int pgnchar2int(char c){
    return (int)c - (int)'a';
}
char pgnint2char(int i){
    return (char)((int)'a' + i);
}

int char2int(char c){
    return (int)c - (int)'0';
}
char int2char(int i){
    return (char)((int)'0' + i);
}

struct Move
{
    int source_x,source_y,target_x,target_y;
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

    bool operator ==(const Move& other)const{
        return source_x == other.source_x && source_y == other.source_y && target_x == other.target_x && target_y == other.target_y;
    }
};

//网格上的一点的x y确定后，确定棋子类型
struct Grid{
    stoneType type;
    colorType color;
    Grid() : type(None),color(EMPTY){};
    Grid(stoneType mtype,colorType mcolor):type(mtype),color(mcolor){};
    
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
    std::vector<std::vector<Grid>> state;
    int currTurnId;
    colorType currColor;
    vector<int> continuePeaceTurn;
    vector<int> lastMoveEaten;
public:
    Chess();
    void resetBoard();  //初始化
    void generateMoves(vector<Move>& legalMoves,bool mustDefend = true);  //总移动逻辑，若将/帅被将，必须防御
    void generateMovesWithForbidden(vector<Move>& legalMoves,bool mustDefend = true);
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

int main(){
    Chess chess;
    getInputBotzone(chess);
    giveOutputBotzone(chess);

    return 0;
}

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

//从botzone平台中提取棋局数据
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

Chess::Chess(){
    resetBoard();
    continuePeaceTurn.clear();
    lastMoveEaten.clear();
    continuePeaceTurn.push_back(0);
    lastMoveEaten.push_back(false);
    currColor = RED;
    currTurnId = 0;
}

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

//对于假设的移动，判断其是否正确
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


bool Chess::isMoveValid(const Move& move, bool mustDefend) {
    return isLegalMove(move, mustDefend);
}

bool Chess::winAfterMove(const Move &move) {
    makeMoveAssumeLegal(move);
    vector<Move> oppLegalMoves;
    generateMoves(oppLegalMoves);
    undoMove();
    return oppLegalMoves.empty();
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