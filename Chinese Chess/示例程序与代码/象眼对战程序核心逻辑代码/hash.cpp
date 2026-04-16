/*
hash.h/hash.cpp - Source Code for ElephantEye, Part V

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.31, Last Modified: May 2012
Copyright (C) 2004-2012 www.xqbase.com

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

#ifndef CCHESS_A3800
  #include <stdio.h>
#endif
#include "../base/base.h"
#include "position.h"
#include "hash.h"

int nHashMask;
HashStruct *hshItems;
#ifdef HASH_QUIESC
  HashStruct *hshItemsQ;
#endif

// 存储置换表（Hash表）局面信息
// 作用：将搜索树中某个节点（局面）的评估结果保存到置换表中，以便后续遇到相同局面时直接复用，从而大幅度裁剪搜索树
// pos: 当前局面; nFlag: 当前结点的类型(HASH_ALPHA上边界, HASH_BETA下边界, 或Exact精确值); vl: 评估分值; nDepth: 搜索深度; mv: 最佳着法
void RecordHash(const PositionStruct &pos, int nFlag, int vl, int nDepth, int mv) {
  HashStruct hsh;
  int i, nHashDepth, nMinDepth, nMinLayer;
  // 存储置换表局面信息的过程包括以下几个步骤：

  // 1. 对分值做杀棋步数调整；
  // 断言判断评估值是否在合法的胜负分值范围内
  __ASSERT_BOUND(1 - MATE_VALUE, vl, MATE_VALUE - 1);
  // 断言判断如果有最佳着法，该着法必须在当前局面下是合法的
  __ASSERT(mv == 0 || pos.LegalMove(mv));
  
  // 处理赢棋或输棋的分数（因为杀棋分数跟距离根节点的步数相关，存入哈希表需要转为绝对步数）
  if (vl > WIN_VALUE) {
    if (mv == 0 && vl <= BAN_VALUE) {
      // 导致长将的局面(不进行置换裁剪)如果连最佳着法也没有，那么没有必要写入置换表
      return; 
    }
    // 对于赢棋，加上到当前节点的距离(nDistance)，将其转换成离当前局面的绝对杀棋分保存
    vl += pos.nDistance;
  } else if (vl < -WIN_VALUE) {
    if (mv == 0 && vl >= -BAN_VALUE) {
      return; // 同理，对于长捉/长将导致的判负局面，无最佳走法不记录
    }
    // 对于输棋，减去距离，存入绝对被杀分数
    vl -= pos.nDistance;
  } else if (vl == pos.DrawValue() && mv == 0) {
    return;   // 和棋无最佳着法时同样不记录，避免图历史交互问题(GHI)
  }

  // 2. 逐层试探置换表；
  // 置换表往往采用多层机制（如主层和副层）来减少冲突
  nMinDepth = 512; // 初始设一个极大的深度
  nMinLayer = 0;   // 准备记录最浅（最没价值被替换）的层索引
  for (i = 0; i < HASH_LAYERS; i ++) {
    hsh = HASH_ITEM(pos, i); // 获取第i层的哈希项

    // 3. 如果试探到一样的局面（Zobrist锁匹配），那么更新置换表信息即可；
    if (HASH_POS_EQUAL(hsh, pos)) {
      // 如果本次搜索边界包含Alpha（最多只能证明不超过vl），且深度更深或上界变小(更严格)，则更新Alpha边界
      if ((nFlag & HASH_ALPHA) != 0 && (hsh.ucAlphaDepth <= nDepth || hsh.svlAlpha >= vl)) {
        hsh.ucAlphaDepth = nDepth;
        hsh.svlAlpha = vl;
      }
      // Beta结点要注意：不要用空步(Null-Move)裁剪产生的结点轻易覆盖包含正常着法和更准确边界的结点
      if ((nFlag & HASH_BETA) != 0 && (hsh.ucBetaDepth <= nDepth || hsh.svlBeta <= vl) && (mv != 0 || hsh.wmv == 0)) {
        hsh.ucBetaDepth = nDepth;
        hsh.svlBeta = vl;
      }
      // 最佳着法是始终覆盖的，因为新的搜索或更靠后的搜索可能产生更好的启发
      if (mv != 0) {
        hsh.wmv = mv;
      }
      // 把更新后的项写回 Hash 表
      HASH_ITEM(pos, i) = hsh;
      return;
    }

    // 4. 如果不是一样的局面（发生散列冲突），则准备替换现有记录。
    // 计算当前检查位置的记录优先级(基于深度)，深度越深价值越大越要保留
    nHashDepth = MAX((hsh.ucAlphaDepth == 0 ? 0 : hsh.ucAlphaDepth + 256),
        (hsh.wmv == 0 ? hsh.ucBetaDepth : hsh.ucBetaDepth + 256));
    __ASSERT(nHashDepth < 512);
    // 找出所有层中深度最小、最值得被踢出的那个位置
    if (nHashDepth < nMinDepth) {
      nMinDepth = nHashDepth;
      nMinLayer = i;
    }
  }

  // 5. 记录置换表。此时说明当前局面不在表中，写入我们刚刚挑选的最弱的 nMinLayer 槽位
  hsh.dwZobristLock0 = pos.zobr.dwLock0; // 保存 64 位哈希高 32 位锁
  hsh.dwZobristLock1 = pos.zobr.dwLock1; // 保存 64 位哈希低 32 位锁
  hsh.wmv = mv; // 记录最佳着法
  hsh.ucAlphaDepth = hsh.ucBetaDepth = 0; // 初始化深度
  hsh.svlAlpha = hsh.svlBeta = 0;         // 初始化边界分
  // 根据传入的标志，分别填入对应边界类型、深度及分数
  if ((nFlag & HASH_ALPHA) != 0) {
    hsh.ucAlphaDepth = nDepth;
    hsh.svlAlpha = vl;
  }
  if ((nFlag & HASH_BETA) != 0) {
    hsh.ucBetaDepth = nDepth;
    hsh.svlBeta = vl;
  }
  // 写回到找出的最佳替换层
  HASH_ITEM(pos, nMinLayer) = hsh;
}

/* 判断获取置换表要符合哪些条件，置换表的分值针对四个不同的区间有不同的处理：
 * 一、如果分值在"WIN_VALUE"以内(即介于"-WIN_VALUE"到"WIN_VALUE"之间，下同)，则只获取满足搜索深度要求的局面；
 * 二、如果分值在"WIN_VALUE"和"BAN_VALUE"之间，则不能获取置换表中的值(只能获取最佳着法仅供参考)，目的是防止由于长将而导致的“置换表的不稳定性”；
 * 三、如果分值在"BAN_VALUE"以外，则获取局面时不必考虑搜索深度要求，因为这些局面已经被证明是杀棋了；
 * 四、如果分值是"DrawValue()"(是第一种情况的特殊情况)，则不能获取置换表中的值(原因与第二种情况相同)。
 * 注意：对于第三种情况，要对杀棋步数进行调整！
 */
