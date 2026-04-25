// 包含标准库头文件
#include <cctype>              // 字符分类函数，用于字符类型判断
#include <fstream>             // 文件流操作，用于读写文件
#include <iostream>            // 标准输入输出流
#include <map>                 // STL map容器，用于键值对存储
#include <random>              // 随机数生成器，用于Zobrist哈希初始化
#include <sstream>             // 字符串流，用于字符串和数值转换
#include <stdexcept>           // 标准异常类
#include <string>              // STL字符串类
#include <vector>              // STL动态数组容器
#include <windows.h>           // Windows API头文件，用于文件操作和编码转换

// 定义宏以重用现有的引擎实现
// GENERATE_OPENING_BOOK宏禁用Chinese_Chess_trial_2.cpp中的原main()函数
#define GENERATE_OPENING_BOOK
// 临时暴露private/protected成员，使离线脚本能够检查棋盘并格式化合法移动到中文记谱法
#define private public
#define protected public
#include "Chinese_Chess_trial_2.cpp"  // 包含中国象棋引擎的主实现文件
#undef protected                        // 取消宏定义，恢复访问权限
#undef private

namespace  // 匿名命名空间，限制内部符号的作用域
{
// 开局库应覆盖15个完整回合 = 30步棋（每回合包含红方和黑方各一步）
constexpr int kMaxBookRounds = 15;     // 最大回合数
constexpr int kMaxBookPlies = kMaxBookRounds * 2;  // 最大步数（1回合=2步）

// Zobrist哈希种子，必须与引擎的Zobrist初始化完全匹配
// 否则导出的哈希键将与Chinese_Chess_trial_2.cpp运行时计算的键值不匹配
constexpr uint64_t kBookZobristSeed = 20262026ULL;  // 必须与Chinese_Chess_trial_2.cpp中的InitZobrist()匹配

// 用户指定的本地输入/输出路径（硬编码）
const std::string kPgnDirectory = "C:/Users/yang/Desktop/Game-strategic-algorithms/Code/PGN";  // PGN棋谱文件目录
const std::string kOutputJsonPath = "C:/Users/yang/Desktop/Game-strategic-algorithms/Code/opening_book.json";  // 输出JSON文件路径

// 在一个具体Zobrist位置下，一个具体走棋的频率记录
struct BookMoveStat  // 开局库中每一步棋的统计信息
{
    Move move;   // 走棋对象，包含起点和终点坐标
    int count = 0;  // 该走棋出现的频次计数
};

// MoveBucket：从一个Zobrist位置观察到的所有走棋
using MoveBucket = std::map<std::string, BookMoveStat>;  // 键为走棋的唯一标识，值为统计信息
// OpeningBook：完整的开局库映射
using OpeningBook = std::map<uint64_t, MoveBucket>;  // 键为Zobrist哈希值，值为该位置的所有走棋

// 全局累加器，在扫描每个PGN文件时填充数据
OpeningBook g_openingBook;  // 全局开局库数据结构中

// 去除字符串首尾空格，使后续的标记化更简单
std::string trim(const std::string &text)  // 修剪字符串两端的空白字符
{
    const std::string whitespace = " \t\r\n";  // 定义空白字符集合
    const std::size_t begin = text.find_first_not_of(whitespace);  // 查找第一个非空白字符的位置
    if (begin == std::string::npos)  // 如果全是空白字符
    {
        return "";  // 返回空字符串
    }
    const std::size_t end = text.find_last_not_of(whitespace);  // 查找最后一个非空白字符的位置
    return text.substr(begin, end - begin + 1);  // 返回修剪后的子字符串
}

// Windows目录中的PGN文件采用ANSI/GBK编码
// 在尝试分割或规范化中文记谱法之前，将每个文本行转换为UTF-8
std::string ansiToUtf8(const std::string &ansiText)  // 将ANSI/GBK编码转换为UTF-8编码
{
    if (ansiText.empty())  // 如果输入字符串为空
    {
        return "";  // 直接返回空字符串
    }

    // 调用Windows API将多字节字符串转换为宽字符字符串
    const int wideLength = MultiByteToWideChar(CP_ACP, 0, ansiText.c_str(), static_cast<int>(ansiText.size()), nullptr, 0);
    if (wideLength <= 0)  // 如果转换失败
    {
        return ansiText;  // 返回原始字符串
    }

    std::wstring wideText(static_cast<std::size_t>(wideLength), L'\0');  // 创建宽字符字符串
    MultiByteToWideChar(CP_ACP, 0, ansiText.c_str(), static_cast<int>(ansiText.size()), &wideText[0], wideLength);  // 执行转换

    // 将宽字符字符串转换为UTF-8编码
    const int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideText.c_str(), wideLength, nullptr, 0, nullptr, nullptr);
    if (utf8Length <= 0)  // 如果转换失败
    {
        return ansiText;  // 返回原始字符串
    }

