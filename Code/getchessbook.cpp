// 离线开局库生成器使用的标准库头文件。
#include <cctype>      // 包含字符处理函数，如判断数字
#include <fstream>     // 包含文件输入输出流类，用于读写文件
#include <iostream>    // 包含标准输入输出流
#include <map>         // 包含红黑树映射容器，用于存储开局库键值对
#include <random>      // 包含随机数生成器，用于Zobrist哈希
#include <sstream>     // 包含字符串流，用于拆分字符串
#include <stdexcept>   // 包含标准异常类，用于错误处理
#include <string>      // 包含字符串类
#include <vector>      // 包含动态数组容器
#include <windows.h>   // 包含Windows API，用于处理文件搜索和编码转换

// 直接复用现有的引擎实现。
// GENERATE_OPENING_BOOK 会禁用 Chinese_Chess_trial_2.cpp 中原本运行时的 main() 函数。
#define GENERATE_OPENING_BOOK // 定义宏，控制源码编译行为，避免多个main函数冲突
// 临时将 private/protected 成员暴露为 public，以便离线脚本能够在不修改原类的情况下，
// 检查棋盘并将合法的走法格式化回中文记谱法。
#define private public   // 宏替换，打破类的封装性以便外部访问私有成员
#define protected public // 宏替换，打破类的封装性以便外部访问保护成员
#include "Chinese_Chess_trial_2.cpp" // 直接包含引擎源码以复用其逻辑
#undef protected         // 恢复protected的原始语义
#undef private           // 恢复private的原始语义

namespace // 匿名命名空间，限制内部全为本文件局部可见，避免符号冲突
{
// 开局库应涵盖前 15 回合 = 30 步。
constexpr int kMaxBookRounds = 15; // 定义常量，最大开局库记录回合数
constexpr int kMaxBookPlies = kMaxBookRounds * 2; // 计算对应的半回合也就是总步数
// 该种子必须与引擎的 Zobrist 初始化完全匹配，否则导出的哈希键
// 将与运行时 Chinese_Chess_trial_2.cpp 计算的键不匹配。
constexpr uint64_t kBookZobristSeed = 20262026ULL; // 必须匹配 Chinese_Chess_trial_2.cpp 中的 InitZobrist()，保证哈希一致性

// 用户请求的硬编码本地输入/输出路径。
const std::string kPgnDirectory = "C:/Users/yang/Desktop/Game-strategic-algorithms/Code/PGN"; // PGN文件的读取目录
const std::string kOutputJsonPath = "C:/Users/yang/Desktop/Game-strategic-algorithms/Code/opening_book.json"; // 最终生成的开局库JSON路径

// 在一个具体 Zobrist 局面下的一种具体走法的频率记录。
struct BookMoveStat // 定义结构体用于统计某一步的走法和频率
{
    Move move;      // 引擎定义的走法结构体对象
    int count = 0;  // 记录该走法在不同对局中出现的次数
};

// 走法桶：在一个 Zobrist 局面下观察到的所有走法。
using MoveBucket = std::map<std::string, BookMoveStat>; // 为长类型起别名，以走法字符串为键
// 开局库：完整的开局库映射。
using OpeningBook = std::map<uint64_t, MoveBucket>; // 以局面Zobrist哈希为键，映射出所有可能走法及其频率

// 全局累加器，在扫描每个 PGN 文件时填充。
OpeningBook g_openingBook; // 声明全局变量存储整棵开局哈希树

// 去除首尾的空格，使后续的分词变得更简单。
std::string trim(const std::string &text) // 声明trim函数，接收要处理的常量引用字符串
{
    const std::string whitespace = " \t\r\n"; // 定义可能作为空白符号的所有字符集
    const std::size_t begin = text.find_first_not_of(whitespace); // 查找到首个不是上述留白的有效字符位
    if (begin == std::string::npos) // 若整个字符串全是空白
    {
        return ""; // 则直接返回空串
    }
    const std::size_t end = text.find_last_not_of(whitespace); // 查找到最后一个不是留白的有效字符位
    return text.substr(begin, end - begin + 1); // 根据定位好的首尾坐标截取有效部分返回
}

// Windows 上的这些目录下的 PGN 文件一般为 ANSI/GBK 编码。
// 在我们分割和标准化中文记谱前，将每行文本转换为 UTF-8 编码。
std::string ansiToUtf8(const std::string &ansiText) // ANSI到UTF8的转换器，参数为源本文字符串
{
    if (ansiText.empty()) // 判断文本是否为空
    {
        return ""; // 若空则什么都不转换返回
    }

    const int wideLength = MultiByteToWideChar(CP_ACP, 0, ansiText.c_str(), static_cast<int>(ansiText.size()), nullptr, 0); // 假运算以计算目标所需宽字符缓冲区长
    if (wideLength <= 0) // 若转换长度不符合逻辑出错
    {
        return ansiText; // 那么我们退回到直接丢弃原样处理不影响程序的稳定性
    }

    std::wstring wideText(static_cast<std::size_t>(wideLength), L'\0'); // 定义与结果相同长度的全宽字符串并清空
    MultiByteToWideChar(CP_ACP, 0, ansiText.c_str(), static_cast<int>(ansiText.size()), &wideText[0], wideLength); // 实际执行转换ANSI至宽字符

    const int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideText.c_str(), wideLength, nullptr, 0, nullptr, nullptr); // 接着再假换算至UTF8形式算出长
    if (utf8Length <= 0) // 同样的，判断是不是无法换算长度有误
    {
        return ansiText; // 如果出错的话保护程序安全返回未经二次操作原文
    }

    std::string utf8Text(static_cast<std::size_t>(utf8Length), '\0'); // 定义长与结果一样的长UTF8结果带0缓存
    WideCharToMultiByte(CP_UTF8, 0, wideText.c_str(), wideLength, &utf8Text[0], utf8Length, nullptr, nullptr); // 实弹操作执行宽字符至真正UTF8目标内存上
    return utf8Text; // 把安全无误完成全过程流转完的中文字符串传出去
}

