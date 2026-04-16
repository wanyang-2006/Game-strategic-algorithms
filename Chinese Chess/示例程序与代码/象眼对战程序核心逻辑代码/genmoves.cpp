/*
genmoves.cpp - Source Code for ElephantEye, Part IV

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.0, Last Modified: Nov. 2007
Copyright (C) 2004-2007 www.xqbase.com

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

#include "../base/base.h"
#include "pregen.h"
#include "position.h"

/* ElephantEye源程序使用的匈牙利记号约定：
 *
 * sq: 格子序号(整数，从0到255，参阅"pregen.cpp")
 * pc: 棋子序号(整数，从0到47，参阅"position.cpp")
 * pt: 棋子类型序号(整数，从0到6，参阅"position.cpp")
 * mv: 着法(整数，从0到65535，参阅"position.cpp")
 * sd: 走子方(整数，0代表红方，1代表黑方)
 * vl: 局面价值(整数，从"-MATE_VALUE"到"MATE_VALUE"，参阅"position.cpp")
 * (注：以上四个记号可与uc、dw等代表整数的记号配合使用)
 * pos: 局面(PositionStruct类型，参阅"position.h")
 * sms: 位行和位列的着法生成预置结构(参阅"pregen.h")
 * smv: 位行和位列的着法判断预置结构(参阅"pregen.h")
 */

// 本模块只涉及到"PositionStruct"中的"sdPlayer"、"ucpcSquares"和"ucsqPieces"三个成员，故省略前面的"this->"

