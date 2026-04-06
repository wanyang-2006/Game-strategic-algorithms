# Nim 博弈与 Sprague-Grundy 定理

## 一、Nim 博弈

### 1. 规则

**Nim 博弈** 是最经典的组合博弈问题之一：

- 有若干堆石子，每堆有一定数量
- 两个玩家轮流操作，每次从**某一堆**中取走**至少一个**石子
- 取走最后一颗石子的玩家**获胜**（Normal Play Convention）

### 2. Nim 和（Nim-Sum）

定义所有堆的石子数的**异或和（XOR）** 为 **Nim 和**：

$$\text{Nim-Sum} = a_1 \oplus a_2 \oplus a_3 \oplus \cdots \oplus a_n$$

其中 $a_i$ 为第 $i$ 堆的石子数，$\oplus$ 为按位异或运算。

### 3. 必胜与必败判定

**Bouton 定理（1902）：**

> 当且仅当 Nim 和为 0 时，当前局面为**必败态（P-position）**；否则为**必胜态（N-position）**。

- **Nim-Sum = 0**：无论当前玩家怎么操作，对手总能将 Nim-Sum 再次归零 → **后手必胜**
- **Nim-Sum ≠ 0**：当前玩家总能找到一种操作使 Nim-Sum 变为 0 → **先手必胜**

### 4. 示例

三堆石子：$\{3, 4, 5\}$

$$3 \oplus 4 \oplus 5 = 011 \oplus 100 \oplus 101 = 010 = 2 \neq 0$$

Nim 和不为 0，所以**先手必胜**。

先手可以从第三堆取 2 个（5→3），使得局面变为 $\{3, 4, 3\}$：

$$3 \oplus 4 \oplus 3 = 011 \oplus 100 \oplus 011 = 100 \oplus 011 = \cdots$$

实际上更简单：从第二堆取 2 个（4→2），使得局面变为 $\{3, 2, 5\}$：

看哪种操作使异或和为 0 即可。$3 \oplus 4 \oplus 5 = 2$，需要改变某堆使异或和为 0。对于第三堆：$3 \oplus 4 = 7$，所以第三堆应变为 7，但 7 > 5 不合法。对于第二堆：$3 \oplus 5 = 6$，第二堆应变为 6，但 6 > 4 不合法。对于第一堆：$4 \oplus 5 = 1$，第一堆应变为 1，即从第一堆取 2 个（3→1），局面变为 $\{1, 4, 5\}$：

$$1 \oplus 4 \oplus 5 = 001 \oplus 100 \oplus 101 = 000 = 0 \checkmark$$

---

## 二、Nim 博弈的变体

### 1. Misère Nim（反 Nim）

规则变化：取走最后一颗石子的玩家**输**。

**结论：**
- 如果所有堆的石子数都 $\leq 1$：堆数为奇数时后手胜，偶数时先手胜
- 否则：与正常 Nim 判定一致（Nim-Sum ≠ 0 时先手胜，但最终策略需要调整）

### 2. 阶梯 Nim（Staircase Nim）

有一排编号为 $0, 1, 2, \ldots, n$ 的位置，每个位置有若干石子。每次操作可以从位置 $i$（$i > 0$）取任意数量的石子移到位置 $i-1$。

**结论：** 只对**奇数位置**上的石子数求 Nim-Sum。

### 3. 取石子博弈（Bash Game）

一堆 $n$ 个石子，每次最多取 $m$ 个（至少取 1 个）。

**结论：** 当 $n \mod (m+1) = 0$ 时后手胜，否则先手胜。

### 4. 威佐夫博弈（Wythoff's Game）

两堆石子，每次可以从一堆取任意个，或从两堆取相同个数。

**结论：** 必败态为 $(\lfloor k \cdot \varphi \rfloor, \lfloor k \cdot \varphi^2 \rfloor)$，其中 $\varphi = \frac{1+\sqrt{5}}{2}$（黄金比例），$k = 0, 1, 2, \ldots$

前几个必败态：$(0,0), (1,2), (3,5), (4,7), (6,10), \ldots$

---

## 三、Sprague-Grundy 定理

### 1. 有向图博弈（ICG）

**公平组合游戏（Impartial Combinatorial Game, ICG）** 的特点：
- 两个玩家交替行动
- 双方的合法动作完全相同（不区分"我的棋子"和"你的棋子"）
- 无法行动的玩家判负
- 完全信息、无随机因素、有限步数内必然结束

这类游戏可以用一个**有向无环图（DAG）** 来建模：
- 节点 = 游戏状态
- 有向边 = 合法操作
- 没有出边的节点 = 终止状态（无法行动 → 判负）

### 2. SG 函数的定义

对于有向图中的每个节点 $v$，定义其 **Grundy 值（SG 值）**：

$$SG(v) = \text{mex}\{SG(u) \mid u \text{ 是 } v \text{ 的后继节点}\}$$

其中 **mex（minimal excludant）** 是指不在集合中的最小非负整数：

$$\text{mex}\{0, 1, 3\} = 2, \quad \text{mex}\{1, 2\} = 0, \quad \text{mex}\{\} = 0$$