// 将 UTF-8 字符串分割为单个字符组成的子字符串。
// 因为我们需要逐个中文字符标准化记谱。
std::vector<std::string> splitUtf8(const std::string &text) // 输入为UTF8原始单行本
{
    std::vector<std::string> result; // 存放切割好单字中文字段与其它标识的载体结构
    for (std::size_t i = 0; i < text.size();) // 用迭代器指针位置移动遍历整串记录文本字符串
    {
        unsigned char c = static_cast<unsigned char>(text[i]); // 转拿文本在此序列索引定位游标第一字节值出来看特征
        std::size_t length = 1; // 假使长度只有为普通英语的单占字符长处理
        if ((c & 0x80U) == 0) // 如首字节与下128比较为假，即代表 ASCII 单个标准单字节的字符的情况
        {
            length = 1; // 则该字符仅仅占据了长的一位长，不更改跳长参数
        }
        else if ((c & 0xE0U) == 0xC0U) // 若是符合带控制前项特征为占位用宽二字节
        {
            length = 2; // 就说明是2字节大小的一个宽文字，赋长值跳幅准备跨字节
        }
        else if ((c & 0xF0U) == 0xE0U) // 如果它前缀带有为特征占三字节的一个编码首表示结构位时
        {
            length = 3; // 常规使用的汉字的多数落脚这三种特征区之内
        }
        else if ((c & 0xF8U) == 0xF0U) // 或四位字节极偏门语言区的编码宽情况标志头
        {
            length = 4; // 这个极少但是要为了保持UTF防错也支持给占到了四大字节跳法
        }
        if (i + length > text.size()) // 出于保险，比如最后面残缺字符被截没导致长跑超的情况需要补漏
        {
            length = 1; // 如果真是被拦腰截断了那就防崩溃只能退回到强制认为这个乱码当单字吞了跳走
        }
        result.push_back(text.substr(i, length)); // 将完整这几个宽长度从原文切出来压在拆字返回总串最后尾部
        i += length; // 根据刚才判别到它包含的宽长来推进跨越跳开去审阅判定解析下一个字的头部情况
    }
    return result; // 将被逐一切成单独汉字宽字节分字独立后合规的文本装入集合返走交付后层标准化接手
}

// 标准化记谱字符，确保等价的变体能一致匹配：
// 比如全角数字、汉字数字，以及异形棋子变种字。
std::string normalizeToken(const std::string &token) // 给入已经被切割好的单个或连组文本结构标记
{
    static const std::map<std::string, std::string> replacements = { // 把常见易错被各大平台变乱写的记录习惯建静态统转表
        {"１", "1"}, {"２", "2"}, {"３", "3"}, {"４", "4"}, {"５", "5"}, // 将带全宽中式阿拉伯一到九等字符纠成普通码
        {"６", "6"}, {"７", "7"}, {"８", "8"}, {"９", "9"},              // 将带全宽中式阿拉伯六到九等字符纠成普通码
        {"一", "1"}, {"二", "2"}, {"三", "3"}, {"四", "4"}, {"五", "5"}, // 把标准纯汉字小写表达直接打转变数字以便于同向
        {"六", "6"}, {"七", "7"}, {"八", "8"}, {"九", "9"},              // 把标准纯汉字小写表达直接打转变数字以便于同向
        {"壹", "1"}, {"贰", "2"}, {"叁", "3"}, {"肆", "4"}, {"伍", "5"}, // 大部分老版古风对弈的甚至是大写汉文计数组记方式
        {"陆", "6"}, {"柒", "7"}, {"捌", "8"}, {"玖", "9"},              // 大部分老版古风对弈的甚至是大写汉文计数组记方式六至九
        {"車", "车"}, {"馬", "马"}, {"砲", "炮"}, {"將", "将"}, {"帅", "将"}, {"帥", "将"}, // 各常见混用古繁和将帅异名通化纠查匹配
        {"仕", "士"}, {"相", "相"}, {"象", "相"}, {"兵", "兵"}, {"卒", "兵"},               // 黑红对应兵士变体与各自叫法不同的棋类化一转同
        {"後", "后"}, {"進", "进"}, {"退", "退"}, {"平", "平"}};                            // 包括行子指代方向等进或退与繁体统一翻译规范处理

    std::string normalized; // 接收重整规范化处理后生成的准确对应记谱组合容器字符串
    for (const std::string &cp : splitUtf8(token)) // 使用迭代方式把刚刚已经切过分汉字块函数里的单字块取出流历处理
    {
        auto it = replacements.find(cp); // 把从上面取单中文宽字符当键上静态的规定规范统一表格去按键映射查值
        if (it != replacements.end()) // 根据返回值指针位置是否未超映射末尾，即判这字符是不是变构规范记录错构内容内需改
        {
            normalized += it->second; // 将查到的被修正对的统文记法累加上我们要还给输出组合的内容后位上
        }
        else if (cp != " " && cp != "\t" && cp != "\r" && cp != "\n") // 如没有异构改法，查它是不是其实是被塞给我们的控制隐符之属的空位之类
        {
            normalized += cp; // 否则确信它是其它正确有效不待转化变规范的字符本原存在它本身如车马加在字上组合
        }
    }
    return normalized; // 把完全把一切异常习惯和不同文本流记棋本源文本全部规束转换后能用的标准识别字符串送出给下一步比较分析
}