inline int ValueAdjust(const PositionStruct &pos, bool &bBanNode, bool &bMateNode, int vl) {
  bBanNode = bMateNode = false;
  if (vl > WIN_VALUE) {
    if (vl <= BAN_VALUE) {
      bBanNode = true;
    } else {
      bMateNode = true;
      vl -= pos.nDistance;
    }
  } else if (vl < -WIN_VALUE) {
    if (vl >= -BAN_VALUE) {
      bBanNode = true;
    } else {
      bMateNode = true;
      vl += pos.nDistance;
    }
  } else if (vl == pos.DrawValue()) {
    bBanNode = true;
  }
  return vl;
}

// 检测下一个着法是否稳定，有助于减少置换表的不稳定性
inline bool MoveStable(PositionStruct &pos, int mv) {
  // 判断下一个着法是否稳定的依据是：
  // 1. 没有后续着法，则假定是稳定的；
  if (mv == 0) {
    return true;
  }
  // 2. 吃子着法是稳定的；
  __ASSERT(pos.LegalMove(mv));
  if (pos.ucpcSquares[DST(mv)] != 0) {
    return true;
  }
  // 3. 可能因置换表引起路线迁移，使得路线超过"MAX_MOVE_NUM"，此时应立刻终止路线，并假定是稳定的。
  if (!pos.MakeMove(mv)) {
    return true;
  }
  return false;
}

