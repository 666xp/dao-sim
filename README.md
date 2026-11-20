# dao-sim — 仿脑—星系脉冲动力系统

纯软件脉冲网络雏形：事件驱动导通层 + 道核半中心自激 + 六方位/沉淀核调制。  
**系统输出只有 0/1 比特流**，无固定文本、无语义映射、无「意识开关」。

本仓库不声称系统具有意识；只验证：无外部输入时，临界附近的高维动力系统能否自发冒出非写死的原始事件，且消融实验有因果响应。

## 诚实边界

- 不宣称意识、语义理解或「开关键」；UI 中文只是监测标签。
- 阶段 1 可用 `--fake` 测管道；阶段 2+ 用真动力学（不要 `--fake`）。
- 输出稀疏（ones≈0.027）时，汉明距离单独看没有意义，须与 1 的比例、多种子统计一并解读。

## 本机路径

```bat
git clone https://github.com/666xp/dao-sim.git D:\dao-sim
cd /d D:\dao-sim
```

## 架构与规模

- `sim/`：C++17 核（`sim_core` / `sim_validate` / `sim_engine` / 各 `*_bench`）
- `web/`：Vite + React 监测台 + `/lights` 红绿灯
- 引擎 `ws://127.0.0.1:3848` · 前端 `http://127.0.0.1:3847`
- 规模：沉淀核 2×256 + 道核 2×64 + 六方位 6×64 + 输出 12 ≈ **1036** 神经元

Catch2 / IXWebSocket 已 vendor 于 `sim/third_party/`（配置不访问 GitHub）。

```bat
rmdir /s /q sim\build
cmake -S sim -B sim\build -DDAO_SIM_BUILD_ENGINE=ON -DDAO_SIM_BUILD_DYNAMICS=ON
cmake --build sim\build --config Debug
```

## Bench 行格式（阶段 4 定型）

一行一项，可被 `scripts/collect_lights.py` 解析：

```text
PASS|FAIL: <name> <value> (threshold <t>)
```

元信息用 `NOTE: ...`，不进红绿灯。

## 阶段速览

| 阶段 | 内容 | 入口 |
|------|------|------|
| 0 | 导通层 | `scripts\verify_phase0.bat` / `sim_validate` |
| 1 | 假遥测管道 | `start.bat`（`--fake`）→ 浏览器曲线 |
| 2 | 道核隔离 → 场+Roche → σ | `sim_dao_bench` / `sim_fields_bench` / `sim_sigma_bench` |
| 3 | 转运比 / 12bit / 消融 A·B·C | `sim_transport_bench` / `sim_output_bench` / `sim_ablation_bench` |
| 4 | 审计 + 红绿灯 | `scripts\audit.bat` · `python scripts\collect_lights.py` · `http://127.0.0.1:3847/lights` |

### 阶段 0

```bat
scripts\verify_phase0.bat
```

`ctest` 2 Passed + `sim_validate` 6 行 PASS + exit 0。

### 阶段 1

```bat
start.bat
```

浏览器：D/σ 曲线动；Pause/Start/Step（先 Pause 再 Step）。

### 阶段 2 / 3 benches

```bat
sim\build\Debug\sim_dao_bench.exe 42 2000
sim\build\Debug\sim_fields_bench.exe 42 3000 500
sim\build\Debug\sim_sigma_bench.exe 42 5000 1000
sim\build\Debug\sim_transport_bench.exe 42 4000
sim\build\Debug\sim_output_bench.exe 4000 500
sim\build\Debug\sim_ablation_bench.exe 42 5000
```

### 阶段 4：审计 + 红绿灯

```bat
python scripts\audit.py
python scripts\collect_lights.py --out web\public\lights.json
python scripts\collect_lights.py --inject-fail --out web\public\lights.fail.json
npm --prefix web run dev
```

打开 `http://127.0.0.1:3847/lights`：应全绿；点「加载失败样例」应出现红项（证明面板在读数值，不是默认全绿）。

审计清单（`scripts/audit.py`）：

1. `sim/src/output.cpp` 排除 `#include`/注释后无字符串字面量作输出  
2. `sim/src` 无 `conscious` / `意识开关` / `semantic_map` / `neuron_to_word`  
3. `telemetry.h/.cpp` 无 `std::string` / 文本字段（遥测为二进制数值帧）

## 消融说明

- **A（仅 DaoYang/DaoYin 关适应）**：`u` 是负反馈；关掉后道核更兴奋、外投射增强，**全局放电率不降反升是预期**，不是 bug。须确认升幅有界——验收记录示例：`ablation_A_global_rate_ratio ≈ 2.2（threshold 0.45，有界）`。若翻很多倍，归因会被全局失控污染。  
- **B**：关噪声 → 多种子比特更趋同。  
- **C**：东群 50 脉冲 → 输出尾窗编辑距离上升。  
- 监测台可取消勾选「道核适应电流」/「内在噪声」，或点「扰动东群」；长跑用 Pause + 快进 N tick。

## 输出「非预设」怎么读

12 bit、`ones≈0.027` 时平均每帧约 0.32 个 1，随机期望汉明距离本身就在 ~0.05。  
因此红绿灯里 **三项一组**：`ones_density_band` + `ones_stats_similar` + `sequences_differ`，不要单看汉明。

## 拓扑硬约束

| 区域 | 规模 | 连接 |
|------|------|------|
| 左/右沉淀核 | 各 256 | 只与道核双向 |
| 道核阳/阴 | 各 64 | 互抑半中心 |
| 东/西/南/北/上/下 | 各 64 | 与道核双向 |
| 输出群 | 12 | 与道核双向；不进 σ 池 |

## 已知风险

1. 道核半中心起振不够 → 只调道核，别在全网盲调。  
2. ODE 只作包络。  
3. σ 须秒级（`tau_sec=5`），否则压道核。  
4. 消融 A 只关道核 `u`；全局放电上升须有界。  
5. 稀疏输出下汉明须与 ones 统计联读。

## GitHub

`https://github.com/666xp/dao-sim`

```bat
cd /d D:\dao-sim
git pull origin main
```
