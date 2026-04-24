#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <cassert>
#include <algorithm>
#include <vector>
#include <stdexcept>
#include <time.h>
#include "jsoncpp/json.h"
#include <queue>
#include <utility>
#include <cstdint>
#include <random>
#include"TuningParams.h"

using std::uint64_t;   //64位hash
using std::cin;
using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;
using std::pair;

#define BOARDWIDTH 9   // 棋盘宽度
#define BOARDHEIGHT 10 // 棋盘高度
enum stoneType
{
    None = 0,
    King = 1,
    Bishop = 2,
    Knight = 3,
    Rook = 4,
    Pawn = 5,
    Cannon = 6,
    Assistant = 7
}; // 棋子类型枚举
enum colorType
{
    BLACK = 0,
    RED = 1,
    EMPTY = 2
};

enum HashFlag{
    EXACT = 0, //准确值
    ALPHA = 1, //上边界为Alpha(alpha型结点，评估分数很差)
    BETA = 2   //下边界为Beta(beta型结点，评估分数很好)
};

// 阵营颜色枚举
int baseLine[2] = {9, 0};                                                                                                                                                                                           // 红黑双方的底线Y坐标
char stoneSym[] = "*kbnrpca";                                                                                                                                                                                       // 棋子类型对应的字符表示
map<char, stoneType> charSym = {{stoneSym[0], None}, {stoneSym[1], King}, {stoneSym[2], Bishop}, {stoneSym[3], Knight}, {stoneSym[4], Rook}, {stoneSym[5], Pawn}, {stoneSym[6], Cannon}, {stoneSym[7], Assistant}}; // 字符到棋子类型的映射枚举

//Zobirst随机数表,在程序启动时启动一次
//压缩存储 ———— 使用Zobrist哈希，用一个64位的整数就能唯一地代表一个复杂的象棋局面，记录棋盘上的状态。
//局势一模一样，此时着棋方不同 ———— (ZobristTable,ZobristPlayer)可以唯一确定一个棋局！
uint64_t ZobristTable[8][2][90];                   //静态只读数组，用于Zobrist运算 类型 颜色(0-RED 1-BLACK) 位置xy2pos(x,y),第y列，第x行
uint64_t ZobristPlayer;                            //静态只读数，用于标识当前轮到谁走棋的随机数
int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};            // 8个方向的X轴偏移（通用）
int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};            // 8个方向的Y轴偏移（通用）
int dx_ob[] = {-1, 1, -1, 1};                      // 士（仕）的斜向X轴偏移（4个对角方向）
int dy_ob[] = {-1, -1, 1, 1};                      // 士（仕）的斜向Y轴偏移（4个对角方向）
int dx_strai[] = {-1, 0, 0, 1};                    // 直线的X轴偏移（上下左右）
int dy_strai[] = {0, -1, 1, 0};                    // 直线的Y轴偏移（上下左右）
int dx_lr[] = {-1, 1};                             // 兵（卒）过河后的左右X轴偏移
int dx_knight[] = {-2, -2, -1, -1, 1, 1, 2, 2};    // 马走“日”目的地的X轴偏移
int dy_knight[] = {-1, 1, -2, 2, -2, 2, -1, 1};    // 马走“日”目的地的Y轴偏移
int dx_knight_foot[] = {-1, -1, 0, 0, 0, 0, 1, 1}; // 马走“日”对应的蹩马腿检查点X轴偏移
int dy_knight_foot[] = {0, 0, -1, 1, -1, 1, 0, 0}; // 马走“日”对应的蹩马腿检查点Y轴偏移
int dx_bishop[] = {-2, -2, 2, 2};                  // 象（相）走“田”目的地的X轴偏移
int dy_bishop[] = {-2, 2, -2, 2};                  // 象（相）走“田”目的地的Y轴偏移
int dx_bishop_eye[] = {-1, -1, 1, 1};              // 象（相）走“田”对应的塞象眼检查点X轴偏移
int dy_bishop_eye[] = {-1, 1, -1, 1};              // 象（相）走“田”对应的塞象眼检查点Y轴偏移

//类似于算法题目中的记忆化搜索 ———— 将先前搜索得到的节点存入置换表
//初始化Zobrist随机数表和Zobrist 轮次标识
void InitZobrist(){
    std::mt19937_64 randomnum(time(0));

    for(int type = 0;type < 8;type ++){
        for(int color = 0;color < 2;color ++){
            for(int pos = 0;pos < 90;pos ++){
                ZobristTable[type][color][pos] = randomnum();
            }
        }
    }
    ZobristPlayer = randomnum();
}

// PGN字符横坐标转数值
int pgnchar2int(char c)
{
    return (int)c - (int)'a';
}

// 数值转PGN字符横坐标
char pgnint2char(int i)
{
    return (char)((int)'a' + i);
}

// 字符数字转整型数值
int char2int(char c)
{
    return (int)c - (int)'0';
}

// 整型数值转字符数字
char int2char(int i)
{
    return (char)((int)'0' + i);
}

class TimeOutException : public std::runtime_error
{
public:
    TimeOutException() : std::runtime_error("Time limit exceeded") {}
};

class SearchTime
{
private:
    clock_t startTime;
    int timeLimit_ms; // 毫秒
    int checkerCount;

public:
    SearchTime(int limit_ms = 900) : startTime(0), timeLimit_ms(limit_ms), checkerCount(0) {}

    void setLimitMs(int limit_ms)
    {
        timeLimit_ms = limit_ms;
    }

    void start()
    {
        startTime = clock();
        checkerCount = 0;
    }

    inline void check()
    {
        if (checkerCount++ % 1024 == 0) // 每1024次检查一次时间
        {
            clock_t currentTime = clock();
            double durationTime_ms = double(currentTime - startTime) / CLOCKS_PER_SEC * 1000;
            if (durationTime_ms > timeLimit_ms)
            {
                throw TimeOutException();
            }
        }
    }
};

struct Move
{
    int source_x, source_y, target_x, target_y;
    Move()
    {
        source_x = -1;
        source_y = -1;
        target_x = -1;
        target_y = -1;
    }
    Move(int msx, int msy, int mtx, int mty) : source_x(msx), source_y(msy),
                                               target_x(mtx), target_y(mty) { ; }
    Move(string msource, string mtarget)
    {
        source_x = pgnchar2int(msource[0]);
        source_y = char2int(msource[1]);
        target_x = pgnchar2int(mtarget[0]);
        target_y = char2int(mtarget[1]);
    }
    bool operator==(const Move &other) const
    {
        return source_x == other.source_x && source_y == other.source_y && target_x == other.target_x && target_y == other.target_y;
    }
}; // 描述一步行棋（起点和终点坐标）的数据结构

//？？？如何再考虑使用？
// MoveNode，根据当前的着棋算出相应的收益，同时可增添其它的数据属性 博弈树上的局面应当是局面节点构成的树，根据局面做估值
//数学理论上，是根据状态构建博弈树，实际上使用着法节点构成博弈树 —— 避免程序过大的开销
struct MoveNode
{
    Move strategy;
    int value = -1; //初始值为-1
    bool operator==(const MoveNode &other)
    {
        return (this->strategy) == (other.strategy);
    }
};

struct Grid
{
    stoneType type;
    colorType color;
    Grid() : type(None), color(EMPTY) { ; }
    Grid(stoneType mtype, colorType mcolor) : type(mtype), color(mcolor) { ; }
    Grid &operator=(const Grid &other)
    {
        if (this != &other)
        {
            type = other.type;
            color = other.color;
        }
        return *this;
    }
    bool operator==(const Grid &other) const
    {
        return type == other.type && color == other.color;
    }
}; // 描述棋盘上单个格子（当前包含的棋子类型和颜色）的数据结构

//TTStateNode为置换表单条数据,置换表存储经过Zobirst计算的 的棋盘数据
struct TTStateNode{
    uint64_t zobrist_key;    //zobrist计算所得局面值
    int depth;               //搜素深度
    int score;               //评估分数
    HashFlag flag;           //节点标志
    Move best_move;          //最佳着法
};

//置换表本身
const int TT_SIZE = 1048576; //1M个
TTStateNode HashTable[TT_SIZE];


bool probeHash(uint64_t key,int depth,int alpha,int beta,int &val,Move& bestMove){
    TTStateNode& entry = HashTable[key & (TT_SIZE - 1)];

    if(entry.zobrist_key == key){ //索引相同，但是棋盘不同，本key下已经存在棋盘状态存储，两个状态所在博弈树的深度不同
        bestMove = entry.best_move;
        
        //记录的深度 >= 当前需要的深度，分数可信
        if(entry.depth >= depth){
            if(entry.flag == EXACT){
                val = entry.score;
                return true;
            }
            if(entry.flag == ALPHA && entry.score <= alpha){
                val = alpha;
                return true;     //alpha型的结点，上边界ALPHA比我当前alpha还差，剪枝
            }
            if(entry.flag == BETA && entry.score >= beta){
                val = beta;
                return true;     //同上，beta型的结点，下边界BETA比我当前还好，剪枝
            }
        }

    }
    return false;
}

//写入置换表
void RecordHash(uint64_t key,int depth,int score,HashFlag flag,Move bestMove){
    TTStateNode& entry = HashTable[key & (TT_SIZE - 1)];
    //算法：优先保留更深的搜素结果
    //entry = HashTable[key]，如果entry.zobrist_key != key,说明没有初始化
    if(entry.zobrist_key != key || depth >= entry.depth){
        entry.zobrist_key = key;
        entry.depth = depth;
        entry.score = score;
        entry.flag = flag;
        entry.best_move = bestMove;
    } 
}
class Chess
{
    friend class ChessAI; // ChessAI拥有调用Chess所有方法和属性的权限！
    friend class EvalRecord; 
private:
    std::vector<std::vector<Grid>> state;
    int currTurnId;
    colorType currColor;
    vector<int> continuePeaceTurn;
    vector<int> lastMoveEaten; //历史每一步是否发生了吃子 0或1
public:
    Chess();                                                                           // 构造函数，初始化棋盘
    void resetBoard();                                                                 // 重置棋盘为初始状态
    void generateMoves(vector<Move> &legalMoves, bool mustDefend = true);              // 生成常规合法移动步
    void generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend = true); // 生成包含禁手规则的合法移动步
    void generateMoveNodesWithForbidden(vector<MoveNode> &legalNodes, bool mustDefend = true);  //新增————生成合法MoveNode
    bool repeatAfterMove(const Move &move);                                            // 判断移动后是否出现长将/长捉等重复局面
    static bool inBoard(int mx, int my);                                               // 判断坐标是否在棋盘内
    static bool inKingArea(int mx, int my, colorType mcolor);                          // 判断是否在九宫格内
    static bool inColorArea(int mx, int my, colorType mcolor);                         // 判断是否在己方半场
    colorType oppColor();                                                              // 获取对手颜色
    colorType oppColor(colorType mcolor);                                              // 获取指定颜色的对手颜色
    bool isMoveValid(const Move &move, bool mustDefend = true);                        // 验证移动是否有效
    bool isLegalMove(const Move &move, bool mustDefend = true);                        // 判断是否为合法移动
    bool isLegalMoveWithForbidden(const Move &move, bool mustDefend = true);           // 判断包含禁手规则下是否合法
    bool isMoveValidWithForbidden(const Move &move, bool mustDefend = true);           // 验证包含禁手规则下的移动
    bool makeMoveAssumeLegal(const Move &move);                                        // 假设合法并执行移动，更新状态
    bool attacked(colorType color, int mx, int my);                                    // 检查己方将/帅是否被某方颜色攻击
    bool betweenIsEmpty(int sx, int sy, int ex, int ey);                               // 判断两点之间是否没有棋子
    int betweenNotEmptyNum(int sx, int sy, int ex, int ey);                            // 计算两点之间非空棋子的数量
    static bool inSameLine(int sx, int sy, int ex, int ey);                            // 判断两点是否在同一直线或对角线
    static bool inSameStraightLine(int sx, int sy, int ex, int ey);                    // 判断两点是否在同行或同列
    static bool inSameObiqueLine(int sx, int sy, int ex, int ey);                      // 判断两点是否在同斜线
    bool isMyKingAttackedAfterMove(const Move &move);                                  // 判断移动后己方主帅是否被将军
    bool isOppKingAttackedAfterMove(const Move &move);                                 // 判断移动后是否将对手的军
    bool currKingAttacked(const Move &move);                                           // 判断当前主帅是否正在被攻击
    bool winAfterMove(const Move &move);                                               // 判断该移动是否能直接获胜
    void undoMove();                                                                   // 撤销上一步移动
    bool exceedMaxPeaceState();                                                        // 判断是否超过最大求和/平局步数
    static int xy2pos(int mx, int my);                                                 // 二维坐标转一维索引
    static int pos2x(int mpos);                                                        // 一维索引转X坐标
    static int pos2y(int mpos);                                                        // 一维索引转Y坐标
    void printBoard();                                                                 // 打印当前棋盘

    bool isattacked(colorType color, int mx, int my);

    //Zobrist Key计算方法 ———— 添加到Chess类中
    uint64_t getZobristKey();
};