// 将引擎内部的棋子枚举类型转成本脚本使用的一般记谱规则中的中文字符形式。
std::string pieceToCanonicalName(stoneType type) // 给这游戏内部定义的兵种代码进行翻译
{
    switch (type) // 按类型枚举分支
    {
    case King:    // 如果是老将或统帅标志
        return "将"; // 直接全部认做这个叫将
    case Bishop:  // 如果是相或是象
        return "相"; // 也统一在提取的时候翻译为这个字相
    case Knight:  // 这马子不需要区别红黑
        return "马"; // 直还翻译马
    case Rook:    // 车不需要区别叫法
        return "车"; // 同上转车
    case Pawn:    // 给小卒子进行辨别
        return "兵"; // 也不区分叫法统一处理兵方便对比
    case Cannon:  // 如果这枚举说是火炮或者有石炮之类
        return "炮"; // 记谱也强制统叫做炮
    case Assistant: // 给卫士翻译
        return "士"; // 翻译为士
    default:      // 出错或未知空的情况下
        return "*";  // 返回星号这个通配不合规字符以防止后续出非可理解意外崩溃
    }
}

// 将棋盘所在列（x 坐标）换算为此方移动视角的列数表示。
std::string fileToCanonicalNumber(int x, colorType color) // 给入数组内X坐标跟颜色求这个人在记谱棋盘几路位置
{
    const int value = (color == RED) ? (9 - x) : (x + 1); // 按由于红方是从右往左数路，黑是反着左到右算所以计算各方的坐标记数字
    return std::to_string(value); // 给这个已经翻译出来的对方视角所在的棋局横数组行算完还成一个字符串
}

// 判断该走步相对于当前执子的视角上是否是“向前”的方向。
bool isForwardMove(colorType color, int sourceY, int targetY) // 取这个落子操作看他到底是进子或是退的
{
    return (color == RED) ? (targetY > sourceY) : (targetY < sourceY); // 用对于红方红盘Y轴是否大过原位置来断定否则对黑方判断也是反之算Y增加或减
}

// 用于应对同列有两个或多个同类棋子，按进退视角判别“前/中/后”的需求。
bool isMoreFront(colorType color, int lhsY, int rhsY) // 判断同兵种如双车时哪个棋比较在这走棋方偏前一点
{
    return (color == RED) ? (lhsY > rhsY) : (lhsY < rhsY); // 因为阵势是对向的所以判红谁的Y值坐标比较靠红的对面Y就大算前，黑也是反反面朝前看算前
}

// 将其下标转换为前/中/后等标识字符。
std::string disambiguationLabel(std::size_t index, std::size_t total) // 用在同位置并连有同类型棋需要区分名字避免不知是那个的同子歧义解重时处理
{
    if (total == 2)  // 最最常见就同类有两只棋重在同一直阵线上
    {
        return (index == 0) ? "前" : "后"; // 按上面判断在数组里是属于打头的还是靠身后面的还回来这个前或者是后的字符中文字
    }
    if (total == 3)  // 小兵特有并直3行可能
    {
        static const char *labels[] = {"前", "中", "后"}; // 弄一个固定三个位置的前中后中文字库直接供其在数组位置拿走字
        return labels[index]; // 提供拿那个特定中文字块来拼接使用解并直3兵
    }
    if (index == 0) // 并多4到5兵特首
    {
        return "前"; // 这时特地如果第一必然返回前兵之称
    }
    if (index + 1 == total) // 并4或5小兵的时候它正数倒着是排到了最位底部情况
    {
        return "后"; // 这个时候因为在末底定死返回为后兵的名称标识
    }
    return std::to_string(index + 1); // 若是有好四五兵以上同直列不是首尾只能说用几兵去作数字拼名称传还了
}

// 稳定地将一次走法输出为特定键格式以做 std::map 键。
std::string moveKey(const Move &move) // 用于给树或者表作散列用的字符串方法格式化转换
{
    std::ostringstream oss; // 起一个处理字符转换文本数据流用的实例类
    oss << move.source_x << ',' << move.source_y << ',' // 把此步原本坐标原X和Y写入并在它间用一个标点逗留作为定界符号连接着
        << move.target_x << ',' << move.target_y; // 继续将落地走步的靶标去处XY填装在其结尾上生成结构定列文字段
    return oss.str(); // 将此刚才拼成能用于比较唯一特地步骤的字符信息提交出去作标识返回
}

// 将 Move 导出到形如 "b2e2" 的字符串坐标，便于打印调试或存入 JSON 中。
std::string moveToCoordinateString(const Move &move) // 这是生成那种机器读取或国际用的坐标字母走谱系统表示以给记录存取或者检查用
{
    return std::string(1, pgnint2char(move.source_x)) + std::string(1, int2char(move.source_y)) + // 先转起点盘列的X由字形表变成A到I对应字母再合并对应起位置原本整数Y变为阿拉伯表数字码
           std::string(1, pgnint2char(move.target_x)) + std::string(1, int2char(move.target_y)); // 利用同样转字盘算的方式和上面同样拼接这目标走法落下点的去处那栏并整个组起来变成字符坐标记位词
}

// 拼接两个文件路径目录/名字字符串时自动带入'/'。
std::string joinPath(const std::string &lhs, const std::string &rhs) // 提供针对系统的拼接地址字符的帮助性的小方法
{
    if (lhs.empty()) // 这个被加上头如果是全空的根本就没东西
    {
        return rhs; // 直接抛出仅有后面路径名的数据
    }
    if (lhs.back() == '/' || lhs.back() == '\\') // 这被挂上的前缀结尾其实就是有系统分化地址符的话它不用自己加
    {
        return lhs + rhs; // 顺着它它已经有了斜或者反斜线的地址字符跟后来接着给的文件之名自然合在一块儿就可以组好
    }
    return lhs + "/" + rhs; // 上述都不是的情况那么需要强塞着给他个合法系统横斜划分结构符号进去组成能够访问的合法系统长地址并丢回
}

// 检查字符串忽视大小写匹配后缀，以此判断如 ".pgn" 与 ".PGN" 文件。
bool endsWithIgnoreCase(const std::string &text, const std::string &suffix)
{
    if (text.size() < suffix.size())
    {
        return false;
    }

    const std::size_t offset = text.size() - suffix.size();
    for (std::size_t i = 0; i < suffix.size(); ++i)
    {
        const unsigned char lhs = static_cast<unsigned char>(text[offset + i]);
        const unsigned char rhs = static_cast<unsigned char>(suffix[i]);
        if (std::tolower(lhs) != std::tolower(rhs))
        {
            return false;
        }
    }
    return true;
}