// 检测后续路线是否稳定(不是循环路线)，有助于减少置换表的不稳定性
static bool PosStable(const PositionStruct &pos, int mv) {
  HashStruct hsh;
  int i, nMoveNum;
  bool bStable;
  // pos会沿着路线变化，但最终会还原，所以被视为"const"，而让"posMutable"承担非"const"的角色
  PositionStruct &posMutable = (PositionStruct &) pos;

  __ASSERT(mv != 0);
  nMoveNum = 0;
  bStable = true;
  while (!MoveStable(posMutable, mv)) {
    nMoveNum ++; // "!MoveStable()"表明已经执行了一个着法，以后需要撤消
    // 执行这个着法，如果产生循环，那么终止后续路线，并确认该路线不稳定
    if (posMutable.RepStatus() > 0) {
      bStable = false;
      break;
    }
    // 逐层获取置换表项，方法同"ProbeHash()"
    for (i = 0; i < HASH_LAYERS; i ++) {
      hsh = HASH_ITEM(posMutable, i);
      if (HASH_POS_EQUAL(hsh, posMutable)) {
        break;
      }
    }
    mv = (i == HASH_LAYERS ? 0 : hsh.wmv);
  }
  // 撤消前面执行过的所有着法
  for (i = 0; i < nMoveNum; i ++) {
    posMutable.UndoMakeMove();
  }
  return bStable;
}

// 获取置换表局面信息(如果在表中没有命中任何有价值信息时，返回非常小值的"-MATE_VALUE")
int ProbeHash(const PositionStruct &pos, int vlAlpha, int vlBeta, int nDepth, bool bNoNull, int &mv) {
  HashStruct hsh;
  int i, vl;
  bool bBanNode, bMateNode;
  // 获取置换表局面信息的过程包括以下几个步骤：

  // 1. 逐层获取（遍历）置换表项，通过Zobrist匹配寻找该局面是否已存在
  mv = 0;
  for (i = 0; i < HASH_LAYERS; i ++) {
    hsh = HASH_ITEM(pos, i); // 读取对应层的哈希条目
    if (HASH_POS_EQUAL(hsh, pos)) { // 如果高低32位Zobrist锁均能匹配上
      mv = hsh.wmv; // 取出表内记录的该局面最佳着法
      __ASSERT(mv == 0 || pos.LegalMove(mv)); // 断言判断表里的着法合法性，防止因为哈希碰撞引发不合法动作
      break; // 一旦找到则立刻终止寻找过程
    }
  }
  // 若扫描完所有层(HASH_LAYERS)仍没有匹配，证明缓存未命中，返回无用值
  if (i == HASH_LAYERS) {
    return -MATE_VALUE;
  }

  // 2. 判断是否符合Beta边界（下界 cut-off 触发判断）
  // Beta边界意味着这个搜索分支对于对手来说太差，肯定不会走，可以被立刻剪掉
  if (hsh.ucBetaDepth > 0) { // 置换表内保存了Beta分数
    vl = ValueAdjust(pos, bBanNode, bMateNode, hsh.svlBeta); // 对分数做相关的提取转换(特别是关于杀棋步数差等)
    // 满足以下几个条件才采纳截断并返回值：
    //  a. 不是长将/长捉节点(!bBanNode)
    //  b. 不是（表中没有着法并且当前禁止空步启发(bNoNull)）
    //  c. 表中的搜索深度(ucBetaDepth)不比我们目前需要求的深度(nDepth)浅，或者本身就是个肯定能赢的杀棋节点(bMateNode)
    //  d. 表里的分数值能够直接令其产生Beta裁剪(vl >= vlBeta)
    if (!bBanNode && !(hsh.wmv == 0 && bNoNull) && (hsh.ucBetaDepth >= nDepth || bMateNode) && vl >= vlBeta) {
      __ASSERT_BOUND(1 - MATE_VALUE, vl, MATE_VALUE - 1);
      // 此处最后还要确认表里记录的最佳着法是否会在当前棋盘产生不稳定环路影响图历史(PosStable)
      if (hsh.wmv == 0 || PosStable(pos, hsh.wmv)) {
        return vl; // 确认没问题，把该值当结果返回，成功避免了大量的子树搜索
      }
    }
  }

  // 3. 判断是否符合Alpha边界 (上界 fail-low 触发判断)
  // Alpha边界意味着这个搜索分支当前连保底的分数都没达到，没找到好的走法
  if (hsh.ucAlphaDepth > 0) { // 置换表内保存了Alpha分数
    vl = ValueAdjust(pos, bBanNode, bMateNode, hsh.svlAlpha); // 同样地做提取转换
    // 条件类似：不是循环禁手节点，且表中的搜索深度足够或者是一个肯定输的局面(bMateNode)，且值(vl <= vlAlpha)证明其差劲
    if (!bBanNode && (hsh.ucAlphaDepth >= nDepth || bMateNode) && vl <= vlAlpha) {
      __ASSERT_BOUND(1 - MATE_VALUE, vl, MATE_VALUE - 1);
      if (hsh.wmv == 0 || PosStable(pos, hsh.wmv)) {
        return vl; // 返回极低价值，导致该次搜索向上层直接汇报差成绩
      }
    }
  }
  return -MATE_VALUE; // 虽然找得到了结点且提供了最佳参考着法，但不满足深度或边界截断条件，那么也得重搜。
}

