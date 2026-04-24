#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <windows.h>

#define GENERATE_OPENING_BOOK
#define private public
#define protected public
#include "Chinese_Chess_trial_2.cpp"
#undef protected
#undef private

namespace
{
constexpr int kMaxBookRounds = 15;
constexpr int kMaxBookPlies = kMaxBookRounds * 2;
constexpr uint64_t kBookZobristSeed = 20262026ULL; // Must match InitZobrist() in Chinese_Chess_trial_2.cpp.

const std::string kPgnDirectory = "C:/Users/yang/Desktop/Game-strategic-algorithms/Code/PGN";
const std::string kOutputJsonPath = "C:/Users/yang/Desktop/Game-strategic-algorithms/Code/opening_book.json";

struct BookMoveStat
{
    Move move;
    int count = 0;
};

using MoveBucket = std::map<std::string, BookMoveStat>;
using OpeningBook = std::map<uint64_t, MoveBucket>;

OpeningBook g_openingBook;

std::string trim(const std::string &text)
{
    const std::string whitespace = " \t\r\n";
    const std::size_t begin = text.find_first_not_of(whitespace);
    if (begin == std::string::npos)
    {
        return "";
    }
    const std::size_t end = text.find_last_not_of(whitespace);
    return text.substr(begin, end - begin + 1);
}

std::string ansiToUtf8(const std::string &ansiText)
{
    if (ansiText.empty())
    {
        return "";
    }

    const int wideLength = MultiByteToWideChar(CP_ACP, 0, ansiText.c_str(), static_cast<int>(ansiText.size()), nullptr, 0);
    if (wideLength <= 0)
    {
        return ansiText;
    }

    std::wstring wideText(static_cast<std::size_t>(wideLength), L'\0');
    MultiByteToWideChar(CP_ACP, 0, ansiText.c_str(), static_cast<int>(ansiText.size()), &wideText[0], wideLength);

    const int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideText.c_str(), wideLength, nullptr, 0, nullptr, nullptr);
    if (utf8Length <= 0)
    {
        return ansiText;
    }

    std::string utf8Text(static_cast<std::size_t>(utf8Length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideText.c_str(), wideLength, &utf8Text[0], utf8Length, nullptr, nullptr);
    return utf8Text;
}

std::vector<std::string> splitUtf8(const std::string &text)
{
    std::vector<std::string> result;
    for (std::size_t i = 0; i < text.size();)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        std::size_t length = 1;
        if ((c & 0x80U) == 0)
        {
            length = 1;
        }
        else if ((c & 0xE0U) == 0xC0U)
        {
            length = 2;
        }
        else if ((c & 0xF0U) == 0xE0U)
        {
            length = 3;
        }
        else if ((c & 0xF8U) == 0xF0U)
        {
            length = 4;
        }
        if (i + length > text.size())
        {
            length = 1;
        }
        result.push_back(text.substr(i, length));
        i += length;
    }
    return result;
}

std::string normalizeToken(const std::string &token)
{
    static const std::map<std::string, std::string> replacements = {
        {"１", "1"}, {"２", "2"}, {"３", "3"}, {"４", "4"}, {"５", "5"},
        {"６", "6"}, {"７", "7"}, {"８", "8"}, {"９", "9"},
        {"一", "1"}, {"二", "2"}, {"三", "3"}, {"四", "4"}, {"五", "5"},
        {"六", "6"}, {"七", "7"}, {"八", "8"}, {"九", "9"},
        {"壹", "1"}, {"贰", "2"}, {"叁", "3"}, {"肆", "4"}, {"伍", "5"},
        {"陆", "6"}, {"柒", "7"}, {"捌", "8"}, {"玖", "9"},
        {"車", "车"}, {"馬", "马"}, {"砲", "炮"}, {"將", "将"}, {"帅", "将"}, {"帥", "将"},
        {"仕", "士"}, {"相", "相"}, {"象", "相"}, {"兵", "兵"}, {"卒", "兵"},
        {"後", "后"}, {"進", "进"}, {"退", "退"}, {"平", "平"}};

    std::string normalized;
    for (const std::string &cp : splitUtf8(token))
    {
        auto it = replacements.find(cp);
        if (it != replacements.end())
        {
            normalized += it->second;
        }
        else if (cp != " " && cp != "\t" && cp != "\r" && cp != "\n")
        {
            normalized += cp;
        }
    }
    return normalized;
}

std::string pieceToCanonicalName(stoneType type)
{
    switch (type)
    {
    case King:
        return "将";
    case Bishop:
        return "相";
    case Knight:
        return "马";
    case Rook:
        return "车";
    case Pawn:
        return "兵";
    case Cannon:
        return "炮";
    case Assistant:
        return "士";
    default:
        return "*";
    }
}

std::string fileToCanonicalNumber(int x, colorType color)
{
    const int value = (color == RED) ? (9 - x) : (x + 1);
    return std::to_string(value);
}

bool isForwardMove(colorType color, int sourceY, int targetY)
{
    return (color == RED) ? (targetY > sourceY) : (targetY < sourceY);
}

