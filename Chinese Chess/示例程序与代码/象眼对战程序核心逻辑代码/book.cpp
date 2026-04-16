/*
book.h/book.cpp - Source Code for ElephantEye, Part VI

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.25, Last Modified: Apr. 2011
Copyright (C) 2004-2011 www.xqbase.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "position.h"
#include "book.h"

// 从开局库中获取当前局面的建议着法
// pos: 传入当前的棋局局面对象引用
// szBookFile: 开局库的文件路径字符串
// lpbks: 用于接收从开局库中找到的所有建议着法的结构体数组指针
// 返回值: 找到的适用着法总数
int GetBookMoves(const PositionStruct &pos, const char *szBookFile, BookStruct *lpbks) {
  BookFileStruct BookFile; // 定义一个处理开局库文件的对象
  PositionStruct posScan;  // 临时局面对象，用于扫描原局面以及镜像局面
  BookStruct bk;           // 临时着法结构体，用于读取库文件中单条记录
  int nScan, nLow, nHigh, nPtr; // 定义扫描控制和二分查找的变量
  int i, j, nMoves;        // 定义循环和着法计数的变量

  // 尝试打开开局库文件
  // 若打开失败(文件不存在或无法读取)则返回0，表示在库中没有找到着法
  if (!BookFile.Open(szBookFile)) {
    return 0; // 若打开失败则返回0个着法
  }

  posScan = pos; // 将传入的当前局面拷贝给用于扫描的临时局面
  // nScan用于控制扫描的轮次: 第一轮(0)扫描原局面，第二轮(1)扫描轴对称镜像局面
  // 因为有些开局在棋盘左右镜像是等价的，为了节省库空间能复用这些对称局面
  for (nScan = 0; nScan < 2; nScan ++) {
    nPtr = nLow = 0; // nLow 指向开局库记录的头部(起始索引)
    nHigh = BookFile.nLen - 1; // nHigh 指向开局库记录的尾部(最大索引)
    // 使用二分查找算法在有序的开局库中快速定位当前的 Zobrist 哈希锁
    while (nLow <= nHigh) {
      nPtr = (nLow + nHigh) / 2; // nPtr 取当前查找区间的中间索引
      BookFile.Read(bk, nPtr); // 从文件中读出第 nPtr 条记录(包含局面哈希和着法)到 bk 中
      // BOOK_POS_CMP是一个宏，用于比较库中读取记录的哈希值与当前待匹配局面的哈希值
      if (BOOK_POS_CMP(bk, posScan) < 0) {
        nLow = nPtr + 1;       // 如果库中哈希值偏小，说明目标在右半区，调整低位指针
      } else if (BOOK_POS_CMP(bk, posScan) > 0) {
        nHigh = nPtr - 1;      // 如果库中哈希值偏大，说明目标在左半区，调整高位指针
      } else {
        break;                 // 相等则说明找到了完全对应的局面，跳出二分查找循环
      }
    }
    // 如果找到了匹配项 (即没有因为没找到而导致 nLow > nHigh)，则直接退出 nScan 循环
    if (nLow <= nHigh) {
      break;
    }
    // 未找到原局面，则在第二轮循环前将局面进行左右镜像翻转，准备下一次查找镜像局面
    posScan.Mirror();
  }

  // 如果两次查找(原局面和镜像局面)都没有找到匹配的开局记录
  if (nScan == 2) {
    BookFile.Close(); // 关闭开局库文件以释放资源
    return 0;         // 返回0个着法
  }
  // 确保找到的指针 nPtr 在合法范围内(防止文件读取越界)
  __ASSERT_BOUND(0, nPtr, BookFile.nLen - 1);

  // 因为开局库中同一局面可能有多种不同的建议走法，它们在库中紧挨着存放
  // 这里需要向前回退寻找第一个匹配当前局面的条目起点
  for (nPtr --; nPtr >= 0; nPtr --) {
    BookFile.Read(bk, nPtr); // 往前读一条记录
    if (BOOK_POS_CMP(bk, posScan) < 0) {
      break; // 如果上一条记录的哈希值已经小于当前局面（不匹配了），则搜索终止
    }
  }

  nMoves = 0; // 初始化检索出来的可用着法数量计数量为0
  // 从确认的第一个匹配的条目开始(所以将倒推一格的nPtr再+1)，往后连续读取所有该局面的走法
  for (nPtr ++; nPtr < BookFile.nLen; nPtr ++) {
    BookFile.Read(bk, nPtr); // 往后读取每一条开局记录
    if (BOOK_POS_CMP(bk, posScan) > 0) {
      break; // 如果条目哈希值已经大于当前目标局面了，说明越过了目标局面的记录区，结束遍历
    }
    // 检查库中记录的着法 (bk.wmv) 在当前局面下是否确实符合象棋规则 (合法着法)
    if (posScan.LegalMove(bk.wmv)) {
      lpbks[nMoves].nPtr = nPtr; // 保存该着法在库中的文件索引，可用于后续引用分析等
      // 如果是在检索镜像局面（nScan==1）时找到的走法，则必须把取出的走法做左右颠倒还原给原局面
      // 这里的 MOVE_MIRROR 宏将着法进行左右轴对称转换
      lpbks[nMoves].wmv = (nScan == 0 ? bk.wmv : MOVE_MIRROR(bk.wmv));
      lpbks[nMoves].wvl = bk.wvl; // 装载该走法的开局权重/价值分数 (数值越大通常越是被推荐的走法)
      nMoves ++; // 有效着法数加1
      // 为防止写入结果的数组溢出，当着法数达到定义的最大生成走法上限 MAX_GEN_MOVES 时强行终止解析
      if (nMoves == MAX_GEN_MOVES) {
        break;
      }
    }
  }
  BookFile.Close(); // 解析完毕，关闭库文件流释放资源

  // 整理找到的开局着法：根据着法的价值(wvl)对建议着法进行冒泡排序（从高到低）
  for (i = 0; i < nMoves - 1; i ++) {
    for (j = nMoves - 1; j > i; j --) {
      if (lpbks[j - 1].wvl < lpbks[j].wvl) {
        SWAP(lpbks[j - 1], lpbks[j]); // 若后面的价值分数高于前面的，则交换它们的位置
      }
    }
  }
  return nMoves; // 最终返回符合条件的推荐开局着法数量
}