//局面评估类 包含评估的各个指标量(特征提取)和评估方法 
class EvalRecord{
public:
    Chess *board; //评估类含有的Chess指针，用于调用Chess中的方法
    void copyBoard(Chess *currentBoard) { board = currentBoard; } //

    int GridControl[2][90];  //红 / 黑方在9*10棋盘上控制的 点数(红:0 黑:1)
    bool havecontrol[90];    //所控制的点上，有对方的棋子为true

    bool isKingattacked;
    vector<Grid> curstones;
    vector<bool> curattacked;
    
    bool isattacked(colorType color, int mx, int my);                          //(对方的棋子颜色，我方的棋子位置x y) 某一方方位于mx my点的棋子是否可能被吃
    bool hasattackchance(int mx,int my);                                       //当前位于x y处的棋子是否有吃子的可能
    bool ismiddlecompostion(vector<Grid>& target);                             //是否为中局 / 残局
    int calstaticstate(vector<Grid> &target, colorType color);                 //根据当前局面(Grid)，得到当前静态局面评分
    int caldynamicstate(vector<Grid> &target, colorType color);                //根据当前局面(Grid), 得到当前动态局面评分
    int calstate(vector<Grid>& target,colorType color);                        //计算出当前局面的总得分
    void getGridcontrol();                                                     //填充控制点字典，为了保证效率，一次遍历，进行多种"特征选择"，收集多种数据。(虽然会破坏耦合性)
    int calcurmove(Move& curmove,vector<Grid>& curstate,colorType color);      //(State[cur],Move[cur]) --> State[after] : score

    
    EvalRecord() : board(nullptr), isKingattacked(false) {}
};

//产生策略类
class ChessAI
{
public:
    // ChessAI属性 —— 由棋局状态 、 评估结构体组成
    Chess *board; //储存现有 棋局状态类(着法生成和状态显示类)
    EvalRecord evaluator;       // 独立负责局面打分的评估器,能够返回各个结果
    
    vector<MoveNode> curlegalmoves; //由chess产生，当前合法的状态
    vector<MoveNode> eatenmoves;    //"死手"——产生吃子的着棋    ———————— 这里可能需要再次打分~
    vector<MoveNode> plainmoves;    //"白手"——不产生吃子的着棋
    vector<MoveNode> select;      //经过处理后的Move方法
    SearchTime searchTime;          // 搜索计时器

    // ChessAI方法
    bool getWin();                                                            //更新isWin,判断是否胜利
    void getcurstones(void);
    void generateMovesWithForbidden();                                        //对应board的同名方法
    void getdiffmoves();                                                      //得到吃子 / 非吃子 着棋
    int alpha_beta_search();                                                  //alpha-beta剪枝，获得最佳得分
    void fillinMoveNodes(vector<MoveNode>& nodes);                            //将博弈树的MoveNodes节点进行赋值，并排序
    void fill_sort_inMoveNodes(vector<MoveNode>& nodes,vector<Move>& target); //对某层的MoveNodes节点从target填充并排序
    void MoveNodesort(vector<MoveNode>& nodes);                               //着法排序，考虑使用priority_queue<Move,vector<Move(),cmp>
    void MoveNodeVoid();                                                      //当前的棋步为空着
    void MoveNodeFront(MoveNode& tmpmove);                                    //(用于博弈树的向下遍历) ———— 进行当前的着法 
    //MoveNode的值为delta值，还原回上一棋着的值减去delta即可
    void MoveNodeBack(MoveNode& tmpmove);                                     //(用于博弈树的回溯) ———— 撤销当前的着法
    
    //将搜索入口和搜索体相互剥离
    void alpha_beta_select(vector<MoveNode>& selected);                       //无参alpha_beta 基本alpha_beta搜索入口(初始化和边界条件)
    int alpha_beta_search(int depth,int alpha,int beta);                      //基本alpha_beta搜素函数
    void alpha_beta_PVS_select(vector<MoveNode>& selected);                   //alpha_beta_PMV 搜索入口
    int alpha_beta_PVS_search(int depth,int alpha,int beta);                  //alpha_beta_PMV 搜索函数
    bool haveattack(vector<Grid>& tmpstate,colorType color);                  //我方棋子是否还存在“攻击”
    bool havethreat(vector<Grid>& tmpstate,colorType color);                  //我方棋子是否还存在"威胁"
    bool havethreat_of_valuestone(vector<Grid>& tmpstate,colorType color);    //我方重要的棋子————"将/帅"、车马炮
    bool haveattack_of_valuestone(vector<Grid>& tmpstate,colorType color);    //我方重要的棋子————"将/帅"、车马炮
    bool isQuietMove(vector<Grid>& tmpstate,MoveNode& tmpmove);               //我方的该步，是否为"白手" ———— 我方无吃子机会也无威胁，用于剪枝大量无意义着法

    int ChessAI::quiescence_search(int alpha, int beta) ;                      //静态搜索，解决水平线效应

    ChessAI();
    ~ChessAI();
};
void getInputBotzone(Chess &chess);   // 处理Botzone平台输入
void giveOutputBotzone(Chess &chess); // 向Botzone平台输出落子

int main()
{
    Chess chess;
    getInputBotzone(chess);
    giveOutputBotzone(chess);
    return 0;
}