// 棋子保护判断 如果棋子受到其它棋子保护，后续会降低评分
bool PositionStruct::Protected(int sd, int sqSrc, int sqExcept) const {
  // 参数说明: sd为保护方标志; sqSrc是被保护(判定)的棋子格子编号; sqExcept是排除产生保护计算的格子编号(常用于牵制逻辑)
  int i, sqDst, sqPin, pc, x, y, nSideTag;
  SlideMaskStruct *lpsmsRank, *lpsmsFile;
  
  // 校验目标格子是否在有效得棋盘范围内
  __ASSERT_SQUARE(sqSrc);
  
  // 获取保护方的掩码(用于位运算和数组偏移)
  nSideTag = SIDE_TAG(sd);
  
  // 如果被保护的棋子在己方半场(包含九宫)
  if (HOME_HALF(sqSrc, sd)) {
    // 如果此子处于己方九宫内
    if (IN_FORT(sqSrc)) {

      // 1. 判断是否受到帅(将)的保护
      // 获取帅(将)的位置
      sqDst = ucsqPieces[nSideTag + KING_FROM];
      // 如果帅(将)存在并且不是被排除计算的那个子
      if (sqDst != 0 && sqDst != sqExcept) {
        // 断言帅(将)所在格子合法
        __ASSERT_SQUARE(sqDst);
        // 如果帅(将)能够直接走到被保护子格(即跨度合法)，说明受帅(将)保护
        if (KING_SPAN(sqSrc, sqDst)) {
          return true;
        }
      }

      // 2. 判断是否受到仕(士)的保护
      // 遍历己方的两枚仕(士)
      for (i = ADVISOR_FROM; i <= ADVISOR_TO; i ++) {
        sqDst = ucsqPieces[nSideTag + i];
        if (sqDst != 0 && sqDst != sqExcept) {
          __ASSERT_SQUARE(sqDst);
          // 如果仕(士)的位置跨度等于1，说明可以走到且提供保护
          if (ADVISOR_SPAN(sqSrc, sqDst)) {
            return true;
          }
        }
      }
    }

    // 3. 判断是否受到相(象)的保护
    // 遍历己方的两枚相(象)
    for (i = BISHOP_FROM; i <= BISHOP_TO; i ++) {
      sqDst = ucsqPieces[nSideTag + i];
      if (sqDst != 0 && sqDst != sqExcept) {
        __ASSERT_SQUARE(sqDst);
        // 判断相(象)走到目标格是否合法（跨度是否符合“田”字）,
        // 且象眼位置(两点中间格)上必须为空（没有塞象眼）
        if (BISHOP_SPAN(sqSrc, sqDst) && ucpcSquares[BISHOP_PIN(sqSrc, sqDst)] == 0) {
          return true;
        }
      }
    }
  } else {

    // 如果不在己方半区(即已经过河)，判断受到横向兵(卒)保护
    // 4. 判断受到过河兵(卒)横向的保护 (因为兵只能向前或横着走)，检查被保护子的左边(sqDst=sqSrc-1)和右边(sqDst=sqSrc+1)格
    for (sqDst = sqSrc - 1; sqDst <= sqSrc + 1; sqDst += 2) {
      if (sqDst != sqExcept) {
        // 如果棋子未越出边界并找到对应格子中的棋子
        pc = ucpcSquares[sqDst];
        // 如果格子里棋子属于保护方，且并且至少是兵(卒)(兵兵编号是从PAWN_FROM开始)
        if ((pc & nSideTag) != 0 && PIECE_INDEX(pc) >= PAWN_FROM) {
          return true;
        }
      }
    }
  }

  // 5. 判断是否受到兵(卒)纵向的保护
  // 计算如果有一个兵能在后方保护当前格子(只看后一格)
  sqDst = SQUARE_BACKWARD(sqSrc, sd);
  if (sqDst != sqExcept) {
    pc = ucpcSquares[sqDst];
    // 如果后一格子里的子是我方的兵(卒)，说明受到兵的纵向保护
    if ((pc & nSideTag) != 0 && PIECE_INDEX(pc) >= PAWN_FROM) {
      return true;
    }
  }

  // 6. 判断受到马的保护
  // 遍历己方的所有马
  for (i = KNIGHT_FROM; i <= KNIGHT_TO; i ++) {
    sqDst = ucsqPieces[nSideTag + i];
    if (sqDst != 0 && sqDst != sqExcept) {
      __ASSERT_SQUARE(sqDst);
      // 计算马跳到受保护子所需的“马腿”格(也就是所谓的牵制格，如果马能跳过去的话)
      sqPin = KNIGHT_PIN(sqDst, sqSrc); 
      // 验证马能跳过去(不是在原地打转)且马腿(蹩马腿的格子)上没有棋子！
      if (sqPin != sqDst && ucpcSquares[sqPin] == 0) {
        return true;
      }
    }
  }

  // 取被保护子所在的列(X=FILE_X)和行(Y=RANK_Y)
  x = FILE_X(sqSrc);
  y = RANK_Y(sqSrc);
  // 获取列和行上的滑动攻击掩码指针(查表优化算法，看这个位置能被车或炮如何击中)
  lpsmsRank = RankMaskPtr(x, y);
  lpsmsFile = FileMaskPtr(x, y);

  // 7. 判断受到车的保护
  for (i = ROOK_FROM; i <= ROOK_TO; i ++) {
    sqDst = ucsqPieces[nSideTag + i];
    // 如果存在我方车、且不是被保护子本身、也不是排除计算的那个子
    if (sqDst != 0 && sqDst != sqSrc && sqDst != sqExcept) {
      // 检查车是否同在该列(FILE)中
      if (x == FILE_X(sqDst)) {
        // 使用预存的Mask阵位直接做位运算：判断此车能否吃/保护到被护子位置(利用位行位列图查询是否存在阻挡子)
        if ((lpsmsFile->wRookCap & PreGen.wBitFileMask[sqDst]) != 0) {
          return true;
        }
      } 
      // 检查车是否同在该行(RANK)中
      else if (y == RANK_Y(sqDst)) {
        // 使用预存的Mask阵位查询，此车在此行内能否护住该位置(即中间无阻挡)
        if ((lpsmsRank->wRookCap & PreGen.wBitRankMask[sqDst]) != 0) {
          return true;
        }
      }
    }
  }

  // 8. 判断受到炮的保护
  for (i = CANNON_FROM; i <= CANNON_TO; i ++) {
    sqDst = ucsqPieces[nSideTag + i];
    // 如果存在我方炮、不是被保护子本身、也不是被排除的子
    if (sqDst && sqDst != sqSrc && sqDst != sqExcept) {
      // 如果处于同列
      if (x == FILE_X(sqDst)) {
        // 利用位预存表去检查，被保护子和该炮之间是不是有且刚好有一个“炮架子”（这样才形成炮打过来的保护线路）
        if ((lpsmsFile->wCannonCap & PreGen.wBitFileMask[sqDst]) != 0) {
          return true;
        }
      } 
      // 如果处于同行
      else if (y == RANK_Y(sqDst)) {
        // 同样检查行掩码，确认是否同行恰好隔了一子形成保护
        if ((lpsmsRank->wCannonCap & PreGen.wBitRankMask[sqDst]) != 0) {
          return true;
        }
      }
    }
  }
  
  // 没有任何我方的兵种能够保护该格子上的子，返回false
  return false;
}

