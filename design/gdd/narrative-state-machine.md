# Narrative State Machine

> **Status**: Designed
> **Author**: User + Claude Code (narrative-director, systems-designer, unreal-specialist)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Foundation — all narrative systems depend on this

## Overview

叙事状态机是 Gaia Chronicles 的全局记忆系统。它以 `UGameInstanceSubsystem` 形式存在，持久化整个游戏会话，记录所有叙事事件——记忆碎片的触发与完成、对话选项的选择、文物的收集、谜题的解决——并通过 `FGameplayTag` 键名和委托广播向所有其他系统暴露读写接口。玩家不直接操作这个系统；他们通过探索和选择向它写入，通过世界的变化看到它的输出。它是让"盖亚发现的东西改变她眼中的世界"这一核心体验得以实现的数据基础。

## Player Fantasy

这个系统是隐形的——玩家永远不会"使用叙事状态机"。他们感受到的是它的存在所带来的**重量感**：

- 盖亚回到三小时前路过的废墟，墙上的刻文现在有了不同的含义——因为她已经看过那段碎片。
- 一个 NPC 对她说话的方式变了，因为她做出了某个选择。
- 她的日志里多出了一行笔记，她没有手动写，但它就在那里。

**玩家的感受是：这个世界在记着我做过的事。** 不是通过分数或等级，而是通过细节的积累——那种只有认真探索才会注意到的、世界在悄悄回应你的感觉。这是叙事游戏里最难复制、也最令人难忘的东西：发现自己的行为留下了痕迹。

## Detailed Design

### Core Rules

系统以 `UNarrativeStateSubsystem`（继承 `UGameInstanceSubsystem`）实现，持有两类数据存储：

**1. 标记存储（Flag Store）**
- 数据类型：`FGameplayTagContainer`
- 用途：记录所有布尔型叙事事件（碎片已完成、对话选项已选、文物已收集、谜题已解决、区域已访问）
- 写入：`SetFlag(FGameplayTag)` — 幂等，重复设置无副作用
- 查询：`HasFlag(FGameplayTag) → bool`
- 删除：`ClearFlag(FGameplayTag)` — 仅用于特殊撤销场景，正常流程不调用

**2. 整数存储（Int Store）**
- 数据类型：`TMap<FGameplayTag, int32>`
- 用途：记录累积数值——三条结局轨道得分、收集计数、访问次数
- 写入：`SetInt(FGameplayTag, int32)` / `IncrementInt(FGameplayTag, int32 Delta=1)`
- 查询：`GetInt(FGameplayTag) → int32`

**3. 事件广播**
- `FOnFlagSet`（`FGameplayTag`）：任意标记写入时广播
- `FOnIntChanged`（`FGameplayTag, int32 NewValue`）：任意整数变化时广播
- 下游系统（视觉过渡、对话、谜题）订阅各自关心的 Tag，无需轮询

### States and Transitions

**记忆碎片的四态模型：**

| 状态 | 含义 | 进入条件 |
|------|------|----------|
| `Undiscovered` | 默认状态，玩家不知道它存在 | 游戏开始 |
| `Available` | 触发条件满足，触碰器已激活 | 前置标记全部设置（由 DataTable 配置） |
| `InProgress` | 玩家进入碎片场景 | 触碰对应文物 |
| `Complete` | 碎片场景结束，结果写回 | 场景完成事件触发 |

状态存储方式：以 `FGameplayTag` 为键，约定命名：
```
Fragment.<Area>.<FragmentName>.Available
Fragment.<Area>.<FragmentName>.Complete
```
`InProgress` 是运行时瞬态，不持久化（存档时若值为 InProgress，读档后重置为 Available）。

**结局轨道的三条得分线：**

| Tag | 含义 | 增量触发来源 |
|-----|------|-------------|
| `Ending.Score.Restore` | 盖亚倾向公开真相 | 对话选择、某些谜题解法、特定文物收集 |
| `Ending.Score.Protect` | 盖亚倾向保护神话 | 同上，不同分支 |
| `Ending.Score.Release` | 盖亚倾向留给后人发现 | 同上，不同分支 |

三条得分线在整个游戏中并行积累，互不排斥。最终章读取三个值，选最高者触发对应结局；若最高分相同，触发专属的"两难结局"变体。