// 从给定的本地目录枚举所有 PGN 文件。
// 这里选择了 Win32 API 以适应并维持较为广泛的编译兼容性，而不是使用 std::filesystem。
std::vector<std::string> listPgnFiles(const std::string &directory) // 使用提供的目录字符变量查找下面拥有的同类文件列表
{
    std::vector<std::string> files; // 准备存所有的匹配文件系统字符名字的路径集合组
    WIN32_FIND_DATAA findData; // 开辟一块给系统寻找文件方法所使用传递回来存放数据的数据快
    const std::string searchPattern = joinPath(directory, "*"); // 将输入的那个大目录连接带上星作通配全目录搜集条件传
    HANDLE handle = FindFirstFileA(searchPattern.c_str(), &findData); // 先系统里调用一次来探首个存在的符合星号的路径柄
    if (handle == INVALID_HANDLE_VALUE) // 看是否没拿得到合理句柄代表这根本是不存在没文件等报错
    {
        return files; // 就什么都没找到直接以空列表空集合的模样作为回复
    }

    do // 这就是为了通过柄反复寻找下面的东西启动循环操作了
    {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) // 看刚用柄查系统找到并挂传挂的数据里属性这是不是它其实又是个夹子
        {
            continue; // 我们只需要这个表层下的具体档案既然是夹着文件系统的目录而不是棋本就不去拿它并进行下一个项的查找操作了
        }

        const std::string filename = findData.cFileName; // 它若非名录而是实实在在本层具体文档则取得系统回传的它的文档名信息
        if (endsWithIgnoreCase(filename, ".pgn")) // 然后丢给我们上面那个用作不管它大小写检查是不是这种特定的文件延展标志的方法去判定
        {
            files.push_back(joinPath(directory, filename)); // 即判定为PGN本我们就要它的长位置将其合并挂上去完整串地址推压进入我们的所有档案清单大列表里
        }
    } while (FindNextFileA(handle, &findData)); // 当上述针对当前挂的文档弄完了则让系统接柄推进至查找接下一个直到没有的了就不在里面转

    FindClose(handle); // 当再没有任何后续可抓档案就代表目录到头这时候关了探寻用的柄完成一次系统探访交互释放
    std::sort(files.begin(), files.end()); // 出于规范与每次生成库重现的一定顺次要求给找出来混乱的全档案在容器里按字母字型序次拍整齐
    return files; // 全列表已排查并整理装库完毕向调用主进程的需要点提供给这个完事的文档位置队列
}

// 基于引擎内的要求准确重建 Zobrist 哈希表初始值。
// 如本逻辑表格值不同，那么导出的对应开局局面哈希键就会无法跟游戏运行时查到的匹配中。
void initBookZobrist() // 开始装配出那个给全棋盘六十四位状态码对应各种乱数映射表
{
    std::mt19937_64 randomnum(kBookZobristSeed); // 把跟引擎定义常量中的统一六十四位随机器的特定种子喂入产生这把专用重制大随机器
    for (int type = 0; type < 8; ++type) // 将引擎的所有区分这枚举大类八种棋种按每一项种类遍历过去配乱数码
    {
        for (int color = 0; color < 2; ++color) // 给黑和红两大区别颜色种类遍历下分配特码用遍历处理
        {
            for (int pos = 0; pos < 90; ++pos) // 这里是全场盘总数所有的可能坑点位置每个过一遍装它的独个标识码
            {
                ZobristTable[type][color][pos] = randomnum(); // 此位置此颜色此棋存在这坑上会有一个这大随机机器此时摇的专属不重复随机码记下来放进原引擎复现表的这格内
            }
        }
    }
    ZobristPlayer = randomnum(); // 最后还要除了全局存在的摆法外的另一重要标志这局这一动究竟到谁的一手这状态的一个特定随机大码给它附值上
}

