#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include "jsoncpp/json.h"

using namespace std;

int board[3][3] = { 0 }; // 0:空, 1:黑子, -1:白子

int main() {
    // 1. 读取输入
    string inputStr;
    getline(cin, inputStr);

    Json::Reader reader;
    Json::Value root;
    reader.parse(inputStr, root);

    // 2. 恢复棋盘状态
    // 根据裁判代码，input["log"] 记录了历史所有回合
    Json::Value log = root["log"];
    int myColor = (log.size() % 2 == 0) ? 1 : -1; // 偶数条记录说明该黑棋下，奇数条该白棋下

    // 解析历史步骤 (跳过第0项，因为第0项通常是空的初始请求)
    for (int i = 1; i < log.size(); ++i) {
        int color = (i % 2 == 1) ? 1 : -1; // 第1,3,5...条记录是黑棋，2,4,6...是白棋
        Json::Value action;
        
        // 尝试从 content 或 response 中获取坐标（兼容裁判的解析逻辑）
        if (color == 1)
            action = log[i]["0"]["response"].isNull() ? log[i]["0"]["content"] : log[i]["0"]["response"];
        else
            action = log[i]["1"]["response"].isNull() ? log[i]["1"]["content"] : log[i]["1"]["response"];

        if (action.isString()) {
            Json::Value temp;
            reader.parse(action.asString(), temp);
            action = temp;
        }

        if (action.isObject() && action["x"].isInt() && action["y"].isInt()) {
            board[action["x"].asInt()][action["y"].asInt()] = color;
        }
    }

    // 3. 决策逻辑：寻找可用的空位
    vector<pair<int, int>> availableMoves;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] == 0) {
                availableMoves.push_back({i, j});
            }
        }
    }

    // 选一个落子点
    int resX = -1, resY = -1;
    if (!availableMoves.empty()) {
        srand(time(0));
        int idx = rand() % availableMoves.size();
        resX = availableMoves[idx].first;
        resY = availableMoves[idx].second;
    }

    // 4. 按照裁判要求的格式输出
    Json::Value output;
    Json::Value response;
    response["x"] = resX;
    response["y"] = resY;
    output["response"] = response;

    Json::FastWriter writer;
    cout << writer.write(output) << endl;

    return 0;
}