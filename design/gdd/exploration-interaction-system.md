# Exploration / Interaction System

> **Status**: Designed
> **Author**: User + Claude Code (game-designer, ue-blueprint-specialist)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Core — the player's action language in the world

## Overview

探索/交互系统定义了盖亚与世界互动的所有方式。它管理玩家在遗迹中的探索行为（移动至物件附近、检视环境细节）以及具体的交互触发（拾取文物、进入记忆碎片触发区、与 NPC 对话、激活谜题）。系统以 `UInteractionComponent` 挂载于盖亚的角色上，通过近身范围检测识别可交互物，向叙事状态机查询该交互是否当前有效，并在确认后触发对应系统（碎片系统、对话系统等）。玩家不需要精确瞄准——走近即可。

## Player Fantasy

探索的感觉应该是**好奇心得到回应**：盖亚走近一块刻有奇怪符文的石头，世界轻轻提示"这里有什么"，她伸手触碰，一切就此不同。交互不应该有摩擦感——没有复杂的按键组合，没有需要对准的准心，没有打破沉浸感的 UI 弹窗。是那种"我想看看这个"然后直接就看了的流畅感。

## Detailed Design

### Core Rules

1. 盖亚的角色组件 `UInteractionComponent` 每帧维护一个**当前最近可交互物**列表，通过球形范围检测（Sphere Overlap，半径 `InteractionRadius`，默认 180cm）识别实现了 `IInteractable` 接口的 Actor。

2. 列表中优先级最高的可交互物为**焦点目标**（Focus Target）。优先级规则：
   - 距盖亚正前方角度最小者优先（朝向权重）
   - 距离相同时，叙事状态机标记为 `Available` 的优先于普通可交互物

3. 玩家按下交互键（默认 `F` / 手柄 `A`）时：
   - 若焦点目标存在且 `HasFlag(Target.Available)` 为 true → 触发该目标的 `OnInteract(AGaiaCharacter*)`
   - 若目标存在但标记为不可用 → 播放"感知到但无法交互"的视觉反馈（轻微发光脉冲），不触发交互
   - 若无焦点目标 → 无反应

4. 所有可交互物实现 `IInteractable` 接口，提供：
   - `GetInteractPriority() → int32`：覆盖默认优先级
   - `GetInteractHintText() → FText`：交互提示文本（显示于 HUD）
   - `OnInteract(AGaiaCharacter* Gaia)`：交互执行逻辑
   - `OnFocusBegin() / OnFocusEnd()`：进入/离开焦点时的视觉反馈回调

5. 检视（Examine）与交互（Interact）是两种不同动作：
   - **检视**：长按交互键（0.5s）→ 触发 `OnExamine()`，显示盖亚的内心独白文本（不改变叙事状态）
   - **交互**：短按交互键 → 触发 `OnInteract()`，可能改变叙事状态

### States and Transitions

| 状态 | 说明 | 转换条件 |
|------|------|----------|
| `Idle` | 无焦点目标 | 进入 InteractionRadius 的 IInteractable 存在时 → `Focused` |
| `Focused` | 有焦点目标，显示交互提示 | 目标离开范围 → `Idle`；按下交互键 → `Interacting` |
| `Interacting` | 交互进行中（动画/过渡帧） | 交互完成 → `Idle` 或 `Focused`（取决于目标是否仍在范围） |
| `Locked` | 强制禁用交互（过场/碎片进行中） | `OnFragmentEnd` / `OnCinematicEnd` 事件 → 恢复 `Idle` |

`Locked` 状态由记忆碎片系统和过场系统写入，防止玩家在碎片场景内再次触发交互。

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 叙事状态机 | 读 | `HasFlag(Target.Available)` — 验证交互有效性 |
| 记忆碎片系统 | 触发 | `OnInteract()` → 碎片系统接管 |
| 对话系统 | 触发 | `OnInteract()` → 对话系统开启对话树 |
| 谜题系统 | 触发 | `OnInteract()` → 谜题系统激活 |
| 文物系统 | 触发 | `OnInteract()` + `OnExamine()` → 文物系统记录收集/检视 |
| HUD/UI 系统 | 推送 | 焦点目标变化时推送 `FInteractionHintData`，HUD 据此显示/隐藏提示 |
| 角色控制器 | 依赖 | 依赖角色位置与朝向（未设计，暂定接口） |

## Formulas

### 公式 1：焦点优先级评分

`FocusScore = (1 - NormalizedAngle) * 0.6 + (1 - NormalizedDistance) * 0.4`

| 变量 | 符号 | 类型 | 范围 | 说明 |
|------|------|------|------|------|
| 归一化朝向角 | `NormalizedAngle` | float | 0.0–1.0 | 目标与盖亚正前方夹角 / 180° |
| 归一化距离 | `NormalizedDistance` | float | 0.0–1.0 | 目标距离 / InteractionRadius |
| 输出 | `FocusScore` | float | 0.0–1.0 | 分值越高越优先成为焦点 |

**输出范围：** 0.0（身后最远处）到 1.0（正前方最近处）  
**示例：** 目标在正前方 90cm（InteractionRadius=180cm）→ NormalizedAngle=0, NormalizedDistance=0.5 → FocusScore = 0.6 + 0.2 = 0.8

### 公式 2：检视判定（长按阈值）

`IsExamine = HoldDuration >= ExamineThreshold`

