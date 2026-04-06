# Alpha-Beta 剪枝算法

## 一、算法概述

**Alpha-Beta 剪枝（Alpha-Beta Pruning）** 是对极大极小（Minimax）算法的一种优化。它通过维护两个边界值 $\alpha$ 和 $\beta$，在搜索博弈树时跳过（剪掉）那些不可能影响最终决策的分支，从而大幅减少搜索的节点数，而不影响最终结果的正确性。

在最好的情况下，Alpha-Beta 剪枝可以将搜索节点数从 $O(b^d)$ 降低到 $O(b^{d/2})$，相当于在相同时间内把搜索深度加倍。

---

## 二、核心概念

### 1. $\alpha$ 值与 $\beta$ 值

| 符号 | 含义 | 维护者 |
|:---:|:---:|:---:|
| $\alpha$ | 当前搜索路径上，MAX 玩家已知的**最佳下界**（至少能获得的分值） | MAX 层更新 |
| $\beta$ | 当前搜索路径上，MIN 玩家已知的**最佳上界**（至多允许的分值） | MIN 层更新 |

初始时，$\alpha = -\infty$，$\beta = +\infty$。

### 2. 剪枝条件

- **Beta 剪枝（在 MAX 层）**：当某个 MAX 节点的值 $\geq \beta$ 时，MIN 父节点不会选择这个分支，剪掉后续兄弟节点。
- **Alpha 剪枝（在 MIN 层）**：当某个 MIN 节点的值 $\leq \alpha$ 时，MAX 父节点不会选择这个分支，剪掉后续兄弟节点。

简单来说：**如果发现当前分支的结果已经不可能比已知的最优选择更好，就不再继续搜索该分支。**

---

## 三、算法伪代码

```
function alphabeta(node, depth, α, β, maximizingPlayer):
    if depth == 0 or node is terminal:
        return evaluate(node)

    if maximizingPlayer:
        value = -∞
        for each child of node:
            value = max(value, alphabeta(child, depth - 1, α, β, FALSE))
            α = max(α, value)
            if α ≥ β:
                break    // β 剪枝
        return value

    else:  // minimizingPlayer
        value = +∞
        for each child of node:
            value = min(value, alphabeta(child, depth - 1, α, β, TRUE))
            β = min(β, value)
            if α ≥ β:
                break    // α 剪枝
        return value

// 初始调用
alphabeta(root, maxDepth, -∞, +∞, TRUE)
```

---

## 四、图解示例

考虑以下博弈树（MAX 先行）：

```
              MAX
           /       \
        MIN         MIN
       / | \        / \
      3  5  2      9   1
```

**搜索过程：**

1. 进入左边 MIN 节点：
   - 评估子节点 3 → MIN 当前值 = 3，$\beta = 3$
   - 评估子节点 5 → MIN 取 min(3,5) = 3
   - 评估子节点 2 → MIN 取 min(3,2) = 2
   - 左 MIN 返回 2

2. MAX 层：$\alpha = 2$

3. 进入右边 MIN 节点：
   - 评估子节点 9 → MIN 当前值 = 9，$\beta = 9$
   - 但 $\alpha = 2 < \beta = 9$，继续搜索
   - 评估子节点 1 → MIN 取 min(9,1) = 1
   - 右 MIN 返回 1

4. MAX 层：max(2, 1) = 2，最终选择左分支

**剪枝发生的例子：** 如果将右边 MIN 的第一个子节点换成 1，则 $\beta = 1 \leq \alpha = 2$，右 MIN 剩余子节点将被剪掉。

---

## 五、C++ 实现示例

