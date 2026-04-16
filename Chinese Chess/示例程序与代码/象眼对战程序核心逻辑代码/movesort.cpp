/*
movesort.h/movesort.cpp - Source Code for ElephantEye, Part VII

ElephantEye - a Chinese Chess Program (UCCI Engine)
Designed by Morning Yellow, Version: 3.11, Last Modified: Dec. 2007
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
#include "position.h"
#include "movesort.h"

int nHistory[65536]; // 历史表，记录每种着法在过去搜索中导致截断的成功次数，数组大小对应所有的起点终点组合(256*256=65536)

// 根据历史表对着法列表赋值
void MoveSortStruct::SetHistory(void) {
  int i, j, vl, nShift, nNewShift;
  nShift = 0; // 初始位移量，用于按比例缩小历史得分防止溢出
  // 遍历当前尚未排序的着法（从 nMoveIndex 到 nMoveNum）
  for (i = nMoveIndex; i < nMoveNum; i ++) {
    // 获取该着法(wmv)在历史表中的得分，并进行当前要求的右移缩小
    vl = nHistory[mvs[i].wmv] >> nShift;
    // 如果着法的分值超过65536，就必需对所有着法的分值作缩减，使它们都不超过65536，保证可以存入 wvl 字段(16位)
    if (vl > 65535) {
      // 计算需要进一步右移的位数：最高有效位索引减去15，比如最高位在16位，就需要右移1位
      nNewShift = Bsr(vl) - 15;
      // 将前面已经赋过值的着法得分也同样进行右移缩减，保持相对大小
      for (j = nMoveIndex; j < i; j ++) {
        mvs[j].wvl >>= nNewShift;
      }
      // 当前着法的值也进行右移
      vl >>= nNewShift;
      // 断言确保缩减后的值在 [32768, 65535] 之间，防止过度缩减
      __ASSERT_BOUND(32768, vl, 65535);
      // 累加总的位移量，使得以后的着法提取时直接用 nShift 就能对齐
      nShift += nNewShift;
    }
    // 把计算好并缩减到安全范围的历史得分赋给着法的排序权重参数 wvl
    mvs[i].wvl = vl;
  }
}

// Shell排序法，这里用"1, 4, 13, 40 ..."的序列，这样要比"1, 2, 4, 8, ..."好
// 希尔排序的步长表(Knuth等推荐的序列，比常规折半步长更高效)
static const int cnShellStep[8] = {0, 1, 4, 13, 40, 121, 364, 1093};

void MoveSortStruct::ShellSort(void) {
  int i, j, nStep, nStepLevel;
  MoveStruct mvsBest;
  nStepLevel = 1; // 从查表序列的索引1开始寻找合适的最大步长
  // 寻找小于待排序元素数量的最大差距步长级别
  while (cnShellStep[nStepLevel] < nMoveNum - nMoveIndex) {
    nStepLevel ++;
  }
  nStepLevel --; // 退回一个级别，作为希尔排序第一趟的步长索引
  // 按步长递减进行多趟插入排序，直到步长级别为0
  while (nStepLevel > 0) {
    nStep = cnShellStep[nStepLevel]; // 获取当前趟的步长
    // 以下是对跨度为 nStep 的各个子序列进行直接插入排序
    for (i = nMoveIndex + nStep; i < nMoveNum; i ++) {
      mvsBest = mvs[i]; // 保存当前要插入的着法元素
      j = i - nStep;
      // 向前比较，如果前面的元素权值较小(说明当前元素分高，需要排在前面)，则将前面的元素后移
      while (j >= nMoveIndex && mvsBest.wvl > mvs[j].wvl) {
        mvs[j + nStep] = mvs[j];
        j -= nStep; // 继续向前比对
      }
      // 将元素插入到正确的位置
      mvs[j + nStep] = mvsBest;
    }
    nStepLevel --; // 步长级别减1，进入下一趟更精细的排序(最终步长是1)
  }
}

/* 生成解将着法，返回唯一应将着法(有多个应将着法则返回零)
 * 
 * 解将着法的顺序如下：
 * 1. 置换表着法(SORT_VALUE_MAX)；
 * 2. 两个杀手着法(SORT_VALUE_MAX - 1或2)；
 * 3. 其他着法，按历史表排序(从1到SORT_VALUE_MAX - 3)；
 * 4. 不能解将的着法(0)，这些着法会过滤掉。
 */