    std::string utf8Text(static_cast<std::size_t>(utf8Length), '\0');  // 创建UTF-8字符串
    WideCharToMultiByte(CP_UTF8, 0, wideText.c_str(), wideLength, &utf8Text[0], utf8Text.length(), nullptr, nullptr);  // 执行转换
    return utf8Text;  // 返回转换后的UTF-8字符串
}

// 将一个UTF-8字符串分割成码点大小的子字符串
// 使用此方法逐个中文字符规范化记谱法标记
std::vector<std::string> splitUtf8(const std::string &text)  // 将UTF-8字符串分割为单个字符
{
    std::vector<std::string> result;  // 存储分割结果的容器
    for (std::size_t i = 0; i < text.size();)  // 遍历字符串
    {
        unsigned char c = static_cast<unsigned char>(text[i]);  // 获取当前字节
        std::size_t length = 1;  // 字符长度，默认为1个字节
        if ((c & 0x80U) == 0)  // 如果是ASCII字符（0xxxxxxx）
        {
            length = 1;  // ASCII字符占1个字节
        }
        else if ((c & 0xE0U) == 0xC0U)  // 如果是2字节UTF-8字符（110xxxxx）
        {
            length = 2;  // 2字节字符
        }
        else if ((c & 0xF0U) == 0xE0U)  // 如果是3字节UTF-8字符（1110xxxx）
        {
            length = 3;  // 3字节字符（中文通常为此类）
        }
        else if ((c & 0xF8U) == 0xF0U)  // 如果是4字节UTF-8字符（11110xxx）
        {
            length = 4;  // 4字节字符
        }
        if (i + length > text.size())  // 防止越界
        {
            length = 1;  // 如果长度异常，强制设为1
        }
        result.push_back(text.substr(i, length));  // 将单个字符添加到结果中
        i += length;  // 移动到下一个字符
    }
    return result;  // 返回分割后的字符数组
}

// 规范化记谱法标记，使等效的拼写仍然匹配
// 包括全角数字、中文数字、异体棋子名称等
std::string normalizeToken(const std::string &token)  // 规范化记谱法标记
{
    // 定义替换映射表，将各种等效表示统一为标准形式
    static const std::map<std::string, std::string> replacements = {
        {"１", "1"}, {"２", "2"}, {"３", "3"}, {"４", "4"}, {"５", "5"},  // 全角数字转半角
        {"６", "6"}, {"７", "7"}, {"８", "8"}, {"９", "9"},
        {"一", "1"}, {"二", "2"}, {"三", "3"}, {"四", "4"}, {"五", "5"},  // 中文数字转阿拉伯数字
        {"六", "6"}, {"七", "7"}, {"八", "8"}, {"九", "9"},
        {"壹", "1"}, {"贰", "2"}, {"叁", "3"}, {"肆", "4"}, {"伍", "5"},  // 大写中文数字转阿拉伯数字
        {"陆", "6"}, {"柒", "7"}, {"捌", "8"}, {"玖", "9"},
        {"車", "车"}, {"馬", "马"}, {"砲", "炮"}, {"將", "将"}, {"帅", "将"}, {"帥", "将"},  // 异体棋子名称统一
        {"仕", "士"}, {"相", "相"}, {"象", "相"}, {"兵", "兵"}, {"卒", "兵"},  // 棋子名称统一
        {"後", "后"}, {"進", "进"}, {"退", "退"}, {"平", "平"}};  // 繁体动作字转简体

    std::string normalized;  // 存储规范化后的字符串
    for (const std::string &cp : splitUtf8(token))  // 遍历每个字符
    {
        auto it = replacements.find(cp);  // 查找是否需要替换
        if (it != replacements.end())  // 如果找到替换项
        {
            normalized += it->second;  // 添加替换后的字符
        }
        else if (cp != " " && cp != "\t" && cp != "\r" && cp != "\n")  // 如果不是空白字符
        {
            normalized += cp;  // 直接添加原字符
        }
    }
    return normalized;  // 返回规范化后的字符串
}

