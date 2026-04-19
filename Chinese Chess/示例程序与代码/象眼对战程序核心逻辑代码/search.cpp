/*
search.h/search.cpp - Source Code for ElephantEye, Part VIII

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.32, Last Modified: May 2012
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
#include "../base/base2.h"
#include "pregen.h"
#include "position.h"
#include "hash.h"
#ifndef CCHESS_A3800
  #include "ucci.h"
  #include "book.h"
#endif
#include "movesort.h"
#include "search.h"

const int IID_DEPTH = 2;         // 内部迭代加深的深度
const int SMP_DEPTH = 6;         // 并行搜索的深度
const int UNCHANGED_DEPTH = 4;   // 未改变最佳着法的深度

const int DROPDOWN_VALUE = 20;   // 落后的分值
const int RESIGN_VALUE = 300;    // 认输的分值
const int DRAW_OFFER_VALUE = 40; // 提和的分值

SearchStruct Search;

// 搜索信息，是封装在模块内部的
static struct {
  int64_t llTime;                     // 计时器
  bool bStop, bPonderStop;            // 中止信号和后台思考认为的中止信号
  bool bPopPv, bPopCurrMove;          // 是否输出pv和currmove
  int nPopDepth, vlPopValue;          // 输出的深度和分值
  int nAllNodes, nMainNodes;          // 总结点数和主搜索树的结点数
  int nUnchanged;                     // 未改变最佳着法的深度
  uint16_t wmvPvLine[MAX_MOVE_NUM];   // 主要变例路线上的着法列表
  uint16_t wmvKiller[LIMIT_DEPTH][2]; // 杀手着法表
  MoveSortStruct MoveSort;            // 根结点的着法序列
} Search2;

#ifndef CCHESS_A3800

void BuildPos(PositionStruct &pos, const UcciCommStruct &UcciComm) {
  int i, mv;
  pos.FromFen(UcciComm.szFenStr);
  for (i = 0; i < UcciComm.nMoveNum; i ++) {
    mv = COORD_MOVE(UcciComm.lpdwMovesCoord[i]);
    if (mv == 0) {
      break;
    }
    if (pos.LegalMove(mv) && pos.MakeMove(mv) && pos.LastMove().CptDrw > 0) {
      // 始终让pos.nMoveNum反映没吃子的步数
      pos.SetIrrev();
    }
  }
}

#endif

// 中断例程
static bool Interrupt(void) {
  if (Search.bIdle) {
    Idle();
  }
  if (Search.nGoMode == GO_MODE_NODES) {
    if (!Search.bPonder && Search2.nAllNodes > Search.nNodes * 4) {
      Search2.bStop = true;
      return true;
    }
  } else if (Search.nGoMode == GO_MODE_TIMER) {
    if (!Search.bPonder && (int) (GetTime() - Search2.llTime) > Search.nMaxTimer) {
      Search2.bStop = true;
      return true;
    }
  }
  if (Search.bBatch) {
    return false;
  }

#ifdef CCHESS_A3800
  return false;
#else
  UcciCommStruct UcciComm;
  PositionStruct posProbe;
  // 如果不是批处理模式，那么先调用UCCI解释程序，再判断是否中止
  switch (BusyLine(UcciComm, Search.bDebug)) {
  case UCCI_COMM_ISREADY:
    // "isready"指令实际上没有意义
    printf("readyok\n");
    fflush(stdout);
    return false;
  case UCCI_COMM_PONDERHIT:
    // "ponderhit"指令启动计时功能，如果"SearchMain()"例程认为已经搜索了足够的时间， 那么发出中止信号
    if (Search2.bPonderStop) {
      Search2.bStop = true;
      return true;
    } else {
      Search.bPonder = false;
      return false;
    }
  case UCCI_COMM_PONDERHIT_DRAW:
    // "ponderhit draw"指令启动计时功能，并设置提和标志
    Search.bDraw = true;
    if (Search2.bPonderStop) {
      Search2.bStop = true;
      return true;
    } else {
      Search.bPonder = false;
      return false;
    }
  case UCCI_COMM_STOP:
    // "stop"指令发送中止信号
    Search2.bStop = true;
    return true;
  case UCCI_COMM_PROBE:
    // "probe"指令输出Hash表信息
    BuildPos(posProbe, UcciComm);
    PopHash(posProbe);
    return false;
  case UCCI_COMM_QUIT:
    // "quit"指令发送退出信号
    Search.bQuit = Search2.bStop = true;
    return true;
  default:
    return false;
  }
#endif
}

#ifndef CCHESS_A3800

// 输出主要变例
static void PopPvLine(int nDepth = 0, int vl = 0) {
  uint16_t *lpwmv;
  uint32_t dwMoveStr;
  // 如果尚未达到需要输出的深度，那么记录该深度和分值，以后再输出
  if (nDepth > 0 && !Search2.bPopPv && !Search.bDebug) {
    Search2.nPopDepth = nDepth;
    Search2.vlPopValue = vl;
    return;
  }
  // 输出时间和搜索结点数
  printf("info time %d nodes %d\n", (int) (GetTime() - Search2.llTime), Search2.nAllNodes);
  fflush(stdout);
  if (nDepth == 0) {
    // 如果是搜索结束后的输出，并且已经输出过，那么不必再输出
    if (Search2.nPopDepth == 0) {
      return;
    }
    // 获取以前没有输出的深度和分值
    nDepth = Search2.nPopDepth;
    vl = Search2.vlPopValue;
  } else {
    // 达到需要输出的深度，那么以后不必再输出
    Search2.nPopDepth = Search2.vlPopValue = 0;
  }
  printf("info depth %d score %d pv", nDepth, vl);
  lpwmv = Search2.wmvPvLine;
  while (*lpwmv != 0) {
    dwMoveStr = MOVE_COORD(*lpwmv);
    printf(" %.4s", (const char *) &dwMoveStr);
    lpwmv ++;
  }
  printf("\n");
  fflush(stdout);
}

#endif

// 无害裁剪
static int HarmlessPruning(const PositionStruct &pos, int vlBeta) {
  int vl, vlRep;

  // 1. 杀棋步数裁剪；
  vl = pos.nDistance - MATE_VALUE;
  if (vl >= vlBeta) {
    return vl;
  }

  // 2. 和棋裁剪；
  if (pos.IsDraw()) {
    return 0; // 安全起见，这里不用"pos.DrawValue()";
  }

  // 3. 重复裁剪；
  vlRep = pos.RepStatus();
  if (vlRep > 0) {
    return pos.RepValue(vlRep);
  }

  return -MATE_VALUE;
}

// 调整型局面评价函数
inline int Evaluate(const PositionStruct &pos, int vlAlpha, int vlBeta) {
  int vl;
  vl = Search.bKnowledge ? pos.Evaluate(vlAlpha, vlBeta) : pos.Material();
  return vl == pos.DrawValue() ? vl - 1 : vl;
}

// 静态搜索例程
static int SearchQuiesc(PositionStruct &pos, int vlAlpha, int vlBeta) {
  int vlBest, vl, mv;
  bool bInCheck;
  MoveSortStruct MoveSort;  
  // 静态搜索例程包括以下几个步骤：
  Search2.nAllNodes ++;

  // 1. 无害裁剪；
  vl = HarmlessPruning(pos, vlBeta);
  if (vl > -MATE_VALUE) {
    return vl;
  }

#ifdef HASH_QUIESC
  // 3. 置换裁剪；
  vl = ProbeHashQ(pos, vlAlpha, vlBeta);
  if (Search.bUseHash && vl > -MATE_VALUE) {
    return vl;
  }
#endif

  // 4. 达到极限深度，直接返回评价值；
  if (pos.nDistance == LIMIT_DEPTH) {
    return Evaluate(pos, vlAlpha, vlBeta);
  }
  __ASSERT(Search.pos.nDistance < LIMIT_DEPTH);

  // 5. 初始化；
  vlBest = -MATE_VALUE;
  bInCheck = (pos.LastMove().ChkChs > 0);

  // 6. 对于被将军的局面，生成全部着法；
  if (bInCheck) {
    MoveSort.InitAll(pos);
  } else {

    // 7. 对于未被将军的局面，在生成着法前首先尝试空着(空着启发)，即对局面作评价；
    vl = Evaluate(pos, vlAlpha, vlBeta);
    __ASSERT_BOUND(1 - WIN_VALUE, vl, WIN_VALUE - 1);
    __ASSERT(vl > vlBest);
    if (vl >= vlBeta) {
#ifdef HASH_QUIESC
      RecordHashQ(pos, vl, MATE_VALUE);
#endif
      return vl;
    }
    vlBest = vl;
    vlAlpha = MAX(vl, vlAlpha);

    // 8. 对于未被将军的局面，生成并排序所有吃子着法(MVV(LVA)启发)；
    MoveSort.InitQuiesc(pos);
  }

  // 9. 用Alpha-Beta算法搜索这些着法；
  while ((mv = MoveSort.NextQuiesc(bInCheck)) != 0) {
    __ASSERT(bInCheck || pos.ucpcSquares[DST(mv)] > 0);
    if (pos.MakeMove(mv)) {
      vl = -SearchQuiesc(pos, -vlBeta, -vlAlpha);
      pos.UndoMakeMove();
      if (vl > vlBest) {
        if (vl >= vlBeta) {
#ifdef HASH_QUIESC
          if (vl > -WIN_VALUE && vl < WIN_VALUE) {
            RecordHashQ(pos, vl, MATE_VALUE);
          }
#endif
          return vl;
        }
        vlBest = vl;
        vlAlpha = MAX(vl, vlAlpha);
      }
    }
  }

  // 10. 返回分值。
  if (vlBest == -MATE_VALUE) {
    __ASSERT(pos.IsMate());
    return pos.nDistance - MATE_VALUE;
  } else {
#ifdef HASH_QUIESC
    if (vlBest > -WIN_VALUE && vlBest < WIN_VALUE) {
      RecordHashQ(pos, vlBest > vlAlpha ? vlBest : -MATE_VALUE, vlBest);
    }
#endif
    return vlBest;
  }
}

#ifndef CCHESS_A3800

// UCCI支持 - 输出叶子结点的局面信息
void PopLeaf(PositionStruct &pos) {
  int vl;
  Search2.nAllNodes = 0;
  vl = SearchQuiesc(pos, -MATE_VALUE, MATE_VALUE);
  printf("pophash lowerbound %d depth 0 upperbound %d depth 0\n", vl, vl);
  fflush(stdout);
}

#endif

const bool NO_NULL = true; // "SearchCut()"的参数，是否禁止空着裁剪

// 零窗口完全搜索例程
static int SearchCut(int vlBeta, int nDepth, bool bNoNull = false) {
  int nNewDepth, vlBest, vl;
  int mvHash, mv, mvEvade;
  MoveSortStruct MoveSort;
  // 完全搜索例程包括以下几个步骤：

  // 1. 在叶子结点处调用静态搜索；
  if (nDepth <= 0) {
    __ASSERT(nDepth >= -NULL_DEPTH);
    return SearchQuiesc(Search.pos, vlBeta - 1, vlBeta);
  }
  Search2.nAllNodes ++;

  // 2. 无害裁剪；
  vl = HarmlessPruning(Search.pos, vlBeta);
  if (vl > -MATE_VALUE) {
    return vl;
  }

  // 3. 置换裁剪；
  vl = ProbeHash(Search.pos, vlBeta - 1, vlBeta, nDepth, bNoNull, mvHash);
  if (Search.bUseHash && vl > -MATE_VALUE) {
    return vl;
  }

  // 4. 达到极限深度，直接返回评价值；
  if (Search.pos.nDistance == LIMIT_DEPTH) {
    return Evaluate(Search.pos, vlBeta - 1, vlBeta);
  }
  __ASSERT(Search.pos.nDistance < LIMIT_DEPTH);

  // 5. 中断调用；
  Search2.nMainNodes ++;
  vlBest = -MATE_VALUE;
  if ((Search2.nMainNodes & Search.nCountMask) == 0 && Interrupt()) {
    return vlBest;
  }

  // 6. 尝试空着裁剪；
  if (Search.bNullMove && !bNoNull && Search.pos.LastMove().ChkChs <= 0 && Search.pos.NullOkay()) {
    Search.pos.NullMove();
    vl = -SearchCut(1 - vlBeta, nDepth - NULL_DEPTH - 1, NO_NULL);
    Search.pos.UndoNullMove();
    if (Search2.bStop) {
      return vlBest;
    }

    if (vl >= vlBeta) {
      if (Search.pos.NullSafe()) {
        // a. 如果空着裁剪不带检验，那么记录深度至少为(NULL_DEPTH + 1)；
        RecordHash(Search.pos, HASH_BETA, vl, MAX(nDepth, NULL_DEPTH + 1), 0);
        return vl;
      } else if (SearchCut(vlBeta, nDepth - NULL_DEPTH, NO_NULL) >= vlBeta) {
        // b. 如果空着裁剪带检验，那么记录深度至少为(NULL_DEPTH)；
        RecordHash(Search.pos, HASH_BETA, vl, MAX(nDepth, NULL_DEPTH), 0);
        return vl;
      }
    }
  }

  // 7. 初始化；
  if (Search.pos.LastMove().ChkChs > 0) {
    // 如果是将军局面，那么生成所有应将着法；
    mvEvade = MoveSort.InitEvade(Search.pos, mvHash, Search2.wmvKiller[Search.pos.nDistance]);
  } else {
    // 如果不是将军局面，那么使用正常的着法列表。
    MoveSort.InitFull(Search.pos, mvHash, Search2.wmvKiller[Search.pos.nDistance]);
    mvEvade = 0;
  }

  // 8. 按照"MoveSortStruct::NextFull()"例程的着法顺序逐一搜索；
  while ((mv = MoveSort.NextFull(Search.pos)) != 0) {
    if (Search.pos.MakeMove(mv)) {

      // 9. 尝试选择性延伸；
      nNewDepth = (Search.pos.LastMove().ChkChs > 0 || mvEvade != 0 ? nDepth : nDepth - 1);

      // 10. 零窗口搜索；
      vl = -SearchCut(1 - vlBeta, nNewDepth);
      Search.pos.UndoMakeMove();
      if (Search2.bStop) {
        return vlBest;
      }

      // 11. 截断判定；
      if (vl > vlBest) {
        vlBest = vl;
        if (vl >= vlBeta) {
          RecordHash(Search.pos, HASH_BETA, vlBest, nDepth, mv);
          if (!MoveSort.GoodCap(Search.pos, mv)) {
            SetBestMove(mv, nDepth, Search2.wmvKiller[Search.pos.nDistance]);
          }
          return vlBest;
        }
      }
    }
  }

  // 12. 不截断措施。
  if (vlBest == -MATE_VALUE) {
    __ASSERT(Search.pos.IsMate());
    return Search.pos.nDistance - MATE_VALUE;
  } else {
    RecordHash(Search.pos, HASH_ALPHA, vlBest, nDepth, mvEvade);
    return vlBest;
  }
}

// 连接主要变例
static void AppendPvLine(uint16_t *lpwmvDst, uint16_t mv, const uint16_t *lpwmvSrc) {
  *lpwmvDst = mv;
  lpwmvDst ++;
  while (*lpwmvSrc != 0) {
    *lpwmvDst = *lpwmvSrc;
    lpwmvSrc ++;
    lpwmvDst ++;
  }
  *lpwmvDst = 0;
}

/* 主要变例完全搜索例程，和零窗口完全搜索的区别有以下几点：
 *
 * 1. 启用内部迭代加深启发；
 * 2. 不使用有负面影响的裁剪；
 * 3. Alpha-Beta边界判定复杂；
 * 4. PV结点要获取主要变例；
 * 5. 考虑PV结点处理最佳着法的情况。
 */