int MoveSortStruct::InitEvade(PositionStruct &pos, int mv, const uint16_t *lpwmvKiller) {
  int i, nLegal;
  nPhase = PHASE_REST; // 阶段设置为剩余阶段，因为解将时是一次性生成并排序好
  nMoveIndex = 0; // 重置着法列表的读取索引
  nMoveNum = pos.GenAllMoves(mvs); // 生成所有的着法(不论是否能解将)，并存入 mvs
  SetHistory(); // 给所有初生成的着法赋上基于历史表的排序权重 wvl
  nLegal = 0; // 记录合法的解将着法数量
  // 遍历生成的所有着法，筛选合法解将着法并赋上启发式的额外权重
  for (i = nMoveIndex; i < nMoveNum; i ++) {
    // 若当前着法为最佳记录(置换表中得到的提示着法)
    if (mvs[i].wmv == mv) {
      nLegal ++;
      mvs[i].wvl = SORT_VALUE_MAX; // 置换表着法权重最高，排在第1
    } else if (pos.MakeMove(mvs[i].wmv)) { 
      // 尝试走这个着法，如果在这个着法走完后本方不再是被将军状态，则这是个合法的解将着法
      pos.UndoMakeMove(); // 试完以后立马撤销，恢复局面
      nLegal ++;
      // 检查该合法着法是否是第一杀手着法
      if (mvs[i].wmv == lpwmvKiller[0]) {
        mvs[i].wvl = SORT_VALUE_MAX - 1; // 给第一杀手着法赋值，排在第2
      // 检查是否是第二杀手着法
      } else if (mvs[i].wmv == lpwmvKiller[1]) {
        mvs[i].wvl = SORT_VALUE_MAX - 2; // 给第二杀手着法赋值，排在第3
      } else {
        // 其余的合法应将着法，使用历史启发分数加1，封顶为最大值减3，排在后面
        mvs[i].wvl = MIN(mvs[i].wvl + 1, SORT_VALUE_MAX - 3);
      }
    } else {
      // 走这步后依然处于被将军状态，说明无法解将，是不合法着法，分数清零
      mvs[i].wvl = 0;
    }
  }
  ShellSort(); // 根据赋予的权重信息，对生成的全量着法排序
  // 最终的有效着法数量为基础索引+合法数量(那些得分为0的会被排序法踢到数组尾部)
  nMoveNum = nMoveIndex + nLegal;
  // 如果只有一个合法的解将着法，直接返回这个着法(用于直接沿用或者判定强制局面)，否则返回0
  return (nLegal == 1 ? mvs[0].wmv : 0);
}

