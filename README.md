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

## 依赖

- CMake ≥ 3.16、C++17 编译器
- Node.js ≥ 18（前端）
- 网络：首次配置会用 CMake FetchContent 拉取 Catch2、IXWebSocket

## 构建与运行

```bash
# 引擎 + 测试
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
# output.cpp 不得把字符串当系统输出
rg -n '"' sim/src/output.cpp || true
# 禁忌语义/意识开关
rg -n 'conscious|意识开关|semantic_map|neuron_to_word' sim/src || true
```

`sim/src/output.cpp` 只产生 `uint16` 比特；`worker`/`ws` 遥测为二进制数值帧。

## 已知风险（阶段 2）

1. ODE 参考轨迹不与 SNN 逐点对齐；面板以 SNN 的 D(t) 为准，ODE 作包络。  
2. 消融 A 必须只关道核 `u`，禁止全局 `u=0`。

## GitHub 远程

目标：`https://github.com/666xp/dao-sim`  

若 Cloud Agent 无法 `gh`/`git push` 到 github.com，请在网页新建空库后，本机：

```bat
cd /d D:\dao-sim
git remote add origin https://github.com/666xp/dao-sim.git
git push -u origin main
```
