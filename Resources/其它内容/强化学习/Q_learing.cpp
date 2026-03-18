#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <ctime>

using namespace std;

static const int MAX_STONES = 15;   // 最大石子数
static const int ACTIONS = 2;       // 动作: 取1或取2

struct QLearningAgent {
    vector<vector<double>> Q; // Q[stones][action], action用1或2
    double alpha = 0.1;       // 学习率
    double gamma = 0.95;      // 折扣因子*
    double epsilon = 0.2;     // 探索率（训练时）
    mt19937 rng;

    QLearningAgent() : Q(MAX_STONES + 1, vector<double>(ACTIONS + 1, 0.0)) {
        rng.seed(static_cast<unsigned>(time(nullptr)));
    }

    vector<int> validActions(int stones) {
        vector<int> acts;
        if (stones >= 1) acts.push_back(1);
        if (stones >= 2) acts.push_back(2);
        return acts;
    }

    int randomAction(const vector<int>& acts) {
        uniform_int_distribution<int> dist(0, static_cast<int>(acts.size()) - 1);
        return acts[dist(rng)];
    }

    int chooseAction(int stones, bool training = true) {
        vector<int> acts = validActions(stones);
        if (acts.empty()) return 1;

        if (training) {
            uniform_real_distribution<double> prob(0.0, 1.0);
            if (prob(rng) < epsilon) {
                return randomAction(acts); // 探索
            }
        }

        // 利用：选Q值最大动作
        int bestA = acts[0];
        double bestQ = Q[stones][bestA];
        for (int a : acts) {
            if (Q[stones][a] > bestQ) {
                bestQ = Q[stones][a];
                bestA = a;
            }
        }
        return bestA;
    }

    double maxQ(int stones) {
        vector<int> acts = validActions(stones);
        if (acts.empty()) return 0.0;
        double m = Q[stones][acts[0]];
        for (int a : acts) m = max(m, Q[stones][a]);
        return m;
    }

    // 与随机对手训练
    void train(int episodes = 50000) {
        uniform_int_distribution<int> startDist(10, MAX_STONES);

        for (int ep = 0; ep < episodes; ++ep) {
            int stones = startDist(rng);

            while (stones > 0) {
                int s = stones;
                int a = chooseAction(s, true); // AI动作
                int afterAI = s - a;

                // AI直接赢
                if (afterAI == 0) {
                    double reward = 1.0;
                    Q[s][a] += alpha * (reward - Q[s][a]);
                    break;
                }

                // 对手随机动作
                vector<int> oppActs = validActions(afterAI);
                int oppA = randomAction(oppActs);
                int s2 = afterAI - oppA;

                // 对手赢，AI输
                if (s2 == 0) {
                    double reward = -1.0;
                    Q[s][a] += alpha * (reward - Q[s][a]);
                    break;
                }

                // 中间状态，继续学习未来价值
                double reward = 0.0;
                Q[s][a] += alpha * (reward + gamma * maxQ(s2) - Q[s][a]);
                stones = s2;
            }

            // 训练后期逐渐降低探索率
            if ((ep + 1) % 5000 == 0 && epsilon > 0.02) {
                epsilon *= 0.9;
            }
        }
    }
};

int main() {
    QLearningAgent ai;
    ai.train(60000);

    cout << "=== 取石子游戏（Q-learning AI）===\n";
    cout << "规则：每次取1或2颗，取到最后一颗者获胜。\n";

    int stones = 15;
    char first;
    cout << "你想先手吗？(y/n): ";
    cin >> first;
    bool humanTurn = (first == 'y' || first == 'Y');

    while (stones > 0) {
        cout << "\n当前石子数: " << stones << "\n";

        if (humanTurn) {
            int take;
            cout << "你取几颗(1或2): ";
            cin >> take;

            if ((take != 1 && take != 2) || take > stones) {
                cout << "非法操作，默认取1颗。\n";
                take = 1;
                if (take > stones) take = stones;
            }

            stones -= take;
            if (stones == 0) {
                cout << "你赢了！\n";
                break;
            }
        } else {
            int take = ai.chooseAction(stones, false); // 实战纯利用
            if (take > stones) take = 1;
            cout << "AI 取了: " << take << " 颗\n";
            stones -= take;
            if (stones == 0) {
                cout << "AI 赢了！\n";
                break;
            }
        }

        humanTurn = !humanTurn;
    }

    system("pause");
    return 0;
}