static int SearchPV(int vlAlpha, int vlBeta, int nDepth, uint16_t *lpwmvPvLine) {
  int nNewDepth, nHashFlag, vlBest, vl;
  int mvBest, mvHash, mv, mvEvade;
  MoveSortStruct MoveSort;
  uint16_t wmvPvLine[LIMIT_DEPTH];
  // 完全搜索例程包括以下几个步骤：

  // 1. 在叶子结点处调用静态搜索；
  *lpwmvPvLine = 0;
  if (nDepth <= 0) {
    __ASSERT(nDepth >= -NULL_DEPTH);
    return SearchQuiesc(Search.pos, vlAlpha, vlBeta);
  }
  Search2.nAllNodes ++;

  // 2. 无害裁剪；
  vl = HarmlessPruning(Search.pos, vlBeta);
  if (vl > -MATE_VALUE) {
    return vl;
  }

  // 3. 置换裁剪；
  vl = ProbeHash(Search.pos, vlAlpha, vlBeta, nDepth, NO_NULL, mvHash);
  if (Search.bUseHash && vl > -MATE_VALUE) {
    // 由于PV结点不适用置换裁剪，所以不会发生PV路线中断的情况
    return vl;
  }

  // 4. 达到极限深度，直接返回评价值；
  __ASSERT(Search.pos.nDistance > 0);
  if (Search.pos.nDistance == LIMIT_DEPTH) {
    return Evaluate(Search.pos, vlAlpha, vlBeta);
  }
  __ASSERT(Search.pos.nDistance < LIMIT_DEPTH);

  // 5. 中断调用；
  Search2.nMainNodes ++;
  vlBest = -MATE_VALUE;
  if ((Search2.nMainNodes & Search.nCountMask) == 0 && Interrupt()) {
    return vlBest;
  }

  // 6. 内部迭代加深启发；
  if (nDepth > IID_DEPTH && mvHash == 0) {
    __ASSERT(nDepth / 2 <= nDepth - IID_DEPTH);
    vl = SearchPV(vlAlpha, vlBeta, nDepth / 2, wmvPvLine);
    if (vl <= vlAlpha) {
      vl = SearchPV(-MATE_VALUE, vlBeta, nDepth / 2, wmvPvLine);
    }
    if (Search2.bStop) {
      return vlBest;
    }
    mvHash = wmvPvLine[0];
  }

  // 7. 初始化；
  mvBest = 0;
  nHashFlag = HASH_ALPHA;
  if (Search.pos.LastMove().ChkChs > 0) {
    // 如果是将军局面，那么生成所有应将着法；
    mvEvade = MoveSort.InitEvade(Search.pos, mvHash, Search2.wmvKiller[Search.pos.nDistance]);
  } else {
    // 如果不是将军局面，那么使用正常的着法列表。
    MoveSort.InitFull(Search.pos, mvHash, Search2.wmvKiller[Search.pos.nDistance]);
    mvEvade = 0;
  }

  // 8. 按照"MoveSortStruct::NextFull()"例程的着法顺序逐一搜索；
  while ((mv = MoveSort.NextFull(Search.pos)) != 0) {
    if (Search.pos.MakeMove(mv)) {

      // 9. 尝试选择性延伸；
      nNewDepth = (Search.pos.LastMove().ChkChs > 0 || mvEvade != 0 ? nDepth : nDepth - 1);

      // 10. 主要变例搜索；
      if (vlBest == -MATE_VALUE) {
        vl = -SearchPV(-vlBeta, -vlAlpha, nNewDepth, wmvPvLine);
      } else {
        vl = -SearchCut(-vlAlpha, nNewDepth);
        if (vl > vlAlpha && vl < vlBeta) {
          vl = -SearchPV(-vlBeta, -vlAlpha, nNewDepth, wmvPvLine);
        }
      }
      Search.pos.UndoMakeMove();
      if (Search2.bStop) {
        return vlBest;
      }

      // 11. Alpha-Beta边界判定；
      if (vl > vlBest) {
        vlBest = vl;
        if (vl >= vlBeta) {
          mvBest = mv;
          nHashFlag = HASH_BETA;
          break;
        }
        if (vl > vlAlpha) {
          vlAlpha = vl;
          mvBest = mv;
          nHashFlag = HASH_PV;
          AppendPvLine(lpwmvPvLine, mv, wmvPvLine);
        }
      }
    }
  }

  // 12. 更新置换表、历史表和杀手着法表。
  if (vlBest == -MATE_VALUE) {
    __ASSERT(Search.pos.IsMate());
    return Search.pos.nDistance - MATE_VALUE;
  } else {
    RecordHash(Search.pos, nHashFlag, vlBest, nDepth, mvEvade == 0 ? mvBest : mvEvade);
    if (mvBest != 0 && !MoveSort.GoodCap(Search.pos, mvBest)) {
      SetBestMove(mvBest, nDepth, Search2.wmvKiller[Search.pos.nDistance]);
    }
    return vlBest;
  }
}

