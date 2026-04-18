//��botΪbotzoneʾ��bot��������壬��֤�Ϸ���δʹ���κ������㷨

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <cassert>
#include <algorithm>
#include "jsoncpp/json.h"
#include <time.h>
using std::string;
using std::vector;
using std::map;
using std::cin;
using std::cout;
using std::endl;

// 9· 10��
#define BOARDWIDTH 9
#define BOARDHEIGHT 10

// ��������
// None=��, King=��/˧, Bishop=��/��, Knight=��, Rook=��, Pawn=��/��, Cannon=��, Assistant=ʿ/��
enum stoneType {None = 0, King=1, Bishop=2, Knight=3, Rook=4, Pawn=5, Cannon=6, Assistant=7};

// ö�٣�������ɫ��BLACK=�ڷ�, RED=�췽, EMPTY=��λ�ã�
enum colorType {BLACK=0, RED=1, EMPTY = 2};

// �Ź���ĵ���Y���꣨�ڷ�����Ϊ9���췽����Ϊ0��
int baseLine[2] = {9, 0};

// ���Ӷ�Ӧ���ַ���ʾ�����ڴ�ӡ����ԣ�
char stoneSym[] = "*kbnrpca";
map<char,stoneType> charSym = {{stoneSym[0], None}, {stoneSym[1], King},
                               {stoneSym[2], Bishop}, {stoneSym[3], Knight},
                               {stoneSym[4], Rook}, {stoneSym[5], Pawn},
                               {stoneSym[6], Cannon}, {stoneSym[7], Assistant}};

// ==========================================
// �����ƶ�����������
// ==========================================
// �����ͷ�����Χ8���㣩
int dx[] = {-1,-1,-1,0,0,1,1,1};
int dy[] = {-1,0,1,-1,1,-1,0,1};
// ʿ/�˵��ƶ�����б��4���㣩
int dx_ob[] = {-1,1,-1,1};
int dy_ob[] = {-1,-1,1,1};
// ��/��/����ֱ���ƶ������������ң�
int dx_strai[] = {-1,0,0,1};
int dy_strai[] = {0,-1,1,0};
// ��/����Ӻ�������ƶ�
int dx_lr[] = {-1,1};

// �����ƶ����������֣���8����㣩
int dx_knight[] = {-2,-2,-1,-1,1,1,2,2};
int dy_knight[] = {-1,1,-2,2,-2,2,-1,1};
// ���ȵ��������������ڼ�⡰�����ȡ���
int dx_knight_foot[] = {-1,-1,0,0,0,0,1,1};
int dy_knight_foot[] = {0,0,-1,1,-1,1,0,0};

// ��/����ƶ����������֣���4����㣩
int dx_bishop[] = {-2,-2,2,2};
int dy_bishop[] = {-2,2,-2,2};
// ���۵��������������ڼ�⡰�����ۡ���
int dx_bishop_eye[] = {-1,-1,1,1};
int dy_bishop_eye[] = {-1,1,-1,1};

// ==========================================
// ����ϵת�����ߺ���
// ==========================================
// �� PGN ��ĸ(a-i)תΪ 0-8 ����
int pgnchar2int(char c) { return (int)c - (int)'a'; }
// �� 0-8 ����תΪ PGN ��ĸ(a-i)
char pgnint2char(int i) { return (char)((int)'a' + i); }
// ���ַ�����('0'-'9')תΪ���� 0-9
int char2int(char c) { return (int)c - (int)'0'; }
// ������ 0-9 תΪ�ַ�����
char int2char(int i) { return (char)((int)'0' + i); }

// �ṹ�壺��ʾһ����
struct Move {
    int source_x, source_y, target_x, target_y; // ����������յ�����
    Move() {source_x = -1; source_y = -1; target_x = -1; target_y = -1;}
    Move(int msx, int msy, int mtx, int mty): source_x(msx), source_y(msy),
                                                                target_x(mtx), target_y(mty) {;}
    // ��ƽ̨���ص��ַ�����ʽ(��"a0", "a1")����
    Move(string msource, string mtarget) {
        source_x = pgnchar2int(msource[0]);
        source_y = char2int(msource[1]);
        target_x = pgnchar2int(mtarget[0]);
        target_y = char2int(mtarget[1]);
    }
    // �ж��������Ƿ���ȫ��ͬ
    bool operator ==(const Move& other) const {
        return source_x == other.source_x && source_y == other.source_y && target_x == other.target_x
               && target_y == other.target_y;
    }
};