### 3. SG 值的含义

- $SG(v) = 0$：节点 $v$ 是**必败态（P-position）**
- $SG(v) \neq 0$：节点 $v$ 是**必胜态（N-position）**

### 4. Sprague-Grundy 定理（核心）

> 如果一个游戏由多个**独立的子游戏**组成，玩家每次在一个子游戏中行动一步，那么整个游戏的 SG 值等于各子游戏 SG 值的异或和：
>
> $$SG(G_1 + G_2 + \cdots + G_n) = SG(G_1) \oplus SG(G_2) \oplus \cdots \oplus SG(G_n)$$

**这就是为什么 Nim 博弈可以用异或来判定的根本原因！** 每一堆石子可以看作一个独立的子游戏，一堆有 $k$ 个石子的 SG 值恰好是 $k$。

---

## 四、SG 值计算示例

### 示例：取石子游戏（只能取 1 或 3 个）

一堆 $n$ 个石子，每次只能取 1 个或 3 个。

| $n$ | 后继状态 | 后继 SG 值 | $SG(n) = \text{mex}\{\cdots\}$ |
|:---:|:---:|:---:|:---:|
| 0 | 无 | $\{\}$ | $\text{mex}\{\} = 0$ |
| 1 | $\{0\}$ | $\{0\}$ | $\text{mex}\{0\} = 1$ |
| 2 | $\{1\}$ | $\{1\}$ | $\text{mex}\{1\} = 0$ |
| 3 | $\{2, 0\}$ | $\{0, 0\}$ | $\text{mex}\{0\} = 1$ |
| 4 | $\{3, 1\}$ | $\{1, 1\}$ | $\text{mex}\{1\} = 0$ |
| 5 | $\{4, 2\}$ | $\{0, 0\}$ | $\text{mex}\{0\} = 1$ |

规律：$SG(n) = n \mod 2$（偶数必败，奇数必胜）

---

## 五、C++ 实现

### SG 值的递推计算

```cpp
#include <vector>
#include <set>

// 计算 mex
int mex(const std::set<int>& s) {
    int val = 0;
    while (s.count(val)) val++;
    return val;
}

// 取石子游戏：每次可以取 moves 集合中的数量
// 计算 0 到 maxN 的所有 SG 值
std::vector<int> computeSG(int maxN, const std::vector<int>& moves) {
    std::vector<int> sg(maxN + 1, 0);
    for (int i = 1; i <= maxN; i++) {
        std::set<int> reachable;
        for (int m : moves) {
            if (i >= m) {
                reachable.insert(sg[i - m]);
            }
        }
        sg[i] = mex(reachable);
    }
    return sg;
}

// 多堆博弈判定
bool isFirstPlayerWin(const std::vector<int>& piles,
                      const std::vector<int>& sg) {
    int xorSum = 0;
    for (int pile : piles) {
        xorSum ^= sg[pile];
    }
    return xorSum != 0;
}
```

### 标准 Nim 博弈判定

```cpp
#include <vector>

bool nimFirstPlayerWin(const std::vector<int>& piles) {
    int nimSum = 0;
    for (int p : piles) {
        nimSum ^= p;
    }
    return nimSum != 0;
}
```

---

## 六、经典问题与解题模板

### 问题类型一：标准 Nim

直接异或所有堆的石子数。

### 问题类型二：限制取法的 Nim

先计算单堆的 SG 值（找规律或递推），再异或。

### 问题类型三：图上的博弈

建模为有向无环图，计算每个节点的 SG 值。

### 问题类型四：组合博弈

多个独立子游戏，分别算 SG 值再异或。

### 解题通用步骤

1. **识别子游戏**：将博弈分解为若干独立的部分
2. **计算 SG 值**：对每个子游戏，用递推/打表求 SG 值
3. **找规律**：观察 SG 值序列是否存在周期性
4. **合并判定**：异或所有子游戏的 SG 值，非零则先手胜

---

## 七、总结

| 概念 | 核心内容 |
|:---:|:---:|
| Nim 博弈 | 异或和为 0 ↔ 后手必胜 |
| SG 函数 | $SG(v) = \text{mex}\{SG(u)\}$，$SG = 0$ ↔ 必败 |
| SG 定理 | 组合游戏的 SG 值 = 子游戏 SG 值的异或 |
| 核心技巧 | 递推打表 → 观察周期 → 异或合并 |

Sprague-Grundy 定理是组合博弈论的基石，几乎所有公平组合游戏都可以通过它来分析。掌握 SG 定理，等于掌握了解决此类博弈问题的万能钥匙。

---

## 参考资料

- Sprague, R. (1935). *Über mathematische Kampfspiele*
- Grundy, P.M. (1939). *Mathematics and Games*
- [OI Wiki - 公平组合游戏](https://oi-wiki.org/math/game-theory/impartial-game/)
- [OI Wiki - Sprague-Grundy 定理](https://oi-wiki.org/math/game-theory/sprague-grundy/)
- Ferguson, T.S. *Game Theory* (UCLA lecture notes)