/* 根结点搜索例程，和完全搜索的区别有以下几点：
 *
 * 1. 省略无害裁剪(也不获取置换表着法)；
 * 2. 不使用空着裁剪；
 * 3. 选择性延伸只使用将军延伸；
 * 4. 过滤掉禁止着法；
 * 5. 搜索到最佳着法时要做很多处理(包括记录主要变例、结点排序等)；
 * 6. 不更新历史表和杀手着法表。
 */
static int SearchRoot(int nDepth) {
  int nNewDepth, vlBest, vl, mv, nCurrMove;
#ifndef CCHESS_A3800
  uint32_t dwMoveStr;
#endif
  uint16_t wmvPvLine[LIMIT_DEPTH];
  // 根结点搜索例程包括以下几个步骤：

  // 1. 初始化
  vlBest = -MATE_VALUE;
  Search2.MoveSort.ResetRoot();

  // 2. 逐一搜索每个着法(要过滤禁止着法)
  nCurrMove = 0;
  while ((mv = Search2.MoveSort.NextRoot()) != 0) {
    if (Search.pos.MakeMove(mv)) {
#ifndef CCHESS_A3800
      if (Search2.bPopCurrMove || Search.bDebug) {
        dwMoveStr = MOVE_COORD(mv);
        nCurrMove ++;
        printf("info currmove %.4s currmovenumber %d\n", (const char *) &dwMoveStr, nCurrMove);
        fflush(stdout);
      }
#endif

      // 3. 尝试选择性延伸(只考虑将军延伸)
      nNewDepth = (Search.pos.LastMove().ChkChs > 0 ? nDepth : nDepth - 1);

      // 4. 主要变例搜索
      if (vlBest == -MATE_VALUE) {
        vl = -SearchPV(-MATE_VALUE, MATE_VALUE, nNewDepth, wmvPvLine);
        __ASSERT(vl > vlBest);
      } else {
        vl = -SearchCut(-vlBest, nNewDepth);
        if (vl > vlBest) { // 这里不需要" && vl < MATE_VALUE"了
          vl = -SearchPV(-MATE_VALUE, -vlBest, nNewDepth, wmvPvLine);
        }
      }
      Search.pos.UndoMakeMove();
      if (Search2.bStop) {
        return vlBest;
      }

      // 5. Alpha-Beta边界判定("vlBest"代替了"SearchPV()"中的"vlAlpha")
      if (vl > vlBest) {

        // 6. 如果搜索到第一着法，那么"未改变最佳着法"的计数器加1，否则清零
        Search2.nUnchanged = (vlBest == -MATE_VALUE ? Search2.nUnchanged + 1 : 0);
        vlBest = vl;

        // 7. 搜索到最佳着法时记录主要变例
        AppendPvLine(Search2.wmvPvLine, mv, wmvPvLine);
#ifndef CCHESS_A3800
        PopPvLine(nDepth, vl);
#endif

        // 8. 如果要考虑随机性，则Alpha值要作随机浮动，但已搜索到杀棋时不作随机浮动
        if (vlBest > -WIN_VALUE && vlBest < WIN_VALUE) {
          vlBest += (Search.rc4Random.NextLong() & Search.nRandomMask) -
              (Search.rc4Random.NextLong() & Search.nRandomMask);
          vlBest = (vlBest == Search.pos.DrawValue() ? vlBest - 1 : vlBest);
        }

        // 9. 更新根结点着法列表
        Search2.MoveSort.UpdateRoot(mv);
      }
    }
  }
  return vlBest;
}