// 向Botzone平台输出落子的函数定义
void giveOutputBotzone(Chess &chess)
{
    vector<Move> retMoves;
    chess.generateMovesWithForbidden(retMoves);
    Json::Value ret;

    if (retMoves.empty())
    {
        ret["response"]["source"] = string("-1");
        ret["response"]["target"] = string("-1");
    }
    else
    {
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
void getInputBotzone(Chess &chess)
{
    string str;
    getline(cin, str);
    Json::Reader reader;
    Json::Value input;
    reader.parse(str, input);
    int turnID = input["responses"].size();
    // 第一回合收到的起点是"-1", 说明我是红方
    colorType currBotColor =
        input["requests"][(Json::Value::UInt)0]["source"].asString() == "-1" ? RED : BLACK;
    string curSource, curTarget;
    for (int i = 0; i < turnID; i++)
    {
        // 根据这些输入输出逐渐恢复状态到当前回合
        curSource = input["requests"][i]["source"].asString();
        curTarget = input["requests"][i]["target"].asString();
        if (curSource != "-1")
        {
            Move curMove(curSource, curTarget);
            if (!chess.isMoveValidWithForbidden(curMove))
            {
                throw std::runtime_error("input is not valid!");
            }
            chess.makeMoveAssumeLegal(curMove);
        }

        curSource = input["responses"][i]["source"].asString();
        curTarget = input["responses"][i]["target"].asString();
        Move curMove(curSource, curTarget);
        if (!chess.isMoveValidWithForbidden(curMove))
        {
            throw std::runtime_error("input is not valid!");
        }
        chess.makeMoveAssumeLegal(curMove);
    }

    curSource = input["requests"][turnID]["source"].asString();
    curTarget = input["requests"][turnID]["target"].asString();
    if (curSource != "-1")
    {
        Move curMove(curSource, curTarget);
        if (!chess.isMoveValidWithForbidden(curMove))
        {
            throw std::runtime_error("input is not valid!");
        }
        chess.makeMoveAssumeLegal(curMove);
    }
}

// 重置棋盘状态为初始状态的函数定义
void Chess::resetBoard()
{
    vector<Grid> currState;
    Grid boardInfo[BOARDWIDTH][BOARDHEIGHT];
    for (int x = 0; x < BOARDWIDTH; x++)
    {
        for (int y = 0; y < BOARDHEIGHT; y++)
        {
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

    for (int mpos = 0; mpos < BOARDWIDTH * BOARDHEIGHT; mpos++)
    {
        currState.push_back(boardInfo[pos2x(mpos)][pos2y(mpos)]);
    }
    state.clear();
    state.push_back(currState);
}

// Chess类构造函数的定义
Chess::Chess()
{
    resetBoard();
    continuePeaceTurn.clear();
    lastMoveEaten.clear();
    continuePeaceTurn.push_back(0);
    lastMoveEaten.push_back(false);
    currColor = RED;
    currTurnId = 0;
}

// 生成当前所有伪合法或合法步的函数定义
void Chess::generateMoves(vector<Move> &legalMoves, bool mustDefend)
{
    legalMoves.clear();
    for (int x = 0; x < BOARDWIDTH; x++)
    {
        for (int y = 0; y < BOARDHEIGHT; y++)
        {
            auto &curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == currColor)
            {
                switch (curGrid.type)
                {
                case King:
                {
                    for (int dir = 0; dir < 4; dir++)
                    {
                        int tx = x + dx_strai[dir];
                        int ty = y + dy_strai[dir];
                        if (!inKingArea(tx, ty, curGrid.color))
                        {
                            continue;
                        }
                        auto &tGrid = state[currTurnId][xy2pos(tx, ty)];
                        if (tGrid.color == currColor)
                            continue;
                        if (mustDefend)
                        {
                            if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty)))
                            {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                        else
                        {
                            legalMoves.emplace_back(x, y, tx, ty);
                        }
                    }
                    break;
                }
                case Assistant:
                {
                    for (int dir = 0; dir < 4; dir++)
                    {
                        int tx = x + dx_ob[dir], ty = y + dy_ob[dir];
                        if (!inKingArea(tx, ty, curGrid.color))
                        {
                            continue;
                        }
                        auto &tGrid = state[currTurnId][xy2pos(tx, ty)];
                        if (tGrid.color == currColor)
                        {
                            continue;
                        }
                        if (mustDefend)
                        {
                            if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty)))
                            {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                        else
                        {
                            legalMoves.emplace_back(x, y, tx, ty);
                        }
                    }
                    break;
                }
                case Pawn:
                {
                    int forwardOne = (curGrid.color == RED ? 1 : -1);
                    bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5));
                    int tx = x, ty = y + forwardOne;
                    if (!inBoard(tx, ty))
                    {
                        continue;
                    }
                    auto &tGrid = state[currTurnId][xy2pos(tx, ty)];
                    if (tGrid.color == currColor)
                    {
                        continue;
                    }
                    if (mustDefend)
                    {
                        if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty)))
                        {
                            legalMoves.emplace_back(x, y, tx, ty);
                        }
                    }
                    else
                    {
                        legalMoves.emplace_back(x, y, tx, ty);
                    }
                    if (crossMidLine)
                    {
                        for (int dir = 0; dir < 2; dir++)
                        {
                            int ox = x + dx_lr[dir], oy = y;
                            if (!inBoard(ox, oy))
                            {
                                continue;
                            }
                            auto &oGrid = state[currTurnId][xy2pos(ox, oy)];
                            if (oGrid.color == currColor)
                            {
                                continue;
                            }
                            if (mustDefend)
                            {
                                if (!isMyKingAttackedAfterMove(Move(x, y, ox, oy)))
                                {
                                    legalMoves.emplace_back(x, y, ox, oy);
                                }
                            }
                            else
                            {
                                legalMoves.emplace_back(x, y, ox, oy);
                            }
                        }
                    }
                    break;
                }
                case Knight:
                {
                    for (int dir = 0; dir < 8; dir++)
                    {
                        int tx = x + dx_knight[dir], ty = y + dy_knight[dir];
                        if (!inBoard(tx, ty))
                        {
                            continue;
                        }
                        auto &tGrid = state[currTurnId][xy2pos(tx, ty)];
                        int fx = x + dx_knight_foot[dir], fy = y + dy_knight_foot[dir];
                        auto &fGrid = state[currTurnId][xy2pos(fx, fy)];
                        if (tGrid.color == currColor || fGrid.color != EMPTY)
                        {
                            continue;
                        }
                        if (mustDefend)
                        {
                            if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty)))
                            {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                        else
                        {
                            legalMoves.emplace_back(x, y, tx, ty);
                        }
                    }
                    break;
                }
                case Bishop:
                {
                    for (int dir = 0; dir < 4; dir++)
                    {
                        int tx = x + dx_bishop[dir], ty = y + dy_bishop[dir];
                        if (!inBoard(tx, ty) || !inColorArea(tx, ty, currColor))
                        {
                            continue;
                        }
                        int ex = x + dx_bishop_eye[dir], ey = y + dy_bishop_eye[dir];
                        auto &tGrid = state[currTurnId][xy2pos(tx, ty)];
                        auto &eGrid = state[currTurnId][xy2pos(ex, ey)];
                        if (tGrid.color == currColor || eGrid.color != EMPTY)
                        {
                            continue;
                        }
                        if (mustDefend)
                        {
                            if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty)))
                            {
                                legalMoves.emplace_back(x, y, tx, ty);
                            }
                        }
                        else
                        {
                            legalMoves.emplace_back(x, y, tx, ty);
                        }
                    }
                    break;
                }
                case Rook:
                {
                    for (int dir = 0; dir < 4; dir++)
                    {
                        int tx = x + dx_strai[dir];
                        int ty = y + dy_strai[dir];
                        while (inBoard(tx, ty))
                        {
                            auto &tGrid = state[currTurnId][xy2pos(tx, ty)];
                            if (tGrid.color == currColor)
                            {
                                break;
                            }
                            else if (tGrid.color == oppColor())
                            {
                                if (mustDefend)
                                {
                                    if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty)))
                                    {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                }
                                else
                                {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                                break;
                            }
                            else
                            {
                                assert(tGrid.color == EMPTY);
                                if (mustDefend)
                                {
                                    if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty)))
                                    {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                }
                                else
                                {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            }
                            tx += dx_strai[dir];
                            ty += dy_strai[dir];
                        }
                    }
                    break;
                }
                case Cannon:
                {
                    for (int dir = 0; dir < 4; dir++)
                    {
                        int tx = x + dx_strai[dir];
                        int ty = y + dy_strai[dir];
                        while (inBoard(tx, ty))
                        {
                            int betweenNum = betweenNotEmptyNum(x, y, tx, ty);
                            if (betweenNum > 1)
                            {
                                break;
                            }
                            else if (betweenNum == 1)
                            {
                                auto &tGrid = state[currTurnId][xy2pos(tx, ty)];
                                if (tGrid.color == currColor || tGrid.color == EMPTY)
                                {
                                    tx += dx_strai[dir];
                                    ty += dy_strai[dir];
                                    continue;
                                }
                                if (mustDefend)
                                {
                                    if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty)))
                                    {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                }
                                else
                                {
                                    legalMoves.emplace_back(x, y, tx, ty);
                                }
                            }
                            else if (betweenNum == 0)
                            {
                                auto &tGrid = state[currTurnId][xy2pos(tx, ty)];
                                if (tGrid.color != EMPTY)
                                {
                                    tx += dx_strai[dir];
                                    ty += dy_strai[dir];
                                    continue;
                                }
                                if (mustDefend)
                                {
                                    if (!isMyKingAttackedAfterMove(Move(x, y, tx, ty)))
                                    {
                                        legalMoves.emplace_back(x, y, tx, ty);
                                    }
                                }
                                else
                                {
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
bool Chess::inBoard(int mx, int my)
{
    return mx >= 0 && mx < BOARDWIDTH && my >= 0 && my < BOARDHEIGHT;
}

// 返回当前行动方对手颜色的函数定义
colorType Chess::oppColor()
{
    return currColor == BLACK ? RED : BLACK;
}

// 判断给定移动是否为合法步的函数定义
bool Chess::isLegalMove(const Move &move, bool mustDefend)
{
    vector<Move> currLegalMoves;
    generateMoves(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

// 在假设移动合法的前提下执行该移动，更新棋盘状态的函数定义
bool Chess::makeMoveAssumeLegal(const Move &move)
{
    int nextPeaceTrun = continuePeaceTurn[currTurnId] + 1;
    vector<Grid> copyState = state[currTurnId];
    state.push_back(copyState);
    currTurnId++;

    auto &sourceGrid = state[currTurnId][xy2pos(move.source_x, move.source_y)];
    auto &targetGrid = state[currTurnId][xy2pos(move.target_x, move.target_y)];
    if (targetGrid.color == oppColor())
    {
        nextPeaceTrun = 0;
        lastMoveEaten.push_back(true);
    }
    else
    {
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
// 对方的颜色 自身的将/帅位置
bool Chess::attacked(colorType color, int mx, int my)
{
    for (int x = 0; x < BOARDWIDTH; x++)
    {
        for (int y = 0; y < BOARDHEIGHT; y++)
        {
            auto &curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color != color)
            {
                continue;
            }
            int dex = mx - x, dey = my - y;
            switch (curGrid.type)
            {
            case King:
            {
                if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my))
                {
                    return true;
                }
                break;
            }
            case Assistant:
            {
                if (inKingArea(mx, my, oppColor(color)))
                {
                    if (abs(dex) == 1 && abs(dey) == 1)
                    {
                        return true;
                    }
                }
                break;
            }
            case Knight:
            {
                if (abs(dex) == 2 && abs(dey) == 1)
                {
                    int fx = x + (dex > 0 ? 1 : -1), fy = y;
                    auto &fGrid = state[currTurnId][xy2pos(fx, fy)];
                    if (fGrid.color == EMPTY)
                    {
                        return true;
                    }
                }
                else if (abs(dex) == 1 && abs(dey) == 2)
                {
                    int fx = x, fy = y + (dey > 0 ? 1 : -1);
                    auto &fGrid = state[currTurnId][xy2pos(fx, fy)];
                    if (fGrid.color == EMPTY)
                    {
                        return true;
                    }
                }
                break;
            }
            case Bishop:
            {
                if (abs(dex) == 2 && abs(dey) == 2)
                {
                    int ex = x + dex / 2, ey = y + dey / 2;
                    auto &eGrid = state[currTurnId][xy2pos(ex, ey)];
                    if (eGrid.color == EMPTY)
                    {
                        return true;
                    }
                }
                break;
            }
            case Rook:
            {
                if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my))
                {
                    return true;
                }
                break;
            }
            case Pawn:
            {
                int forwardOne = (curGrid.color == RED ? 1 : -1);
                bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5));
                if (dex == 0 && dey == forwardOne)
                {
                    return true;
                }
                if (crossMidLine && (abs(dex) == 1 && dey == 0))
                {
                    return true;
                }
                break;
            }
            case Cannon:
            {
                if (inSameStraightLine(x, y, mx, my) && betweenNotEmptyNum(x, y, mx, my) == 1)
                {
                    return true;
                }
                break;
            }
            case None:
            {
                throw std::runtime_error("color is not empty but type is none");
                break;
            }
            }
        }
    }
    return false;
}

// 判断两坐标点之间是否无任何棋子的函数定义
bool Chess::betweenIsEmpty(int sx, int sy, int ex, int ey)
{
    assert(inSameStraightLine(sx, sy, ex, ey));
    int dex = ex > sx ? 1 : (ex == sx ? 0 : -1);
    int dey = ey > sy ? 1 : (ey == sy ? 0 : -1);
    int tx = sx + dex, ty = sy + dey;
    while (inBoard(tx, ty) && !(tx == ex && ty == ey))
    {
        auto &tGrid = state[currTurnId][xy2pos(tx, ty)];
        if (tGrid.color != EMPTY)
            return false;
        tx += dex;
        ty += dey;
    }
    return true;
}

// 判断两坐标点是否处于同一横线、竖线或对角线的函数定义
bool Chess::inSameLine(int sx, int sy, int ex, int ey)
{
    int dex = ex - sx, dey = ey - sy;
    return dex == dey || dex == -dey || dex == 0 || dey == 0;
}

// 判断两坐标点是否处于同一横线或竖线的函数定义
bool Chess::inSameStraightLine(int sx, int sy, int ex, int ey)
{
    int dex = ex - sx, dey = ey - sy;
    return dex == 0 || dey == 0;
}

// 判断两坐标点是否处于同一对角线的函数定义
bool Chess::inSameObiqueLine(int sx, int sy, int ex, int ey)
{
    int dex = ex - sx, dey = ey - sy;
    return dex == dey || dex == -dey;
}

