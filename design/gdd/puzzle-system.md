# Puzzle System

> **Status**: Designed
> **Author**: User + Claude Code (game-designer, level-designer)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Gameplay — giving exploration a purpose

## Overview

谜题系统为盖亚编年史中的环境谜题提供统一框架。游戏中的谜题分为两类：**现实层谜题**（盖亚在遗迹中解决的物理/观察谜题）和**碎片内谜题**（在记忆碎片中以历史人物视角解决的情境谜题）。两类谜题共享同一状态管理接口（叙事状态机），但触发流程不同。系统不提供通用谜题引擎，而是定义谜题的**状态约定、触发接口和完成回调**，具体谜题逻辑在各自的关卡蓝图中实现。

## Player Fantasy

解谜的感觉不应该是"卡住了然后查攻略"，而是"我绕着它走了三圈，突然明白了"。谜题的答案应该藏在已经展示给玩家的世界细节里——看过某个碎片后回来看同一个谜题，会发现它变得显而易见。解开谜题的快感不来自操作技巧，来自**理解**。

## Detailed Design

### Core Rules

**谜题类型定义：**

| 类型 | 场景 | 核心逻辑 | 示例 |
|------|------|----------|------|
| **观察谜题** | 现实层 | 找到隐藏信息，用于解锁序列或路径 | 四面墙上有符文，需按特定顺序触碰 |
| **重构谜题** | 现实层 | 将破碎物件还原至正确排列 | 拼合散落的石板以显示完整地图 |
| **时态谜题** | 现实层（需碎片知识） | 谜题答案只有看过某碎片后才能解读 | 碎片里见过的密码，用于现实层的锁 |
| **情境谜题** | 碎片内 | 以历史人物身份，在当时的情境限制下完成任务 | 作为建国初代的人，在无现代工具的条件下引水 |

**谜题状态机（每个谜题独立）：**

```
Locked → Available → InProgress → Solved
```

- `Locked`：前置条件未满足，谜题元素不可见/不可交互
- `Available`：可以开始，但玩家尚未介入
- `InProgress`：玩家已触碰谜题元素，进行中
- `Solved`：完成，状态写入叙事状态机，触发奖励

状态以 `FGameplayTag` 存储：`Puzzle.<Area>.<PuzzleName>.Solved`

**谜题失败处理：**
- 本游戏**无惩罚性失败**——错误操作只是"没有进展"，不扣血、不触发警报、不重置（除非重置是谜题机制本身的一部分）
- 玩家可以随时走开再回来

**提示系统：**
- 无内置提示按钮（破坏沉浸感）
- 代替方案：若玩家在同一谜题元素附近停留 > 90 秒，盖亚自动发出内心独白（检视文本），给出隐晦提示
- 盖亚的提示语气是"她自己在思考"，不是"游戏在告诉你答案"

### States and Transitions

| 状态 | 进入条件 | 标记 |
|------|----------|------|
| `Locked` | 默认 | 无标记 |
| `Available` | NSM 前置标记满足 | `Puzzle.X.Available`（可选，有些谜题无需标记） |
| `InProgress` | 玩家触碰第一个谜题元素 | 运行时瞬态，不持久化 |
| `Solved` | 完成条件满足 | `Puzzle.X.Solved` |

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 叙事状态机 | 读+写 | 前置查询 + `SetFlag(Puzzle.X.Solved)` |
| 探索/交互系统 | 被触发 | 谜题元素实现 `IInteractable` |
| 记忆碎片系统 | 上下文 | 碎片内谜题由 `UMemoryFragmentSubsystem` 管理执行上下文 |
| 日志系统 | 触发 | `Puzzle.Solved` 可解锁对应日志条目 |
| 音频系统 | 触发 | 谜题解决时播放特定音效 |

## Formulas

### 公式 1：提示触发

`ShowHint = (TimeNearPuzzle > HintThreshold) AND (PuzzleState == InProgress) AND NOT HintShownAlready`

| 变量 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `TimeNearPuzzle` | float (秒) | — | 玩家在谜题 200cm 范围内的累计时间 |
| `HintThreshold` | float (秒) | 90 | 触发提示的等待时长 |
| `HintShownAlready` | bool | false | 每次进入 InProgress 状态只触发一次提示 |

### 公式 2：时态谜题解锁条件

`TimeGatedPuzzle_Available = HasFlag(Fragment.<RelatedFragment>.Complete)`

时态谜题（需要碎片知识的现实层谜题）的唯一解锁条件是对应碎片已完成。没有其他前置。设计意图：玩家不需要"想出答案"，而是需要"去看那段历史"。

## Edge Cases

- **如果谜题在 InProgress 状态时存档**：读档后恢复为 Available（与记忆碎片 InProgress 相同处理，InProgress 不持久化）。
- **如果谜题元素在解决过程中因关卡流送被卸载**：谜题状态回到 Available，重新进入区域后重新开始（设计规范：同一区域内的谜题元素不跨流送边界）。
- **如果同一谜题有多个互动元素同时被触发**：按 `Actor.GetUniqueID()` 顺序串行处理，不并发执行。
- **如果玩家已解决谜题后再次触碰谜题元素**：元素保持解决后的视觉状态，交互提示变为"已解决"或直接无交互反应（由各谜题关卡蓝图自定义）。

## Dependencies

**上游依赖：** 叙事状态机、探索/交互系统、记忆碎片系统（碎片内谜题）

**下游依赖：** 日志/编年史系统、音频系统

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| 提示触发等待时间 | 90 秒 | 60–180 秒 | 设计师根据谜题难度个别覆盖 |
| 提示触发距离 | 200 cm | 150–300 cm | 玩家离谜题多近才开始计时 |
| 每个区域谜题数量（设计规范） | 2–4 个 | — | 过多导致节奏碎片化；过少失去探索目的 |

## Visual/Audio Requirements

- 谜题元素激活状态：母系青（`#4AADAA`）轮廓高光，与普通交互物同色（统一视觉语言）
- 谜题解决瞬间：记忆金（`#E8B84B`）短暂全亮 + 粒子扩散效果
- 音效：解锁音（古代石门类）/ 错误音（软性阻尼，不刺耳）/ 完成音（共鸣类，三音阶上行）

## UI Requirements

- 无专属 UI（谜题通过环境视觉传达状态）
- 盖亚的提示独白通过检视文本系统显示（复用 HUD 已有组件）

## Acceptance Criteria

- **GIVEN** `Puzzle.Sourceholm.WaterGate.Available` 已设置，**WHEN** 玩家触碰第一个谜题元素，**THEN** 谜题进入 `InProgress`，提示计时开始。
- **GIVEN** 玩家完成谜题，**THEN** `Puzzle.Sourceholm.WaterGate.Solved` 写入 NSM，解锁音效和记忆金粒子效果播放。
- **GIVEN** 玩家在谜题旁停留 90 秒且未解决，**THEN** 盖亚独白提示文本出现，且本次 InProgress 内不再重复触发。
- **GIVEN** 时态谜题的前置碎片未完成，**WHEN** 玩家接触谜题元素，**THEN** 谜题元素无交互（不可触发，处于 Locked 状态）。
- **GIVEN** 谜题 Solved 后，**WHEN** 玩家再次触碰元素，**THEN** 无交互提示（或显示"已解决"静态文本，具体由关卡蓝图决定）。

## Open Questions

- **OQ-01**：是否需要一个谜题编辑器工具（方便关卡设计师配置谜题序列）？或者全部在关卡蓝图中手写？建议 DataTable + 蓝图模板，待架构阶段确认。
