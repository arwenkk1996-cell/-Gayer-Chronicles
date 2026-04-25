# Journal / Chronicle System

> **Status**: Designed
> **Author**: User + Claude Code (narrative-director, ue-umg-specialist)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Narrative — the player's map through time

## Overview

日志/编年史系统是盖亚随身携带的"日记本"——自动记录她的所有发现，以她的第一人称视角呈现。当玩家完成一段记忆碎片、完成一次对话或收集一件文物时，对应的日志条目自动解锁并写入，无需玩家手动记录。系统的核心 UI 是双页展开的日记本界面，左页为四条时间轴（对应四个时代），右页为当前选中条目的详情文本。日志是叙事的副读物，也是追踪主线进度的唯一工具。

## Player Fantasy

翻开日志，不是在查数据库——是在重新翻盖亚的思考。某个条目里她写着"我不确定这是不是真的"，三个碎片之后，同一件事她的语气变了。玩家翻阅日志的感觉应该是：**我在看一个人在理解世界的过程，不是在核对收集进度**。

## Detailed Design

### Core Rules

**条目结构 `FJournalEntry`：**
```
FJournalEntry {
    FGameplayTag     EntryID          // 唯一标识，如 Journal.Fragment.Sourceholm.WaterGate
    ETimeLayer       TimeLayer        // 归属时代（决定在哪条时间轴上显示）
    FText            Title            // 短标题（< 10 字）
    FText            BodyText         // 盖亚第一人称内心独白（100–300 字）
    FText            RevealText       // 可选：后续发现后追加的"盖亚的补记"
    FDateTime        UnlockTimestamp  // 解锁时间（时间轴排序用）
    bool             bHasReveal       // 是否有追加文本（已触发时为 true）
}
```

**自动解锁触发：**
- 记忆碎片完成（`Fragment.X.Complete` 被设置）→ 解锁 `Journal.Fragment.X`
- 对话完成（`Dialogue.NPC.Node.Done` 被设置）→ 若 DataTable 中有对应映射，解锁对应条目
- 文物收集（`Artifact.X.Collected`）→ 解锁 `Journal.Artifact.X`（文物专属条目）
- 谜题解决（`Puzzle.X.Solved`）→ 若 DataTable 中有映射，解锁附加背景条目

所有触发通过订阅 `UNarrativeStateSubsystem::FOnFlagSet` 实现，日志系统在收到对应 Tag 时查询 DataTable 确认是否需要新建条目。

**追加文本（Reveal）机制：**
某些日志条目在后续发现后会有"补记"——盖亚重新审视同一件事的新想法。例如：
- `Journal.Fragment.Sourceholm.WaterGate` 初始解锁时：盖亚对水渠设计感到好奇
- 看完 `Fragment.Labyrinth.Archive` 后：追加"原来这个水渠的设计来自于……"

追加文本通过同一个 Flag 监听触发，DataTable 中配置 `RevealCondition`（哪个标记触发追加）。

**时间轴显示：**
- 四条并行竖轴，从上（早）到下（晚）
- 每个条目是轴上的一个圆点节点，已解锁为实心，未发现为完全不显示
- 节点之间以细线相连（同一时代的条目形成链条）
- 玩家点击节点，右页显示该条目详情

### States and Transitions

日志系统无复杂状态机。核心状态：
- `Closed`：日志界面关闭，监听事件仍在后台运行
- `Open`：玩家打开日志界面（按 `J` / 手柄 `Back` 键），暂停游戏时钟

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 叙事状态机 | 订阅 | `FOnFlagSet` — 监听 Fragment/Dialogue/Artifact/Puzzle 完成事件 |
| 记忆碎片系统 | 触发源 | `Fragment.X.Complete` 触发碎片条目解锁 |
| 对话系统 | 触发源 | `Dialogue.X.Done` 触发对话相关条目解锁 |
| 文物系统 | 触发源 | `Artifact.X.Collected` 触发文物条目解锁 |
| HUD/UI 系统 | 共用 | 新条目解锁时推送 HUD 右下角脉冲通知 |
| 存档系统 | 依赖 | 日志解锁状态通过叙事状态机 Flag 持久化，无独立存档 |

## Formulas

### 公式 1：时间轴位置计算

`AxisPosition = (EntryIndex / TotalEntriesInLayer) * AxisHeight`