// 判断执行某步移动后，己方主帅是否处于被攻击状态的函数定义
bool Chess::isMyKingAttackedAfterMove(const Move &move)
{
    colorType originColor = currColor;
    makeMoveAssumeLegal(move);
    int kingX = -1, kingY = -1;
    for (int x = 0; x < BOARDWIDTH; x++)
    {
        for (int y = 0; y < BOARDHEIGHT; y++)
        {
            auto &curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == originColor && curGrid.type == King)
            {
                kingX = x;
                kingY = y;
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
bool Chess::isMoveValid(const Move &move, bool mustDefend)
{
    return isLegalMove(move, mustDefend);
}

// 评估某步移动之后是否能造成对方无路可走(获胜)的函数定义
bool Chess::winAfterMove(const Move &move)
{
    makeMoveAssumeLegal(move);
    vector<Move> oppLegalMoves;
    generateMoves(oppLegalMoves);
    undoMove();
    return oppLegalMoves.empty();
}

// 判断当前行动方主帅是否正处于被攻击状态的函数定义
bool Chess::currKingAttacked(const Move &move)
{
    int kingX = -1, kingY = -1;
    for (int x = 0; x < BOARDWIDTH; x++)
    {
        for (int y = 0; y < BOARDHEIGHT; y++)
        {
            auto &curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == currColor && curGrid.type == King)
            {
                kingX = x;
                kingY = y;
            }
        }
    }
    return attacked(oppColor(), kingX, kingY);
}

// 评估执行某步移动后是否对敌方主帅造成攻击(将军)的函数定义
bool Chess::isOppKingAttackedAfterMove(const Move &move)
{
    colorType originColor = currColor;
    makeMoveAssumeLegal(move);
    int kingX = -1, kingY = -1;
    for (int x = 0; x < BOARDWIDTH; x++)
    {
        for (int y = 0; y < BOARDHEIGHT; y++)
        {
            auto &curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color == oppColor(originColor) && curGrid.type == King)
            {
                kingX = x;
                kingY = y;
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
bool Chess::exceedMaxPeaceState()
{
    return continuePeaceTurn[currTurnId] >= 60 - 1;
}

// 将二维坐标映射到一维数组索引的函数定义
int Chess::xy2pos(int mx, int my)
{
    return my * BOARDWIDTH + mx;
}

// 将一维数组索引解码为X轴坐标的函数定义
int Chess::pos2x(int mpos)
{
    return mpos % BOARDWIDTH;
}

// 将一维数组索引解码为Y轴坐标的函数定义
int Chess::pos2y(int mpos)
{
    return mpos / BOARDWIDTH;
}

// 判断给定坐标是否位于指定方九宫格内的函数定义
bool Chess::inKingArea(int mx, int my, colorType mcolor)
{
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED)
    {
        return mx >= 3 && mx <= 5 && my >= 0 && my <= 2;
    }
    else
    {
        return mx >= 3 && mx <= 5 && my >= 7 && my <= 9;
    }
}

// 判断给定坐标是否位于指定方半场内的函数定义
bool Chess::inColorArea(int mx, int my, colorType mcolor)
{
    assert(mcolor == RED || mcolor == BLACK);
    if (mcolor == RED)
    {
        return my <= 4;
    }
    else
    {
        return my >= 5;
    }
}

// 统计同行或同列两坐标点之间障碍棋子数量的函数定义
int Chess::betweenNotEmptyNum(int sx, int sy, int ex, int ey)
{
    assert(inSameStraightLine(sx, sy, ex, ey));
    int dex = ex > sx ? 1 : (ex == sx ? 0 : -1);
    int dey = ey > sy ? 1 : (ey == sy ? 0 : -1);
    int tx = sx + dex, ty = sy + dey;
    int retNum = 0;
    while (inBoard(tx, ty) && !(tx == ex && ty == ey))
    {
        auto &tGrid = state[currTurnId][xy2pos(tx, ty)];
        if (tGrid.color != EMPTY)
        {
            retNum++;
        }
        tx += dex;
        ty += dey;
    }
    return retNum;
}

// 返回给定颜色相对对手颜色的函数定义
colorType Chess::oppColor(colorType mcolor)
{
    return mcolor == RED ? BLACK : RED;
}

// 撤销落子并恢复系统至上一步状态的函数定义
// 注意先前的状态均需要改变
void Chess::undoMove()
{
    if (currTurnId == 0)
    {
        return;
    }
    state.pop_back();
    currTurnId--;
    currColor = oppColor();
    continuePeaceTurn.pop_back();
    lastMoveEaten.pop_back();
}

// 生成满足禁手规则的合法移动步集合的函数定义
void Chess::generateMovesWithForbidden(vector<Move> &legalMoves, bool mustDefend)
{
    vector<Move> firstMoves;
    generateMoves(firstMoves, mustDefend);
    legalMoves.clear();
    for (const auto &move : firstMoves)
    {
        if (!repeatAfterMove(move))
        {
            legalMoves.push_back(move);
        }
    }
}



// 评估某步移动后是否会导致达到三次重复局面的函数定义
bool Chess::repeatAfterMove(const Move &move)
{
    makeMoveAssumeLegal(move);
    int tmp_id = currTurnId;
    if (lastMoveEaten[tmp_id])
    {
        undoMove();
        return false;
    }
    while (true)
    {
        if (tmp_id <= 1)
        {
            break;
        }
        if (lastMoveEaten[tmp_id - 2])
        {
            break;
        }
        tmp_id -= 2;
    }
    int repeatTime = 0;
    for (int id = currTurnId; id >= tmp_id; id -= 2)
    { //从后往前，而非从前往后，跳跃比对
        if (state[id] == state[currTurnId])
        {
            repeatTime++;
        }
    }
    undoMove();
    return repeatTime >= 3;
}

// 综合判断给定移动是否遵守禁手规则而合法的函数定义
bool Chess::isLegalMoveWithForbidden(const Move &move, bool mustDefend)
{
    vector<Move> currLegalMoves;
    generateMovesWithForbidden(currLegalMoves, mustDefend);
    auto fi_iter = std::find(currLegalMoves.begin(), currLegalMoves.end(), move);
    return fi_iter != currLegalMoves.end();
}

// 将当前棋盘通过控制台可视化的函数定义
void Chess::printBoard()
{
    std::cout << std::endl
              << "Board" << std::endl;
    for (int y = 9; y >= 0; y--)
    {
        for (int x = 0; x <= 8; x++)
        {
            std::cout << stoneSym[state[currTurnId][xy2pos(x, y)].type] << " ";
        }
        std::cout << std::endl;
    }
}

//======================以下为添加的函数方法=======================\\

//计算当前局面的 Zobrist Key
uint64_t Chess::getZobristKey(){

    uint64_t key = 0;
    //棋盘格点遍历顺序无所谓
    for(int x = 0;x < BOARDWIDTH;x ++){
        for(int y = 0;y < BOARDHEIGHT; y++){
            auto& curGrid = state[currTurnId][xy2pos(x,y)];
            if(curGrid.type != None && curGrid.color != EMPTY){
                int coloridx = (curGrid.color == RED) ? 0 : 1; //红0 黑1
                key ^= ZobristTable[curGrid.type][coloridx][xy2pos(x,y)];
            }
        }
    }
    if(currColor == BLACK){
        key ^= ZobristPlayer;
    }

    return key;
}
//在makeMoveAssumeLegal 和 undoMove中可以通过两次XOR抹去旧棋子，增加新棋子，不需要遍历全盘，可优化


// 验证在禁手规则限制下某移动是否有效的函数定义
bool Chess::isMoveValidWithForbidden(const Move &move, bool mustDefend)
{
    return isLegalMoveWithForbidden(move, mustDefend);
}

void ChessAI::getcurstones()
{
    int Color = this->board->currColor;
    int tmp = this->board->currTurnId;
    for (int y = 9; y >= 0; y--)
    {
        for (int x = 0; x <= 8; x++)
        {
            if (board->state[tmp][board->xy2pos(x, y)].color == Color)
            {
                evaluator.curstones.push_back(board->state[tmp][board->xy2pos(x, y)]); 
                evaluator.curattacked.push_back(evaluator.isattacked(board->oppColor(), x, y));  
            }
        }
    }
}

//由attacked接口 ： 非将帅，这一点是否受特定棋子攻击?
//我方的各个棋子是否会被攻击
// color为对方的color，我方的点为(mx,my)
//保留该接口，在ChessAI中使用
bool Chess::isattacked(colorType color, int mx, int my)
{
    for (int x = 0; x < BOARDWIDTH; x++)
    {
        for (int y = 0; y < BOARDHEIGHT; y++)
        {
            auto &curGrid = state[currTurnId][xy2pos(x, y)];
            if (curGrid.color != color)
            {
                continue;
            }
            int dex = mx - x, dey = my - y; //当前敌方棋子的坐标为(x,y)
            switch (curGrid.type)
            {
            case King:
            {
                // King不能超出九宫格  我方是否在对方 将/帅 的九宫格内？
                if ((abs(dex) == 1 && abs(dey) == 0 && inKingArea(mx, my, oppColor(currColor))) || (abs(dex) == 0 && abs(dey) == 1 && abs(dey) == 0 && inKingArea(mx, my, oppColor(currColor))))
                {
                    return true;
                }
                break;
            }
            case Assistant:
            {
                if (inKingArea(mx, my, oppColor(color)))
                {
                    if (abs(dex) == 1 && abs(dey) == 1)
                    {
                        return true;
                    }
                }
                break;
            }
            case Knight:
            {
                if (abs(dex) == 2 && abs(dey) == 1)
                {
                    int fx = x + (dex > 0 ? 1 : -1), fy = y;
                    auto &fGrid = state[currTurnId][xy2pos(fx, fy)];
                    if (fGrid.color == EMPTY)
                    {
                        return true;
                    }
                }
                else if (abs(dex) == 1 && abs(dey) == 2)
                {
                    int fx = x, fy = y + (dey > 0 ? 1 : -1);
                    auto &fGrid = state[currTurnId][xy2pos(fx, fy)];
                    if (fGrid.color == EMPTY)
                    {
                        return true;
                    }
                }
                break;
            }
            case Bishop:
            {
                if (abs(dex) == 2 && abs(dey) == 2)
                {
                    int ex = x + dex / 2, ey = y + dey / 2;
                    auto &eGrid = state[currTurnId][xy2pos(ex, ey)];
                    if (eGrid.color == EMPTY)
                    {
                        return true;
                    }
                }
                break;
            }
            case Rook:
            {
                if (inSameStraightLine(x, y, mx, my) && betweenIsEmpty(x, y, mx, my))
                {
                    return true;
                }
                break;
            }
            case Pawn:
            {
                int forwardOne = (curGrid.color == RED ? 1 : -1);
                bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5));
                if (dex == 0 && dey == forwardOne)
                {
                    return true;
                }
                if (crossMidLine && (abs(dex) == 1 && dey == 0))
                {
                    return true;
                }
                break;
            }
            case Cannon:
            {
                if (inSameStraightLine(x, y, mx, my) && betweenNotEmptyNum(x, y, mx, my) == 1)
                {
                    return true;
                }
                break;
            }
            case None:
            {
                throw std::runtime_error("color is not empty but type is none");
                break;
            }
            }
        }
    }
    return false;
}

//传入对方的颜色 我方棋子的位置，验证对方是否能攻击到我方棋子
bool EvalRecord::isattacked(colorType color, int mx, int my)
{
    if (color == board->currColor)
    {
        throw std::runtime_error("False color:expected oppsite color");
    }

    if (board->state[board->currTurnId][board->xy2pos(mx, my)].type == King)
    {
        return board->attacked(color, mx, my);
    }
    else
    {
        return board->isattacked(color, mx, my);
    }
}

void ChessAI::generateMovesWithForbidden()
{
    board->generateMoveNodesWithForbidden(this->curlegalmoves, evaluator.isKingattacked);
    //仿照lastMoveEaten，还要防止长将手、长捉手，已经生成所有满足条件的着法
}

void ChessAI::getdiffmoves()
{
    for (auto &s : this->curlegalmoves)
    {
        if (board->state[board->currTurnId][board->xy2pos(s.strategy.target_x, s.strategy.target_y)].color == board->oppColor())
        {
            this->eatenmoves.push_back(s);
        }
        else
        {
            this->plainmoves.push_back(s);
        }
    }
}

ChessAI::ChessAI()
    : searchTime(900)
{
    this->board = new Chess();
    this->evaluator.copyBoard(this->board);

    //初始化其它成员变量
    evaluator.isKingattacked = false;
}

ChessAI::~ChessAI()
{
    //有指针变量，必须指定析构函数
    if (board != nullptr)
    {
        delete board;
        board = nullptr;
    }
}


//中局有进攻机会的兵 / 帅
static const int mid_KingPawn_attack[90] = {
    10, 10, 10, 15, 15, 15, 10, 10, 10,
    50, 55, 60, 85,100, 85, 60, 55, 50,
    65, 70, 70, 75, 75, 75, 70, 70, 65,
    75, 80, 80, 80, 80, 80, 80, 80, 75,
    70, 70, 65, 70, 70, 70, 65, 70, 70,
    45,  0, 40, 45, 45, 45, 40,  0, 45,
    40,  0, 35, 40, 40, 40, 35,  0, 40,
     0,  0,  5,  5, 15,  5,  5,  0,  0,
     0,  0,  3,  3, 13,  3,  3,  0,  0,
     0,  0,  1,  1, 11,  1,  1,  0,  0,
};

//中局无进攻机会的兵 / 帅
static const int mid_KingPawn_no_attack[90] = {
    10, 10, 10, 15, 15, 15, 10, 10, 10,  
    10, 15, 20, 45, 60, 45, 20, 15, 10, 
    25, 30, 30, 35, 35, 35, 30, 30, 25,  
    35, 40, 40, 45, 45, 45, 40, 40, 35, 
    25, 30, 30, 35, 35, 35, 30, 30, 25, 
    25,  0, 25, 25, 25, 25, 25,  0, 25,  
    20,  0, 20, 20, 20, 20, 20,  0, 20, 
    0,  0,  5,  5, 13,  5,  5,  0,  0, 
    0,  0,  3,  3, 12,  3,  3,  0,  0, 
    0,  0,  1,  1, 11,  1,  1,  0,  0,
};
//残局有进攻机会的兵 / 帅
static const int end_KingPawn_attack[90] = {

    10, 10, 10, 15, 15, 15, 10, 10, 10, 
    50, 55, 60, 85,100, 85, 60, 55, 50, 
    65, 70, 70, 75, 75, 75, 70, 70, 65, 
    75, 80, 80, 80, 80, 80, 80, 80, 75, 
    70, 70, 65, 70, 70, 70, 65, 70, 70,
    45,  0, 40, 45, 45, 45, 40,  0, 45,
    40,  0, 35, 40, 40, 40, 35,  0, 40,
    0,  0,  5,  5, 15,  5,  5,  0,  0,
    0,  0,  3,  3, 13,  3,  3,  0,  0,
    0,  0,  1,  1, 11,  1,  1,  0,  0, 
};

//残局无进攻机会的兵 / 帅
static const int end_KingPawn_no_attack[90] = {
    10, 10, 10, 15, 15, 15, 10, 10, 10, 
    10, 15, 20, 45, 60, 45, 20, 15, 10,
    25, 30, 30, 35, 35, 35, 30, 30, 25, 
    35, 40, 40, 45, 45, 45, 40, 40, 35, 
    25, 30, 30, 35, 35, 35, 30, 30, 25, 
    25,  0, 25, 25, 25, 25, 25,  0, 25, 
    20,  0, 20, 20, 20, 20, 20,  0, 20, 
    0,  0,  5,  5, 13,  5,  5,  0,  0, 
    0,  0,  3,  3, 12,  3,  3,  0,  0,  
    0,  0,  1,  1, 11,  1,  1,  0,  0,
};
//有威胁的士和象
static const int mid_AdvBis_no_Threat[90] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0, 20,  0,  0,  0, 20,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,
    18,  0,  0, 20, 23, 20,  0,  0, 18,
     0,  0,  0,  0, 23,  0,  0,  0,  0,
     0,  0, 20, 20,  0, 20, 20,  0,  0,
};

//有威胁的士和象
static const int mid_AdvBis_Threat[90] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0, 40,  0,  0,  0, 40,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,
    38,  0,  0, 40, 43, 40,  0,  0, 38,
     0,  0,  0,  0, 43,  0,  0,  0,  0,
     0,  0, 40, 40,  0, 40, 40,  0,  0,
};

