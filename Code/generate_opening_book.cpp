// 生成开局库工具脚本
#define GENERATE_OPENING_BOOK

#include "Chinese_Chess_trial_2.cpp"  // 引入现有的Chess类
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <dirent.h>  // 替代 filesystem
#include "jsoncpp/json.h"

using namespace std;

// 存储 Zobrist 值对应的着法记录
struct BookEntry {
    Move move;
    int count;
    
    // 自定义序列化
    Json::Value toJson() const {
        Json::Value val;
        val["move"] = string(1, pgnint2char(move.source_x)) + string(1, int2char(move.source_y)) + 
                      string(1, pgnint2char(move.target_x)) + string(1, int2char(move.target_y));
        val["count"] = count;
        return val;
    }
};

map<uint64_t, vector<BookEntry>> opening_book;

// 这里需要一个将中文记谱法转换为合法 Move 的函数，可根据需要补充完整逻辑
Move parseChineseNotation(Chess& chess, const string& notation, colorType color) {
    // 简化的解析桩（需用户自己提供具体中文向坐标映射逻辑）
    // 通过 chess.generateMoves 获得当面局面的所有合法着法，匹配即可
    vector<Move> legalMoves;
    chess.generateMoves(legalMoves, false);
    
    // 此处省去复杂的中文匹配，直接根据合法步等进行尝试/筛选。
    // 在真实场景中，需要解析如“炮八平五”到起点/终点坐标
    // 假设返回找到的第一步着法作为演示（实际需要根据notation正确解析）
    if (!legalMoves.empty()) return legalMoves[0];
    
    return Move(-1, -1, -1, -1);
}

void processPGN(const string& filename) {
    ifstream infile(filename);
    if (!infile.is_open()) return;
    
    string line;
    bool inGame = false;
    
    Chess chess;
    chess.resetBoard(); // 初始棋盘
    int halfMoves = 0;  // 记录半回合数，15回合对应30次半回合

    while (getline(infile, line)) {
        if (line.empty() || line[0] == '[') continue;
        
        // 解析PGN的回合行(如 "1. 炮八平五 马２进３")
        // 每行可能包含 红方 黑方的着法
        // 将中文着法分离出来
        vector<string> moves;
        size_t start = line.find(".");
        if (start != string::npos) {
            string movesPart = line.substr(start + 1);
            // 分割空格
            size_t pos = 0;
            while ((pos = movesPart.find(" ")) != string::npos) {
                if (pos > 0) {
                    moves.push_back(movesPart.substr(0, pos));
                }
                movesPart.erase(0, pos + 1);
            }
            if (!movesPart.empty()) moves.push_back(movesPart);
        }

        // 处理着法
        for (const string& moveStr : moves) {
            if (moveStr == "1-0" || moveStr == "0-1" || moveStr == "1/2-1/2") break; // 单局结束
            if (moveStr == "*") break;

            if (halfMoves >= 30) continue; // 只记录前15回合数据 (15回合=30手)

            uint64_t zkey = chess.getZobristKey();
            colorType curColor = (halfMoves % 2 == 0) ? RED : BLACK;

            Move m = parseChineseNotation(chess, moveStr, curColor);
            if (m.source_x == -1) break; // 解析失败

            // 统计频次
            bool found = false;
            for (auto& entry : opening_book[zkey]) {
                if (entry.move == m) {
                    entry.count++;
                    found = true;
                    break;
                }
            }
            if (!found) {
                opening_book[zkey].push_back({m, 1});
            }

            chess.makeMoveAssumeLegal(m); // 执行着法以进入下一状态
            halfMoves++;
        }
    }
}

int main() {
    InitZobrist(); // 初始化Zobrist哈希表的随机数 (20262026ULL)

    string pgnDir = "PGN";
    
    DIR* dir = opendir(pgnDir.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            string filename = entry->d_name;
            if (filename == "." || filename == "..") continue;
            
            // 简单通过后缀判定扩展名
            if (filename.length() >= 4 && 
                (filename.substr(filename.length() - 4) == ".pgn" || 
                 filename.substr(filename.length() - 4) == ".PGN")) {
                
                string fullPath = pgnDir + "/" + filename;
                cout << "Processing: " << fullPath << endl;
                processPGN(fullPath);
            }
        }
        closedir(dir);
    } else {
        cerr << "Failed to open directory: " << pgnDir << endl;
    }

    // 导出 opening_book.json
    Json::Value root;
    for (const auto& pair : opening_book) {
        string keyStr = to_string(pair.first);
        Json::Value movesArray(Json::arrayValue);
        for (const auto& entry : pair.second) {
            movesArray.append(entry.toJson());
        }
        root[keyStr] = movesArray;
    }

    ofstream outfile("opening_book.json");
    Json::StyledWriter writer;
    outfile << writer.write(root);
    outfile.close();

    cout << "Opening book generated successfully: opening_book.json" << endl;
    return 0;
}