bool isMoreFront(colorType color, int lhsY, int rhsY)
{
    return (color == RED) ? (lhsY > rhsY) : (lhsY < rhsY);
}

std::string disambiguationLabel(std::size_t index, std::size_t total)
{
    if (total == 2)
    {
        return (index == 0) ? "前" : "后";
    }
    if (total == 3)
    {
        static const char *labels[] = {"前", "中", "后"};
        return labels[index];
    }
    if (index == 0)
    {
        return "前";
    }
    if (index + 1 == total)
    {
        return "后";
    }
    return std::to_string(index + 1);
}

std::string moveKey(const Move &move)
{
    std::ostringstream oss;
    oss << move.source_x << ',' << move.source_y << ','
        << move.target_x << ',' << move.target_y;
    return oss.str();
}

std::string moveToCoordinateString(const Move &move)
{
    return std::string(1, pgnint2char(move.source_x)) + std::string(1, int2char(move.source_y)) +
           std::string(1, pgnint2char(move.target_x)) + std::string(1, int2char(move.target_y));
}

std::string joinPath(const std::string &lhs, const std::string &rhs)
{
    if (lhs.empty())
    {
        return rhs;
    }
    if (lhs.back() == '/' || lhs.back() == '\\')
    {
        return lhs + rhs;
    }
    return lhs + "/" + rhs;
}

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

std::vector<std::string> listPgnFiles(const std::string &directory)
{
    std::vector<std::string> files;
    WIN32_FIND_DATAA findData;
    const std::string searchPattern = joinPath(directory, "*");
    HANDLE handle = FindFirstFileA(searchPattern.c_str(), &findData);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return files;
    }

    do
    {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            continue;
        }

        const std::string filename = findData.cFileName;
        if (endsWithIgnoreCase(filename, ".pgn"))
        {
            files.push_back(joinPath(directory, filename));
        }
    } while (FindNextFileA(handle, &findData));

    FindClose(handle);
    std::sort(files.begin(), files.end());
    return files;
}

void initBookZobrist()
{
    std::mt19937_64 randomnum(kBookZobristSeed);
    for (int type = 0; type < 8; ++type)
    {
        for (int color = 0; color < 2; ++color)
        {
            for (int pos = 0; pos < 90; ++pos)
            {
                ZobristTable[type][color][pos] = randomnum();
            }
        }
    }
    ZobristPlayer = randomnum();
}

std::string formatMoveNotation(const Chess &chess, const Move &move)
{
    const Grid &sourceGrid = chess.state[chess.currTurnId][Chess::xy2pos(move.source_x, move.source_y)];
    if (sourceGrid.color == EMPTY || sourceGrid.type == None)
    {
        throw std::runtime_error("move source square is empty");
    }

    std::string prefix;
    std::vector<int> sameFileYs;
    for (int y = 0; y < BOARDHEIGHT; ++y)
    {
        const Grid &grid = chess.state[chess.currTurnId][Chess::xy2pos(move.source_x, y)];
        if (grid.color == sourceGrid.color && grid.type == sourceGrid.type)
        {
            sameFileYs.push_back(y);
        }
    }

    if (sameFileYs.size() >= 2)
    {
        std::sort(sameFileYs.begin(), sameFileYs.end(), [&](int lhs, int rhs) {
            return isMoreFront(sourceGrid.color, lhs, rhs);
        });

        std::size_t pieceIndex = sameFileYs.size();
        for (std::size_t i = 0; i < sameFileYs.size(); ++i)
        {
            if (sameFileYs[i] == move.source_y)
            {
                pieceIndex = i;
                break;
            }
        }
        if (pieceIndex == sameFileYs.size())
        {
            throw std::runtime_error("failed to locate moving piece for notation");
        }
        prefix = disambiguationLabel(pieceIndex, sameFileYs.size()) + pieceToCanonicalName(sourceGrid.type);
    }
    else
    {
        prefix = pieceToCanonicalName(sourceGrid.type) + fileToCanonicalNumber(move.source_x, sourceGrid.color);
    }

    if (move.target_y == move.source_y)
    {
        return prefix + "平" + fileToCanonicalNumber(move.target_x, sourceGrid.color);
    }

    const bool forward = isForwardMove(sourceGrid.color, move.source_y, move.target_y);
    const std::string action = forward ? "进" : "退";

    if (sourceGrid.type == Rook || sourceGrid.type == Cannon || sourceGrid.type == King || sourceGrid.type == Pawn)
    {
        return prefix + action + std::to_string(std::abs(move.target_y - move.source_y));
    }
    return prefix + action + fileToCanonicalNumber(move.target_x, sourceGrid.color);
}