//中局的马
static const int mid_horse[90] = {
    90, 90, 90, 96, 90, 96, 90, 90, 90,
    90, 96,103, 97, 94, 97,103, 96, 90,
    92, 98, 99,103, 99,103, 99, 98, 92,
    93,108,100,107,100,107,100,108, 93,
    90,100, 99,103,104,103, 99,100, 90,
    90, 98,101,102,103,102,101, 98, 90,
    92, 94, 98, 95, 98, 95, 98, 94, 92,
    93, 92, 94, 95, 92, 95, 94, 92, 93,
    85, 90, 92, 93, 78, 93, 92, 90, 85,
    88, 85, 90, 88, 90, 88, 90, 85, 88,
};

//残局的马
static const int end_horse[90] = {
    92, 94, 96, 96, 96, 96, 96, 94, 92,
    94, 96, 98, 98, 98, 98, 98, 96, 94,
    96, 98,100,100,100,100,100, 98, 96,
    96, 98,100,100,100,100,100, 98, 96,
    96, 98,100,100,100,100,100, 98, 96,
    94, 96, 98, 98, 98, 98, 98, 96, 94,
    94, 96, 98, 98, 98, 98, 98, 96, 94,
    92, 94, 96, 96, 96, 96, 96, 94, 92,
    90, 92, 94, 92, 92, 92, 94, 92, 90,
    88, 90, 92, 90, 90, 90, 92, 90, 88,
};

//中局的车
static const int mid_Rook[90] = {
   206,208,207,213,214,213,207,208,206,
   206,212,209,216,233,216,209,212,206,
   206,208,207,214,216,214,207,208,206,
   206,213,213,216,216,216,213,213,206,
   208,211,211,214,215,214,211,211,208,
   208,212,212,214,215,214,212,212,208,
   204,209,204,212,214,212,204,209,204,
   198,208,204,212,212,212,204,208,198,
   200,208,206,212,200,212,206,208,200,
   194,206,204,212,200,212,204,206,194,
};

//残局的车
static const int end_Rook[90] = {
   182,182,182,184,186,184,182,182,182,
   184,184,184,186,190,186,184,184,184,
   182,182,182,184,186,184,182,182,182,
   180,180,180,182,184,182,180,180,180,
   180,180,180,182,184,182,180,180,180,
   180,180,180,182,184,182,180,180,180,
   180,180,180,182,184,182,180,180,180,
   180,180,180,182,184,182,180,180,180,
   180,180,180,182,184,182,180,180,180,
   180,180,180,182,184,182,180,180,180,
};

//中局的炮
static const int mid_Cannon[90] = {
   100,100, 96, 91, 90, 91, 96,100,100,
    98, 98, 96, 92, 89, 92, 96, 98, 98,
    97, 97, 96, 91, 92, 91, 96, 97, 97,
    96, 99, 99, 98,100, 98, 99, 99, 96,
    96, 96, 96, 96,100, 96, 96, 96, 96,
    95, 96, 99, 96,100, 96, 99, 96, 95,
    96, 96, 96, 96, 96, 96, 96, 96, 96,
    97, 96,100, 99,101, 99,100, 96, 97,
    96, 97, 98, 98, 98, 98, 98, 97, 96,
    96, 96, 97, 99, 99, 99, 97, 96, 96,
};

//残局的炮
static const int end_Cannon[90] = {
   100,100,100,100,100,100,100,100,100,
   100,100,100,100,100,100,100,100,100,
   100,100,100,100,100,100,100,100,100,
   100,100,100,102,104,102,100,100,100,
   100,100,100,102,104,102,100,100,100,
   100,100,100,102,104,102,100,100,100,
   100,100,100,102,104,102,100,100,100,
   100,100,100,102,104,102,100,100,100,
   100,100,100,104,106,104,100,100,100,
   100,100,100,104,106,104,100,100,100,
};