// 给出下一个即将搜索的着法 (基于分阶段生成技术，避免一次性生成和排序所有着法，节约时间)
int MoveSortStruct::NextFull(const PositionStruct &pos) {
  switch (nPhase) {
  // "nPhase"表示着法启发的若干阶段，依次为：

  // 0. 置换表着法启发，完成后立即进入下一阶段；
  case PHASE_HASH:
    nPhase = PHASE_GEN_CAP; // 将状态更新为下一阶段 (生成吃子)
    if (mvHash != 0) { // 如果存在置换表提供的建议着法
      __ASSERT(pos.LegalMove(mvHash)); // 断言确保它是一个局面上合法的着法
      return mvHash; // 第一步直接返回这个很可能引起截断的着法
    }
    // 技巧：这里没有"break"，表示"switch"的上一个"case"执行完后紧接着做下一个"case"，下同
    // 这样当 mvHash 为 0 (没有置换表着法)时，它会自动进入下一阶段生成着法，无需再次调用

  // 1. 生成所有吃子着法，完成后立即进入下一阶段；
  case PHASE_GEN_CAP:
    nPhase = PHASE_GOODCAP; // 状态更新
    nMoveIndex = 0;
    nMoveNum = pos.GenCapMoves(mvs); // 生成所有的吃子着法(具有进攻性)，存入mvs，此时着法的wvl是通过MVV/LVA自动赋的值
    ShellSort(); // 对这些吃子着法进行排序(有很大几率吃掉更高价值子力的排在前面)

  // 2. MVV(LVA)启发，可能要循环若干次；  (返回良好吃子信息的阶段)
  case PHASE_GOODCAP:
    // 如果列表还没读完，而且该着法的权值大于1
    if (nMoveIndex < nMoveNum && mvs[nMoveIndex].wvl > 1) {
      // 注意：MVV(LVA)值不超过1，则说明吃子不是直接能获得优势的，这些着法将留在以后搜索
      nMoveIndex ++; // 指针下移
      // 检查当前抓出的属于合格的吃子着法(目标格子上面确实有对方的子)
      __ASSERT_PIECE(pos.ucpcSquares[DST(mvs[nMoveIndex - 1].wmv)]);
      return mvs[nMoveIndex - 1].wmv; // 返回良好的吃子着法给外面继续搜索
    }
    // 当良好的吃子着法都被取光了之后，顺延进入非吃子着法的启发阶段（同样无break）

  // 3. 杀手着法启发(第一个杀手着法)，完成后立即进入下一阶段；
  case PHASE_KILLER1:
    nPhase = PHASE_KILLER2; // 更新状态
    // 如果存在一号杀手着法，且被判断在当前盘面上依旧合法
    if (mvKiller1 != 0 && pos.LegalMove(mvKiller1)) {
      // 注意：杀手着法必须检验着法合理性（因为它是从同层级其他分支拿来的经验，在这可能不合法），下同
      return mvKiller1; // 返回1号杀手着法
    }

  // 4. 杀手着法启发(第二个杀手着法)，完成后立即进入下一阶段；
  case PHASE_KILLER2:
    nPhase = PHASE_GEN_NONCAP; // 状态更新
    if (mvKiller2 != 0 && pos.LegalMove(mvKiller2)) { // 检验二号杀手着法的存在与合法性
      return mvKiller2;
    }

  // 5. 生成所有不吃子着法，完成后立即进入下一阶段；
  case PHASE_GEN_NONCAP:
    nPhase = PHASE_REST; // 更新状态到最后的部分
    // 把当前盘面里不是吃子的所有平凡着法追加在刚生成的列表尾部，并累加到 nMoveNum 数量上
    nMoveNum += pos.GenNonCapMoves(mvs + nMoveNum);
    SetHistory(); // 刚刚添加了不吃子着法，需利用历史得分表给它们赋上启发权值
    ShellSort(); // 根据刚刚赋予的历史得分，把这一批和剩余不好的吃子都排一下序

  // 6. 对剩余着法做历史表启发(包括返回解将着法)；
  case PHASE_REST:
    // 将剩下的所有着法一个接一个地返回出去被搜索
    if (nMoveIndex < nMoveNum) {
      nMoveIndex ++;
      return mvs[nMoveIndex - 1].wmv; 
    }

  // 7. 没有着法了，返回零。
  default:
    return 0; // 若以上阶段执行完毕也没找到合法着法，即陷入绝杀或者无子可动的状态
  }
}

// 生成根结点的着法 (在根节点，往往需要排除掉某些特定的重复或禁走的步子)
void MoveSortStruct::InitRoot(const PositionStruct &pos, int nBanMoves, const uint16_t *lpwmvBanList) {
  int i, j, nBanned;
  nMoveIndex = 0;
  nMoveNum = pos.GenAllMoves(mvs); // 一次性产生根节点下所有的备选着法
  nBanned = 0; // 记录被禁止掉的着法数量
  for (i = 0; i < nMoveNum; i ++) {
    mvs[i].wvl = 1; // 默认所有节点分数是1（此时可能还不用深入基于历史和启发排序，之后可能另作调整）
    for (j = 0; j < nBanMoves; j ++) { // 检查它是否在我们要求禁走的列表里面
      if (mvs[i].wmv == lpwmvBanList[j]) {
        mvs[i].wvl = 0; // 属于禁走着法，分数设为0并踢出
        nBanned ++;
        break;
      }
    }  
  }
  ShellSort(); // 按前面的权值（1或0）粗略排一下，将禁着踢到数组最后
  nMoveNum -= nBanned; // 把所有分数为0的抛弃掉，缩短实际可走的着法总数
}

// 更新根结点的着法排序列表 (在迭代加深搜索中，用上一层的最好结果给着法进行排序以加速)
void MoveSortStruct::UpdateRoot(int mv) {
  int i;
  for (i = 0; i < nMoveNum; i ++) {
    // 找到在这次短时迭代中得到的最佳着法 mv
    if (mvs[i].wmv == mv) {
      mvs[i].wvl = SORT_VALUE_MAX; // 把它提权为最大值，放在最前
    } else if (mvs[i].wvl > 0) {
      mvs[i].wvl --; // 其他剩余着法的评分下降，实现一种类似年龄老化的策略
    }
  }
}