### Interactions with Other Systems

| 系统 | 方向 | 接口 | 说明 |
|------|------|------|------|
| 记忆碎片系统 | 读+写 | `HasFlag` / `SetFlag` / `IncrementInt` | 碎片完成时写入 Complete 标记和结局分；启动前查询前置条件 |
| 探索/交互系统 | 读 | `HasFlag` | 检查文物触发器是否激活（Fragment.Available） |
| 视觉过渡系统 | 读+订阅 | `OnFlagSet` | 监听 Fragment.Complete 事件，触发时间层视觉切换 |
| 对话系统 | 读+写 | `HasFlag` / `SetFlag` | 对话条件判断 + 选项选择后写入选择标记 |
| 谜题系统 | 读+写 | `HasFlag` / `SetFlag` | 谜题解锁条件 + 解决后写入 Solved 标记 |
| 存档/读档系统 | 读+写（全量） | `ExportState() → FSaveData` / `ImportState(FSaveData)` | 存档时导出全部 Flag + Int；读档时恢复 |
| 日志/编年史系统 | 订阅 | `OnFlagSet` | 监听 Fragment.Complete 和 Artifact.Collected，自动解锁对应日志条目 |
| NPC AI 系统 | 读 | `HasFlag` | NPC 根据已知对话选择调整行为状态 |
| 世界地图系统 | 读 | `HasFlag` / `GetInt` | 区域解锁状态、访问进度显示 |

## Formulas

### 公式 1：碎片可用性判定

`Fragment_Available = ALL(Prerequisites[i].Complete == true)`

| 变量 | 符号 | 类型 | 范围 | 说明 |
|------|------|------|------|------|
| 前置标记列表 | `Prerequisites` | `FGameplayTag[]` | 0–5 个 | 该碎片解锁所需的已完成标记，配置于 DataTable |
| 输出 | `Fragment_Available` | `bool` | true/false | 全部满足才激活触发器 |

**输出范围：** 布尔值；Prerequisites 为空时默认 true（游戏开始即可触发）  
**示例：** `Sourceholm.WaterGate` 要求 `Area.Sourceholm.Visited` 且 `Fragment.Sourceholm.MarketStall.Complete`，两者满足后激活。

---

### 公式 2：结局得分累积

`EndingScore[k] += Weight[choice]`

| 变量 | 符号 | 类型 | 范围 | 说明 |
|------|------|------|------|------|
| 结局轨道 | `k` | `enum {Restore, Protect, Release}` | — | 三条轨道之一 |
| 单次权重 | `Weight[choice]` | `int32` | 1–3 | 配置于对话/谜题 DataTable；主线选项 Weight=3，支线 Weight=1 |
| 当前得分 | `EndingScore[k]` | `int32` | 0–∞（实际约 0–60） | 全游戏累计 |

**输出范围：** 满分游戏约 60 分；一般玩家单轨约 20–35 分  
**示例：** 对话选"我要把这件事写进正式记录" → `EndingScore[Restore] += 3`

---

### 公式 3：最终结局判定

```
WinningEnding = argmax(EndingScore[Restore], EndingScore[Protect], EndingScore[Release])

IF max_score 唯一:
    触发 WinningEnding 对应结局
ELSE IF 平局（两条或三条得分相同且均为最高）:
    触发 "两难变体"（Ambivalent variant）—— 不可通过单一路线主动追求
```

| 变量 | 类型 | 范围 | 说明 |
|------|------|------|------|
| `EndingScore[Restore/Protect/Release]` | `int32` | 0–60 | 三条轨道最终值 |
| `WinningEnding` | `enum` | 4 种（含平局变体） | 最终章读取 |

**输出范围：** 4 种可能结局（Restore / Protect / Release / Ambivalent）  
**示例：** 游戏结束时 Restore=28, Protect=15, Release=12 → 触发 Restore 结局

## Edge Cases

- **如果存档时碎片状态为 `InProgress`**：读档后重置为 `Available`，不保存 InProgress 状态。玩家重新进入该区域后可再次触碰文物进入碎片。（碎片场景是过场性质，中途中断无法保存场景内进度。）