/* 计算MVV(LVA)值的函数
 *
 * MVV(LVA)指的是：如果被吃子无保护，那么取值MVV，否则取值MVV-LVA。
 * 由于ElephantEye的MVV(LVA)值在计算完毕后再加了1，并且有其它考虑，因此有以下几种含义：
 * a. MVV(LVA)大于1，说明被吃子价值大于攻击子(表面上是赚的)，这种吃子将首先搜索，静态搜索也将考虑这种吃子；
 * b. MVV(LVA)等于1，说明被吃子有一定价值(吃车马炮或吃过河兵卒，即便表面上是亏的，也值得一试)，静态搜索也将考虑这种吃子；
 * c. MVV(LVA)等于0，说明被吃子没有价值，静态搜索将不考虑这种吃子。
 *
 * MVV价值表"SIMPLE_VALUE"是按照帅(将)=5、车=4、马炮=3、兵(卒)=2、仕(士)相(象)=1设定的；
 * LVA价值直接体现在吃子着法生成器中。
 */
int PositionStruct::MvvLva(int sqDst, int pcCaptured, int nLva) const {
  int nMvv, nLvaAdjust;
  // 获取被吃子的基础价值（简单价值，例如将帅=5，车=4，马炮=3等）
  nMvv = SIMPLE_VALUE(pcCaptured);
  
  // 判断目标格(sqDst)上的被吃子是否受到对方(OPP_SIDE(sdPlayer))的保护
  // 如果受到保护，则将被吃子的LVA（攻击子价值）记录下来，否则记为0
  nLvaAdjust = (Protected(OPP_SIDE(sdPlayer), sqDst) ? nLva : 0);
  
  // 如果被吃掉的棋子价值(nMvv)大于或等于攻击方的调整价值(nLvaAdjust)
  // 说明吃子是赚的（或至少不亏），返回计算后的得分（加1以保证结果大于0）
  if (nMvv >= nLvaAdjust) {
    return nMvv - nLvaAdjust + 1;
  } else {
    // 如果吃子表面上是亏的（例如兵吃车受保护，但此时nMvv<nLvaAdjust）
    // 判断被吃子是否有较高战术价值（nMvv >= 3 即车、马、炮）
    // 或者该位置在本方半场（可能是必须消除的威胁），如果是则返回1，否则返回0表示不值得考虑
    return (nMvv >= 3 || HOME_HALF(sqDst, sdPlayer)) ? 1 : 0;
  }
}