//填充Gridcontrol 格点控制表
void EvalRecord::getGridcontrol(){
    if(board == nullptr){
        throw std::runtime_error("(getGridcontrol)board is null");
    }

    //每次使用前都重新清空控制点字典
    for(int side = 0; side < 2; side ++){
        for(int pos = 0; pos < 90; pos ++){
            GridControl[side][pos] = 0;
        }
    }

    //当前棋局:
    int tmp = board->currTurnId;
    for(int y = 0; y < BOARDHEIGHT; y ++){
        for(int x = 0; x < BOARDWIDTH; x ++){
            auto &curGrid = board->state[tmp][board->xy2pos(x, y)];
            if(curGrid.color == EMPTY){
                continue;
            }

            //红色映射到0，黑色映射到1
            int colorId = (curGrid.color == RED ? 0 : 1);

            switch (curGrid.type)
            {
            case None:
                break;
            case King: //将帅是否能够攻击：普通攻击 / 照面将
            {
                for(int dir = 0; dir < 4; dir ++){
                    int tx = x + dx_strai[dir], ty = y + dy_strai[dir];
                    if(!board->inKingArea(tx, ty, curGrid.color)){
                        continue;
                    }
                    GridControl[colorId][board->xy2pos(tx, ty)] ++;
                    //这里不使用color传参，使用board中的currColor
                    if(curGrid.color == board->oppColor() && board->state[tmp][board->xy2pos(tx,ty)].type != None){
                        this->havecontrol[board->xy2pos(x,y)] = true;
                    }
                }
                //将帅照面：King沿竖线遇到对方King时，同样计为可控制
                for(int dir = -1; dir <= 1; dir += 2){
                    int ty = y + dir;
                    while(board->inBoard(x, ty)){
                        auto &tGrid = board->state[tmp][board->xy2pos(x, ty)];
                        if(tGrid.color != EMPTY){
                            if(tGrid.type == King && tGrid.color != curGrid.color){
                                GridControl[colorId][board->xy2pos(x, ty)] ++;
                                
                                //填充havecontrol
                                if(curGrid.color == board->oppColor() && board->state[tmp][board->xy2pos(x,ty)].type != None){
                                    this->havecontrol[board->xy2pos(x,y)] = true;
                                }
                            }
                            break;
                        }
                        ty += dir;
                    }
                }
                break;
            }
            case Assistant:
            {
                for(int dir = 0; dir < 4; dir ++){
                    int tx = x + dx_ob[dir], ty = y + dy_ob[dir];
                    if(!board->inKingArea(tx, ty, curGrid.color)){
                        continue;
                    }
                    GridControl[colorId][board->xy2pos(tx, ty)] ++;
                    
                    //填充havecontrol
                    if(curGrid.color == board->oppColor() && board->state[tmp][board->xy2pos(tx,ty)].type != None){
                        this->havecontrol[board->xy2pos(x,y)] = true;
                    }
                }
                break;
            }
            case Bishop:
            {
                for(int dir = 0; dir < 4; dir ++){
                    int tx = x + dx_bishop[dir], ty = y + dy_bishop[dir];
                    if(!board->inBoard(tx, ty) || !board->inColorArea(tx, ty, curGrid.color)){
                        continue;
                    }
                    int ex = x + dx_bishop_eye[dir], ey = y + dy_bishop_eye[dir];
                    auto &eGrid = board->state[tmp][board->xy2pos(ex, ey)];
                    if(eGrid.color != EMPTY){
                        continue;
                    }
                    GridControl[colorId][board->xy2pos(tx, ty)] ++;

                    //填充havecontrol
                    if(curGrid.color == board->oppColor() && board->state[tmp][board->xy2pos(tx,ty)].type != None){
                        this->havecontrol[board->xy2pos(x,y)] = true;
                    }
                }
                break;
            }
            case Knight:
            {
                for(int dir = 0; dir < 8; dir ++){
                    int tx = x + dx_knight[dir], ty = y + dy_knight[dir];
                    if(!board->inBoard(tx, ty)){
                        continue;
                    }
                    int fx = x + dx_knight_foot[dir], fy = y + dy_knight_foot[dir];
                    auto &fGrid = board->state[tmp][board->xy2pos(fx, fy)];
                    if(fGrid.color != EMPTY){
                        continue;
                    }
                    GridControl[colorId][board->xy2pos(tx, ty)] ++;


                    if(curGrid.color == board->oppColor() && board->state[tmp][board->xy2pos(tx,ty)].type != None){
                        this->havecontrol[board->xy2pos(x,y)] = true;
                    }
                }
                break;
            }
            case Rook:
            {
                for(int dir = 0; dir < 4; dir ++){
                    int tx = x + dx_strai[dir], ty = y + dy_strai[dir];
                    while(board->inBoard(tx, ty)){
                        GridControl[colorId][board->xy2pos(tx, ty)] ++;
                        if(curGrid.color == board->oppColor() && board->state[tmp][board->xy2pos(tx,ty)].type != None){
                            this->havecontrol[board->xy2pos(x,y)] = true;
                        }
                        auto &tGrid = board->state[tmp][board->xy2pos(tx, ty)];
                        if(tGrid.color != EMPTY){
                            break;
                        }
                        tx += dx_strai[dir];
                        ty += dy_strai[dir];
                    }
                }
                break;
            }
            case Cannon:
            {
                for(int dir = 0; dir < 4; dir ++){
                    int tx = x + dx_strai[dir], ty = y + dy_strai[dir];
                    bool hasScreen = false;
                    while(board->inBoard(tx, ty)){
                        auto &tGrid = board->state[tmp][board->xy2pos(tx, ty)];
                        if(!hasScreen){
                            if(tGrid.color == EMPTY){
                                GridControl[colorId][board->xy2pos(tx, ty)] ++;
                            }else{
                                hasScreen = true;  //被遮挡，能架炮。第一个架炮点之前的位置都不算控制
                            }
                        }else{
                            if(tGrid.color != EMPTY){
                                GridControl[colorId][board->xy2pos(tx, ty)] ++;
                            if(curGrid.color == board->oppColor() && board->state[tmp][board->xy2pos(tx,ty)].type != None){
                                this->havecontrol[board->xy2pos(x,y)] = true;
                            }
                                break;
                            }
                        }
                        tx += dx_strai[dir];
                        ty += dy_strai[dir];
                    }
                }
                break;
            }
            case Pawn:
            {
                int forwardOne = (curGrid.color == RED ? 1 : -1);
                int tx = x, ty = y + forwardOne;
                if(board->inBoard(tx, ty)){
                    GridControl[colorId][board->xy2pos(tx, ty)] ++;
                    if(curGrid.color == board->oppColor() && board->state[tmp][board->xy2pos(tx,ty)].type != None){
                        this->havecontrol[board->xy2pos(x,y)] = true;
                    }
                }
                bool crossMidLine = (curGrid.color == RED ? (y > 4) : (y < 5));
                if(crossMidLine){
                    for(int dir = 0; dir < 2; dir ++){
                        tx = x + dx_lr[dir], ty = y;
                        if(board->inBoard(tx, ty)){
                            GridControl[colorId][board->xy2pos(tx, ty)] ++;
                        }
                        if(curGrid.color == board->oppColor() && board->state[tmp][board->xy2pos(tx,ty)].type != None){
                            this->havecontrol[board->xy2pos(x,y)] = true;
                        }
                    }
                }
                break;
            }
            }
        }
    }
}

bool EvalRecord::hasattackchance(int mx,int my){

    //依靠board->getGridcontrol
    return this->havecontrol[board->xy2pos(mx,my)];
}

//当前是否为中局：车 6 马炮 3 其它 1
const int Rook_state = 6;
const int HorseCannon_state = 3;
const int other_state = 1;
const int Total_state = 66;
//判断当前是否为中局 
bool EvalRecord::ismiddlecompostion(vector<Grid>& vec){
    int tmp = 0;
    for(int y = 9; y >= 0;y --){
        for(int x = 0;x <= 8;x ++){
            if(vec[board->xy2pos(x,y)].type == Rook){
                tmp += Rook_state;
            }else if(vec[board->xy2pos(x,y)].type == Knight || vec[board->xy2pos(x,y)].type == Cannon){
                tmp += HorseCannon_state;
            }else{
                tmp += other_state;
            }
        }
    }
    return tmp > (Total_state / 2);
}

//注意：静态棋力表中的数据均为对红方而言的数据，颜色不同时，需要翻转————(9 - x,10 - y)
//参考Eleeye中的静态估值 vec调用board->state中的当前状态
int EvalRecord::calstaticstate(vector<Grid> &vec, colorType color)
{
    if(this->board == nullptr){
        throw(std::runtime_error("(calstaticstate)board is null"));
    }
    bool ismiddle = ismiddlecompostion(vec);
    colorType oppCol = (color == RED) ? BLACK : RED;
    int res = 0;

    //红方 中局
    if(color == RED && ismiddle){
        for(int y = 9;y >= 0;y --){
            for(int x = 0;x <= 8;x ++){
                if(vec[board->xy2pos(x,y)].color != RED)continue;

                switch (vec[board->xy2pos(x,y)].type)
                {
                case None:
                    break;
                case Pawn:
                case King:{
                    bool flag = this->hasattackchance(x,y);
                    res += (flag) ? mid_KingPawn_attack[81 - 9 * y + x] : mid_KingPawn_no_attack[81 - 9 * y  + x];
                    break;
                }
                case Assistant:
                case Bishop:{
                    bool flag = this->isattacked(oppCol, x, y);
                    res += (flag) ? mid_AdvBis_Threat[81 - 9 * y + x] : mid_AdvBis_no_Threat[81 - 9 * y + x];
                    break;
                }
                case Knight:
                    res += mid_horse[81 - 9 * y + x];
                    break;
                case Rook:
                    res += mid_Rook[81 - 9 * y + x];
                    break;
                case Cannon:
                    res += mid_Cannon[81 - 9 * y + x];
                    break;
                default:
                    break;
                }
            }
        }
    }else if(color == RED && !ismiddle){ //红色 残局
        for(int y = 9;y >= 0;y --){
            for(int x = 0;x <= 8;x ++){
                if(vec[board->xy2pos(x,y)].color != RED)continue;

                switch (vec[board->xy2pos(x,y)].type)
                {
                case None:
                    break;
                case Pawn:
                case King:{
                    bool flag = this->hasattackchance(x,y);
                    res += (flag) ? end_KingPawn_attack[81 - 9 * y + x] : end_KingPawn_no_attack[81 - 9 * y  + x];
                    break;
                }
                case Assistant:
                case Bishop:{
                    bool flag = this->isattacked(oppCol, x, y);
                    res += (flag) ? mid_AdvBis_Threat[81 - 9 * y + x] : mid_AdvBis_no_Threat[81 - 9 * y + x];
                    break;
                }
                case Knight:
                    res += end_horse[81 - 9 * y + x];
                    break;
                case Rook:
                    res += end_Rook[81 - 9 * y + x];
                    break;
                case Cannon:
                    res += end_Cannon[81 - 9 * y + x];
                    break;
                default:
                    break;
                }
            }
        }
    }else if(color == BLACK && ismiddle){
        for(int y = 9;y >= 0;y --){
            for(int x = 0;x <= 8;x ++){
                if(vec[board->xy2pos(x,y)].color != BLACK)continue;

                switch (vec[board->xy2pos(x,y)].type)
                {
                case None:
                    break;
                case Pawn:
                case King:{
                    bool flag = this->hasattackchance(x,y);
                    res += (flag) ? mid_KingPawn_attack[9 * y + x] : mid_KingPawn_no_attack[9 * y + x];
                    break;
                }
                case Assistant:
                case Bishop:{
                    bool flag = this->isattacked(oppCol, x, y);
                    res += (flag) ? mid_AdvBis_Threat[9 * y + x] : mid_AdvBis_no_Threat[9 * y + x];
                    break;
                }
                case Knight:
                    res += mid_horse[9 * y + x];
                    break;
                case Rook:
                    res += mid_Rook[9 * y + x];
                    break;
                case Cannon:
                    res += mid_Cannon[9 * y + x];
                    break;
                default:
                    break;
                }
            }
        }
    }else{ //color == BLACK && !ismiddle
        for(int y = 9;y >= 0;y --){
            for(int x = 0;x <= 8;x ++){
                if(vec[board->xy2pos(x,y)].color != BLACK)continue;

                switch (vec[board->xy2pos(x,y)].type)
                {
                case None:
                    break;
                case Pawn:
                case King:{
                    bool flag = this->hasattackchance(x,y);
                    res += (flag) ? end_KingPawn_attack[9 * y + x] : end_KingPawn_no_attack[9 * y + x];
                    break;
                }
                case Assistant:
                case Bishop:{
                    bool flag = this->isattacked(oppCol, x, y);
                    res += (flag) ? mid_AdvBis_Threat[9 * y + x] : mid_AdvBis_no_Threat[9 * y + x];
                    break;
                }
                case Knight:
                    res += end_horse[9 * y + x];
                    break;
                case Rook:
                    res += end_Rook[9 * y + x];
                    break;
                case Cannon:
                    res += end_Cannon[9 * y + x];
                    break;
                default:
                    break;
                }
            }
        }
    }

    return res;
}

//借鉴eleeye中的一系列动态得分机制

//1.特殊棋型 空头炮 / 窝心马(暂时不考虑)
//2.进攻度分析 —————— 对对方半场的控制度
//3.防御度分析 —————— 对我方半场的控制度
const int EnemyKingArea = 8;  //敌方九宫区域 ———— 最为重要，唯有控制敌方九宫才有夺取胜利的机会！
const int EnemyHalf = 5; 
const int OwnHalf = 2;        //我方半场
const int OwnKingArea = 3;    //我方九宫区域 ———— 次次重要
const int attackweight = 1;       //或使用double，进攻权重
const int defenceweight = 1;      //或使用double，防御权重
int EvalRecord::caldynamicstate(vector<Grid>& target,colorType color){
    //借鉴Eleeye中的参数和计算方法，现在仅考虑使用Gridcontrol，完成动态局面评估
    int myattack = 0,myDefense = 0; 
    int oppattack = 0,oppDefence = 0;

    //区分颜色
    int idx = (color == RED) ? 0 : 1; //对应控制表内不同映射
    for(int y = 9;y >= 0;y --){
        for(int x = 0;x <= 8;x ++){
            auto& tmpgrid = target[board->xy2pos(x,y)];
            int pos = board->xy2pos(x,y);

            if(idx == 0){
                if(y <= 4){ //红方半场
                    if(board->inKingArea(x,y,RED)){
                        myDefense += GridControl[idx][pos] * OwnKingArea;
                    }else{
                        myDefense += GridControl[idx][pos] * OwnHalf;
                    }
                }else{
                    if(board->inKingArea(x,y,RED)){
                        myattack += GridControl[idx][pos] * EnemyKingArea;
                    }else{
                        myattack += GridControl[idx][pos] * EnemyHalf;
                    }
                }
            }else{
                if(y >= 5){ //黑方半场
                    if(board->inKingArea(x,y,BLACK)){
                        myDefense += GridControl[idx][pos] * OwnKingArea;
                    }else{
                        myDefense += GridControl[idx][pos] * OwnHalf;
                    }
                }else{
                    if(board->inKingArea(x,y,BLACK)){
                        myattack += GridControl[idx][pos] * EnemyKingArea;
                    }else{
                        myattack += GridControl[idx][pos] * EnemyHalf;
                    }
                }
            }
        }
    }
    
    //进攻度分析 ———— Gridcontrol 我方在对方半场的控制点、对方在对方半场的控制点

    //防御度分析 ———— Gridcontrol 我方在我方半场的控制点、对方在我方半场的控制点

    //最终结果为：我方总得分 - 对方总得分
    int res = attackweight * (myattack - oppattack) + defenceweight * (myDefense - oppDefence);
    return res;
}