- **如果同一帧内多个系统同时调用 `SetFlag`**：全部写入，全部广播，顺序不保证。所有 Flag 写入为幂等操作，无竞态风险。

- **如果 `ClearFlag` 被调用但该 Tag 从未被设置**：静默忽略，不报错，不广播。（合法的"确保未设置"防御性调用。）

- **如果碎片的前置 DataTable 条目缺失或引用了不存在的 Tag**：编辑器下触发 `ensure()` 断言并打印警告；发行版中该碎片视为前置条件为空，立即 Available。（宁可让碎片提前出现，也不让游戏卡死。）

- **如果三条结局得分全部为 0**：不可能发生——至少三个主线碎片的完成事件强制写入权重为 3 的得分，这些碎片是游戏主干，无法跳过。

- **如果两条得分并列最高（非三条）**：触发 Ambivalent 变体，叙事文本反映盖亚"在两种真相之间无法决断"，与三条并列时的文本有所不同。

- **如果玩家开始新游戏**：调用 `ResetAllState()`，清空 FlagStore 和 IntStore，广播 `OnStateReset` 事件通知所有订阅者。新游戏与读档走同一条初始化路径，区别仅在于传入的 `FSaveData` 是否为空。

- **如果存档文件损坏或反序列化失败**：回退到全空状态（等同于新游戏），在 HUD 显示"存档数据异常，已重置"提示。不静默失败，不崩溃。

- **如果同一个碎片的 Complete 标记被重复写入**：幂等，忽略；`OnFlagSet` 广播不重复触发（避免日志系统重复解锁同一条目）。实现上：`SetFlag` 在写入前检查容器是否已包含该 Tag，若已存在则不广播。

## Dependencies

**上游依赖（本系统依赖的）：**
无。本系统是 Foundation 层，不依赖任何其他游戏系统。

---

**下游依赖（依赖本系统的）：**

| 系统 | 依赖性质 | 本系统提供的接口 |
|------|----------|-----------------|
| 探索/交互系统 | 硬依赖 | `HasFlag(Fragment.<Area>.<Name>.Available)` |
| 记忆碎片系统 | 硬依赖 | `HasFlag`（前置查询）/ `SetFlag` + `IncrementInt`（完成写入） |
| 视觉过渡系统 | 硬依赖 | `FOnFlagSet` 订阅（监听 Fragment.Complete） |
| 存档/读档系统 | 硬依赖 | `ExportState()` / `ImportState()` |
| 对话系统 | 硬依赖 | `HasFlag`（对话条件）/ `SetFlag`（选项选择记录） |
| 谜题系统 | 硬依赖 | `HasFlag` / `SetFlag` |
| 日志/编年史系统 | 硬依赖 | `FOnFlagSet` 订阅 |
| NPC AI 系统 | 软依赖 | `HasFlag`（NPC 行为调整，无此系统 NPC 仍可运行） |
| 世界地图系统 | 软依赖 | `HasFlag` / `GetInt`（进度显示，无此系统地图仍可运行） |

**双向一致性说明：** 上述所有下游系统的 GDD 在编写时须在其 Dependencies 节反向声明"依赖叙事状态机"。

## Tuning Knobs

| 调节项 | 位置 | 默认值 | 安全范围 | 影响 | 极端情况 |
|--------|------|--------|----------|------|----------|
| **选项权重：主线选择** | 对话/谜题 DataTable `Weight` 列 | 3 | 2–5 | 主线选择对结局走向的影响力 | 设为 1：主线与支线等权，结局随机化；设为 10：单个选择即可锁定结局 |
| **选项权重：支线选择** | 同上 | 1 | 1–2 | 支线探索对结局的边际贡献 | 设为 0：支线完全不影响结局；设为 3：与主线等权 |
| **Ambivalent 触发阈值** | `UNarrativeStateSubsystem::AmbivalentThreshold` 常量 | 0（精确相等） | 0–5 | 两条得分差值在此范围内触发两难变体 | 设为 5：两难结局频率大幅提升 |
| **碎片最大前置数量** | 软性设计规范（非代码常量） | 3 | 1–5 | 单碎片前置条件数量上限 | 超过 5：设计复杂度失控；设为 0：碎片游戏开始即全部开放 |
| **结局得分上限警告** | 编辑器专用断言阈值 | 60 | 40–80 | 内容量控制辅助工具，超出时打印警告 | 不影响运行时逻辑 |