// 吃子着法生成器，按MVV(LVA)设定分值
int PositionStruct::GenCapMoves(MoveStruct *lpmvs) const {
  int i, sqSrc, sqDst, pcCaptured;
  int x, y, nSideTag, nOppSideTag;
  bool bCanPromote;
  SlideMoveStruct *lpsmv;
  uint8_t *lpucsqDst, *lpucsqPin;
  MoveStruct *lpmvsCurr;
  // 生成吃子着法的过程包括以下几个步骤：

  lpmvsCurr = lpmvs;
  nSideTag = SIDE_TAG(sdPlayer);
  nOppSideTag = OPP_SIDE_TAG(sdPlayer);
  bCanPromote = PreEval.bPromotion && CanPromote();

  // 1. 生成帅(将)的着法
  sqSrc = ucsqPieces[nSideTag + KING_FROM];
  if (sqSrc != 0) {
    __ASSERT_SQUARE(sqSrc);
    lpucsqDst = PreGen.ucsqKingMoves[sqSrc];
    sqDst = *lpucsqDst;
    while (sqDst != 0) {
      __ASSERT_SQUARE(sqDst);
      // 找到一个着法后，首先判断吃到的棋子是否是对方棋子，技巧是利用"nOppSideTag"的标志(16和32颠倒)，
      // 如果是对方棋子，则保存MVV(LVA)值，即如果被吃子无保护，则只记MVV，否则记MVV-LVA(如果MVV>LVA的话)。
      pcCaptured = ucpcSquares[sqDst];               //获取即将走到的目标格子
      if ((pcCaptured & nOppSideTag) != 0) {
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));       //是否符合走子规则
        lpmvsCurr->wmv = MOVE(sqSrc, sqDst);           //将吃子动作 存入当前的看法列表指针lpmvsCurr指向的结构体中
        lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 5); // 帅(将)的价值是5(自身的子力价值) 
        lpmvsCurr ++;                    
      }
      lpucsqDst ++;         //下一个能走的方向
      sqDst = *lpucsqDst;   //拿到下一个目标格子的编号
    }
  }

  // 2. 生成仕(士)的着法
  
  for (i = ADVISOR_FROM; i <= ADVISOR_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqAdvisorMoves[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 1); // 仕(士)的价值是1
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
      }
      if (bCanPromote && CAN_PROMOTE(sqSrc)) {
        lpmvsCurr->wmv = MOVE(sqSrc, sqSrc);
        lpmvsCurr->wvl = 0;
        lpmvsCurr ++;
      }
    }
  }

  // 3. 生成相(象)的着法
  for (i = BISHOP_FROM; i <= BISHOP_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqBishopMoves[sqSrc];
      lpucsqPin = PreGen.ucsqBishopPins[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[*lpucsqPin] == 0) {
          pcCaptured = ucpcSquares[sqDst];
          if ((pcCaptured & nOppSideTag) != 0) {
            __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
            lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
            lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 1); // 相(象)的价值是1
            lpmvsCurr ++;
          }
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
        lpucsqPin ++;
      }
      if (bCanPromote && CAN_PROMOTE(sqSrc)) {
        lpmvsCurr->wmv = MOVE(sqSrc, sqSrc);
        lpmvsCurr->wvl = 0;
        lpmvsCurr ++;
      }
    }
  }

  // 4. 生成马的着法
  for (i = KNIGHT_FROM; i <= KNIGHT_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqKnightMoves[sqSrc];
      lpucsqPin = PreGen.ucsqKnightPins[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[*lpucsqPin] == 0) {
          pcCaptured = ucpcSquares[sqDst];
          if ((pcCaptured & nOppSideTag) != 0) {
            __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
            lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
            lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 3); // 马的价值是3
            lpmvsCurr ++;
          }
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
        lpucsqPin ++;
      }
    }
  }

  // 5. 生成车的着法
  for (i = ROOK_FROM; i <= ROOK_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      x = FILE_X(sqSrc);
      y = RANK_Y(sqSrc);

      lpsmv = RankMovePtr(x, y);
      sqDst = lpsmv->ucRookCap[0] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 4); // 车的价值是4
          lpmvsCurr ++;
        }
      }
      sqDst = lpsmv->ucRookCap[1] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 4); // 车的价值是4
          lpmvsCurr ++;
        }
      }

      lpsmv = FileMovePtr(x, y);
      sqDst = lpsmv->ucRookCap[0] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 4); // 车的价值是4
          lpmvsCurr ++;
        }
      }
      sqDst = lpsmv->ucRookCap[1] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 4); // 车的价值是4
          lpmvsCurr ++;
        }
      }
    }
  }

  // 6. 生成炮的着法
  for (i = CANNON_FROM; i <= CANNON_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      // 提取炮所在的列(X)与行(Y)
      x = FILE_X(sqSrc);
      y = RANK_Y(sqSrc);

      // 获取行移动预置掩码
      lpsmv = RankMovePtr(x, y);
      // 炮在横向上吃到的第一个子（左方向隔子打）
      sqDst = lpsmv->ucCannonCap[0] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) { // 吃到对方棋子
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 3); // 炮由于需要炮架，自身价值为3
          lpmvsCurr ++;
        }
      }
      // 炮在横向上吃到的第二个子（右方向隔子打）
      sqDst = lpsmv->ucCannonCap[1] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) { // 吃到对方棋子
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 3); // 炮的价值是3
          lpmvsCurr ++;
        }
      }

      // 获取列移动预置掩码
      lpsmv = FileMovePtr(x, y);
      // 炮在纵向上吃到的第一个子（前方向隔子打）
      sqDst = lpsmv->ucCannonCap[0] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) { // 吃到对方棋子
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 3); // 炮的价值是3
          lpmvsCurr ++;
        }
      }
      // 炮在纵向上吃到的第二个子（后方向隔子打）
      sqDst = lpsmv->ucCannonCap[1] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      if (sqDst != sqSrc) {
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) { // 吃到对方棋子
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 3); // 炮的价值是3
          lpmvsCurr ++;
        }
      }
    }
  }

  // 7. 生成兵(卒)的着法
  for (i = PAWN_FROM; i <= PAWN_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqPawnMoves[sdPlayer][sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        pcCaptured = ucpcSquares[sqDst];
        if ((pcCaptured & nOppSideTag) != 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
          lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 2); // 兵(卒)的价值是2
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
      }
    }
  }
  return lpmvsCurr - lpmvs;
}

