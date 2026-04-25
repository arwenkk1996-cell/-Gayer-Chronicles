# Dialogue System

> **Status**: Designed
> **Author**: User + Claude Code (narrative-director, game-designer, ue-umg-specialist)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Narrative — the player's voice in the world

## Overview

对话系统管理盖亚与所有 NPC 的交谈：分支对话树的展示与选择、条件分支的判断（依赖叙事状态机）、对话选择的记录与回写、以及 NPC 对话状态的持久化（他们记得上次聊了什么）。系统采用 **Yarn Spinner for Unreal**（开源对话中间件）作为对话数据层，配合自定义 UMG 对话界面。Yarn Spinner 处理脚本解析和分支逻辑；UE5 处理角色动画、摄像机和 UI 呈现。

## Player Fantasy

和纳雅说话，不是在读 NPC 台词——是在和一个有性格的人说话，她会记得盖亚上次说了什么。选哪个选项不只是剧情进度的按钮，而是在表达盖亚对这件事的态度：怀疑、好奇、漠然，还是愤怒。每次对话结束后玩家应该感觉：**我刚才做了一个选择，我不确定它会带来什么**。

## Detailed Design

### Core Rules

**对话触发：**
- NPC 实现 `IInteractable`，`OnInteract()` 调用 `UDialogueSubsystem::BeginDialogue(NPCTag, NodeID)`
- `NodeID` 由叙事状态机决定（NPC 记得上次对话进展到哪个节点）
- 对话开始时：摄像机软切至对话构图（NPC 半侧脸 + 盖亚后脑勺），移动输入挂起

**Yarn Spinner 集成：**
- 对话内容存储为 `.yarn` 脚本文件，按 NPC 和场景组织（`Assets/Dialogue/Naya/Sourceholm_Act1.yarn`）
- Yarn 脚本通过自定义 Command 与叙事状态机交互：
  - `<<set_flag Fragment.Sourceholm.WaterGate.Available>>` — 写入标记
  - `<<check_flag Dialogue.Naya.FirstMeeting.Done>>` — 条件分支
  - `<<add_ending_score Restore 2>>` — 结局得分
- 所有 Yarn 变量通过 `UNarrativeStateSubsystem` 存取，不使用 Yarn 内置变量存储（存档统一管理）

**选项展示规则：**
- 最多同时显示 4 个选项
- 选项文字不超过 20 个汉字（确保 HUD 可读性）
- 不可用选项（条件未满足）**不显示**（不是灰色禁用——叙事游戏不该让玩家看到"锁着的门"）
- 超时机制：**不设置**（玩家有权慢慢思考）

**对话结束：**
- Yarn 脚本执行到结束节点时，`UDialogueSubsystem::EndDialogue()` 被调用
- 写入 `Dialogue.<NPCTag>.<NodeID>.Done` 标记到叙事状态机
- 摄像机恢复，移动输入解锁
- 若有结局得分变化，异步触发自动存档

### States and Transitions

| 状态 | 说明 | 转换 |
|------|------|------|
| `Idle` | 无对话进行中 | `BeginDialogue()` → `Active` |
| `Active` | 对话进行中，显示文本和选项 | 对话结束 → `Closing`；玩家选择 → 继续 `Active` |
| `Closing` | 结束动画（摄像机返回，文本淡出） | 动画完成 → `Idle` |

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 叙事状态机 | 读+写 | `HasFlag` 条件判断 / `SetFlag` + `IncrementInt` 写回 |
| 探索/交互系统 | 被触发 | `OnInteract()` 启动对话 |
| 摄像机系统 | 控制 | `SetDialogueCameraState(NPCTag)` |
| 角色控制器 | 挂起/恢复 | 对话期间挂起移动输入 |
| HUD/UI 系统 | 推送渲染数据 | `FDialogueLineData`（台词文本、说话者、选项列表）|
| 日志系统 | 触发 | `OnFlagSet` 监听，某些对话完成后解锁日志条目 |
| 音频系统 | 触发 | 对话开始/结束时广播，音频系统播放对应状态音乐 |

## Formulas

### 公式 1：NPC 对话节点选择

`StartNodeID = NarrativeState.GetInt(NPC.<Tag>.LastNodeID) ?? DefaultStartNodeID`