| 变量 | 符号 | 类型 | 范围 | 说明 |
|------|------|------|------|------|
| 按键持续时间 | `HoldDuration` | float (秒) | 0.0–∞ | 玩家持续按住交互键的时长 |
| 检视阈值 | `ExamineThreshold` | float (秒) | 0.3–1.0 | 默认 0.5s，低于此值为短按触发交互 |

## Edge Cases

- **如果多个可交互物得分相同**：选择 `GetInteractPriority()` 返回值更高的；仍相同则选最先进入范围的（稳定排序）。

- **如果玩家在交互动画播放中途走出范围**：动画继续播完，交互正常完成。交互一旦触发即不可中断。

- **如果焦点目标在交互过程中被叙事状态机标记为不可用**：当前交互继续完成；下次进入焦点时才应用新状态。

- **如果 `IInteractable` Actor 在焦点持有期间被销毁**：`UInteractionComponent` 在下一帧检测时自动清除焦点，恢复 `Idle` 状态，不崩溃。

- **如果系统处于 `Locked` 状态时玩家按下交互键**：完全无响应，不播放任何反馈音效（碎片场景中不应有任何游戏 UI 提示）。

- **如果 `InteractionRadius` 内同时存在 50+ 个 IInteractable**：仅对前 8 个（按粗距离排序）计算精确 FocusScore，其余忽略。遗迹场景设计规范：同时可见的可交互物不超过 5 个。

## Dependencies

**上游依赖：**
| 系统 | 性质 | 说明 |
|------|------|------|
| 叙事状态机 | 硬依赖 | 交互有效性验证 |
| 角色控制器 | 硬依赖 | 位置、朝向数据（接口暂定，待角色控制器 GDD） |

**下游依赖（依赖本系统的）：**
记忆碎片系统、对话系统、谜题系统、文物系统、HUD/UI 系统 — 均通过 `IInteractable::OnInteract()` 接入。

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| `InteractionRadius` | 180 cm | 120–250 cm | 玩家需要多近才能触发；过大显得不真实，过小操作摩擦感高 |
| `ExamineThreshold` | 0.5 s | 0.3–1.0 s | 长按检视的灵敏度；过短容易误触，过长操作迟钝 |
| 焦点朝向权重 | 0.6 | 0.4–0.8 | 朝向 vs 距离对焦点选择的影响比；偏向朝向让玩家感觉"看哪里交互哪里" |
| 焦点距离权重 | 0.4 | 0.2–0.6 | 同上（两者之和须为 1.0） |
| 最大精确评分数量 | 8 | 4–16 | 性能 vs 精度；遗迹场景密度低，默认 8 已充裕 |

## Visual/Audio Requirements

- **焦点进入**：目标物件轻微边缘发光（Outline Post-Process，颜色待 Art Bible 确定），无音效
- **焦点离开**：发光消失，无音效
- **交互触发**：短促触觉反馈音（手柄震动 + 轻点击音）；具体音效由音频系统设计
- **不可用状态触碰**：发光脉冲一次（暗色调），配合轻微否定音效
- **检视触发**：盖亚轻微转头/倾身动画（由角色控制器动画层实现），配合翻阅纸张质感音效

📌 **Asset Spec** — 视觉反馈需求已定义。Art Bible 完成后运行 `/asset-spec system:exploration-interaction-system`。

## UI Requirements

- **交互提示**：焦点目标存在时，HUD 显示浮动提示（目标名称 + 交互键图标）；目标不可用时提示变灰
- **检视文本**：长按触发时，全屏下方显示盖亚内心独白文本条（淡入淡出）
- 具体 UI 样式待 `/ux-design` 阶段定义

📌 **UX Flag — Exploration/Interaction System**：交互提示和检视文本有 UI 需求，Pre-Production 阶段运行 `/ux-design` 为其创建 UX spec。

## Acceptance Criteria

- **GIVEN** 盖亚站在 IInteractable 物件 150cm 处正前方，**WHEN** 系统运行，**THEN** 该物件成为焦点目标，HUD 显示交互提示。

- **GIVEN** 焦点目标的 `Fragment.X.Available` 标记已设置，**WHEN** 玩家短按交互键，**THEN** `OnInteract()` 被调用，提示隐藏，系统进入 `Interacting` 状态。

- **GIVEN** 焦点目标标记不可用，**WHEN** 玩家短按交互键，**THEN** 目标播放脉冲发光，`OnInteract()` 不被调用。

- **GIVEN** 系统处于 `Locked` 状态，**WHEN** 玩家按下交互键，**THEN** 无任何响应，无音效，无 UI 变化。

- **GIVEN** 玩家对焦点目标按住交互键 0.5s 以上，**WHEN** 松开，**THEN** `OnExamine()` 被调用而非 `OnInteract()`（不改变叙事状态）。

- **GIVEN** 半径内有两个 IInteractable，A 在正前方 160cm，B 在侧方 60cm，**WHEN** 系统评分，**THEN** A 的 FocusScore 高于 B，A 成为焦点（朝向权重 0.6 > 距离权重 0.4）。

- **GIVEN** 焦点目标在交互动画中被销毁，**WHEN** 下一帧检测，**THEN** 系统恢复 `Idle`，不崩溃。

## Open Questions

- **OQ-01**：角色控制器 GDD 完成后，确认 `UInteractionComponent` 获取盖亚朝向的具体接口（当前暂定通过 `GetActorForwardVector()`）。
- **OQ-02**：检视文本（盖亚内心独白）是否需要语音配音？若是，需提前通知音频系统规划 VO 资产量。