//静态评估系数(暂时采用int)、动态评估系数(暂时采用int)
const int mystaticweight = 1;
const int mydynamicweight = 1;
const int oppstaticweight = 1;
const int oppdynamicweight = 1;

//集成局面总评估方法(后续进行补充)
int EvalRecord::calstate(vector<Grid>& target,colorType color){

    int res = 0;
    res += this->calstaticstate(target,color);
    res += this->caldynamicstate(target,color);
    return res;
}

// 替换原有的复杂 delta 计算
int EvalRecord::calcurmove(Move& cur, vector<Grid>& curstate, colorType color) {
    int target_pos = board->xy2pos(cur.target_x, cur.target_y);
    int source_pos = board->xy2pos(cur.source_x, cur.source_y);

    auto target_piece = curstate[target_pos].type;
    auto source_piece = curstate[source_pos].type;

    return Tuning::getMVVLVAScore(source_piece, target_piece);
}

/*
//评估单步着法的估值，可能不只是预估我方的某个着法
int EvalRecord::calcurmove(Move& cur,vector<Grid>& curstate,colorType color){
    //求delta值，现在的评分 —— 先前的评分
    //评价方为Move所属的着棋一方，着棋一方使用"my...",对方使用"opp..."
    assert(curstate[board->xy2pos(cur.source_x,cur.source_y)].color == color);
    
    int mypreval = 0,mycurval = 0;
    int opppreval = 0,oppcurval = 0;
    colorType oppcol = (color == RED) ? BLACK : RED;

    mypreval = calstate(curstate,color);
    opppreval = calstate(curstate,oppcol);

    //不可以修改curstate！拷贝或者使用回撤，但一旦Grid某处被覆盖掉，则不知道覆盖之前的值
    auto copyst = curstate;
    
    int source_pos = board->xy2pos(cur.source_x, cur.source_y);
    int target_pos = board->xy2pos(cur.target_x, cur.target_y);

    //更新临时对象copyst
    copyst[source_pos].color = EMPTY;
    copyst[source_pos].type = None;  // 吃子/移动不仅要变空颜色，连棋子类型也要抹去
    copyst[target_pos].color = color;
    copyst[target_pos].type = curstate[source_pos].type;   // 赋予新走到这里的棋子类型

    mycurval = calstate(copyst,color);
    oppcurval = calstate(copyst,oppcol);

    //自身增量减去对方增量
    return (mycurval - mypreval) - (oppcurval - opppreval);
}
*/
/*
int EvalRecord::calcurmove(Move& cur, vector<Grid>& curstate, colorType color){
    int score = 0;
    
    int target_pos = board->xy2pos(cur.target_x, cur.target_y);
    int source_pos = board->xy2pos(cur.source_x, cur.source_y);

    auto target_piece = curstate[target_pos].type;
    auto source_piece = curstate[source_pos].type;

    // 如果目标位置有敌方棋子（发生吃子）
    if (target_piece != None) {
        int target_val = 0;
        int source_val = 0;

        // 获取受害者价值 (MVV)
        switch(target_piece) {
            case King: target_val = 10000; break;
            case Rook: target_val = 900; break;
            case Cannon: target_val = 450; break;
            case Knight: target_val = 400; break;
            case Bishop: target_val = 200; break;
            case Assistant: target_val = 200; break;
            case Pawn: target_val = 100; break;
            default: break;
        }

        // 获取攻击者价值 (LVA)
        switch(source_piece) {
            case King: source_val = 10000; break;
            case Rook: source_val = 900; break;
            case Cannon: source_val = 450; break;
            case Knight: source_val = 400; break;
            case Bishop: source_val = 200; break;
            case Assistant: source_val = 200; break;
            case Pawn: source_val = 100; break;
            default: break;
        }

        // 确保排在不吃子的走法前面。
        // target_val*10 确保受害者的价值占据绝对主导。
        score = 10000 + target_val*10 - source_val;
    }
    
    // 如果没有吃子，返回 0
    return score;
}
*/
//现有策略随着递归深度加深会导致速度严重下降
//上面一个极简的MVV/LVA评估函数，时间复杂度O(1)确保吃子走法优先于不吃子走法，并在吃子走法中优先考虑高价值受害者和低价值攻击者的组合，分数生成应该采用更合适等等策略。
//方案1：浅层保持全面评估，深层使用MVV/LVA评估以节省时间
//方案2：将棋子价值进行排序，优先搜索高价值受害者的吃子走法，让关键子的棋布被优先搜索，避免时间浪费在低价值棋子上，在深度到达一定程度时可以考虑完全舍弃非关键棋子的搜索
//方案3：在搜索中如果发现敌我双方分数差异过大，可以考虑提前剪枝，舍弃那些明显不利的走法
//注意！第二种方案在评估阶段就进行可能导致被将军时严重忽略士相的保护步法，为防止该情况建议分支在排序中沉底而不是过早剪枝
//注意！第三种方案需要谨慎使用，过早剪枝可能导致错过关键的防守或反击机会，建议设置一个合理的阈值，并结合其他评估指标进行判断，以避免过度剪枝导致的误判。
//不建议三种方案同时启用，会使搜索异常复杂，建议使用单一或一种主导第二种辅助的方案


//填充MoveNode，并排序
void ChessAI::fillinMoveNodes(vector<MoveNode>& nodes) {
    for(auto& M : nodes){
        M.value = this->evaluator.calcurmove(M.strategy, board->state[board->currTurnId], board->currColor);
    }
    this->MoveNodesort(nodes);
}

void ChessAI::fill_sort_inMoveNodes(vector<MoveNode>& nodes,vector<Move>& target){
    for(auto& M : target){
        nodes.push_back({M,-1});
    }
    for(auto& M : nodes){
        M.value = this->evaluator.calcurmove(M.strategy, board->state[board->currTurnId], board->currColor);
    }
    this->MoveNodesort(nodes);
}

//根据当前对于局面的评分函数，针对当前的Move进行评分，降序排列
bool MoveNodecmp(const MoveNode& Nodex, const MoveNode& Nodey) {
    return Nodex.value > Nodey.value;
}

void ChessAI::MoveNodesort(vector<MoveNode>& nodes) {
    //快速排序 vector<MoveNode> 
    sort(nodes.begin(), nodes.end(), MoveNodecmp);
}
void ChessAI::MoveNodeVoid(){
    Move tmpmove;
    //让棋局在此步原地踏步
    tmpmove.source_x = 1;
    tmpmove.source_y = 1;
    tmpmove.target_x = 1;
    tmpmove.target_y = 1;
    board->makeMoveAssumeLegal(tmpmove);
}

void ChessAI::MoveNodeFront(MoveNode& tmpmove){
    // 直接下棋即可，因为在alpha-beta搜索进来的着法肯定是合法的(由generateMovesWithForbidden得到)
    board->makeMoveAssumeLegal(tmpmove.strategy);
}
// 对应撤销走法
void ChessAI::MoveNodeBack(MoveNode& tmpmove){
    this->board->undoMove();
}

struct MoveGuard {
    ChessAI* ai;
    MoveNode& node;
    MoveGuard(ChessAI* a, MoveNode& n) : ai(a), node(n) { ai->MoveNodeFront(node); }
    ~MoveGuard() { ai->MoveNodeBack(node); }
};

//该步是否获得胜利
bool ChessAI::getWin(){
    auto& tmpstate = this->board->state[board->currTurnId];
    for(auto& s : tmpstate){
        if(s.color == board->currColor && s.type == King){
            return false;
        }
    }
    return true;
}

//初始化min max值
const int minstateval = -9999999;
const int maxstateval = 9999999;
//着重考虑selected等全局变量应该如何处理
void ChessAI::alpha_beta_select(vector<MoveNode>& selected){
    int max_depth = 10;  //最大搜素深度
    int alpha = minstateval;
    int beta = maxstateval;

    //当前所有合法的着法
    vector<Move> tmpMoves;
    board->generateMovesWithForbidden(tmpMoves);

    //selected初始化 
    selected.clear();
    for(auto& M : tmpMoves){
        MoveNode newMoveNode;
        newMoveNode.strategy = M;
        newMoveNode.value = alpha;      //初始值赋值成极小值
        selected.push_back(newMoveNode);
    }

    //不要忘记预先评价
    MoveNodesort(selected);

    // 搜索计时从根节点开始
    searchTime.start();

    //类似DFS —— 对深层的每一个着法进行探索并记录评分
    try {
        for(auto& Node : selected){
            searchTime.check();
            
            {
                MoveGuard guard(this, Node); //局面按照当前Move进行一次，相当于博弈树向下一层遍历
                int score = -alpha_beta_search(max_depth - 1,-beta,-alpha);
                
                //将深层传回来的得分，记录到根节点的MoveNode属性中
                Node.value = score;
            }

            //更新alpha
            if(Node.value > alpha){
                alpha = Node.value;
            }
        }
    } catch (const TimeOutException &) {
        // 超时时保留当前已经评估出的最佳结果
    }

    sort(selected.begin(),selected.end(),MoveNodecmp);

    //select 降序大根堆
}

void ChessAI::alpha_beta_PVS_select(vector<MoveNode>& selected){
    int max_depth = 10;
    int alpha = minstateval;
    int beta = maxstateval;
  
    vector<Move> tmpMoves;
    board->generateMovesWithForbidden(tmpMoves);

    selected.clear();
    for(auto& M : tmpMoves){
        MoveNode newMoveNode;
        newMoveNode.strategy = M;
        newMoveNode.value = alpha;
        selected.push_back(newMoveNode);
    }

    MoveNodesort(selected);

    // 搜索计时从根节点开始
    searchTime.start();

    try {
        for(auto& Node : selected){
            searchTime.check();
            
            {
                MoveGuard guard(this, Node); //局面按照当前Move进行一次，相当于博弈树向下一层遍历
                int score = -alpha_beta_PVS_search(max_depth - 1,-beta,-alpha);

                //将深层传回来的得分，记录到根节点的MoveNode属性中
                Node.value = score;
            }

            //更新alpha
            if(Node.value > alpha){
                alpha = Node.value;
            }
        }
    } catch (const TimeOutException &) {
        // 超时时保留当前已经评估出的最佳结果
    }

    sort(selected.begin(),selected.end(),MoveNodecmp);
}

//alpha —— beta 具体搜索函数 将函数入口和函数体相玻璃