| 变量 | 类型 | 说明 |
|------|------|------|
| `EntryIndex` | int32 | 条目在该时代轴上的顺序（按 UnlockTimestamp 排序） |
| `TotalEntriesInLayer` | int32 | 该时代轴上的总条目数（包含未解锁的） |
| `AxisHeight` | float (px) | 时间轴的总像素高度 |

未解锁条目占位但不显示节点，确保已解锁条目的相对位置在整个游戏过程中稳定（不随解锁数量变化而跳动）。

## Edge Cases

- **如果同一帧内多个触发事件同时到来**：条目依次入队解锁，每个间隔 0.3 秒（避免 HUD 通知堆叠）。
- **如果 DataTable 中无对应条目映射（Flag 触发但日志无配置）**：静默跳过，不创建空条目。
- **如果玩家在日志界面打开时完成碎片（理论上不可能，碎片进行中不可开日志）**：碎片期间日志界面无法打开（同 HUD 隐藏逻辑），碎片结束后正常处理。
- **如果 RevealText 触发时日志界面正好打开**：实时更新右页显示内容，加入轻微纸张翻动音效。
- **如果玩家读完所有条目但仍有未解锁条目（时间轴上有空白占位）**：占位节点显示为极淡的虚线圆圈轮廓（仅在玩家解锁了同一时代的相邻条目后才显示，避免剧透存在性）。

## Dependencies

**上游依赖：** 叙事状态机、记忆碎片系统、对话系统、文物系统

**下游依赖：** HUD/UI 系统（通知脉冲）

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| 条目正文字数 | 100–300 字 | 80–500 字 | 阅读时长；叙事密度 |
| 连续解锁间隔 | 0.3 秒 | 0–1.0 秒 | 避免 HUD 通知堆叠 |
| 时间轴节点大小 | 12px | 8–16px | UI 可读性 |
| 追加文本延迟显示 | 0.5 秒（进入条目详情后） | 0–1.0 秒 | 追加内容的"发现感" |

## Visual/Audio Requirements

- 日志界面：双页书本样式，日志米 `#F5EDD6` 底色，纸张质感纹理
- 翻页动画：进入/离开日志时模拟翻书效果（UMG 动画，0.3 秒）
- 时间轴节点：各时代颜色对应 Art Bible 四时代色调（现在=暖橙、旧世界=褪色棕、逃亡=冷灰、建国=暖黄）
- 新条目解锁时：HUD 右下角羽毛笔图标轻微发光脉冲（1.5 秒，然后消失）
- 音效：翻页纸张声（开启/关闭日志）；条目解锁时极轻微的墨水滴落声

📌 **UX Flag — Journal/Chronicle System**：双页日记本界面需要 `/ux-design` 创建 UX spec，优先级高（视觉差异化的核心 UI）。

## UI Requirements

- 双页摊开，左页时间轴，右页条目详情
- 时间轴四列并排，各时代标题在列顶
- 右页：条目标题 + 正文 + 可选追加文本（追加文本用不同墨色区分，如略深的棕色）
- 关闭键：`Esc` / `J` / 手柄 `Back`

## Acceptance Criteria

- **GIVEN** `Fragment.Sourceholm.WaterGate.Complete` 写入 NSM，**THEN** 0.3 秒内 `Journal.Fragment.Sourceholm.WaterGate` 条目解锁，HUD 脉冲通知出现。
- **GIVEN** 玩家打开日志，**THEN** 现在时代轴上显示已解锁条目节点，点击节点右页显示对应正文。
- **GIVEN** `Journal.Fragment.WaterGate.RevealCondition` 触发（对应碎片完成），**WHEN** 玩家查看该条目，**THEN** 追加文本以略深墨色显示在正文下方。
- **GIVEN** 同一帧 3 个 Flag 同时触发，**THEN** 3 个条目依次解锁，各间隔 0.3 秒，HUD 通知不堆叠。
- **GIVEN** 玩家在日志界面，**WHEN** 按 `Esc`/`J`/`Back`，**THEN** 日志关闭，游戏时钟恢复。

## Open Questions

- **OQ-01**：是否在最终游戏中提供"日志全文导出"功能（玩家通关后可保存日志为文本文件）？这是一个有趣的设计点，但实现成本非零。暂留。
- **OQ-02**：盖亚的日志文本在不同结局走向时（Restore/Protect/Release 得分不同）是否有细微差异（同一碎片的不同解读）？若是，文本量翻倍，需要叙事团队确认。