权重值存储于 DataTable，设计师无需改代码即可调整。`AmbivalentThreshold` 是代码常量，修改需重新编译，调整前须经 playtesting 验证。

## Visual/Audio Requirements

无直接视觉/音频需求。本系统为纯数据层基础设施，无可见 Actor 或音效。视觉和音频反馈由订阅本系统事件的下游系统（视觉过渡系统、音频系统）负责。

## UI Requirements

无直接 UI。玩家感知本系统输出的界面入口是日志系统和对话系统；HUD 上不显示状态机内部数据（结局得分不对玩家可见）。

## Acceptance Criteria

**核心规则覆盖：**

- **GIVEN** 系统初始化完成，**WHEN** 调用 `SetFlag(Tag.A)`，**THEN** `HasFlag(Tag.A)` 返回 `true`，且 `FOnFlagSet` 广播一次 `Tag.A`。

- **GIVEN** `Tag.A` 已设置，**WHEN** 再次调用 `SetFlag(Tag.A)`（重复写入），**THEN** `HasFlag(Tag.A)` 仍返回 `true`，但 `FOnFlagSet` **不**再次广播。

- **GIVEN** `Tag.B` 从未设置，**WHEN** 调用 `ClearFlag(Tag.B)`，**THEN** 不抛出异常，不广播，函数静默返回。

- **GIVEN** 碎片 `Fragment.Sourceholm.WaterGate` 配置了前置条件 `[Tag.X, Tag.Y]`，**WHEN** 只有 `Tag.X` 已设置，**THEN** `Fragment_Available` 返回 `false`，触发器未激活。

- **GIVEN** 同一碎片前置条件 `[Tag.X, Tag.Y]` 均已设置，**WHEN** 系统重新评估，**THEN** `Fragment.Sourceholm.WaterGate.Available` 被设置，触碰器激活。

**公式覆盖：**

- **GIVEN** 玩家选择对话选项（`Weight=3, Track=Restore`），**WHEN** 对话系统调用 `IncrementInt(Ending.Score.Restore, 3)`，**THEN** `GetInt(Ending.Score.Restore)` 返回调用前值 +3。

- **GIVEN** 游戏结束时 `Score.Restore=28, Score.Protect=15, Score.Release=12`，**WHEN** 最终章读取结局，**THEN** 触发 Restore 结局，不触发其他变体。

- **GIVEN** 游戏结束时 `Score.Restore=20, Score.Protect=20, Score.Release=10`，**WHEN** 最终章读取结局（`AmbivalentThreshold=0`），**THEN** 触发 Ambivalent 变体。

**存档/读档覆盖：**

- **GIVEN** 碎片状态为 `InProgress` 时触发存档，**WHEN** 读取该存档，**THEN** 碎片状态恢复为 `Available`，`InProgress` 标记不存在。

- **GIVEN** 存档文件已损坏（JSON 结构无效），**WHEN** 系统尝试 `ImportState()`，**THEN** 状态重置为全空，HUD 显示"存档数据异常，已重置"，游戏继续运行。

**新游戏覆盖：**

- **GIVEN** 已有完整游戏进度，**WHEN** 调用 `ResetAllState()`，**THEN** FlagStore 和 IntStore 均为空，`OnStateReset` 广播一次，所有得分归零。

## Open Questions

- **OQ-01**：DataTable 热重载——设计师在编辑器中修改碎片前置条件后，是否需要重启 PIE 才能生效？需要在实现阶段验证 UE5.7 的 DataTable 变更通知机制。（负责人：unreal-specialist，解决于 ADR 阶段）

- **OQ-02**：结局得分是否应该对玩家部分可见（如日志中的隐晦暗示）？当前设计为完全隐藏。这是叙事设计决策，暂缓至叙事导演评审。（负责人：narrative-director）

- **OQ-03**：多存档槽位——当前设计假设单存档。若需要支持多存档，`ExportState`/`ImportState` 的接口设计需要调整。（负责人：存档系统 GDD 阶段确认）