int ChessAI::alpha_beta_search(int depth, int alpha, int beta) {
    searchTime.check();

    if (depth == 0) { 
        // evaluator部分获取当前局面状态 ———— 该方法以ClassAI单例调用为前提 
        //return evaluator.calstate(board->state[board->currTurnId], board->currColor); 
        return quiescence_search(alpha, beta);//进入静态搜索解决水平线效应
    }

    if(getWin()){
        return 999999; 
    }
    const int R = 2; // 深度缩减参数(Reduction)
    if (depth >= R + 1 && !havethreat(this->board->state[board->currTurnId], board->currColor)) {
        this->MoveNodeVoid(); // 执行空步

        // 尝试一个零窗口探测计算空步后的局面价值
        int null_val = -alpha_beta_PVS_search(depth - 1 - R, -beta, -beta + 1);

        this->board->undoMove(); // 撤销空步

        // 如果空着之后的得分比现有的beta更好，则认为此节点为beta节点，产生截断
        if (null_val >= beta) {
            return beta;
        }
    }


    // 这里必须使用局部变量 储存合法的移动节点，不能用类的成员！否则会覆盖类中的原有数据
    vector<Move> tmpMoves;
    board->generateMovesWithForbidden(tmpMoves);
    
    // （如有死局则返回极小的评估值，代表被将死）
    if (tmpMoves.empty()) return minstateval + 100 * (4 - depth); // 越早被将死，分数越低 100为“偏移系数”

    vector<MoveNode> tmpMoveNodes;
    this->fill_sort_inMoveNodes(tmpMoveNodes,tmpMoves);
    for(auto& node : tmpMoveNodes){
        int val;
        {
            MoveGuard guard(this, node);
            val = -alpha_beta_search(depth - 1,-beta,-alpha);
        }
        
        if(val >= beta){ //beta剪枝
            return beta;
        }
        if(val > alpha){ //alpha更新
            alpha = val;
        }
    }
    return alpha;
}
//克服水平线效应————直到局面“稳定”，没有吃子再停止搜索

//Principal Variation ———— 主要变例，非Alpha 也非 Beta的节点,该类节点中可能找到更优节点
int ChessAI::alpha_beta_PVS_search(int depth,int alpha,int beta){
    searchTime.check();
    
    //获取当前的Zobrist Key
    uint64_t curkey = this->board->getZobristKey();
    int TT_val;
    Move best_TT_Move;
    best_TT_Move.source_x = -1; //未修改的标志位,供后面判断是否找到了best_TT_Move
    //探测置换表(有无符合要求的已计算的置换表)
    if(probeHash(curkey,depth,alpha,beta,TT_val,best_TT_Move)){
        return TT_val; 
    }

    bool foundPV = false;
    if(depth == 0){
        //return evaluator.calstate(board->state[board->currTurnId],board->currColor);
        return quiescence_search(alpha, beta);//进入静态搜索解决水平线效应
    }

    if(getWin()){
        return 999999;
    }
    
    // 空步裁剪 
    // 当前局面已经好到对手无法翻盘，本节点即可作为beta截断节点返回。
    const int R = 2; // 深度缩减参数(Reduction)
    if (depth >= R + 1 && !havethreat(this->board->state[board->currTurnId], board->currColor)) {
        this->MoveNodeVoid(); // 执行空步

        // 尝试一个零窗口探测计算空步后的局面价值
        int null_val = -alpha_beta_PVS_search(depth - 1 - R, -beta, -beta + 1);

        this->board->undoMove(); // 撤销空步

        // 如果空着之后的得分比现有的beta更好，则认为此节点为beta节点，产生截断
        if (null_val >= beta) {
            return beta;
        }
    }
    // 这里必须使用局部变量 储存合法的移动节点，不能用类的成员！否则会覆盖类中的原有数据
    vector<Move> tmpMoves;
    board->generateMovesWithForbidden(tmpMoves);
    
    // （如有死局则返回极小的评估值，代表被将死）
    if (tmpMoves.empty()) return minstateval + 100 * (4 - depth); // 越早被将死，分数越低 100为“偏移系数”

    vector<MoveNode> tmpMoveNodes;
    this->fill_sort_inMoveNodes(tmpMoveNodes,tmpMoves);

    //置换表着法启发(Transposition Table Ordering)
    //原理：如果之前搜素过该局面，最佳着法会被保留，并强制将其提到第一位搜素
    if(best_TT_Move.source_x != -1){
        for(int i = 0;i < tmpMoveNodes.size();i ++){
            if(tmpMoveNodes[i].strategy == best_TT_Move){
                std::swap(tmpMoveNodes[0],tmpMoveNodes[i]);
                break;
            }
        }
    }

    //=====HashTable搜素完成=====\\

    //记录TT 表中，所得到的最优节点
    int original_alpha = alpha;
    Move best_move; //当前节点探索出的最佳着法
    int val;

    //LMR改进 ———— 执行探测 val = -alpha_beta_PVS_search(depth - 1,-alpha - 1,-alpha); 
    //零窗口探测之前，先判断是否跳过部分深度。如果判定成功，才进行下一步的补充搜素，从而在残局和中局减少分支节点
    //这一步必须是“白手” ———— 这一步着棋没有吃子和将军
    int moveIndex = 0; //LMR ———— 当前是第几个着法

    for(auto& node : tmpMoveNodes){
        {
            MoveGuard guard(this, node);

            bool isQuiet = this->isQuietMove(board->state[board->currTurnId],node);

            if(foundPV){ //这里的检查区间设为[-alpha - 1,-alpha]

                // =====LMR逻辑===== \\
                
                if(depth >= 3 && moveIndex && isQuiet){
                    int R = 1; //裁剪因子，减去深度R
                    //先进行0窗口试探，如果返回值大于alpha,说明它有潜力越界，这对于QuietMove是反常的！必须重新搜素！
                    val = -alpha_beta_PVS_search(depth - 1 - R, -alpha - 1,-alpha);

                    if(val > alpha){
                        val = -alpha_beta_PVS_search(depth - 1,-alpha - 1,-alpha);
                    }
                }else{
                    //否则，使用常规的PVS零窗口搜素
                    val = -alpha_beta_PVS_search(depth - 1,-alpha - 1,-alpha);
                }
                // =====LMR逻辑结束===== \\

                val = -alpha_beta_PVS_search(depth - 1,-alpha - 1,- alpha);
                if((val > alpha) && (val < beta)){ //本区间内检查失败！
                    val = -alpha_beta_PVS_search(depth - 1,-beta,-alpha);
                }
            }else{
                val = -alpha_beta_PVS_search(depth - 1,-beta,-alpha);
            }
        }

        if(val > alpha){
            best_move = node.strategy;
        }

        if(val >= beta){ //beta剪枝
            //beta剪枝，引发截断，记录为Beta节点
            RecordHash(curkey,depth,beta,BETA,node.strategy);
            return beta;
        }
/*
        if(val > alpha && !havethreat_of_valuestone(this->board->state...,board->currColor)){ //alpha剪枝
        alpha = val;
        ...
        一旦不更新 alpha，此路径的局部极大值就会被永久掩埋（丢掉最佳招法），并且返回错误的非极值打破了极大极小原理（Minimax），导致后续节点评估完全紊乱。
*/

        if(val > alpha){ //alpha更新
            alpha = val;
            foundPV = true;
        }
    }
    //搜素完毕，未发生Beta剪枝。根据Alpha是否被更新，写入EXACT 或者 ALPHA
    HashFlag flag = (alpha > original_alpha) ? EXACT : ALPHA;
    RecordHash(curkey,depth,alpha,flag,best_move);

    return alpha;
}


bool ChessAI::havethreat(vector<Grid>& tmp,colorType color){
    for(int y = 9; y >= 0; y --){
        for(int x = 0;x <= 8;x ++){
            if(tmp[board->xy2pos(x,y)].color == color && evaluator.isattacked(board->oppColor(),x,y)){
                return true; //说明存在威胁
            }
        }
    }

    return false;
}

bool ChessAI::havethreat_of_valuestone(vector<Grid>& tmp,colorType color){
    for(int y = 9; y >= 0; y --){
        for(int x = 0;x <= 8;x ++){
            auto tmptype = tmp[board->xy2pos(x,y)].type;
            bool is_important = (tmptype == Rook || tmptype == Knight || tmptype == Cannon) ? true : false;
            if(tmp[board->xy2pos(x,y)].color == color && is_important && evaluator.isattacked(board->oppColor(),x,y)){
                return true; //说明存在威胁
            }
        }
    }

    return false;
}

bool ChessAI::haveattack(vector<Grid>& tmp,colorType color){
    for(int y = 9; y >= 0; y --){
        for(int x = 0;x <= 8;x ++){
            if(tmp[board->xy2pos(x,y)].color == this->board->oppColor() && evaluator.isattacked(board->currColor,x,y)){
                return true; //说明存在威胁
            }
        }
    }

    return false;
}

bool ChessAI::haveattack_of_valuestone(vector<Grid>& tmp,colorType color){
    for(int y = 9; y >= 0; y --){
        for(int x = 0;x <= 8;x ++){
            auto tmptype = tmp[board->xy2pos(x,y)].type;
            bool is_important = (tmptype == Rook || tmptype == Knight || tmptype == Cannon) ? true : false;
            if(tmp[board->xy2pos(x,y)].color == board->oppColor() && is_important && evaluator.isattacked(board->currColor,x,y)){
                return true; //说明存在威胁
            }
        }
    }

    return false;
}

//是否为“白手”

bool ChessAI::isQuietMove(vector<Grid>& tmpgrid,MoveNode& tmpmove){
    MoveNodeFront(tmpmove);
    this->evaluator.getGridcontrol();
    return havethreat(board->state[board->currTurnId],board->currColor);
}

int ChessAI::quiescence_search(int alpha, int beta) {
    searchTime.check();
    int stand = evaluator.calstate(board->state[board->currTurnId], board->currColor); 
    
    if (stand >= beta) {
        return beta;
    }
    if (stand > alpha) {
        alpha = stand;
    }

    vector<Move> tmpMoves;
    board->generateMovesWithForbidden(tmpMoves); 

    vector<MoveNode> captureNodes;
    auto& currState = board->state[board->currTurnId];
    
    for (const auto& m : tmpMoves) {
        int targetPos = board->xy2pos(m.target_x, m.target_y);
        int sourcePos = board->xy2pos(m.source_x, m.source_y);
        
        stoneType victim = currState[targetPos].type;
        stoneType attacker = currState[sourcePos].type;

        if (victim != None) // 仅筛选吃子着法
        {
            MoveNode node;
            node.strategy = m;
            // 调用封装好的 MVV/LVA 策略
            node.value =Tuning::getMVVLVAScore(attacker, victim); 
            captureNodes.push_back(node);
        }
    }

    // 降序排序
    this->MoveNodesort(captureNodes);

    // 遍历吃子着法深入博弈树
    for (auto& node : captureNodes) {
        int val;
        {
            MoveGuard guard(this, node); // 走这步吃子棋
            val = -quiescence_search(-beta, -alpha); 
        }
        if (val >= beta) {
            return beta;
        }
        if (val > alpha) {
            alpha = val;
        }
    }

    return alpha;
}

//PVS算法
//判断此次下子是否吃子  判断此棋子是否有吃子的机会
//吃子的估值
//非吃子的估值


    // for (int y = 9; y >= 0; y--)
    // {
    //     for (int x = 0; x <= 8; x++)
    //     {
    //         std::cout << stoneSym[state[currTurnId][xy2pos(x, y)].type] << " ";
    //     }
    //     std::cout << std::endl;
    // }

//使用PSW进行搜索 —— 需要合适的MoveSort，否则"假设后面节点都不如A，再验证"意义不大！