// 唯一着法检验是ElephantEye在搜索上的一大特色，用来判断用以某种深度进行的搜索是否找到了唯一着法。
// 其原理是把找到的最佳着法设成禁止着法，然后以(-WIN_VALUE, 1 - WIN_VALUE)的窗口重新搜索，
// 如果低出边界则说明其他着法都将被杀。
static bool SearchUnique(int vlBeta, int nDepth) {
  int vl, mv;
  // 将根节点的着法列表标志位设为 ROOT_UNIQUE，这样在取下一步时会自动跳过第一个着法（即当前找到的最佳着法）
  Search2.MoveSort.ResetRoot(ROOT_UNIQUE);
  
  // 遍历剩余的候选着法。注意，由于 ResetRoot 设置了 ROOT_UNIQUE，这里的循环实际上跳过了之前被认为是最佳的第一着法
  while ((mv = Search2.MoveSort.NextRoot()) != 0) {
    // 尝试走出这一步棋
    if (Search.pos.MakeMove(mv)) {
      // 对这步棋进行一次零窗口搜索（Zero Window Search / Null Window Search）
      // 即用一个极小的窗口 (1-vlBeta, vlBeta) 评估这步棋。在主调用中 vlBeta 通常是 1 - WIN_VALUE
      // 如果对手上一步是将军 (ChkChs > 0)，那么当前处于被将状态，采用将军延伸（不减小搜索深度，nDepth）
      // 否则深度正常递减（nDepth - 1）
      vl = -SearchCut(1 - vlBeta, Search.pos.LastMove().ChkChs > 0 ? nDepth : nDepth - 1);
      
      // 撤销刚才尝试的走法，恢复局面
      Search.pos.UndoMakeMove();
      
      // bStop 为 true 表示搜索被打断（如时间用尽或收到停止指令）
      // 如果在这个极低的期望阈值(vlBeta)下，评价函数返回的得分依然大于等于 vlBeta
      // 说明这步非最佳走步也能至少保住“没被立即将死”的底线，也就是说：目前的最佳走步实际上并不是“唯一解”，还有其他存活路线
      if (Search2.bStop || vl >= vlBeta) {
        return false; // 不是唯一着法，返回 false
      }
    }
  }
  
  // 只要没被打断，且上面循环里所有的备选走法评价得分全都不及格（全都低出了被将死的边界），
  // 说明除最佳着法外，走其它任何步都会被将死。因此，之前的最佳走法是强迫性的“唯一解”
  return true; // 是唯一着法，返回 true
}