std::vector<std::string> extractMoveTokens(const std::string &line)
{
    std::vector<std::string> tokens;
    std::string cleaned = trim(line);
    if (cleaned.empty() || cleaned.front() == '[')
    {
        return tokens;
    }
    if (!std::isdigit(static_cast<unsigned char>(cleaned.front())))
    {
        return tokens;
    }

    std::istringstream iss(cleaned);
    std::string token;
    while (iss >> token)
    {
        if (!token.empty() && token.back() == '.')
        {
            continue;
        }
        if (token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*")
        {
            break;
        }
        tokens.push_back(token);
    }
    return tokens;
}

Move parseChineseMove(const Chess &chess, const std::string &moveToken)
{
    const std::string normalizedTarget = normalizeToken(moveToken);
    std::vector<Move> legalMoves;
    Chess boardCopy = chess;
    boardCopy.generateMoves(legalMoves, true);

    Move matchedMove;
    int matchCount = 0;
    for (const Move &candidate : legalMoves)
    {
        const std::string normalizedCandidate = normalizeToken(formatMoveNotation(chess, candidate));
        if (normalizedCandidate == normalizedTarget)
        {
            matchedMove = candidate;
            ++matchCount;
        }
    }

    if (matchCount == 1)
    {
        return matchedMove;
    }

    std::ostringstream oss;
    oss << "failed to parse move token \"" << moveToken << "\"";
    if (matchCount > 1)
    {
        oss << " because it matches multiple legal moves";
    }
    throw std::runtime_error(oss.str());
}

void recordMove(uint64_t zobristKey, const Move &move)
{
    MoveBucket &bucket = g_openingBook[zobristKey];
    BookMoveStat &stat = bucket[moveKey(move)];
    if (stat.count == 0)
    {
        stat.move = move;
    }
    ++stat.count;
}

void processPgnFile(const std::string &filePath)
{
    std::ifstream input(filePath);
    if (!input.is_open())
    {
        throw std::runtime_error("cannot open PGN file: " + filePath);
    }

    Chess chess;
    int plyCount = 0;
    std::string line;

    while (std::getline(input, line))
    {
        line = ansiToUtf8(line);
        const std::vector<std::string> moveTokens = extractMoveTokens(line);
        for (const std::string &token : moveTokens)
        {
            if (plyCount >= kMaxBookPlies)
            {
                return;
            }

            const Move move = parseChineseMove(chess, token);
            if (!chess.isLegalMove(move))
            {
                throw std::runtime_error("parsed move is not legal: " + token);
            }

            const uint64_t zobristKey = chess.getZobristKey();
            recordMove(zobristKey, move);
            chess.makeMoveAssumeLegal(move);
            ++plyCount;
        }
    }
}

Json::Value moveToJson(const BookMoveStat &stat)
{
    Json::Value item;
    item["count"] = stat.count;
    item["move"]["source_x"] = stat.move.source_x;
    item["move"]["source_y"] = stat.move.source_y;
    item["move"]["target_x"] = stat.move.target_x;
    item["move"]["target_y"] = stat.move.target_y;
    item["move"]["coord"] = moveToCoordinateString(stat.move);
    item["move"]["source"] = std::string(1, pgnint2char(stat.move.source_x)) + std::string(1, int2char(stat.move.source_y));
    item["move"]["target"] = std::string(1, pgnint2char(stat.move.target_x)) + std::string(1, int2char(stat.move.target_y));
    return item;
}

void exportOpeningBook()
{
    Json::Value root;
    for (const auto &entry : g_openingBook)
    {
        std::vector<BookMoveStat> sortedMoves;
        for (const auto &kv : entry.second)
        {
            sortedMoves.push_back(kv.second);
        }

        std::sort(sortedMoves.begin(), sortedMoves.end(), [](const BookMoveStat &lhs, const BookMoveStat &rhs) {
            if (lhs.count != rhs.count)
            {
                return lhs.count > rhs.count;
            }
            return moveKey(lhs.move) < moveKey(rhs.move);
        });

        Json::Value moves(Json::arrayValue);
        for (const BookMoveStat &stat : sortedMoves)
        {
            moves.append(moveToJson(stat));
        }
        root[std::to_string(entry.first)] = moves;
    }

    std::ofstream output(kOutputJsonPath);
    if (!output.is_open())
    {
        throw std::runtime_error("cannot write output json: " + kOutputJsonPath);
    }

    Json::StyledWriter writer;
    output << writer.write(root);
}
} // namespace

int main()
{
    try
    {
        const DWORD attributes = GetFileAttributesA(kPgnDirectory.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            throw std::runtime_error("PGN directory does not exist: " + kPgnDirectory);
        }

        initBookZobrist();

        const std::vector<std::string> pgnFiles = listPgnFiles(kPgnDirectory);
        int fileCount = 0;
        for (const std::string &filePath : pgnFiles)
        {
            const std::size_t slashPos = filePath.find_last_of("/\\");
            const std::string filename = (slashPos == std::string::npos) ? filePath : filePath.substr(slashPos + 1);
            std::cout << "Processing " << filename << std::endl;
            processPgnFile(filePath);
            ++fileCount;
        }

        exportOpeningBook();
        std::cout << "Processed " << fileCount << " PGN files." << std::endl;
        std::cout << "Opening book written to " << kOutputJsonPath << std::endl;
        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "getchessbook failed: " << ex.what() << std::endl;
        return 1;
    }
}