// �ṹ�壺��ʾ�����ϵ�һ������
struct Grid {
    stoneType type;  // ��������
    colorType color; // ������ɫ��������һ��������Ϊ�գ�
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

// �����ࣺ�й����������״̬����
class Chess {
private:
    // ״̬�б���ʹ����ά���鱣����ʷ��ÿһ������������״̬�����������ж��ظ�����
    std::vector<std::vector<Grid>> state;
    int currTurnId;                  // ��ǰ�غ���
    colorType currColor;             // ��ǰ�ֵ���һ������
    vector<int> continuePeaceTurn;   // ����δ���ӵĻغ���������ʵ����Ȼ����60���кͣ�
    vector<int> lastMoveEaten;       // ��¼��ʷÿһ���Ƿ����˳���

public:
    Chess();
    void resetBoard();
    
    // ��������α�Ϸ�����mustDefendΪtrueʱ���޳��ͽ��Ĳ���
    void generateMoves(vector<Move> &legalMoves, bool mustDefend=true);
    // ���ɺϷ������ҹ��˵��ᵼ�������ظ������Υ�沽
    void generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend=true);
    bool repeatAfterMove(const Move& move); // ������ⲽ���Ƿ����ɳ���/��׽���ظ�����
    
    static bool inBoard(int mx, int my);
    static bool inKingArea(int mx, int my, colorType mcolor); // �Ƿ��ھŹ�����
    static bool inColorArea(int mx, int my, colorType mcolor); // �Ƿ�δ����
    
    colorType oppColor();
    colorType oppColor(colorType mcolor);
    
    bool isMoveValid(const Move& move, bool mustDefend=true);
    bool isLegalMove(const Move& move, bool mustDefend=true);
    bool isLegalMoveWithForbidden(const Move& move, bool mustDefend=true);
    bool isMoveValidWithForbidden(const Move& move, bool mustDefend=true);
    
    // ��������ִ��һ���壬����¼��ʷ״̬
    bool makeMoveAssumeLegal(const Move& move);
    // ���壬������һ������
    void undoMove();
    
    // �жϸ�������ĸ����Ƿ����ڱ�ָ��������
    bool attacked(colorType color, int mx, int my);
    
    // ·��������
    bool betweenIsEmpty(int sx, int sy, int ex, int ey);
    int betweenNotEmptyNum(int sx, int sy, int ex, int ey);
    static bool inSameLine(int sx, int sy, int ex, int ey);
    static bool inSameStraightLine(int sx, int sy, int ex, int ey);
    static bool inSameObiqueLine(int sx, int sy, int ex, int ey);
    
    // �����ж����
    bool isMyKingAttackedAfterMove(const Move& move);  // �����ⲽ�󼺷��Ͻ��Ƿ񱻽��������ͽ���
    bool isOppKingAttackedAfterMove(const Move& move); // �����ⲽ��Է��Ͻ��Ƿ񱻽���
    bool currKingAttacked(const Move& move);
    bool winAfterMove(const Move& move); // �ж��Ƿ��ɱ
    
    bool exceedMaxPeaceState(); // �Ƿ�ﵽ�����������к�����
    
    // 1D������2D���껥��ת��
    static int xy2pos(int mx, int my);
    static int pos2x(int mpos);
    static int pos2y(int mpos);
    


    //ԭ��ȡ��Ϣ�ӿ���private�ģ���Ҫ��ȡ��Ϣ����public�Ľӿ�
    void printBoard();
    // ��ȡ��ǰ������ָ���������Ϣ
    Grid getGrid(int x, int y) const {
        return state[currTurnId][xy2pos(x, y)];
    }
    // ��ȡ��ǰ���巽��ɫ
    colorType getCurrColor() const {
        return currColor;
    }
    //


};

void getInputBotzone(Chess& chess);
void giveOutputBotzone(Chess& chess);

int main() {
    Chess chess;
    getInputBotzone(chess);  // ��ȡƽ̨���벢�ָ�����
    giveOutputBotzone(chess); // ����AI���߲����
    return 0;
}