// 在当前局面下把某步引擎内的合法走法对应换算转化为标准的中文走法记录输出。
// 这是本脚本的一个分析核心技巧转换之处：
// 1. 获取当前盘面的所有合理合法走法
// 2. 将它们格式化为中文记录记法
// 3. 直接通过文本记录比对查找到真正匹配的 PGN 标记串
std::string formatMoveNotation(const Chess &chess, const Move &move) // 将某一合法的机器走法还原组合成中文记谱字符串
{
    const Grid &sourceGrid = chess.state[chess.currTurnId][Chess::xy2pos(move.source_x, move.source_y)]; // 获取当前移动回合中走步起点的棋格对象
    if (sourceGrid.color == EMPTY || sourceGrid.type == None) // 检查起点格子是否为空或者没有具体棋种
    {
        throw std::runtime_error("move source square is empty"); // 如果格子为空，抛出运行时异常说明源数据出错
    }

    std::string prefix; // 声明用于存放如“前车”、“炮八”之类记谱前半部的主语定位前缀
    std::vector<int> sameFileYs; // 声明数组用于收集和当前棋子同列且同色同类的所有棋子的Y轴位置
    for (int y = 0; y < BOARDHEIGHT; ++y) // 从上到下纵向遍历整个棋盘的当前列
    {
        const Grid &grid = chess.state[chess.currTurnId][Chess::xy2pos(move.source_x, y)]; // 逐个取出这一列上的每一个格子信息
        if (grid.color == sourceGrid.color && grid.type == sourceGrid.type) // 判断此格该子是否与要移动的子颜色类型完全相同
        {
            sameFileYs.push_back(y); // 完全相同则将其Y坐标收入数组中以备判断是否需要“前中后”等特殊称谓
        }
    }

    if (sameFileYs.size() >= 2) // 若数组大小>=2，说明此列同时存在至少两个己方同类型棋子（如双车在同一条直线上）
    {
        std::sort(sameFileYs.begin(), sameFileYs.end(), [&](int lhs, int rhs) { // 针对同列棋子的Y坐标重新排序
            return isMoreFront(sourceGrid.color, lhs, rhs); // 排序依据是相对于当前颜色谁更偏向“前方”
        });

        std::size_t pieceIndex = sameFileYs.size(); // 初始化当前要移动棋子在同列的顺位索引
        for (std::size_t i = 0; i < sameFileYs.size(); ++i) // 遍历刚刚按“前到后”排好序的本列棋子坐标列表
        {
            if (sameFileYs[i] == move.source_y) // 匹配哪一个才是我们当步要移动的对应棋子的本来位置
            {
                pieceIndex = i; // 找到对应的目标并赋值此索引给变量记录
                break; // 既已找到就提前跳出查询循环
            }
        }
        if (pieceIndex == sameFileYs.size()) // 若遍历结束还未找到（正常逻辑下不可能发生）
        {
            throw std::runtime_error("failed to locate moving piece for notation"); // 说明棋盘发生数据崩坏，抛出异常阻断
        }
        prefix = disambiguationLabel(pieceIndex, sameFileYs.size()) + pieceToCanonicalName(sourceGrid.type); // 获取排序标签(前后等)加棋的名称构建记录前缀
    }
    else // 若本列仅有一个自己这类棋子，就不必加注分辨哪颗的麻烦前后词元
    {
        prefix = pieceToCanonicalName(sourceGrid.type) + fileToCanonicalNumber(move.source_x, sourceGrid.color); // 通常表现为：名字加移动起点的本视角横坐标数字
    }

    if (move.target_y == move.source_y) // 假若目标Y坐标和原本地的Y坐标没变化，说明此步完全横着走
    {
        return prefix + "平" + fileToCanonicalNumber(move.target_x, sourceGrid.color); // 中文直接拼接一个“平”以及到达的目标列号当作完整文本退出
    }

    const bool forward = isForwardMove(sourceGrid.color, move.source_y, move.target_y); // 对于有纵向偏移的走步，查探对于操控者视角是在往前还是后走
    const std::string action = forward ? "进" : "退"; // 将结果转置成记谱里表达动作的"进"或者"退"汉字串

    if (sourceGrid.type == Rook || sourceGrid.type == Cannon || sourceGrid.type == King || sourceGrid.type == Pawn) // 针对车、炮、将、兵这些纵向走步按“步数”直算的特性棋种
    {
        return prefix + action + std::to_string(std::abs(move.target_y - move.source_y)); // 记谱文本就拼接上：前缀+进/退行动动作+Y轴所跨越位移步数的绝对量
    }
    return prefix + action + fileToCanonicalNumber(move.target_x, sourceGrid.color); // 对那些斜向走的(马相士等)，用：前缀+进/退+移动目的地的对应方新横坐标列数字作文本
}