// 不吃子着法生成器
int PositionStruct::GenNonCapMoves(MoveStruct *lpmvs) const {
  int i, sqSrc, sqDst, x, y, nSideTag;
  SlideMoveStruct *lpsmv;
  uint8_t *lpucsqDst, *lpucsqPin;
  MoveStruct *lpmvsCurr;
  // 生成不吃子着法的过程包括以下几个步骤：

  lpmvsCurr = lpmvs;
  nSideTag = SIDE_TAG(sdPlayer);

  // 1. 生成帅(将)的着法
  sqSrc = ucsqPieces[nSideTag + KING_FROM];
  if (sqSrc != 0) {
    __ASSERT_SQUARE(sqSrc);
    lpucsqDst = PreGen.ucsqKingMoves[sqSrc];
    sqDst = *lpucsqDst;
    while (sqDst != 0) {
      __ASSERT_SQUARE(sqDst);
      // 找到一个着法后，首先判断是否吃到棋子
      if (ucpcSquares[sqDst] == 0) {
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
        lpmvsCurr ++;
      }
      lpucsqDst ++;
      sqDst = *lpucsqDst;
    }
  }

  // 2. 生成仕(士)的着法
  for (i = ADVISOR_FROM; i <= ADVISOR_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqAdvisorMoves[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[sqDst] == 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
      }
    }
  }

  // 3. 生成相(象)的着法
  for (i = BISHOP_FROM; i <= BISHOP_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqBishopMoves[sqSrc];
      lpucsqPin = PreGen.ucsqBishopPins[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[*lpucsqPin] == 0 && ucpcSquares[sqDst] == 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
        lpucsqPin ++;
      }
    }
  }

  // 4. 生成马的着法
  for (i = KNIGHT_FROM; i <= KNIGHT_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqKnightMoves[sqSrc];
      lpucsqPin = PreGen.ucsqKnightPins[sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[*lpucsqPin] == 0 && ucpcSquares[sqDst] == 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
        lpucsqPin ++;
      }
    }
  }

  // 5. 生成车和炮的着法，没有必要判断是否吃到本方棋子
  for (i = ROOK_FROM; i <= CANNON_TO; i ++) { // 遍历顺序是从车起至炮止(涵盖所有车和炮)
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      x = FILE_X(sqSrc);
      y = RANK_Y(sqSrc);

      // 获取行方向上的滑步范围(左右移动)
      lpsmv = RankMovePtr(x, y);
      // ucNonCap[0]表示向左滑动不吃子时最远可到达的相对横坐标
      sqDst = lpsmv->ucNonCap[0] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      // 从最远空格依次往回遍历，直到原位置（避免跨子等非法横向移动）
      while (sqDst != sqSrc) {
        __ASSERT(ucpcSquares[sqDst] == 0); // 断言必定为空格
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
        lpmvsCurr ++;
        sqDst --; // 这里--代表向右(原位置方向)移动一格
      }
      // ucNonCap[1]表示向右滑动不吃子时最远可到达的相对横坐标
      sqDst = lpsmv->ucNonCap[1] + RANK_DISP(y);
      __ASSERT_SQUARE(sqDst);
      // 从右侧最远可达空格往回依次遍历生成合法不吃子着法
      while (sqDst != sqSrc) {
        __ASSERT(ucpcSquares[sqDst] == 0);
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
        lpmvsCurr ++;
        sqDst ++; // 这里++是因为之前是从最右边开始回溯
      }

      // 获取列方向上的滑步范围(上下移动)
      lpsmv = FileMovePtr(x, y);
      // ucNonCap[0]表示向前(或上)不吃子最远距离
      sqDst = lpsmv->ucNonCap[0] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      // 以每16为步长（棋盘Y轴跨度，16x16数组）回溯生成纵向合法移动
      while (sqDst != sqSrc) {
        __ASSERT(ucpcSquares[sqDst] == 0);
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
        lpmvsCurr ++;
        sqDst -= 16; 
      }
      // ucNonCap[1]表示向后(或下)不吃子最远距离
      sqDst = lpsmv->ucNonCap[1] + FILE_DISP(x);
      __ASSERT_SQUARE(sqDst);
      // 回溯生成着法
      while (sqDst != sqSrc) {
        __ASSERT(ucpcSquares[sqDst] == 0);
        __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
        lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
        lpmvsCurr ++;
        sqDst += 16;
      }
    }
  }

  // 6. 生成兵(卒)的着法
  for (i = PAWN_FROM; i <= PAWN_TO; i ++) {
    sqSrc = ucsqPieces[nSideTag + i];
    if (sqSrc != 0) {
      __ASSERT_SQUARE(sqSrc);
      lpucsqDst = PreGen.ucsqPawnMoves[sdPlayer][sqSrc];
      sqDst = *lpucsqDst;
      while (sqDst != 0) {
        __ASSERT_SQUARE(sqDst);
        if (ucpcSquares[sqDst] == 0) {
          __ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
          lpmvsCurr->dwmv = MOVE(sqSrc, sqDst);
          lpmvsCurr ++;
        }
        lpucsqDst ++;
        sqDst = *lpucsqDst;
      }
    }
  }
  return lpmvsCurr - lpmvs;
}