// ��� AI �ľ��߸�ƽ̨
void giveOutputBotzone(Chess& chess) {
    vector<Move> retMoves;
    // ��ȡ��ǰ�������кϷ����߲�
    chess.generateMovesWithForbidden(retMoves);
    Json::Value ret;

    if (retMoves.empty()) {
        // ��·���ߣ������������У��������׳� -1
        ret["response"]["source"] = string("-1");
        ret["response"]["target"] = string("-1");
    } else {
        // =====================================================================
        // ���沿��Ϊ���ʾ�����Ż��㷨�����·�����
        // =====================================================================
        srand(time(nullptr));
        int choice = rand() % retMoves.size();
        auto selMove = retMoves[choice]; 
        
        // ���ڲ�����������ת��Ϊ PGN �ַ�����ʽ
        ret["response"]["source"] = string(1, pgnint2char(selMove.source_x)) + string(1, int2char(selMove.source_y));
        ret["response"]["target"] = string(1, pgnint2char(selMove.target_x)) + string(1, int2char(selMove.target_y));
    }
    // תΪ JSON ��ʽ����ӡ����׼���
    Json::FastWriter writer;
    cout << writer.write(ret) << endl;
}

// ��������ƽ̨�����룬�ָ�����״̬
void getInputBotzone(Chess& chess) {
    string str;
    getline(cin, str); // �ӱ�׼�����ȡ JSON �ַ���
    Json::Reader reader;
    Json::Value input;
    reader.parse(str, input);
    
    // ������ʷ�غ���
    int turnID = input["responses"].size();
    
    // �����һ�غ��յ��������"-1", ˵�����Ǻ췽����
    colorType currBotColor =
            input["requests"][(Json::Value::UInt) 0]["source"].asString() == "-1" ?  RED : BLACK;
            
    string curSource, curTarget;
    
    // ������ʷ�غϼ�¼
    for (int i = 0; i < turnID; i++)
    {
        // �ָ����ֵ���ʷ�߲�
        curSource = input["requests"][i]["source"].asString();
        curTarget = input["requests"][i]["target"].asString();
        if (curSource != "-1") {
            Move curMove(curSource, curTarget);
            if (!chess.isMoveValidWithForbidden(curMove)) {
                throw std::runtime_error("input is not valid!");
            }
            chess.makeMoveAssumeLegal(curMove); // �������ڲ�����
        }

        // �ָ���������ʷ�߲�
        curSource = input["responses"][i]["source"].asString();
        curTarget = input["responses"][i]["target"].asString();
        Move curMove(curSource, curTarget);
        if (!chess.isMoveValidWithForbidden(curMove)) {
            throw std::runtime_error("input is not valid!");
        }
        chess.makeMoveAssumeLegal(curMove);
    }

    // ���ն��ָո��ߵ�һ��
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

// ��ʼ�����̣���׼�й����忪��վλ��
void Chess::resetBoard() {
    vector<Grid> currState;
    Grid boardInfo[BOARDWIDTH][BOARDHEIGHT];
    // ȫ����ʼ��Ϊ��
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            boardInfo[x][y] = Grid(None, EMPTY);
        }
    }

    // �ڷź췽
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

    // �ڷźڷ�
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

    // ���л�������ʷ״̬��һ��
    for (int mpos = 0; mpos < BOARDWIDTH * BOARDHEIGHT; mpos++) {
        currState.push_back(boardInfo[pos2x(mpos)][pos2y(mpos)]);
    }
    state.clear();
    state.push_back(currState);
}

// ��ʼ��״̬
Chess::Chess() {
    resetBoard();
    continuePeaceTurn.clear();
    lastMoveEaten.clear();
    continuePeaceTurn.push_back(0);
    lastMoveEaten.push_back(false);
    currColor = RED; // ����
    currTurnId = 0;
}

