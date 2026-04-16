// Eleeye核心代码

//一. genmoves.cpp

1.得到目标棋子(被吃子)是否被保护

2.计算MVV(LVA)

3.吃子着法生成器，按照MVV(LVA)设定分值
//

int PositionStruct::MvvLva(int sqDst,int pcCaptured,int nLva)const {
	int nMvv,nLvaAdjust;

	//判断目标格子上
	nMvv = SIMPLE_VALUE(pcCaptured);

	nLvaAdjust = (Protected(OPP_SIDE(sdPlayer),sqDst) ? nLva : 0);

	//被吃的棋子价值 >= 攻击方的调整花费，则至少不亏

	if(nMvv >= nLvaAdjust) {
		return nMvv - nLvaAdjust + 1;
	} else {

		//即使吃子表面上亏损，但用低价值的棋子吃高价值的棋子，大赚
		//这个位置在本方半场，以逸待劳，相当于后手先攻，小赚
		return (nMww >= 3 || HOME_HALF(sqDst,sdPlayer)) ? 1 : 0;
	}

}


int PositionStruct::GenCapMoves(MoveStrcut *lpmvs)const {
	int i,sqSrc,sqDst,pcCaptured;
	int x,y,nSideTag,nOppSideTag;
	bool bCanPromote;
	SlideMoveStruct *lpsmv;
	uint8_t *lpucsqDst,*lpucsqPin;
	MoveStruct *lpmvsCurr;


	//环境初始化——生成器在极短的时间内，可能被调用上百万次，需要把需要反复用到的一些基础变量，提前提取并缓存
	lpmvsCurr = lpmvs;
	nSideTag = SIDE_TAG(sdPlayer);
	nOppSideTag = OPP_SIDE_TAG(sdPlayer);
	bCanPromote = PreEval.bPromotion && CanPromote();

	//下面生成各种棋子的着法
	//生成帅/将的着法
	sqSrc = ucsqPieces[nSideTag + KING_FROM];
	if (sqSrc != 0) {
		__ASSERT_SQUARE(sqSrc);
		lpucsqDst = PreGen.ucsqKingMoves[sqSrc];
		sqDst = *lpucsqDst;
		while (sqDst != 0) {
			__ASSERT_SQUARE(sqDst);
			// 找到一个着法后，首先判断吃到的棋子是否是对方棋子，技巧是利用"nOppSideTag"的标志(16和32颠倒)，
			// 如果是对方棋子，则保存MVV(LVA)值，即如果被吃子无保护，则只记MVV，否则记MVV-LVA(如果MVV>LVA的话)。
			pcCaptured = ucpcSquares[sqDst];
			if ((pcCaptured & nOppSideTag) != 0) {
				__ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
				lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
				lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 5); // 帅(将)的价值是5
				lpmvsCurr ++;
			}
			lpucsqDst ++;
			sqDst = *lpucsqDst;
		}
	}

	for (i = ADVISOR_FROM; i <= ADVISOR_TO; i ++) {// 仕(士)每方各有两枚，利用循环分别处理这两个仕。
		sqSrc = ucsqPieces[nSideTag + i];// 根据己方阵营掩码加上棋子偏移量，从棋子数组(ucsqPieces)中查出该仕所在的格子编号，作为起点。

		if (sqSrc != 0) {// 坐标为0通常代表该棋子已经被吃掉（不在棋盘上）。不为0则说明棋子存活，开始计算其走法。
			__ASSERT_SQUARE(sqSrc);// 调试用的断言，确保被取出的起点格子坐标未越界。

			lpucsqDst = PreGen.ucsqAdvisorMoves[sqSrc];
			// 核心优化：直接查预先生成好的“仕步法表”。拿到指向“当前位置仕所有可能走到的目标格”的数组指针。

			sqDst = *lpucsqDst;
			// 取出数组中的第一个目标格子编号。

			while (sqDst != 0) {
				// 目标格数组以0作为结尾标志。只要读出的格子不为0，就循环检测。

				__ASSERT_SQUARE(sqDst);
				// 断言目标格子合法。

				pcCaptured = ucpcSquares[sqDst];
				// 查棋盘一维数组，看看这个目标格子当前被哪枚棋子占据。

				if ((pcCaptured & nOppSideTag) != 0) {
					// 利用位运算，判断目标格子上的棋子是不是敌方棋子。如果是，说明这是一个合法的“吃子”动作。

					__ASSERT(LegalMove(MOVE(sqSrc, sqDst)));
					// 断言这个移动动作符合规则。

					lpmvsCurr->wmv = MOVE(sqSrc, sqDst);
					// 把起点和终点合成一个16位的着法数值，存入当前的候选着法列表(lpmvsCurr)中。

					lpmvsCurr->wvl = MvvLva(sqDst, pcCaptured, 1); // 仕(士)的价值是1
					// 给这步吃子动作打分(算MVV/LVA值)，其中参数 `1` 代表发起攻击的仕(士)自身价值为 1，将打分存入列表。

					lpmvsCurr ++;
					// 着法列表记录满一条后，指针向后移动一位，准备记录下一招。
				}

				lpucsqDst ++;
				// 从预存的目标格子数组中看下一个格子。

				sqDst = *lpucsqDst;
				// 把下一个格子的编号赋给 sqDst，继续下一轮 while 循环。
			}

			if (bCanPromote && CAN_PROMOTE(sqSrc)) {
				// 这是引擎为了兼容特定棋牌变种规则（例如兵卒升变、国际象棋变招）留下的扩展判断。
				// 如果棋局引擎全局开启了升变，且当前格子能触发升变。

				lpmvsCurr->wmv = MOVE(sqSrc, sqSrc);
				// 记录一个特殊的着法（起点到起点，原地不动）来向引擎表示在当前格子执行升变操作。

				lpmvsCurr->wvl = 0;
				// 升变着法当前分数评估记为0。

				lpmvsCurr ++;
				// 着法指针后移。
			}
		}
	}

}