#ifdef HASH_QUIESC

// 存储置换表局面信息(针对没有固定深度的“静态搜索” Quiescence Search)
// 发生在上层深度已竭尽，但局面吃子仍未平静时，用另一张表记录静态评估值的上下界
void RecordHashQ(const PositionStruct &pos, int vlBeta, int vlAlpha) {
  volatile HashStruct *lphsh;
  __ASSERT((vlBeta > -WIN_VALUE && vlBeta < WIN_VALUE) || (vlAlpha > -WIN_VALUE && vlAlpha < WIN_VALUE));
  // 静态搜索的哈希表直接根据key和掩码取单格(没有分层和多槽策略，追求极简极速)
  lphsh = hshItemsQ + (pos.zobr.dwKey & nHashMask);
  lphsh->dwZobristLock0 = pos.zobr.dwLock0;
  lphsh->svlAlpha = vlAlpha;
  lphsh->svlBeta = vlBeta;
  lphsh->dwZobristLock1 = pos.zobr.dwLock1; // 两个锁的存入方式跟主表一致
}

// 获取置换表局面信息(静态搜索 Quiescence Search)
int ProbeHashQ(const PositionStruct &pos, int vlAlpha, int vlBeta) {
  volatile HashStruct *lphsh;
  int vlHashAlpha, vlHashBeta;

  lphsh = hshItemsQ + (pos.zobr.dwKey & nHashMask); // 无须多层遍历，直接通过位运算寻址
  // 判断Zobrist两段锁是否均匹配一致(避免哈希碰撞)
  if (lphsh->dwZobristLock0 == pos.zobr.dwLock0) {
    vlHashAlpha = lphsh->svlAlpha;
    vlHashBeta = lphsh->svlBeta;
    if (lphsh->dwZobristLock1 == pos.zobr.dwLock1) {
      // 如果获取到了足够大的Beta分，意味着足以引发静态搜索内的截断
      if (vlHashBeta >= vlBeta) {
        __ASSERT(vlHashBeta > -WIN_VALUE && vlHashBeta < WIN_VALUE);
        return vlHashBeta;
      }
      // 如果获取到了极小的Alpha分，意味着在静态搜索中这依然是个较差节点没必要搜下去
      if (vlHashAlpha <= vlAlpha) {
        __ASSERT(vlHashAlpha > -WIN_VALUE && vlHashAlpha < WIN_VALUE);
        return vlHashAlpha;
      }
    }
  }
  return -MATE_VALUE;
}

#endif

#ifndef CCHESS_A3800

// UCCI支持 - 输出Hash表中的局面信息
bool PopHash(const PositionStruct &pos) {
  HashStruct hsh;
  uint32_t dwMoveStr;
  int i;

  for (i = 0; i < HASH_LAYERS; i ++) {
    hsh = HASH_ITEM(pos, i);
    if (HASH_POS_EQUAL(hsh, pos)) {
      printf("pophash");
      if (hsh.wmv != 0) {
        __ASSERT(pos.LegalMove(hsh.wmv));
        dwMoveStr = MOVE_COORD(hsh.wmv);
        printf(" bestmove %.4s", (const char *) &dwMoveStr);
      }
      if (hsh.ucBetaDepth > 0) {
        printf(" lowerbound %d depth %d", hsh.svlBeta, hsh.ucBetaDepth);
      }
      if (hsh.ucAlphaDepth > 0) {
        printf(" upperbound %d depth %d", hsh.svlAlpha, hsh.ucAlphaDepth);
      }
      printf("\n");
      fflush(stdout);
      return true;
    }
  }
  return false;
}

#endif