// 抽取一条 PGN 内容，仅保留走棋标记字段如 "炮八平五" 和 "马２进３"。
// 将忽略对局头部说明，或空行首尾注脚的额外信息。
std::vector<std::string> extractMoveTokens(const std::string &line) // 从一整行 PGN 文本中拆解并提取出实打实的行棋指令文本序列
{
    std::vector<std::string> tokens; // 初始化一个字符串数组以按序收录所有有效的词块标记
    std::string cleaned = trim(line); // 先把当前读取的文本字串两端多余的所有空白换行等符号彻底修剪光
    if (cleaned.empty() || cleaned.front() == '[') // 如果去除空白后就是空行，或者开头带中括号说明那是在记录棋局属性数据（例如比赛选手名）之类的无用行
    {
        return tokens; // 直接认为此行没包含走法回合跳过其分析过程，返回原本的空词块组
    }
    if (!std::isdigit(static_cast<unsigned char>(cleaned.front()))) // 再判断这段行打头那个有效字是否是个纯阿拉伯式数字（以防有些不合规乱码起手）
    {
        return tokens; // 不是的话也说明这一行不是从步序号为起始开始对局录的实质性行棋文本段
    }

    std::istringstream iss(cleaned); // 利用把文本嵌入成这字符化串流类里面以便做下一步按系统间隔进行连拆取的操作
    std::string token; // 定义临时字符载体准备迎接上面流里的一个个被连带拆下而解出来的短单词串
    while (iss >> token) // 根据这种类的特定输入操作流去把那些按空格相隔出的人类每个单词词块连绵顺流装进给上边这载体，并以此设作执行查勘是否还有下一存货的依据体
    {
        if (!token.empty() && token.back() == '.') // 若这一单独读出来词最后是个小数点后缀那必定多半是这种开“12.”式的这单纯指代回合步次数标记段
        {
            continue; // 我们纯然只在乎实质这步下的啥动作棋步不计其是在这全局步次里排号为第几所以直接无意义掠继续去看流管下一短字块儿
        }
        if (token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*") // 但如果有切出来恰正好处为像这堆标准认可用表明该战收下帷尾停告或和告的那些停结局志号
        {
            break; // 明确获知战况的局下已经彻底记录走尽完了就干脆折断放弃之后无论啥内容拆装跳去停止动作
        }
        tokens.push_back(token); // 不是那无谓标示不无胜败收局符号就说明这是一个有效动作录入了如马进什么，给推进把这小截记入装向刚才数组最后去
    }
    return tokens; // 将整这列给挑精并提取装进这一队列之有效走招列表呈递送付归予那个发起使用的人使用
}

// 在当前对局状况下去直接翻译一条中文格式写出的走棋指令。
// 这里的做法并非文字级别去拆解猜测，而是跟引擎的所有合法走步列表进行格式化匹配。
Move parseChineseMove(const Chess &chess, const std::string &moveToken) // 通过上面给它的文本通过枚举匹配对棋盘实体操作进行反向精准指认
{
    // 将原文本提取标准化。
    const std::string normalizedTarget = normalizeToken(moveToken); // 收紧全角变体错体与繁体统一给标准化以便接下比配
    // 生成这一步所有可能有效的合法棋步。
    std::vector<Move> legalMoves; // 定出一个引擎物理走法的大列库准备存下此刻机器可能所下动作选项
    Chess boardCopy = chess; // 为保留实局棋盘不致推导计算查验污染取这一整副盘面做一备副环境
    boardCopy.generateMoves(legalMoves, true); // 给副盘下达填满列列那上项表命令并使全生发罗布出机器下此时角色合法能够所有能执行下的行棋选项清点

    // 记录目前共存在多少个合法的匹配步法。
    Move matchedMove; // 有一路空项留用来搁定那被挑准验成功那个机器对象动作的单独坑位空
    int matchCount = 0; // 定义开始重查计算重中配中正确次总量到底得能算出来有多少同合次
    for (const Move &candidate : legalMoves) // 将刚产库的每一样单例作为这验证查候提点选项做提领出轮挨巡问看一轮去
    {
        // 格式化当前枚举到的合法走法，把它标准化，看看内容相不相同。
        const std::string normalizedCandidate = normalizeToken(formatMoveNotation(chess, candidate)); // 把在机内盘下选项通过改转转格式文字记录化出再送进统规严齐处理化做可给前要读比正纯式段
        if (normalizedCandidate == normalizedTarget) // 若这个给机盘变转规范后字符项一分没有地能切当应上对的正是文本切查到提并统那条指代文字招要查那原
        {
            matchedMove = candidate; // 便暂视其为此实指招之要而对先拿记给存档下以作准实下作留取
            ++matchCount; // 对有能比上准能对全上的可能选数目往数上走增上一例这好防它如多配记歧情况来防判断歧错
        }
    }

    if (matchCount == 1) // 看一轮匹配最后是否是严丝合缝仅只找到恰到刚好的一路相符合无误匹配的物理步
    {
        return matchedMove; // 如此说那这是明确特出无可纷混便安生认定这是这条了去提交付于系统让下动执
    }

    // 找不到可能说明指令无法在这个局面生效识别出来。
    // 多于一个结果说明翻译后记谱法依旧含有多意模糊的内容未理清。
    std::ostringstream oss; // 起一流对流用来把错向拼起造以好往外发那全连句文说错报号单
    oss << "failed to parse move token \"" << moveToken << "\""; // 向这里写第一部分头语意它这块要不配了不读识
    if (matchCount > 1) // 如这是查中了太多全符等乱那便说此文本录的实乃是引有太重岐并有病法等谱等乱出因
    {
        oss << " because it matches multiple legal moves"; // 给它并告拼个原说这文它能混符着机器它一好多步它配对重不清的后附说出文辞
    }
    throw std::runtime_error(oss.str()); // 生生砸这已拼装全指说它因这坏字而解析完失败不可走而致使这断停全报指扔给后方知并大错出切中截停本乱跑局下全
}

// 将一条捕获的匹配“局面—走法”键值登记进入全局开局库之中。
void recordMove(uint64_t zobristKey, const Move &move) // 送局之盘这唯一特征大随与这指实动到总收集书来做收存记录次重数
{
    MoveBucket &bucket = g_openingBook[zobristKey]; // 通以对取那定哈查全局找它的库寻此对应存局面一状态下所有招大篮来拿个大对给查索
    BookMoveStat &stat = bucket[moveKey(move)]; // 再对此招定变专统用转下存对那去寻找或无就刚生定设新坑这统计动作格以放用
    if (stat.count == 0) // 如若这拿出来计数是原未有走初始的空零就定为初登记录着
    {
        stat.move = move; // 这就是未走那必给先放把机内那招给实体放它内固存那为作头次留
    }
    ++stat.count; // 无这是不是旧都往上每次进过见用便将这出走法次去计数增上其它量给添显火热门比下量法以统分重
}

// 播放回放每一个读入的 PGN 源文件，仅留出开局后的特定回合数信息（如 15 步内）。
// 并就该每一行对应的最新状态求取正确的盘面哈希键，保证和我们游戏中的一样。
void processPgnFile(const std::string &filePath) // 取来外全对弈详程文件总合发去一条大通并记录的操行过串法
{
    // 载入源文件对象。
    std::ifstream input(filePath); // 将传址目标作输入地定对象并开启与那文件沟通读串通道流门
    if (!input.is_open()) // 如没能成功启开这个通入的大串大门比如那不存在或全被别的堵写没得阅览权利
    {
        throw std::runtime_error("cannot open PGN file: " + filePath); // 当报说找拿打不行直接阻这向大总出错误甩离该段执行防错干乱空转
    }

    // 在同引擎的统一初始棋位环境下开始运行推算。
    Chess chess; // 立局建立能同我正常作一摸同样实地的一台局和板初新复位给下动
    // 采用步数（ply）来进行精确把控以切分文本中拆分下来的步记录。
    int plyCount = 0; // 定义走步计量用来查验该已是走多长的一步给对总步多半控制以录限内数
    std::string line; // 留以能暂读把当次扫下一长文行的单行全提装字的大变量装

    while (std::getline(input, line)) // 此向并一刻行读不断朝取它内全部一行行提拿字符装去刚才留准备如仍存则不停歇直向扫
    {
        // 先按所在本地系统转好 UTF-8 后才能使用前面的解析器稳定读取切分内容。
        line = ansiToUtf8(line); // 要过文之前为求稳安转以化前特系统化此本乱化给换为后通用UTF定稳后再重新回存以给用
        const std::vector<std::string> moveTokens = extractMoveTokens(line); // 通该调用提洗前文留纯净指动中文字块并排号排一列还留交回之以拿给行去用
        for (const std::string &token : moveTokens) // 给那一列的每单点词单指名全大循环遍历给提出每单句挨次扫递发出去去拆动
        {
            // 一旦满设定的最大允许记录回合深度则直接舍去剩余不用评估的内容。
            if (plyCount >= kMaxBookPlies) // 此一旦步计超我们起预先留库设之数就截之并不再向深后棋查和留
            {
                return; // 直接抽手出该文件对剩下的哪怕走完都舍不计丢且离大函数出
            }

            // 处理解析这个 PGN 的中文分词词块到真实的 Move 对象上来。
            const Move move = parseChineseMove(chess, token); // 此拿向并经文发给盘下动解意机器让这在当棋局变猜算出其实招机器行传向之
            if (!chess.isLegalMove(move)) // 这个还给并算传其来的那可能机内动若这原机判定它根本非是当情合理的招的错走瞎传了
            {
                throw std::runtime_error("parsed move is not legal: " + token); // 当就甩它一个不可跑的大异常出阻告诉文没法是它是不全合理走报错它让跳
            }

            // 需要注意这里重点是一点，即一定要在进行实际改动此对局的发生状态前获取走步起点的哈希去使用。
            const uint64_t zobristKey = chess.getZobristKey(); // 给没动之前的目前对势索全查取出此时机给的十六进制盘面密记特全一独哈码拿住挂点这记录它招以供记向去认名
            recordMove(zobristKey, move); // 向本库记记登大树下入那个它和此哈码对应的走招进它的那个大桶并去数点数登留
            // 这里推入与真实主局和博弈层同源的正式执行下棋函数推过本回合。
            chess.makeMoveAssumeLegal(move); // 行令这原虚拟大棋板将那真走招实体确操作走上去行完以使得局势转变和推进等应去配待文句推新盘以正其走
            ++plyCount; // 完成一步合法动作计总深跑度也加上一点递多做下个截度留比算以不走中后深处
        }
    }
}

// 汇总格式化一条已处理完毕的具体频次单步结构成为能够转储上盘落盘的 JSON 数据形式。
Json::Value moveToJson(const BookMoveStat &stat) // 提供对这某此点动作其转作可以便直通人眼发配与交储给写档转存那个JSON的生成大包合值方
{
    Json::Value item; // 创建个大对象包来做装那里面此特步名录大框子变装用对给返回出
    item["count"] = stat.count; // 并在这个叫统计次名字之项中配向那经总树频走查来合过的频数字数去
    item["move"]["source_x"] = stat.move.source_x; // 也同于对招里面名发起的起源它列给赋这个走点那来机内列发数字
    item["move"]["source_y"] = stat.move.source_y; // 和给那源也发行的出数指到
    item["move"]["target_x"] = stat.move.target_x; // 与发这行点落去它的到达大列也发指定填之进去给
    item["move"]["target_y"] = stat.move.target_y; // 这个并及目标其行的也给上写入它的里
    item["move"]["coord"] = moveToCoordinateString(stat.move); // 此生成叫字母列对好方便读那全字符直转点出入拼全它项内也以作存备给人好读或机器另外好转识出给
    item["move"]["source"] = std::string(1, pgnint2char(stat.move.source_x)) + std::string(1, int2char(stat.move.source_y)); // 然后把那个起端单独拿取化字码排组供直接这单一给分留入也分栏它记以拆析方便时
    item["move"]["target"] = std::string(1, pgnint2char(stat.move.target_x)) + std::string(1, int2char(stat.move.target_y)); // 亦是对着到地它单独排造独立一字合码组去送入留记
    return item; // 经前造并封合这全是全配并完之招大项目一小包对象传甩出去回给出
}

// 执行完整的存储全局已被组装好了的开局树到我们期待的落点文件当中。
// JSON 支持作为键名的只能是字符串，因此需要把这个十六进制数的结构转换为十进制字符串的形式。
void exportOpeningBook() // 统揽行将所经扫建所有大树转输落地做JSON全实体真文的方法去
{
    // 在这里我们组织出一颗最顶端结构节点包含有映射各种开局 Zobrist 哈希数字映射的数据。
    Json::Value root; // 起名统揽这一切所有那全局盘分支的大挂底首最本顶空点用来包裹并赋出往落存
    for (const auto &entry : g_openingBook) // 开启给全全那所有过登查留那每每盘大全树的各情况历循环扫过去找发提它的步表出来
    {
        // 聚集起下属所有统计频率的键值来重排序，满足最优统计情况写入头部。
        std::vector<BookMoveStat> sortedMoves; // 开辟给某大盘之下的所有曾记录招建一阵空列表用来装其所有招放入重去理理
        for (const auto &kv : entry.second) // 把那盘挂下统统所有的步与频大全集合挨过拿出每一步存过它项招
        {
            sortedMoves.push_back(kv.second); // 那向数组后排一压入挂上供这大队列收集去去理排序用
        }

        // 把出现频率最高，走步数最符合要求的列于输出记录头最前面；其余频率通过确定的键串排解避免输出混乱。
        std::sort(sortedMoves.begin(), sortedMoves.end(), [](const BookMoveStat &lhs, const BookMoveStat &rhs) { // 发起用特别规定组对排法则给上头集合进行清排使高频先与防乱数去
            if (lhs.count != rhs.count) // 看如如在数它俩所被使用频并不相此时的断高下
            {
                return lhs.count > rhs.count; // 以强前列排之高次多热数为前倒排这等降下理排准定下名并送回对组排布序去比
            }
            return moveKey(lhs.move) < moveKey(rhs.move); // 若它数又正一数也只好启用其文本字典固定化之排以防每发同热生乱跑位变文件不同之忧求这固定字键序以全它回
        });

        Json::Value moves(Json::arrayValue); // 列一大对出包要包这被刚刚排得井然顺着列成一列表之这所有行步数去留包数组挂上它之类里
        for (const BookMoveStat &stat : sortedMoves) // 以好排好的按高往下之招数步项大组大循环次它取此每这招项传用
        {
            moves.append(moveToJson(stat)); // 给送刚它向包装此单步包完方转大那小包传还再来插这并串到该全局面表大挂列中连下它项在里面
        }
        // 注意这里的处理为强行作为 JSON 使用的格式转化：把作为数值存在的对应哈希值强换为符合规范要求的映射键串文字形式存入结构中。
        root[std::to_string(entry.first)] = moves; // 终把该大面上大串连起的所有招的排那表数组包赋在以它名十六哈转换至文串之后那十进制名称里名下存作总的一节顶给上最源头树下去
    }

    // 开放且通过标准可打印方式落地写入实际文本的 JSON 分析日志内。
    std::ofstream output(kOutputJsonPath); // 呼一推输出造能向硬盘指定的全全实体落文件那地方推流打建立联系去它发给那送给送它
    if (!output.is_open()) // 如发看推该地无法得立出开不无法打去也发失败如在无权那处无权等法那不让
    {
        throw std::runtime_error("cannot write output json: " + kOutputJsonPath); // 即报生说写不能入并通抛出有那截阻警告甩说在出失败没向的阻错的抛报断说错在那以警不写而报错去停
    }

    Json::StyledWriter writer; // 请用造排大系统可给人看得美可直接有空行与折换好看的写这发式输出工手去
    output << writer.write(root); // 把这全统包裹成大树这数据全根拿给其大包排去全美写输推进刚连通道的实文件内落实存档了生成去
}
} // namespace