// 将引擎的棋子枚举转换为本脚本使用的规范记谱法名称
std::string pieceToCanonicalName(stoneType type)  // 棋子类型转换为规范名称
{
    switch (type)  // 根据棋子类型返回中文名称
    {
    case King:     // 将/帅
        return "将";
    case Bishop:   // 相/象
        return "相";
    case Knight:   // 马
        return "马";
    case Rook:     // 车
        return "车";
    case Pawn:     // 兵/卒
        return "兵";
    case Cannon:   // 炮
        return "炮";
    case Assistant:  // 士/仕
        return "士";
    default:       // 未知类型
        return "*";
    }
}

// 将棋盘文件x坐标转换为走棋方视角的文件编号
std::string fileToCanonicalNumber(int x, colorType color)  // 坐标x转换为规范文件编号
{
    const int value = (color == RED) ? (9 - x) : (x + 1);  // 红方从右到左为1-9，黑方从左到右为1-9
    return std::to_string(value);  // 返回文件编号字符串
}

// 判断走棋是否从走棋方的视角"向前"移动
bool isForwardMove(colorType color, int sourceY, int targetY)  // 判断是否为前进走棋
{
    return (color == RED) ? (targetY > sourceY) : (targetY < sourceY);  // 红方Y坐标增大为前进，黑方Y坐标减小为前进
}

// 当两个或更多相同棋子在同一列时，需要前后中样式消除歧义
bool isMoreFront(colorType color, int lhsY, int rhsY)  // 判断左侧棋子是否比右侧棋子更靠前
{
    return (color == RED) ? (lhsY > rhsY) : (lhsY < rhsY);  // 红方Y坐标大者靠前，黑方Y坐标小者靠前
}

// 将顺序索引转换为标签，如前/中/后
std::string disambiguationLabel(std::size_t index, std::size_t total)  // 生成消除歧义的标签
{
    if (total == 2)  // 如果只有两个相同棋子
    {
        return (index == 0) ? "前" : "后";  // 第一个为"前"，第二个为"后"
    }
    if (total == 3)  // 如果有三个相同棋子
    {
        static const char *labels[] = {"前", "中", "后"};  // 定义前中后标签
        return labels[index];  // 返回对应标签
    }
    if (index == 0)  // 如果有更多棋子，第一个为"前"
    {
        return "前";
    }
    if (index + 1 == total)  // 最后一个为"后"
    {
        return "后";
    }
    return std::to_string(index + 1);  // 中间的用数字编号
}

// 为一步走棋创建稳定键，以便用作std::map的键
std::string moveKey(const Move &move)  // 生成走棋的唯一标识键
{
    std::ostringstream oss;  // 创建字符串流
    oss << move.source_x << ',' << move.source_y << ','   // 起点坐标
        << move.target_x << ',' << move.target_y;         // 终点坐标
    return oss.str();  // 返回坐标字符串，如"0,0,0,2"
}

// 将走棋转换为坐标文本（如"b2e2"），用于调试和JSON输出
std::string moveToCoordinateString(const Move &move)  // 走棋转换为坐标字符串
{
    return std::string(1, pgnint2char(move.source_x)) + std::string(1, int2char(move.source_y)) +  // 起点坐标
           std::string(1, pgnint2char(move.target_x)) + std::string(1, int2char(move.target_y));   // 终点坐标
}

// 在需要时用'/'连接两个路径片段
std::string joinPath(const std::string &lhs, const std::string &rhs)  // 连接路径字符串
{
    if (lhs.empty())  // 如果左侧路径为空
    {
        return rhs;  // 直接返回右侧路径
    }
    if (lhs.back() == '/' || lhs.back() == '\\')  // 如果左侧路径已以分隔符结尾
    {
        return lhs + rhs;  // 直接连接
    }
    return lhs + "/" + rhs;  // 添加分隔符后连接
}