| 变量 | 类型 | 说明 |
|------|------|------|
| `LastNodeID` | `int32` | 上次对话结束时存入的节点 ID |
| `DefaultStartNodeID` | `FName` | Yarn 脚本中的默认起始节点（初次见面） |
| 输出 | `FName` | 本次对话从哪个 Yarn 节点开始 |

**示例：** 玩家第一次与纳雅对话 → `LastNodeID` 未设置 → 从 `Naya_FirstMeeting` 节点开始；第二次 → 从 `Naya_AfterWaterGate` 开始（若该标记已设置）。

## Edge Cases

- **如果对话进行中玩家被强制传送（碎片触发）**：对话立即中断，`EndDialogue()` 以 `Aborted` 状态调用，已说的台词部分写入标记（`Dialogue.<Tag>.Partial`），不写 `Done`；NPC 下次见面时从中断点续。
- **如果 Yarn 脚本文件缺失或解析失败**：显示 fallback 台词"[对话数据加载失败]"，`EndDialogue()` 正常调用，`Done` 标记写入（防止反复触发错误）。
- **如果 NPC 在对话进行中死亡或消失（仅逃亡层碎片场景可能）**：调用 `EndDialogue(Aborted)`，摄像机立即返回。
- **如果玩家在 `Closing` 状态中再次触发同一 NPC**：忽略，等待 `Closing` 完成后方可重新触发。
- **如果所有可用选项均因条件不满足而不显示（空选项列表）**：自动选择第一个隐藏选项（Yarn 脚本设计规范：至少保留一个无条件的"再见"选项）。

## Dependencies

**上游依赖：** 叙事状态机、探索/交互系统

**下游依赖：** 日志/编年史系统（监听对话完成）、HUD/UI 系统（对话 UI 渲染）、NPC AI 系统（行为状态读取对话历史）、本地化系统（所有台词走本地化管道）

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| 选项最大数量 | 4 | 2–4 | 超过 4 个选项认知负荷过高 |
| 选项文字最大长度 | 20 汉字 | 15–30 | UI 布局约束 |
| 对话摄像机距离 | NPC 1.5m 斜侧面 | — | 对话亲密感 |
| 台词推进方式 | 玩家按键推进（非自动） | — | 阅读节奏由玩家控制 |

## Visual/Audio Requirements

- 对话进行中背景音乐降低至 30% 音量（Duck），对话结束后 2 秒内恢复
- 无 VO 配音（文字量大，配音成本高；叙事游戏文字阅读本身是体验的一部分）
- NPC 说话时配轻微嘴部动画（Loop，不做逐字口型同步）
- 对话摄像机：轻微景深（背景虚化），强调人物面部

📌 **UX Flag — Dialogue System**：对话 UI（台词框、选项列表、说话者标签）需要 `/ux-design` 创建专属 UX spec。

## UI Requirements

- 台词框：屏幕下方约 30%，日志米底色，深棕文字
- 说话者标签：台词框左上角，手写字体
- 选项列表：台词框下方，竖向排列，母系青高亮当前悬停选项
- 键盘：数字键 1–4 快速选择 / 鼠标点击 / 手柄上下选择 + A 确认

## Acceptance Criteria

- **GIVEN** 玩家触发纳雅，**WHEN** 首次对话，**THEN** 从 `Naya_FirstMeeting` 节点开始，摄像机切至对话构图，HUD 移动提示隐藏。
- **GIVEN** 对话中有条件选项（`<<check_flag Fragment.WaterGate.Complete>>`），条件未满足，**THEN** 该选项不出现在 UI。
- **GIVEN** 玩家选择包含 `<<add_ending_score Restore 2>>` 的选项，**THEN** 叙事状态机中 `Ending.Score.Restore` 增加 2。
- **GIVEN** 对话结束，**THEN** `Dialogue.Naya.FirstMeeting.Done` 写入叙事状态机，摄像机恢复，移动输入解锁。
- **GIVEN** 玩家第二次触发纳雅，**THEN** 从 `LastNodeID` 对应节点开始，不重复第一次的对话内容。

## Open Questions

- **OQ-01**：Yarn Spinner for Unreal 的最新版本（UE5.7 兼容性）需要在架构阶段验证。若不兼容，备选方案为 Articy Draft 3 + UE5 插件，或自研简化对话树（仅需分支+条件，无需复杂功能）。
- **OQ-02**：是否需要支持对话内的"回顾"功能（翻看上一句台词）？目前设计为不支持，但部分玩家反馈叙事游戏需要。
