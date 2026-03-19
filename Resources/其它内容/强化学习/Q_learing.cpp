// 引入输入输出流库，用于 cout/cin 打印与读取。
#include <iostream>
// 引入动态数组容器 vector。
#include <vector>
// 引入随机数相关工具（随机引擎、分布等）。
#include <random>
// 引入算法库，这里主要用到 max。
#include <algorithm>
// 引入时间库，用于 time(nullptr) 作为随机种子。
#include <ctime>

// 使用标准命名空间，避免频繁写 std::。
using namespace std;

// 游戏总石子上限，状态空间是 0~15。
static const int MAX_STONES = 15;
// 动作种类数量：只能取 1 或 2 颗。
static const int ACTIONS = 2;

// Q-learning 智能体：维护 Q 表并提供训练/决策能力。
struct QLearningAgent {
    // Q[stones][action]：在 stones 状态下采取 action（1或2）的价值估计。
    // 这里第二维多开一位（下标 0 不用），便于直接按动作 1/2 访问。
    vector<vector<double>> Q;
    // 学习率 alpha：每次更新向“新目标值”靠近的步长。
    double alpha = 0.1;
    // 折扣因子 gamma：衡量未来回报的重要性。
    double gamma = 0.95;
    // 探索率 epsilon：训练时按概率随机探索，而非总是贪心。
    double epsilon = 0.2;
    // Mersenne Twister 随机数引擎。
    mt19937 rng;

    // 构造函数：初始化 Q 表并设置随机种子。
    QLearningAgent() : Q(MAX_STONES + 1, vector<double>(ACTIONS + 1, 0.0)) {
        // 用当前时间初始化随机引擎，避免每次运行轨迹完全相同。
        rng.seed(static_cast<unsigned>(time(nullptr)));
    }

    // 计算某个石子数状态下的合法动作集合。
    vector<int> validActions(int stones) {
        // acts 用来存储当前状态可执行的动作。
        vector<int> acts;
        // 只要还有至少 1 颗，就能执行动作“取 1”。
        if (stones >= 1) acts.push_back(1);
        // 只有至少 2 颗时，动作“取 2”才合法。
        if (stones >= 2) acts.push_back(2);
        // 返回合法动作列表。
        return acts;
    }

    // 从合法动作中等概率随机选一个动作。
    int randomAction(const vector<int>& acts) {
        // 构造离散均匀分布，范围是动作下标 [0, acts.size()-1]。
        uniform_int_distribution<int> dist(0, static_cast<int>(acts.size()) - 1);
        // 用随机引擎采样下标，并返回对应动作值（1或2）。
        return acts[dist(rng)];
    }

    // 根据 epsilon-greedy 策略选择动作。
    // training=true 表示训练阶段（会探索），false 表示实战阶段（纯利用）。
    int chooseAction(int stones, bool training = true) {
        // 先拿到当前状态的合法动作。
        vector<int> acts = validActions(stones);
        // 理论上 stones>0 时不会空，这里做防御式返回。
        if (acts.empty()) return 1;

        // 训练阶段：按 epsilon 概率随机探索。
        if (training) {
            // 生成 [0,1] 上的随机实数。
            uniform_real_distribution<double> prob(0.0, 1.0);
            // 若命中探索概率，则随机选动作。
            if (prob(rng) < epsilon) {
                return randomAction(acts);
            }
        }

        // 利用阶段（或训练时未触发探索）：选择 Q 值最大的动作。
        int bestA = acts[0];
        // 用第一个合法动作初始化当前最优 Q。
        double bestQ = Q[stones][bestA];
        // 遍历合法动作，找到价值最大的动作。
        for (int a : acts) {
            // 若发现更大 Q 值则更新最优动作。
            if (Q[stones][a] > bestQ) {
                bestQ = Q[stones][a];
                bestA = a;
            }
        }
        // 返回贪心动作。
        return bestA;
    }

    // 求某状态下 max_a Q(s,a)，用于 Q-learning 的目标值。
    double maxQ(int stones) {
        // 获取该状态所有合法动作。
        vector<int> acts = validActions(stones);
        // 终止状态无动作，未来价值定义为 0。
        if (acts.empty()) return 0.0;
        // 用第一个动作的 Q 值初始化最大值。
        double m = Q[stones][acts[0]];
        // 在合法动作里取最大 Q。
        for (int a : acts) m = max(m, Q[stones][a]);
        // 返回最大动作价值。
        return m;
    }