// 不区分大小写的后缀检查，使".pgn"和".PGN"都被接受
bool endsWithIgnoreCase(const std::string &text, const std::string &suffix)  // 不区分大小写检查后缀
{
    if (text.size() < suffix.size())  // 如果文本短于后缀
    {
        return false;  // 不可能匹配
    }

    const std::size_t offset = text.size() - suffix.size();  // 计算后缀起始位置
    for (std::size_t i = 0; i < suffix.size(); ++i)  // 逐个字符比较
    {
        const unsigned char lhs = static_cast<unsigned char>(text[offset + i]);  // 文本中的字符
        const unsigned char rhs = static_cast<unsigned char>(suffix[i]);          // 后缀中的字符
        if (std::tolower(lhs) != std::tolower(rhs))  // 不区分大小写比较
        {
            return false;  // 不匹配
        }
    }
    return true;  // 匹配成功
}

// 从配置的本地文件夹枚举所有PGN文件
// 这里使用Win32 API而不是std::filesystem，以获得更广泛的编译器兼容性
std::vector<std::string> listPgnFiles(const std::string &directory)  // 列出目录中所有PGN文件
{
    std::vector<std::string> files;  // 存储文件列表
    WIN32_FIND_DATAA findData;  // Windows查找文件数据结构
    const std::string searchPattern = joinPath(directory, "*");  // 搜索模式（所有文件）
    HANDLE handle = FindFirstFileA(searchPattern.c_str(), &findData);  // 开始查找
    if (handle == INVALID_HANDLE_VALUE)  // 如果查找失败
    {
        return files;  // 返回空列表
    }

    do  // 遍历所有文件
    {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)  // 如果是目录
        {
            continue;  // 跳过目录
        }

        const std::string filename = findData.cFileName;  // 获取文件名
        if (endsWithIgnoreCase(filename, ".pgn"))  // 如果是以.pgn结尾的文件（不区分大小写）
        {
            files.push_back(joinPath(directory, filename));  // 添加到文件列表
        }
    } while (FindNextFileA(handle, &findData));  // 继续查找下一个文件

    FindClose(handle);  // 关闭查找句柄
    std::sort(files.begin(), files.end());  // 按文件名排序
    return files;  // 返回文件列表
}

// 重建与主引擎完全相同的Zobrist随机表
// 如果这个表不同，导出的开局库键将与运行时查找不匹配
void initBookZobrist()  // 初始化Zobrist哈希表
{
    std::mt19937_64 randomnum(kBookZobristSeed);  // 使用固定种子创建随机数生成器
    for (int type = 0; type < 8; ++type)  // 遍历所有棋子类型
    {
        for (int color = 0; color < 2; ++color)  // 遍历红黑两方
        {
            for (int pos = 0; pos < 90; ++pos)  // 遍历棋盘所有90个位置
            {
                ZobristTable[type][color][pos] = randomnum();  // 为每个棋子位置生成随机哈希值
            }
        }
    }
    ZobristPlayer = randomnum();  // 为当前走棋方生成随机哈希值
}