// 主搜索例程，这是整个博弈树搜索的入口函数
// 参数 nDepth 表示最大搜索深度
void SearchMain(int nDepth) {
  int i, vl, vlLast, nDraw;
  int nCurrTimer, nLimitTimer, nLimitNodes;
  bool bUnique;
#ifndef CCHESS_A3800
  int nBookMoves;
  uint32_t dwMoveStr;
  BookStruct bks[MAX_GEN_MOVES];
#endif
  // 主搜索例程包括以下几个步骤：

  // 1. 提前判断：遇到和棋则直接返回
  // 如果当前局面已经判定为和棋（例如长将、长捉、或者到达自然限着等），或者重复局面达到3次
  if (Search.pos.IsDraw() || Search.pos.RepStatus(3) > 0) {
#ifndef CCHESS_A3800
    printf("nobestmove\n");  // UCCI协议规定，无法产生着法时输出 nobestmove
    fflush(stdout);
#endif
    return;    
  }

#ifndef CCHESS_A3800
  // 2. 从开局库中搜索着法 (如果配置允许使用开局库)
  // 使用开局库可以极大节省开局阶段的时间，并且避免走出低级错误
  if (Search.bUseBook) {
    // a. 获取开局库中当前局面的所有后续走法
    nBookMoves = GetBookMoves(Search.pos, Search.szBookFile, bks);
    if (nBookMoves > 0) {
      vl = 0;
      // 遍历所有走法，累加权重(分数)，并向UI输出开局库的信息
      for (i = 0; i < nBookMoves; i ++) {
        vl += bks[i].wvl; // 累积权重
        dwMoveStr = MOVE_COORD(bks[i].wmv);
        printf("info depth 0 score %d pv %.4s\n", bks[i].wvl, (const char *) &dwMoveStr);
        fflush(stdout);
      }
      // b. 轮盘赌算法：根据权重随机选择一个走法。权重越大的走法，被选中的概率越高。
      vl = Search.rc4Random.NextLong() % (uint32_t) vl; // 在总权重内取一个随机数
      for (i = 0; i < nBookMoves; i ++) {
        vl -= bks[i].wvl;
        if (vl < 0) {
          break; // 随机落在了这个走法的区间内
        }
      }
      __ASSERT(vl < 0);
      __ASSERT(i < nBookMoves);
      
      // c. 安全性检查：如果选择的开局库着法会导致循环局面(造成和棋或直接判负)，那么不走这个着法
      Search.pos.MakeMove(bks[i].wmv);
      if (Search.pos.RepStatus(3) == 0) { // 确认走完之后没有构成循环
        dwMoveStr = MOVE_COORD(bks[i].wmv);
        printf("bestmove %.4s", (const char *) &dwMoveStr); // 输出最佳走法
        
        // d. 给出后台思考(Ponder)的猜测着法：假设对手会走开局库中权重最大的下一步
        nBookMoves = GetBookMoves(Search.pos, Search.szBookFile, bks);
        Search.pos.UndoMakeMove(); // 恢复局面
        if (nBookMoves > 0) {
          dwMoveStr = MOVE_COORD(bks[0].wmv);
          printf(" ponder %.4s", (const char *) &dwMoveStr);
        }
        printf("\n");
        fflush(stdout);
        return; // 从开局库中找到着法后直接返回，不进行耗时的深层搜索
      }
      Search.pos.UndoMakeMove(); // 如果构成循环，则撤销测试走法，转为常规引擎搜索
    }
  }
#endif

  // 3. 如果引擎要求深度为零（例如仅用来做快速的局面评估），则返回静态搜索（Quiescence Search）评价
  if (nDepth == 0) {
#ifndef CCHESS_A3800
    // 静态搜索会一直搜索直到局面平静（没有吃子威胁），避免水平线效应导致的错误评估
    printf("info depth 0 score %d\n", SearchQuiesc(Search.pos, -MATE_VALUE, MATE_VALUE));
    fflush(stdout);
    printf("nobestmove\n"); // 深度0不产生真正的最佳着法输出
    fflush(stdout);
#endif
    return;
  }

  // 4. 生成根结点的所有合理着法
  // 按照价值(如吃子价值)对第一层节点进行初步排序，并且过滤掉 UI 指定的禁止着法（BanMoves）
  Search2.MoveSort.InitRoot(Search.pos, Search.nBanMoves, Search.wmvBanList);

  // 5. 初始化搜索环境：清空相关标志、历史表、置换表和杀手着法
  Search2.bStop = Search2.bPonderStop = Search2.bPopPv = Search2.bPopCurrMove = false;
  Search2.nPopDepth = Search2.vlPopValue = 0;
  Search2.nAllNodes = Search2.nMainNodes = Search2.nUnchanged = 0;
  Search2.wmvPvLine[0] = 0;
  ClearKiller(Search2.wmvKiller); // 清空杀手着法（导致截断的优秀非吃子着法）
  ClearHistory();                 // 清空历史表（用于着法排序的启发信息）
  ClearHash();                    // 清空置换表（Transposition Table，用于存储已搜索局面，避免重复搜索）
  
  // 由于 ClearHash() 需要消耗一定时间，所以引擎计时从此后开始比较精确
  Search2.llTime = GetTime();
  vlLast = 0;
  
  // 60步（或特定步数）自然限着规则判断。如果走了很多回合没吃子，可以考虑主动提和
  nDraw = -Search.pos.LastMove().CptDrw; 
  if (nDraw > 5 && ((nDraw - 4) / 2) % 8 == 0) {
    Search.bDraw = true; // 开启提和意图
  }
  bUnique = false;
  nCurrTimer = 0;

  // 6. 核心循环：迭代加深搜索 (Iterative Deepening)
  // 不直接搜索目标深度，而是从深度1开始逐渐增加深度。
  // 优点：1. 浅层搜索使得历史表、置换表和PV（主要变例）被预热，大幅提高深层搜索的剪枝效率。2. 方便时间管理，随时中断均有一步可用好棋。
  for (i = 1; i <= nDepth; i ++) {
    // 需要输出主要变例时，第一个"info depth n"是不输出的
#ifndef CCHESS_A3800
    if (Search2.bPopPv || Search.bDebug) {
      printf("info depth %d\n", i);
      fflush(stdout);
    }

    // 7. 根据搜索时间决定UI通讯：超过300ms偶尔输出PV(变例)，超3000ms输出正在考虑的根节点着法
    Search2.bPopPv = (nCurrTimer > 300);
    Search2.bPopCurrMove = (nCurrTimer > 3000);
#endif

    // 8. 调用实际的搜索函数探索当前深度的博弈树
    vl = SearchRoot(i); 
    
    // 如果在搜索中途被主线程（如时间到、用户命令停止）打断
    if (Search2.bStop) {
      if (vl > -MATE_VALUE) { // 确保vl有效
        vlLast = vl; // 记录中断前获得的最可靠评估分，用于之后判断是否认输
      }
      break; // 结束迭代加深循环
    }

    nCurrTimer = (int) (GetTime() - Search2.llTime);
    
    // 9. 启发式时间管理机制 (Time Management)
    // 根据具体引擎模式，动态判断是否要在此时截断搜索(提前停止)
    if (Search.nGoMode == GO_MODE_TIMER) { // 按时间搜索模式
      // a. 如果禁止了空步裁剪（Null Move Pruning），说明搜索树大，原本的适当时限需要减半
      nLimitTimer = (Search.bNullMove ? Search.nProperTimer : Search.nProperTimer / 2);
      
      // b. 稳定性时间策略：如果当前搜索到的最佳分值相比上一层并未大幅恶化 (没掉入陷阱)，说明局面相对单纯，可以将时间限制减半提前返回；
      // 反之，如果分值骤降，说明遇到了严重危机，需要保留全部的思考时间以期发现解法。
      nLimitTimer = (vl + DROPDOWN_VALUE >= vlLast ? nLimitTimer / 2 : nLimitTimer);
      
      // c. 结果一致性判断：如果最佳着法连续多层(UNCHANGED_DEPTH)都没有变化，说明该走法已经非常稳定，无需再继续浪费时间，时限再次减半。
      nLimitTimer = (Search2.nUnchanged >= UNCHANGED_DEPTH ? nLimitTimer / 2 : nLimitTimer);
      
      // 时间超时判断
      if (nCurrTimer > nLimitTimer) {
        if (Search.bPonder) {
          Search2.bPonderStop = true; // 后台思考(Ponder)模式不会主动中断，而是在标志位置位后等真实“Hit”信号到来再停止
        } else {
          vlLast = vl;
          break; // 时间已经耗尽，且已完成当前深度的完整搜索，退出循环
        }
      }
    } else if (Search.nGoMode == GO_MODE_NODES) { // 按节点数搜索模式 (常用于测试)
      // 类似时间管理，启发式缩减允许的极限节点数
      nLimitNodes = (Search.bNullMove ? Search.nNodes : Search.nNodes / 2);
      nLimitNodes = (vl + DROPDOWN_VALUE >= vlLast ? nLimitNodes / 2 : nLimitNodes);
      nLimitNodes = (Search2.nUnchanged >= UNCHANGED_DEPTH ? nLimitNodes / 2 : nLimitNodes);
      if (Search2.nAllNodes > nLimitNodes) {
        vlLast = vl;
        break;
      }
    }
    vlLast = vl; // 将本层的得分保存，用于下一层迭代的时间管理比对

    // 10. 绝杀提前返回：如果已经找到了将死对方或者必被将死的走法，无需继续加深搜索
    if (vlLast > WIN_VALUE || vlLast < -WIN_VALUE) {
      break;
    }

    // 11. 唯一着法启发 (Singular Move)检验：
    // 测试当前最佳着法是不是“唯一选择”。如果除当前走法外，其他所有走法都会导致崩溃（被杀），
    // 证明目前的选择是强迫性的，那就没必要为了计算其他劣势分支继续深搜。
    if (SearchUnique(1 - WIN_VALUE, i)) {
      bUnique = true;
      break;
    }
  }

#ifdef CCHESS_A3800
  Search.mvResult = Search2.wmvPvLine[0]; // 轻量平台直接赋值结果
#else
  // 12. 搜索结束，输出 UCCI 协议标准结果：最佳着法(bestmove) 以及猜测的对手应法(ponder)
  if (Search2.wmvPvLine[0] != 0) {
    PopPvLine(); // 打印最终的 pv 变例路径
    dwMoveStr = MOVE_COORD(Search2.wmvPvLine[0]);
    printf("bestmove %.4s", (const char *) &dwMoveStr);
    if (Search2.wmvPvLine[1] != 0) {
      dwMoveStr = MOVE_COORD(Search2.wmvPvLine[1]);
      printf(" ponder %.4s", (const char *) &dwMoveStr);
    }

    // 13. AI辅助判断：认输(Resign)或提和(Draw)
    // 注意：如果是“唯一着法”强制停止的，不能认输提和，因为可能并没有搜索得足够深
    if (!bUnique) {
      if (vlLast > -WIN_VALUE && vlLast < -RESIGN_VALUE) {
        printf(" resign"); // 分值极低，被虐爆，发送认输指令
      } else if (Search.bDraw && !Search.pos.NullSafe() && vlLast < DRAW_OFFER_VALUE * 2) {
        printf(" draw");   // 长期无进展且分数不高，发出提和意图
      }
    }
  } else {
    printf("nobestmove"); // 没有找到合理的着法（可能已经被死棋）
  }
  printf("\n");
  fflush(stdout);
#endif
}