// 主脚本运行入口处：
// 1. 检验指定的存放读取文本（PGN 格式文件）源库本身是否存在以及可靠可信；
// 2. 重造跟引擎逻辑能保持一贯对应的游戏基础规则和局面相关哈希基础库信息；
// 3. 通过检索对应列表内容从而加载所有的存放盘面文件的内容提取到主引擎系统运算执行出结果并登记并提取信息记录其中；
// 4. 将完成组装归并在一起最终能给引擎正常调用匹配检索查找内容的全部结果进行完整导成格式化文档；
int main() // 程序离线建库专用执行主大发始主行程序发源首大函数
{
    try // 利用大系统自带外的大错误拦截容括袋去发阻不使在不可预出突异等报错去给能捕抓到手做阻处查
    {
        // 在起作用最开头前若无法打开库时应迅速做尽早验证阻断并弹警告出。
        const DWORD attributes = GetFileAttributesA(kPgnDirectory.c_str()); // 先发给系统对设定存放棋谱目录问它要要此那位置属那件查情况这看看有没有它存着它
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) // 判断返回的是是不是说找没有或实在这并非也是一个放东西实真个名目录样夹而是别
        {
            throw std::runtime_error("PGN directory does not exist: " + kPgnDirectory); // 就即刻当场不再进行去丢出中止不实源有错之那大报错来不运行空作
        }

        // 获取构造哈希所必须先备齐的基本数组。
        initBookZobrist(); // 发建复刻建立如引擎完全那那用所的盘特征的大随数哈希基数阵以对日找盘用

        // 从特定地址读回需要加载的全盘的下棋记录。
        const std::vector<std::string> pgnFiles = listPgnFiles(kPgnDirectory); // 找这所有刚认的真夹下拉它带全所有的后缀PGN的合法文件长串给收回来做列阵
        int fileCount = 0; // 先设零总清算去统计记将处了有多长总算量档案过的一用作全毕之计数量的报告
        for (const std::string &filePath : pgnFiles) // 大遍一历刚抓它里那个列取其中传的一串完整档大地址串过用
        {
            // 给反馈提示的时候切出个较为清楚的文件短名称以作输出标志。
            const std::size_t slashPos = filePath.find_last_of("/\\"); // 反寻最后有切地大斜之位置以对定下截只名不用大全径名
            const std::string filename = (slashPos == std::string::npos) ? filePath : filePath.substr(slashPos + 1); // 有就它之尾开始无那就直接整名为那短简纯之名来准备打显出给眼看去
            std::cout << "Processing " << filename << std::endl; // 打推说正干什么事儿且做处理哪个小文件让看的人也知在哪过进着干没着停卡
            // 这时候对单个加载起来的文件内容重放开推解析数据给映射总表喂出记录进去合并。
            processPgnFile(filePath); // 放它发其地址拿去整大开本扫棋并过把其中盘势哈密码并推走的那些招给填记发树上去做汇总积存
            ++fileCount; // 回成完就一加过完之该文本完过总的一份累它记以便得最终数一出
        }

        // 把所汇集到总数表结果给序列转出落存磁盘上去最后一次处理确认。
        exportOpeningBook(); // 排定把整这过所有统总的那大全书集大频次的开局巨排表出全部落印转成那最终所要实录书
        std::cout << "Processed " << fileCount << " PGN files." << std::endl; // 落完毕去就打印报上共合几多此档案等也完说它结告知看使用它的成总数人看清做多大量
        std::cout << "Opening book written to " << kOutputJsonPath << std::endl; // 另附它实已经发它写定向存大去哪的成功印去给说明其放妥位置完告作指了向去
        return 0; // 全成正常归交零以告该机指没阻畅完事指大正结大标志反出去向说正全
    }
    catch (const std::exception &ex) // 若这中间因乱错因打或者没字那各种阻中法异的错报错丢下就它拦并抓之包上在里里不使全大发爆
    {
        // 所有解析或是处理中断时抛出的无法挽留的内部错误和IO的异常终止都在这里抛出拦截然后报错后非零状态结束去让调试感知到错误。
        std::cerr << "getchessbook failed: " << ex.what() << std::endl; // 将被上接这抓到并拦了的那带着它出错内说辞文和说明给抛指向报红错大流字里这以留印留以察这啥大错在它指何发事错以好人查定停错它大
        return 1; // 反返一个没不不是那正常行它事完而是一指出此全事中腰腰崩败说结大错反停去让呼调这法察察系错发知那跑它是不畅出坏大停了结束的
    }
}