// 将一步合法引擎走棋转换回当前棋盘下的标准中文记谱法
// 这是脚本的核心解析技巧：
// 1. 从当前位置生成所有合法走棋
// 2. 将每个合法走棋格式化为中文记谱法
// 3. 将格式化后的字符串与PGN标记进行比较
// 将一步合法引擎走棋转换为PGN文件中出现的确切中文记谱法样式
// 解析变为"生成所有合法走棋并字符串匹配"
std::string formatMoveNotation(const Chess &chess, const Move &move)  // 格式化走棋为记谱法
{
    const Grid &sourceGrid = chess.state[chess.currTurnId][Chess::xy2pos(move.source_x, move.source_y)];  // 获取起点格子信息
    if (sourceGrid.color == EMPTY || sourceGrid.type == None)  // 如果起点为空
    {
        throw std::runtime_error("move source square is empty");  // 抛出异常
    }

    std::string prefix;  // 记谱法前缀（棋子名称+位置）
    std::vector<int> sameFileYs;  // 存储同一列中相同棋子的Y坐标
    for (int y = 0; y < BOARDHEIGHT; ++y)  // 遍历该列所有行
    {
        const Grid &grid = chess.state[chess.currTurnId][Chess::xy2pos(move.source_x, y)];  // 获取该位置格子
        if (grid.color == sourceGrid.color && grid.type == sourceGrid.type)  // 如果是相同颜色和类型的棋子
        {
            sameFileYs.push_back(y);  // 添加到列表
        }
    }

    if (sameFileYs.size() >= 2)  // 如果同一列有超过一个相同棋子
    {
        std::sort(sameFileYs.begin(), sameFileYs.end(), [&](int lhs, int rhs) {  // 按前后顺序排序
            return isMoreFront(sourceGrid.color, lhs, rhs);  // 根据走棋方判断前后
        });

        std::size_t pieceIndex = sameFileYs.size();  // 初始化棋子索引
        for (std::size_t i = 0; i < sameFileYs.size(); ++i)  // 查找当前走棋的棋子索引
        {
            if (sameFileYs[i] == move.source_y)  // 找到匹配的Y坐标
            {
                pieceIndex = i;  // 记录索引
                break;  // 退出循环
            }
        }
        if (pieceIndex == sameFileYs.size())  // 如果未找到
        {
            throw std::runtime_error("failed to locate moving piece for notation");  // 抛出异常
        }
        prefix = disambiguationLabel(pieceIndex, sameFileYs.size()) + pieceToCanonicalName(sourceGrid.type);  // 生成带前后标识的前缀
    }
    else  // 如果只有一个该类型棋子
    {
        prefix = pieceToCanonicalName(sourceGrid.type) + fileToCanonicalNumber(move.source_x, sourceGrid.color);  // 生成"棋子+列号"前缀
    }

    if (move.target_y == move.source_y)  // 如果是平移（横走）
    {
        return prefix + "平" + fileToCanonicalNumber(move.target_x, sourceGrid.color);  // 返回"前缀+平+目标列号"
    }

    const bool forward = isForwardMove(sourceGrid.color, move.source_y, move.target_y);  // 判断是否为前进
    const std::string action = forward ? "进" : "退";  // 确定动作是"进"还是"退"

    if (sourceGrid.type == Rook || sourceGrid.type == Cannon || sourceGrid.type == King || sourceGrid.type == Pawn)  // 如果是车、炮、将、兵
    {
        return prefix + action + std::to_string(std::abs(move.target_y - move.source_y));  // 返回"前缀+动作+步数"
    }
    return prefix + action + fileToCanonicalNumber(move.target_x, sourceGrid.color);  // 返回"前缀+动作+目标列号"（适用于马、相、士）
}