    // 与随机对手进行自我训练。
    void train(int episodes = 50000) {
        // 每局初始石子数随机设为 [10, MAX_STONES]，增加训练多样性。
        uniform_int_distribution<int> startDist(10, MAX_STONES);

        // 外层循环：进行 episodes 局训练。
        for (int ep = 0; ep < episodes; ++ep) {
            // 本局起始状态。
            int stones = startDist(rng);

            // 一局内持续对弈，直到石子取完。
            while (stones > 0) {
                // 记录当前状态 s。
                int s = stones;
                // AI 在状态 s 选择动作 a（训练模式，允许探索）。
                int a = chooseAction(s, true);
                // AI 行动后的石子数。
                int afterAI = s - a;

                // 若 AI 取完最后一颗，则 AI 立即获胜。
                if (afterAI == 0) {
                    // 胜利即时奖励设为 +1。
                    double reward = 1.0;
                    // 终止状态更新：Q(s,a) <- Q + alpha * (r - Q)。
                    Q[s][a] += alpha * (reward - Q[s][a]);
                    // 本局结束。
                    break;
                }

                // 轮到对手：对手按随机策略行动。
                vector<int> oppActs = validActions(afterAI);
                // 随机抽取对手动作。
                int oppA = randomAction(oppActs);
                // 对手行动后的下一状态 s2（再次轮到 AI）。
                int s2 = afterAI - oppA;

                // 若对手取完最后一颗，则 AI 本回合结果为失败。
                if (s2 == 0) {
                    // 失败即时奖励设为 -1。
                    double reward = -1.0;
                    // 终止状态更新：Q(s,a) <- Q + alpha * (r - Q)。
                    Q[s][a] += alpha * (reward - Q[s][a]);
                    // 本局结束。
                    break;
                }

                // 非终止转移：即时奖励为 0，通过未来最优价值进行 bootstrap。
                double reward = 0.0;
                // Q-learning 核心更新：
                // Q(s,a) <- Q(s,a) + alpha * (r + gamma * maxQ(s2) - Q(s,a))。
                Q[s][a] += alpha * (reward + gamma * maxQ(s2) - Q[s][a]);
                // 推进到下一状态，继续本局。
                stones = s2;
            }

            // 每训练 5000 局，逐步降低探索率，后期更偏向利用。
            if ((ep + 1) % 5000 == 0 && epsilon > 0.02) {
                epsilon *= 0.9;
            }
        }
    }
};

// 程序入口。
int main() {
    // 创建一个 Q-learning 智能体实例。
    QLearningAgent ai;
    // 先离线训练，再与人对战。
    ai.train(60000);

    // 打印标题。
    cout << "=== 取石子游戏（Q-learning AI）===\n";
    // 打印规则说明。
    cout << "规则：每次取1或2颗，取到最后一颗者获胜。\n";

    // 实战初始石子数固定为 15。
    int stones = 15;
    // 读取用户是否先手。
    char first;
    cout << "你想先手吗？(y/n): ";
    cin >> first;
    // y/Y 表示人类先手，否则 AI 先手。
    bool humanTurn = (first == 'y' || first == 'Y');

    // 对战主循环：直到石子被取完。
    while (stones > 0) {
        // 显示当前局面。
        cout << "\n当前石子数: " << stones << "\n";

        // 人类回合。
        if (humanTurn) {
            // 记录人类本回合取子数。
            int take;
            cout << "你取几颗(1或2): ";
            cin >> take;

            // 输入非法（不是1/2，或超过剩余石子）时进行纠正。
            if ((take != 1 && take != 2) || take > stones) {
                cout << "非法操作，默认取1颗。\n";
                // 默认改为取 1。
                take = 1;
                // 若此时连 1 都超过剩余（理论上仅 stones=0 才会出现），再兜底修正。
                if (take > stones) take = stones;
            }

            // 扣除人类取走的石子。
            stones -= take;
            // 若石子归零，人类获胜并结束对局。
            if (stones == 0) {
                cout << "你赢了！\n";
                break;
            }
        } else {
            // AI 回合：实战时 training=false，采用纯贪心利用。
            int take = ai.chooseAction(stones, false);
            // 防御式判断，避免异常情况下取子越界。
            if (take > stones) take = 1;
            // 输出 AI 动作。
            cout << "AI 取了: " << take << " 颗\n";
            // 扣除 AI 取走的石子。
            stones -= take;
            // 若石子归零，AI 获胜并结束对局。
            if (stones == 0) {
                cout << "AI 赢了！\n";
                break;
            }
        }

        // 回合切换：人类与 AI 交替行动。
        humanTurn = !humanTurn;
    }

    // Windows 控制台暂停，便于查看结果。
    system("pause");
    // 正常结束程序。
    return 0;
}