```cpp
#include <algorithm>
#include <climits>

// 假设已有 GameState 类型
// bool isTerminal(GameState s);
// int evaluate(GameState s);
// vector<GameState> getChildren(GameState s);

int alphaBeta(GameState state, int depth, int alpha, int beta, bool isMax) {
    if (depth == 0 || isTerminal(state)) {
        return evaluate(state);
    }

    auto children = getChildren(state);

    if (isMax) {
        int value = INT_MIN;
        for (auto& child : children) {
            value = std::max(value, alphaBeta(child, depth - 1, alpha, beta, false));
            alpha = std::max(alpha, value);
            if (alpha >= beta)
                break;  // β 剪枝
        }
        return value;
    } else {
        int value = INT_MAX;
        for (auto& child : children) {
            value = std::min(value, alphaBeta(child, depth - 1, alpha, beta, true));
            beta = std::min(beta, value);
            if (alpha >= beta)
                break;  // α 剪枝
        }
        return value;
    }
}

// 调用入口
// int bestScore = alphaBeta(rootState, maxDepth, INT_MIN, INT_MAX, true);
```

---

## 六、性能分析

| 指标 | Minimax | Alpha-Beta（最坏） | Alpha-Beta（最优排序） |
|:---:|:---:|:---:|:---:|
| 搜索节点数 | $O(b^d)$ | $O(b^d)$ | $O(b^{d/2})$ |
| 等效深度 | $d$ | $d$ | $2d$ |

其中 $b$ 为分支因子（每个节点的子节点数），$d$ 为搜索深度。

### 影响剪枝效率的关键因素

1. **节点排序（Move Ordering）**：如果优先搜索最可能是最优解的分支，剪枝效果最佳。常用手段：
   - 使用浅层搜索结果排序（迭代加深）
   - 使用启发式评估函数预排序
   - 利用历史表/杀手启发（Killer Heuristic）

2. **搜索窗口**：初始窗口 $[\alpha, \beta]$ 越窄，剪枝越多。渴望搜索（Aspiration Window）利用此特点进一步优化。

---

## 七、算法变体

### 1. Negamax 框架下的 Alpha-Beta

利用 $\max(a, b) = -\min(-a, -b)$ 的性质，将 MAX 和 MIN 层统一处理：

```
function negamax(node, depth, α, β, color):
    if depth == 0 or node is terminal:
        return color * evaluate(node)

    for each child of node:
        value = -negamax(child, depth - 1, -β, -α, -color)
        α = max(α, value)
        if α ≥ β:
            break
    return α
```

### 2. PVS（Principal Variation Search）

也称 Negascout。假设第一个子节点是最优的，后续节点用零窗口 $[\alpha, \alpha+1]$ 快速验证：

- 如果验证成功（值 $\leq \alpha$），说明确实不比第一个好，剪掉
- 如果验证失败，再用完整窗口重新搜索

### 3. MTD(f)

Memory-enhanced Test Driver，利用零窗口搜索和置换表，反复逼近真实值，往往比标准 Alpha-Beta 更快。

---

## 八、实际应用

| 游戏 | 搜索深度 | 特殊技术 |
|:---:|:---:|:---:|
| 井字棋 | 完全搜索（9层） | 不需要剪枝即可穷举 |
| 国际象棋 | 10-20+ 层 | Alpha-Beta + 迭代加深 + 置换表 + 杀手启发 |
| 围棋 | 受限 | 传统 Alpha-Beta 难以应对（分支因子约 250），现多用 MCTS |
| 黑白棋/翻转棋 | 10-20 层 | Alpha-Beta + 精确的评估函数 |

---

## 九、总结

- Alpha-Beta 剪枝是博弈树搜索的**基础优化算法**，所有经典棋类 AI 都以此为核心
- 它**不改变 Minimax 的结果**，只是跳过不必要的计算
- 配合良好的**节点排序**，可以将搜索效率提升至理论最优
- 对于分支因子较大的游戏（如围棋），需要结合蒙特卡洛树搜索等方法

---

## 参考资料

- [Alpha-beta pruning - Wikipedia](https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning)
- [OI Wiki - Alpha-Beta 剪枝](https://oi-wiki.org/search/alpha-beta/)
- Russell, S. & Norvig, P. *Artificial Intelligence: A Modern Approach* (Chapter 5: Adversarial Search)