// 读取一行PGN并仅保留走棋标记，如"炮八平五"和"马２进３"
// 忽略标题行、空行和页脚文本
std::vector<std::string> extractMoveTokens(const std::string &line)  // 从行中提取走棋标记
{
    std::vector<std::string> tokens;  // 存储走棋标记
    std::string cleaned = trim(line);  // 去除首尾空格
    if (cleaned.empty() || cleaned.front() == '[')  // 如果是空行或以'['开头（PGN标题行）
    {
        return tokens;  // 返回空列表
    }
    if (!std::isdigit(static_cast<unsigned char>(cleaned.front())))  // 如果行首不是数字（不是走棋行）
    {
        return tokens;  // 返回空列表
    }

    std::istringstream iss(cleaned);  // 创建字符串流
    std::string token;  // 存储单个标记
    while (iss >> token)  // 读取每个标记
    {
        if (!token.empty() && token.back() == '.')  // 如果是回合数字（如"1."）
        {
            continue;  // 跳过
        }
        if (token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*")  // 如果是对局结果标记
        {
            break;  // 停止读取
        }
        tokens.push_back(token);  // 添加走棋标记
    }
    return tokens;  // 返回走棋标记列表
}

// 在当前棋盘状态下解析一个中文记谱法标记
// 这不是直接从文本猜测，而是将标记与formatMoveNotation()格式化的每个合法走棋进行比较
Move parseChineseMove(const Chess &chess, const std::string &moveToken)  // 解析中文记谱法
{
    const std::string normalizedTarget = normalizeToken(moveToken);  // 规范化目标标记
    std::vector<Move> legalMoves;  // 存储所有合法走棋
    Chess boardCopy = chess;  // 复制当前棋盘状态
    boardCopy.generateMoves(legalMoves, true);  // 生成所有合法走棋

    Move matchedMove;  // 存储匹配的走棋
    int matchCount = 0;  // 匹配计数
    for (const Move &candidate : legalMoves)  // 遍历所有合法走棋
    {
        const std::string normalizedCandidate = normalizeToken(formatMoveNotation(chess, candidate));  // 格式化并规范化候选走棋
        if (normalizedCandidate == normalizedTarget)  // 如果匹配
        {
            matchedMove = candidate;  // 记录匹配的走棋
            ++matchCount;  // 增加匹配计数
        }
    }

    if (matchCount == 1)  // 如果只有一个匹配
    {
        return matchedMove;  // 返回该走棋
    }

    // 零匹配意味着无法在此位置理解该标记
    // 多个匹配意味着格式化器/规范化器仍然有歧义
    std::ostringstream oss;  // 创建字符串流
    oss << "failed to parse move token \"" << moveToken << "\"";  // 构建错误信息
    if (matchCount > 1)  // 如果多个匹配
    {
        oss << " because it matches multiple legal moves";  // 添加歧义说明
    }
    throw std::runtime_error(oss.str());  // 抛出异常
}

// 为观察到的一个（位置，走棋）对更新全局开局库映射
void recordMove(uint64_t zobristKey, const Move &move)  // 记录走棋到开局库
{
    MoveBucket &bucket = g_openingBook[zobristKey];  // 获取该Zobrist键对应的走棋桶
    BookMoveStat &stat = bucket[moveKey(move)];  // 获取该走棋的统计信息
    if (stat.count == 0)  // 如果是第一次出现该走棋
    {
        stat.move = move;  // 记录走棋信息
    }
    ++stat.count;  // 增加计数
}

// 从初始位置重放一个PGN文件，但只保留前15回合
// 对于每步走棋，按要求在走棋之前哈希当前位置
void processPgnFile(const std::string &filePath)  // 处理单个PGN文件
{
    std::ifstream input(filePath);  // 打开PGN源文件
    if (!input.is_open())  // 如果打开失败
    {
        throw std::runtime_error("cannot open PGN file: " + filePath);  // 抛出异常
    }

    Chess chess;  // 创建棋盘对象，初始化为标准开局
    int plyCount = 0;  // 步数计数（而非回合数），因为逐个处理标记
    std::string line;  // 存储读取的行

    while (std::getline(input, line))  // 逐行读取文件
    {
        line = ansiToUtf8(line);  // 将本地ANSI/GBK字节转换为UTF-8
        const std::vector<std::string> moveTokens = extractMoveTokens(line);  // 提取走棋标记
        for (const std::string &token : moveTokens)  // 处理每个走棋标记
        {
            if (plyCount >= kMaxBookPlies)  // 如果已达到最大步数（15回合=30步）
            {
                return;  // 停止处理
            }

            const Move move = parseChineseMove(chess, token);  // 解析中文记谱法为走棋对象
            if (!chess.isLegalMove(move))  // 如果走棋不合法
            {
                throw std::runtime_error("parsed move is not legal: " + token);  // 抛出异常
            }

            const uint64_t zobristKey = chess.getZobristKey();  // 重要：在走棋之前记录当前位置的哈希值
            recordMove(zobristKey, move);  // 记录走棋到开局库
            chess.makeMoveAssumeLegal(move);  // 使用与游戏/搜索相同的引擎函数推进棋盘
            ++plyCount;  // 增加步数计数
        }
    }
}

// 将一个聚合的走棋频率记录转换为写入磁盘的JSON格式
Json::Value moveToJson(const BookMoveStat &stat)  // 将走棋统计转换为JSON
{
    Json::Value item;  // 创建JSON对象
    item["count"] = stat.count;  // 设置出现次数
    item["move"]["source_x"] = stat.move.source_x;  // 起点X坐标
    item["move"]["source_y"] = stat.move.source_y;  // 起点Y坐标
    item["move"]["target_x"] = stat.move.target_x;  // 终点X坐标
    item["move"]["target_y"] = stat.move.target_y;  // 终点Y坐标
    item["move"]["coord"] = moveToCoordinateString(stat.move);  // 坐标字符串（如"b2e2"）
    item["move"]["source"] = std::string(1, pgnint2char(stat.move.source_x)) + std::string(1, int2char(stat.move.source_y));  // 起点坐标（如"b2"）
    item["move"]["target"] = std::string(1, pgnint2char(stat.move.target_x)) + std::string(1, int2char(stat.move.target_y));  // 终点坐标（如"e2"）
    return item;  // 返回JSON对象
}

// 将整个开局库映射序列化为opening_book.json
// JSON键是Zobrist值的十进制字符串，因为对象键必须是字符串
void exportOpeningBook()  // 导出开局库到JSON文件
{
    Json::Value root;  // 创建根JSON对象
    for (const auto &entry : g_openingBook)  // 遍历开局库中的每个位置
    {
        std::vector<BookMoveStat> sortedMoves;  // 存储排序后的走棋
        for (const auto &kv : entry.second)  // 遍历该位置的所有走棋
        {
            sortedMoves.push_back(kv.second);  // 添加到向量
        }

        // 按频次降序排序，频次相同时按走棋键排序以保证确定性输出
        std::sort(sortedMoves.begin(), sortedMoves.end(), [](const BookMoveStat &lhs, const BookMoveStat &rhs) {
            if (lhs.count != rhs.count)  // 如果频次不同
            {
                return lhs.count > rhs.count;  // 频次高的在前
            }
            return moveKey(lhs.move) < moveKey(rhs.move);  // 频次相同时按走棋键排序
        });

        Json::Value moves(Json::arrayValue);  // 创建JSON数组
        for (const BookMoveStat &stat : sortedMoves)  // 遍历排序后的走棋
        {
            moves.append(moveToJson(stat));  // 添加走棋的JSON表示
        }
        root[std::to_string(entry.first)] = moves;  // JSON对象键必须是字符串，因此将uint64哈希转换为字符串
    }

    std::ofstream output(kOutputJsonPath);  // 打开目标文件
    if (!output.is_open())  // 如果打开失败
    {
        throw std::runtime_error("cannot write output json: " + kOutputJsonPath);  // 抛出异常
    }

    Json::StyledWriter writer;  // 创建JSON格式化写入器
    output << writer.write(root);  // 写入格式化的JSON到文件
}

} // namespace（匿名命名空间结束）