// “捉”的检测
int PositionStruct::ChasedBy(int mv) const {
  int i, nSideTag, pcMoved, pcCaptured;
  int sqSrc, sqDst, x, y;
  uint8_t *lpucsqDst, *lpucsqPin;
  SlideMoveStruct *lpsmv;

  sqSrc = DST(mv);
  pcMoved = this->ucpcSquares[sqSrc];
  nSideTag = SIDE_TAG(this->sdPlayer);
  __ASSERT_SQUARE(sqSrc);
  __ASSERT_PIECE(pcMoved);
  __ASSERT_BOUND(0, pcMoved - OPP_SIDE_TAG(this->sdPlayer), 15);

  // “捉”的判断包括以下几部分内容：
  switch (pcMoved - OPP_SIDE_TAG(this->sdPlayer)) {

  // 1. 走了马，判断是否捉车或捉有根的炮兵(卒)
  case KNIGHT_FROM:
  case KNIGHT_TO:
    // 逐一检测马踩的八个位置
    lpucsqDst = PreGen.ucsqKnightMoves[sqSrc];
    lpucsqPin = PreGen.ucsqKnightPins[sqSrc];
    sqDst = *lpucsqDst;
    while (sqDst != 0) {
      __ASSERT_SQUARE(sqDst);
      if (ucpcSquares[*lpucsqPin] == 0) {
        pcCaptured = this->ucpcSquares[sqDst];
        if ((pcCaptured & nSideTag) != 0) {
          pcCaptured -= nSideTag;
          __ASSERT_BOUND(0, pcCaptured, 15);
          // 技巧：优化兵种判断的分枝
          if (pcCaptured <= ROOK_TO) {
            // 马捉仕(士)、相(象)和马的情况不予考虑
            if (pcCaptured >= ROOK_FROM) {
              // 马捉到了车
              return pcCaptured;
            }
          } else {
            if (pcCaptured <= CANNON_TO) {
              // 马捉到了炮，要判断炮是否受保护
              if (!Protected(this->sdPlayer, sqDst)) {
                return pcCaptured;
              }
            } else {
              // 马捉到了兵(卒)，要判断兵(卒)是否过河并受保护
              if (AWAY_HALF(sqDst, sdPlayer) && !Protected(this->sdPlayer, sqDst)) {
                return pcCaptured;
              }
            }
          }
        }
      }
      lpucsqDst ++;
      sqDst = *lpucsqDst;
      lpucsqPin ++;
    }
    break;

  // 2. 走了车，判断是否捉有根的马炮兵(卒)
  case ROOK_FROM:
  case ROOK_TO:
    x = FILE_X(sqSrc);
    y = RANK_Y(sqSrc);
    if (((SRC(mv) ^ sqSrc) & 0xf) == 0) {
      // 如果车纵向移动了，则判断车横向吃到的子
      lpsmv = RankMovePtr(x, y);
      for (i = 0; i < 2; i ++) {
        sqDst = lpsmv->ucRookCap[i] + RANK_DISP(y);
        __ASSERT_SQUARE(sqDst);
        if (sqDst != sqSrc) {
          pcCaptured = this->ucpcSquares[sqDst];
          if ((pcCaptured & nSideTag) != 0) {
            pcCaptured -= nSideTag;
            __ASSERT_BOUND(0, pcCaptured, 15);
            // 技巧：优化兵种判断的分枝
            if (pcCaptured <= ROOK_TO) {
              // 车捉仕(士)、相(象)的情况不予考虑
              if (pcCaptured >= KNIGHT_FROM) {
                if (pcCaptured <= KNIGHT_TO) {
                  // 车捉到了马，要判断马是否受保护
                  if (!Protected(this->sdPlayer, sqDst)) {
                    return pcCaptured;
                  }
                }
                // 车捉车的情况不予考虑
              }
            } else {
              if (pcCaptured <= CANNON_TO) {
                // 车捉到了炮，要判断炮是否受保护
                if (!Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              } else {
                // 车捉到了兵(卒)，要判断兵(卒)是否过河并受保护
                if (AWAY_HALF(sqDst, sdPlayer) && !Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              }
            }
          }
        }
      }
    } else {
      // 如果车横向移动了，则判断车纵向吃到的子
      lpsmv = FileMovePtr(x, y);
      for (i = 0; i < 2; i ++) {
        sqDst = lpsmv->ucRookCap[i] + FILE_DISP(x);
        __ASSERT_SQUARE(sqDst);
        if (sqDst != sqSrc) {
          pcCaptured = this->ucpcSquares[sqDst];
          if ((pcCaptured & nSideTag) != 0) {
            pcCaptured -= nSideTag;
            __ASSERT_BOUND(0, pcCaptured, 15);
            // 技巧：优化兵种判断的分枝
            if (pcCaptured <= ROOK_TO) {
              // 车捉仕(士)、相(象)的情况不予考虑
              if (pcCaptured >= KNIGHT_FROM) {
                if (pcCaptured <= KNIGHT_TO) {
                  // 车捉到了马，要判断马是否受保护
                  if (!Protected(this->sdPlayer, sqDst)) {
                    return pcCaptured;
                  }
                }
                // 车捉车的情况不予考虑
              }
            } else {
              if (pcCaptured <= CANNON_TO) {
                // 车捉到了炮，要判断炮是否受保护
                if (!Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              } else {
                // 车捉到了兵(卒)，要判断兵(卒)是否过河并受保护
                if (AWAY_HALF(sqDst, sdPlayer) && !Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              }
            }
          }
        }
      }
    }
    break;

  // 3. 走了炮，判断是否捉车或捉有根的马兵(卒)
  case CANNON_FROM:
  case CANNON_TO:
    x = FILE_X(sqSrc);
    y = RANK_Y(sqSrc);
    if (((SRC(mv) ^ sqSrc) & 0xf) == 0) {
      // 如果炮纵向移动了，则判断炮横向吃到的子
      lpsmv = RankMovePtr(x, y);
      for (i = 0; i < 2; i ++) {
        sqDst = lpsmv->ucCannonCap[i] + RANK_DISP(y);
        __ASSERT_SQUARE(sqDst);
        if (sqDst != sqSrc) {
          pcCaptured = this->ucpcSquares[sqDst];
          if ((pcCaptured & nSideTag) != 0) {
            pcCaptured -= nSideTag;
            __ASSERT_BOUND(0, pcCaptured, 15);
            // 技巧：优化兵种判断的分枝
            if (pcCaptured <= ROOK_TO) {
              // 炮捉仕(士)、相(象)的情况不予考虑
              if (pcCaptured >= KNIGHT_FROM) {
                if (pcCaptured <= KNIGHT_TO) {
                  // 炮捉到了马，要判断马是否受保护
                  if (!Protected(this->sdPlayer, sqDst)) {
                    return pcCaptured;
                  }
                } else {
                  // 炮捉到了车
                  return pcCaptured;
                }
              }
            } else {
              // 炮捉炮的情况不予考虑
              if (pcCaptured >= PAWN_FROM) {
                // 炮捉到了兵(卒)，要判断兵(卒)是否过河并受保护
                if (AWAY_HALF(sqDst, sdPlayer) && !Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              }
            }
          }
        }
      }
    } else {
      // 如果炮横向移动了，则判断炮纵向吃到的子
      lpsmv = FileMovePtr(x, y);
      for (i = 0; i < 2; i ++) {
        sqDst = lpsmv->ucCannonCap[i] + FILE_DISP(x);
        __ASSERT_SQUARE(sqDst);
        if (sqDst != sqSrc) {
          pcCaptured = this->ucpcSquares[sqDst];
          if ((pcCaptured & nSideTag) != 0) {
            pcCaptured -= nSideTag;
            __ASSERT_BOUND(0, pcCaptured, 15);
            // 技巧：优化兵种判断的分枝
            if (pcCaptured <= ROOK_TO) {
              // 炮捉仕(士)、相(象)的情况不予考虑
              if (pcCaptured >= KNIGHT_FROM) {
                if (pcCaptured <= KNIGHT_TO) {
                  // 炮捉到了马，要判断马是否受保护
                  if (!Protected(this->sdPlayer, sqDst)) {
                    return pcCaptured;
                  }
                } else {
                  // 炮捉到了车
                  return pcCaptured;
                }
              }
            } else {
              // 炮捉炮的情况不予考虑
              if (pcCaptured >= PAWN_FROM) {
                // 炮捉到了兵(卒)，要判断兵(卒)是否过河并受保护
                if (AWAY_HALF(sqDst, sdPlayer) && !Protected(this->sdPlayer, sqDst)) {
                  return pcCaptured;
                }
              }
            }
          }
        }
      }
    }
    break;
  }

  return 0;
}
