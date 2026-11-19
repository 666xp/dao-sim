# dao-sim — 仿脑—星系脉冲动力系统

纯软件脉冲网络雏形：事件驱动导通层 + 道核半中心自激 + 六方位/沉淀核调制。  
**系统输出只有 0/1 比特流**，无固定文本、无语义映射、无「意识开关」。

本仓库不声称系统具有意识；只验证：无外部输入时，临界附近的高维动力系统能否自发冒出非写死的原始事件，且消融实验有因果响应。

## 本机路径

推荐 Windows 工作目录：

```bat
git clone https://github.com/666xp/dao-sim.git D:\dao-sim
cd /d D:\dao-sim
```

## 架构

- `sim/`：C++17 仿真核（`sim_core` 静态库 + `sim_validate` 验收 CLI + `sim_engine` WebSocket 引擎）
- `web/`：Vite + React 监测台（只收和画，不参与动力学）
- 引擎：`ws://127.0.0.1:3848` · 前端：`http://127.0.0.1:3847`

CMake 开关：

| 选项 | 默认 | 含义 |
|------|------|------|
| `DAO_SIM_BUILD_DYNAMICS` | ON | 阶段 2+：道核/Roche/σ/`sim_world`；关闭后只留导通层 |
| `DAO_SIM_BUILD_ENGINE` | ON | WebSocket 引擎（需要 DYNAMICS=ON） |
| `DAO_SIM_BUILD_TESTS` | ON | Catch2 单测 |

## 依赖

- CMake ≥ 3.16、C++17 编译器
- Node.js ≥ 18（前端）
- 网络：首次配置会用 CMake FetchContent 拉取 Catch2、IXWebSocket

## 阶段 0 验收（导通层绿灯）

只验证图 / 事件队列 / 神经元骨架 / `sim_validate`。  
**不**编引擎、**不**跑 `test_dao_oscillation` / `test_criticality`（那是阶段 2）。

Windows（`D:\dao-sim`）：

```bat
scripts\verify_phase0.bat
```

Linux/macOS：

```bash
chmod +x scripts/verify_phase0.sh
./scripts/verify_phase0.sh
```

等价手动命令：

```bat
cmake -S sim -B sim/build-phase0 -DDAO_SIM_BUILD_ENGINE=OFF -DDAO_SIM_BUILD_DYNAMICS=OFF
cmake --build sim/build-phase0 -j
ctest --test-dir sim/build-phase0 --output-on-failure
sim\build-phase0\sim_validate.exe 42
```

通过标准：`ctest` 仅 2 项（`test_graph`、`test_queue_long_delay`）全绿；`sim_validate` 的 PASS/FAIL 清单全绿。

## 构建与运行（完整产品）

默认打开动力学 + 引擎：

```bash
# 引擎 + 全部测试（含阶段 2）
cmake -S sim -B sim/build
cmake --build sim/build -j
ctest --test-dir sim/build --output-on-failure

# 导通层验收（PASS/FAIL 行）
./sim/build/sim_validate 42

# 前端
npm --prefix web install
npm --prefix web run dev

# 或一键（Linux/macOS）
chmod +x start.sh
./start.sh
# Windows: start.bat
```

假遥测（只测管道）：

```bash
./sim/build/sim_engine --fake
```

## 拓扑硬约束

| 区域 | 规模 | 连接 |
|------|------|------|
| 左/右沉淀核 | 各 256 | **只与道核双向**；彼此无边；不接方位群 |
| 道核阳/阴 | 各 64 | 互抑半中心；跨核唯一通道 |
| 东/西/南/北/上/下 | 各 64 | 与道核双向；群间稀疏 |
| 输出群 | 12（南群独立子集） | 与道核双向；不参与 σ 控制 |

## 控制协议（JSON → 引擎）

`start` / `pause` / `step{n}` / `seed{value}` /  
`ablation{name:dao_adaptation|noise,on}`（`dao_adaptation` **仅**作用于 DaoYang/DaoYin）/  
`perturb{group:east,pulses:50}` / `param{name:g,value}`

## 消融

- A：关闭道核适应电流 → 交替节律应破坏  
- B：关闭噪声 → 多种子输出更趋同  
- C：向东群注入 50 脉冲 → 输出模式编辑距离上升  

## 代码审计范围

只审计仿真输出路径，**不含 UI 中文标签**：

```bash
# output.cpp：排除 #include 行后，函数体内不得有字符串字面量当作系统输出
rg -n '"' sim/src/output.cpp | rg -v '#include' || true
# 禁忌语义/意识开关
rg -n 'conscious|意识开关|semantic_map|neuron_to_word' sim/src || true
```

说明：`#include "output.h"` 会命中普通 `"` 搜索，**可忽略**；只关心函数体里是否把字符串当输出。

`sim/src/output.cpp` 只产生 `uint16` 比特；遥测为二进制数值帧。

## 已知风险（阶段 2）

1. ODE 参考轨迹不与 SNN 逐点对齐；面板以 SNN 的 D(t) 为准，ODE 作包络。  
2. 消融 A 必须只关道核 `u`，禁止全局 `u=0`。

## GitHub 远程

目标：`https://github.com/666xp/dao-sim`

```bat
cd /d D:\dao-sim
git pull origin main
```
