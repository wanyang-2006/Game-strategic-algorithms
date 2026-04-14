#include<iostream>
#include<string>
#include<vector>
#include<cmath>
#include<map>
#include<cassert>  //提供断言，检查程序在运行条件中是否为真
#include<algorithm>
#include<vector>
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
}

//由chess传回相应方法的参数，再由当前待走的棋子作决定，判断：是否 合法/吃子，实质上是移动是否合法
class Chess{
private:
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
    bool isMoveValid(const Move& move.bool mustDefend = true);
    bool isLegalMove(const Move& move.bool mustDefend = true);
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
};

void getInputBotzone(Chess& chess);
void giveOutputBotzone(Chess& chess);

int main(){
    Chess chess;
    getInputBotzone(chess);
    getOutputBotzone(chess);

    return 0;
}

void giveOutputBotzone(Chess& chess){
    vector<Move> retMoves;
    chess.generateMovesWithForbidden(resMoves);
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