// 井字棋（TicTacToe）简易交互样例程序
// 随机策略
// 作者：zys
// 游戏信息：http://www.botzone.org/games#TicTacToe
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include "jsoncpp/json.h" // 在平台上，C++编译时默认包含此库

using namespace std;

int currBotColor; // 我所执子颜色（1为黑，-1为白，棋盘状态亦同）
int gridInfo[3][3] = { 0 }; // 先x后y，记录棋盘状态
int blackPieceCount = 0, whitePieceCount = 0;
string blackName = "", whiteName = "";

inline bool inMap(int x, int y)
{
	if (x < 0 || x > 2 || y < 0 || y > 2)
		return false;
	return true;
}

// 在坐标处落子，检查是否合法或模拟落子
int ProcStep(int x, int y, int color) // 返回0就是出错，否则就是正确
{
	if (color == 0)
		return false;
	if (!inMap(x, y))
		return false;
	if (gridInfo[x][y] != 0)
		return false;

	if (color == 1)
		blackPieceCount++;
	else
		whitePieceCount++;
	gridInfo[x][y] = color;
	return true;
}

int chechIfWin(int gi[3][3]) { // ret val: 0 No one wins, 1/-1 black/white wins
	vector<int> sum;
	int s1 = 0, s2 = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			s1 += gi[i][j];
			s2 += gi[j][i];
		}
		sum.push_back(s1);
		sum.push_back(s2);
		s1 = 0, s2 = 0;
	}

	for (int i = 0; i < 3; i++) {
		s1 += gi[i][i];
		s2 += gi[i][2 - i];
	}
	sum.push_back(s1);
	sum.push_back(s2);

	for (auto & v : sum) {
		if (v == 3)
			return 1;
		if (v == -3)
			return -1;
	}
	return 0;
}

int main()
{
	int x, y;

	// 读入JSON
	string str;
	getline(cin, str);
	Json::Reader reader;
	Json::Value input;
	reader.parse(str, input);

	// 分析自己收到的输入和自己过往的输出，并恢复状态
	int turnID = input["responses"].size();
	currBotColor = input["requests"][(Json::Value::UInt) 0]["x"].asInt() < 0 ? 1 : -1; // 第一回合收到坐标是-1, -1，说明我是黑方
	for (int i = 0; i < turnID; i++)
	{
		// 根据这些输入输出逐渐恢复状态到当前回合
		x = input["requests"][i]["x"].asInt();
		y = input["requests"][i]["y"].asInt();
		if (x >= 0)
			ProcStep(x, y, -currBotColor); // 模拟对方落子
		x = input["responses"][i]["x"].asInt();
		y = input["responses"][i]["y"].asInt();
		if (x >= 0)
			ProcStep(x, y, currBotColor); // 模拟己方落子
	}

	// 看看自己本回合输入
	x = input["requests"][turnID]["x"].asInt();
	y = input["requests"][turnID]["y"].asInt();
	if (x >= 0)
		ProcStep(x, y, -currBotColor); // 模拟对方落子

									   // 找出合法落子点
	int possiblePos[9][2], posCount = 0, choice; // value表示的是这九个地方落子的估值

	for (y = 0; y < 3; y++)
		for (x = 0; x < 3; x++)
			if (gridInfo[x][y] == 0)
			{
				possiblePos[posCount][0] = x;
				possiblePos[posCount++][1] = y;
			}

	// 做出决策（你只需修改以下部分）
    int resultX = -1, resultY = -1;

    if (posCount > 0)
    {
	    srand(time(0));
	    choice = rand() % posCount;
	    resultX = possiblePos[choice][0];
	    resultY = possiblePos[choice][1];
    }
    //随机落子
	// 决策结束，输出结果（你只需修改以上部分）

	Json::Value ret;
	ret["response"]["x"] = resultX;
	ret["response"]["y"] = resultY;
	Json::FastWriter writer;
	cout << writer.write(ret) << endl;
	return 0;
}