// 程序入口点：
// 1. 验证PGN目录是否存在
// 2. 重建引擎的Zobrist表
// 3. 扫描每个PGN文件
// 4. 将聚合的开局库导出为JSON
int main()  // 主函数
{
    try  // 尝试执行主要逻辑
    {
        // 如果本地PGN目录缺失，尽早失败
        const DWORD attributes = GetFileAttributesA(kPgnDirectory.c_str());  // 获取目录属性
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)  // 如果目录不存在
        {
            throw std::runtime_error("PGN directory does not exist: " + kPgnDirectory);  // 抛出异常
        }

        initBookZobrist();  // 构建getZobristKey()所需的精确Zobrist随机表

        const std::vector<std::string> pgnFiles = listPgnFiles(kPgnDirectory);  // 枚举目标目录下的每个PGN文件
        int fileCount = 0;  // 文件计数
        for (const std::string &filePath : pgnFiles)  // 遍历每个PGN文件
        {
            const std::size_t slashPos = filePath.find_last_of("/\\");  // 查找路径分隔符位置
            const std::string filename = (slashPos == std::string::npos) ? filePath : filePath.substr(slashPos + 1);  // 提取文件名
            std::cout << "Processing " << filename << std::endl;  // 打印处理进度
            processPgnFile(filePath);  // 重放此对局文件并将其开局数据合并到全局映射
            ++fileCount;  // 增加文件计数
        }

        exportOpeningBook();  // 所有文件合并后，导出最终的JSON开局库
        std::cout << "Processed " << fileCount << " PGN files." << std::endl;  // 打印处理文件数
        std::cout << "Opening book written to " << kOutputJsonPath << std::endl;  // 打印输出路径
        return 0;  // 成功返回0
    }
    catch (const std::exception &ex)  // 捕获所有标准异常
    {
        std::cerr << "getchessbook failed: " << ex.what() << std::endl;  // 打印错误信息到标准错误
        return 1;  // 失败返回1
    }
}