// ���ڹ������ɵ�ǰ�����еĺϷ��߷�
void Chess::generateMoves(vector<Move> &legalMoves, bool mustDefend) {
    legalMoves.clear();
    // ������������Ѱ�Ҽ���������
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == currColor) {
                switch (curGrid.type) {
                    case King: { // ��/˧
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            if (!inKingArea(tx, ty, curGrid.color)) { // �����ھŹ�����
                                continue;
                            }
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            if (tGrid.color == currColor) // Ŀ����Ӳ������Լ�������
                                continue;
                            if (mustDefend) {
                                // ������أ����������һ���Ͻ������������ͽ�����������
                                if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            } else {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                        break;
                    }
                    case Assistant: { // ʿ/��
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_ob[dir], ty = y + dy_ob[dir];
                            if (!inKingArea(tx, ty, curGrid.color)) { // �����ھŹ�����б��
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
                    case Pawn: { // ��/��
                        int forwardOne = (curGrid.color == RED ? 1 : -1); // ��ֻ��ǰ��
                        bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5)); // �Ƿ��ѹ���
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
                        if (crossMidLine) { // ��������ˣ���������ƽ��
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
                    case Knight: { // ��
                        for (int dir = 0; dir < 8; dir++) {
                            int tx = x + dx_knight[dir], ty = y + dy_knight[dir];
                            if (!inBoard(tx, ty)) {
                                continue;
                            }
                            auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                            // �����ȼ�飬��ȡ��ǰ�������������
                            int fx = x + dx_knight_foot[dir], fy = y + dy_knight_foot[dir];
                            auto& fGrid = state[currTurnId][xy2pos(fx, fy)];
                            
                            // ���Ŀ������Լ��ˣ���������λ��������(��Ϊ��)���򲻿���
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
                    case Bishop: { // ��/��
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_bishop[dir], ty = y + dy_bishop[dir];
                            // ���ܹ���
                            if (!inBoard(tx, ty) || !inColorArea(tx, ty, currColor)) {
                                continue;
                            }
                            // ��������Ƿ���ס
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
                    case Rook: { // ��
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            while (inBoard(tx, ty)) {
                                auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                                if (tGrid.color == currColor) {
                                    break; // �����Լ��ˣ����·��
                                } else if (tGrid.color == oppColor()) {
                                    // �������ˣ����ԳԵ���ͣס
                                    if (mustDefend) {
                                        if (!isMyKingAttackedAfterMove(Move(x,y,tx,ty))) {
                                            legalMoves.emplace_back(x, y, tx, ty);
                                        }
                                    } else {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                    break;
                                } else {
                                    // �յأ�������
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
                    case Cannon: { // ��
                        for (int dir = 0; dir < 4; dir++) {
                            int tx = x + dx_strai[dir];
                            int ty = y + dy_strai[dir];
                            while (inBoard(tx, ty)) {
                                // ͳ������Ŀ���֮���ж��ٸ�����
                                int betweenNum = betweenNotEmptyNum(x, y, tx, ty);
                                if (betweenNum > 1) {
                                    break; // �������������ϵ����ӣ������ߣ����
                                } else if (betweenNum == 1) {
                                    // �м���1���ڼ�
                                    auto& tGrid = state[currTurnId][xy2pos(tx, ty)];
                                    // �ڳ��ӱ���Ŀ�����ео��ĸ���
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
                                    // �м������ӣ���ͨ�ƶ�
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

// �ж������Ƿ�Խ��
bool Chess::inBoard(int mx, int my) {
    return mx >= 0 && mx < BOARDWIDTH && my >= 0 && my < BOARDHEIGHT;
}

// ��ȡ���ֵ���ɫ
colorType Chess::oppColor() {
    return currColor == BLACK ? RED : BLACK;
}

// ���ĳ�� Move �Ƿ��ںϷ����б���
bool Chess::isLegalMove(const Move &move, bool mustDefend) {
    vector<Move> currLegalMoves;
    generateMoves(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

// ִ��һ���壨�����ⲽ��Ϸ���
bool Chess::makeMoveAssumeLegal(const Move &move) {
    int nextPeaceTrun =  continuePeaceTurn[currTurnId] + 1;
    // ���Ƶ�ǰ״̬��Ϊ�µ�һ��״̬
    vector<Grid> copyState = state[currTurnId];
    state.push_back(copyState);
    currTurnId++;

    auto& sourceGrid = state[currTurnId][xy2pos(move.source_x, move.source_y)];
    auto& targetGrid = state[currTurnId][xy2pos(move.target_x, move.target_y)];
    
    // ������ӣ�������Ȼ���Żغ���
    if (targetGrid.color == oppColor()) {
        nextPeaceTrun = 0;
        lastMoveEaten.push_back(true);
    } else {
        lastMoveEaten.push_back(false);
    }
    
    // ����Ŀ���״̬���ƶ���ȥ��
    targetGrid.type = sourceGrid.type;
    targetGrid.color = sourceGrid.color;
    assert(sourceGrid.color == currColor);

    // �����ʼ��״̬
    sourceGrid.color = EMPTY;
    sourceGrid.type = None;

    continuePeaceTurn.push_back(nextPeaceTrun);
    // �л��ֻغϷ�
    currColor = oppColor();
    return true;
}

// �������Ϊ (mx, my) �ĸ����Ƿ��ܵ� color ���Ĺ���
bool Chess::attacked(colorType color, int mx, int my) {
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color != color) {
                continue;
            }
            int dex = mx - x, dey = my - y;
            switch (curGrid.type) {
                case King: { // �����������Ͻ�������Ͻ���
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
                case Knight: { // ���Ĺ������
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
                case Bishop: { // ��Ĺ������
                    if (abs(dex) == 2 && abs(dey) == 2) {
                        int ex = x + dex / 2, ey = y + dey / 2;
                        auto& eGrid = state[currTurnId][xy2pos(ex, ey)];
                        if (eGrid.color == EMPTY) {
                            return true;
                        }
                    }
                    break;
                }
                case Rook: { // ���Ĺ������
                    if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my)) {
                        return true;
                    }
                    break;
                }
                case Pawn: { // ���Ĺ������
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
                case Cannon: { // �ڵĹ�����⣨�����һ�����ӣ�
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

// ������������м��Ƿ�Ϊ�գ�������ֹ�㣩
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

// ����Ƿ���ͬ��/ͬ��/ͬб��
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

// ����ĳһ��������֮�󣬼����Ͻ��Ƿ���Ȼ���ڱ�����״̬
bool Chess::isMyKingAttackedAfterMove(const Move &move) {
    colorType originColor = currColor;
    // ��������һ��
    makeMoveAssumeLegal(move);
    int kingX = -1, kingY = -1;
    // Ѱ�Ҽ����Ͻ�
    for (int x = 0; x < BOARDWIDTH; x++) {
        for (int y = 0; y < BOARDHEIGHT; y++) {
            auto& curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == originColor && curGrid.type == King) {
                kingX = x; kingY = y;
                break;
            }
        }
    }
    // ����Ƿ񱻹���
    bool myKingStillAttacked = attacked(oppColor(originColor), kingX, kingY);
    // ���볷��״̬
    undoMove();
    assert(currColor == originColor);
    return myKingStillAttacked;
}

bool Chess::isMoveValid(const Move& move, bool mustDefend) {
    return isLegalMove(move, mustDefend);
}

// ���������һ���Ƿ���Ӯ
bool Chess::winAfterMove(const Move &move) {
    makeMoveAssumeLegal(move);
    vector<Move> oppLegalMoves;
    generateMoves(oppLegalMoves);
    undoMove();
    return oppLegalMoves.empty(); // �����޲������ҷ�ʤ��
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

// ����60���޳����к�
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

// ͳ�������յ������м�����������������������⣩
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

// ������һ������
void Chess::undoMove() {
    if (currTurnId == 0) {
        return;
    }
    state.pop_back(); // ������״̬����һ��
    currTurnId--;
    currColor = oppColor();
    continuePeaceTurn.pop_back();
    lastMoveEaten.pop_back();
}

// �������ء���ֹ����/��׽���ظ����桱�ĺϷ���
void Chess::generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend) {
    vector<Move> firstMoves;
    generateMoves(firstMoves, mustDefend);
    legalMoves.clear();
    for (const auto& move : firstMoves) {
        // ����˲�û�е����ظ���ѭ���������Ϸ��б�
        if (!repeatAfterMove(move)) {
            legalMoves.push_back(move);
        }
    }
}

// ���ĳ�����Ƿ�ᵼ�������ظ����棨��������׽ͨ���ڴ˴����أ�
bool Chess::repeatAfterMove(const Move& move) {
    makeMoveAssumeLegal(move);
    int tmp_id = currTurnId;
    // ����г��ӷ��������治����ת�����������ظ���ֱ�ӷ��� false
    if (lastMoveEaten[tmp_id]) {
        undoMove();
        return false;
    }
    
    // ��ǰ׷��ֱ���ϴγ��ӻ��߿���
    while (true) {
        if (tmp_id <= 1) {
            break;
        }
        if (lastMoveEaten[tmp_id-2]) {
            break;
        }
        tmp_id -= 2; // ֻ���÷�����ľֺ�
    }
    
    // ͳ��������ȫһ���Ĵ���
    int repeatTime = 0;
    for (int id = currTurnId; id >= tmp_id; id -= 2) {
        if (state[id] == state[currTurnId]) {
            repeatTime++;
        }
    }
    undoMove();
    return repeatTime >= 3; // �ﵽ�򳬹�3�Σ�����Υ���ظ�����
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



//����Ϊ����ʵ�ֵ��㷨

//1.ԭ�����Ѿ�ʵ���˺Ϸ��������ɣ�����ֻ��Ҫ����void generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend=true) ��ȡ�Ϸ����в�
//2.�������㷨��ģ����һ�������bool makeMoveAssumeLegal(const Move& move)
//3.�������㷨�г���ģ����岽����void undoMove()


// ��������
int evaluateBoard(const Chess& chess)
{

}


// alpha-beta ����
int alphaBetaSearch()
{

}