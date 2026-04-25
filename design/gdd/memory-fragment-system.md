# Memory Fragment System

> **Status**: Designed
> **Author**: User + Claude Code (narrative-director, game-designer, unreal-specialist)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Core differentiator — the mechanic that makes this game unique

## Overview

记忆碎片系统是盖亚编年史的核心机制。当盖亚触碰特定文物时，系统将玩家传送至另一个时代的可玩片段——一段短暂的、以不同角色视角体验的场景，完成后返回现实，遗迹因此呈现新的面貌。系统管理碎片的触发流程（进入/退出）、场景加载与卸载、片段内的角色接管、完成结果写回叙事状态机，以及返回时世界状态的应用。它与视觉过渡系统深度协作完成时代切换的美术表现。

## Player Fantasy

触碰那块石头的瞬间，世界熄灭了——不是黑屏，是**溶解**。颜色从现在的饱满慢慢退成旧照片的色调，盖亚的轮廓消失，另一双手出现在视野里。这双手属于另一个时代的某个人，她们不知道盖亚会来，不知道自己在被人注视。

玩家在这短暂的片段里不是旁观者。她们**就是**那个时代的人，做着那个时代的事——直到场景结束，颜色再次涌回，盖亚重新出现，脚下还是那块石头。但现在，这块石头不一样了。

这种体验的核心情感是**认知的移位**：同一个地方，在不同时代看，意义完全不同。玩家每次返回现实，都带着一个新的视角重新看待眼前的一切。

## Detailed Design

### Core Rules

**触发流程：**

1. 探索/交互系统调用焦点文物的 `OnInteract()`，文物实现为 `AMemoryArtifact`（`IInteractable` 的一个子类）
2. `AMemoryArtifact` 调用 `UMemoryFragmentSubsystem::BeginFragment(FragmentTag)`
3. 系统检查前置条件（通过叙事状态机 `HasFlag(Fragment.X.Available)` — 交互系统已预验证，此处为双重保险）
4. 通知探索/交互系统进入 `Locked` 状态
5. 触发视觉过渡系统开始进入动画（`BeginTransitionTo(TimeLayer)`）
6. 异步加载目标时代的碎片关卡（UE5 Level Streaming，`LoadLevelInstanceByReference()`）
7. 过渡动画完成时，将玩家控制器 Possess 到碎片角色（`AFragmentCharacter`）
8. 碎片场景开始，玩家操控碎片角色完成该时代的任务

**完成流程：**

9. 碎片场景完成条件满足（由碎片关卡内的 `UFragmentCompletionComponent` 检测）
10. `UMemoryFragmentSubsystem::CompleteFragment(FragmentTag, FFragmentResult)` 被调用
11. 将 `FFragmentResult`（选择记录、解锁内容）写入叙事状态机：`SetFlag(Fragment.X.Complete)` + 结局得分增量
12. 视觉过渡系统开始返回动画（`BeginTransitionTo(PresentDay)`）
13. 卸载碎片关卡，玩家控制器重新 Possess 盖亚
14. 探索/交互系统解除 `Locked`
15. 应用世界状态变化（环境变化由 `UWorldStateApplicator` 读取叙事状态机后执行）

**碎片内规则：**

- 碎片角色（`AFragmentCharacter`）使用与盖亚相同的移动和交互接口，但不使用盖亚的叙事状态机写入权限——碎片内的交互由碎片专属逻辑管理，完成后统一通过 `FFragmentResult` 写回
- 碎片时长设计上限：10 分钟（超过此时长的碎片拆分为两段）
- 碎片内不可存档（处于 `InProgress` 状态）

### States and Transitions

| 状态 | 说明 | 转换条件 |
|------|------|----------|
| `Idle` | 无碎片进行中 | `BeginFragment()` 调用 → `Transitioning_In` |
| `Transitioning_In` | 进入动画播放中，关卡异步加载中 | 动画完成 + 关卡加载完成 → `InFragment` |
| `InFragment` | 玩家操控碎片角色 | `CompleteFragment()` 调用 → `Transitioning_Out` |
| `Transitioning_Out` | 返回动画播放中，结果写回中 | 动画完成 + 卸载完成 → `Idle` |
| `Aborted` | 异常中断（关卡加载失败等） | 任何阶段的错误 → `Idle`（回滚，不写入状态） |

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 探索/交互系统 | 被调用 + 控制 | 接收 `OnInteract()` 触发；调用 `SetLocked(true/false)` |
| 叙事状态机 | 读+写 | `HasFlag`（前置验证）/ `SetFlag` + `IncrementInt`（完成写入） |
| 视觉过渡系统 | 控制 | `BeginTransitionTo(ETimeLayer)` — 驱动美术层切换 |
| 角色控制器 | 控制 | `PlayerController->Possess(FragmentCharacter / GaiaCharacter)` |
| 存档系统 | 通知 | 碎片进行中时阻止存档请求 |

## Formulas

### 公式 1：碎片前置验证

`CanBeginFragment = HasFlag(Fragment.X.Available) AND SystemState == Idle`

| 变量 | 类型 | 说明 |
|------|------|------|
| `HasFlag(Fragment.X.Available)` | bool | 叙事状态机已标记该碎片前置满足 |
| `SystemState` | enum | 当前系统状态；非 Idle 时不可触发新碎片 |
| 输出 | bool | false 时触发被静默拒绝（已由交互系统的不可用反馈处理） |

### 公式 2：世界状态应用优先级

碎片完成后，同一场景可能有多条世界状态变化规则同时触发（多个碎片先后完成）。执行顺序：

`ApplicationOrder = SortBy(Fragment.CompletionTimestamp, ASC)`

先完成的碎片对应的世界变化先应用，后完成的覆盖冲突项，确保"最新发现"具有最高展示优先级。

### 公式 3：时间层视觉权重（委托给视觉过渡系统 GDD 定义，此处仅声明接口）

碎片触发时传入 `ETimeLayer` 枚举，视觉过渡系统负责将其映射至具体的后处理配置。

## Edge Cases

- **如果碎片关卡异步加载失败（超时或资源缺失）**：系统进入 `Aborted` 状态，回滚到 `Idle`，叙事状态机不写入任何内容，视觉过渡动画反向播放回现实，HUD 显示"连接失败，请重试"（编辑器下额外打印错误日志）。

- **如果玩家在 `Transitioning_In` 阶段强制关闭游戏（崩溃/强退）**：读档后碎片状态为 `Available`（InProgress 不持久化），玩家回到触发前状态，重新触发即可。

- **如果同一帧内两个 AMemoryArtifact 同时调用 `BeginFragment()`**：第二个调用被静默拒绝（`SystemState != Idle`）。

- **如果碎片场景内玩家找到出口或强行离开触发区**：碎片关卡不设置物理出口。场景边界由不可见碰撞墙围合，尝试离开时盖亚轻微颤抖并内心独白"我还没看完"，阻止离开。

- **如果碎片完成条件在玩家到达之前就已满足（例如自动触发的剧情事件）**：直接调用 `CompleteFragment()`，跳过玩家操作，正常走完成流程。这适用于纯叙事型碎片（无互动任务）。

- **如果碎片内存档被请求**：`UMemoryFragmentSubsystem` 在 `InFragment` 状态下拦截存档请求，返回"无法在记忆中存档"提示，不执行存档。

## Dependencies

**上游依赖：**
| 系统 | 性质 | 说明 |
|------|------|------|
| 探索/交互系统 | 硬依赖 | 触发入口 |
| 叙事状态机 | 硬依赖 | 前置验证 + 结果写回 |
| 视觉过渡系统 | 硬依赖 | 时代切换的美术表现 |
| 角色控制器 | 硬依赖 | Possess/UnPossess 接口 |

**下游依赖（依赖本系统的）：**
谜题系统（碎片内的谜题由本系统管理执行上下文）、日志系统（监听 `Fragment.Complete` 事件）、世界地图系统（碎片完成驱动区域进度更新）。

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| 碎片最大时长（设计规范） | 10 分钟 | 3–15 分钟 | 叙事节奏与玩家专注度；超过 10 分钟需拆分为两段碎片 |
| 进入过渡动画时长 | 2.5 秒 | 1.5–4.0 秒 | 时代切换的仪式感；过短失去沉浸感，过长节奏拖沓 |
| 返回过渡动画时长 | 2.0 秒 | 1.0–3.0 秒 | 返回现实的"落地感"；略短于进入，强调返回的突然性 |
| 世界状态变化延迟 | 0.5 秒（过渡动画结束后） | 0–1.5 秒 | 让玩家先看到原始场景，再看到变化，强化对比感 |

## Visual/Audio Requirements

- **进入过渡**：颜色饱和度渐降至目标时代色调，轻微模糊扩散，最终帧短暂完全白/黑（由目标时代决定）→ 淡入碎片场景。音效：低频共鸣渐起，现实声消失，碎片时代的环境声淡入。
- **返回过渡**：碎片场景淡出，现实色调以略快于进入的速度恢复，世界状态变化在恢复完成后 0.5s 应用（玩家先看到原始场景再看到变化）。音效：碎片声消失，现实环境声重新涌入，略带混响余音。
- **碎片内视觉**：严格遵循时代色调方案（详见 Art Bible — 四时代色彩方案章节）。
- **碎片角色**：不同时代的碎片角色有各自的视觉风格，但操作手感与盖亚保持一致。

📌 **Asset Spec** — 过渡动画的视觉/音频需求已定义。Art Bible 完成后运行 `/asset-spec system:memory-fragment-system`。

## UI Requirements

- 碎片进行中：HUD 完全隐藏（沉浸优先），仅保留碎片完成提示（极简）
- 进入碎片前：无额外 UI，完全依赖视觉过渡表达状态切换
- 返回现实后：若有新日志条目解锁，HUD 右下角出现轻微日志图标脉冲（不打断沉浸感）

## Acceptance Criteria

- **GIVEN** `Fragment.Sourceholm.WaterGate.Available` 已设置，**WHEN** 玩家与对应文物交互，**THEN** 系统进入 `Transitioning_In`，交互系统锁定，视觉过渡开始。

- **GIVEN** 碎片关卡加载完成且过渡动画完成，**THEN** 玩家控制器 Possess 碎片角色，系统状态为 `InFragment`，HUD 隐藏。

- **GIVEN** 碎片完成条件满足，**WHEN** `CompleteFragment()` 调用，**THEN** `Fragment.Sourceholm.WaterGate.Complete` 在叙事状态机中设置，结局得分增量写入，返回过渡开始。

- **GIVEN** 返回过渡完成，**THEN** 玩家重新控制盖亚，交互系统解锁，碎片关卡已卸载，世界状态变化在 0.5s 后应用。

- **GIVEN** 系统处于 `InFragment` 状态时存档请求触发，**THEN** 存档被拒绝，HUD 显示提示，不执行存档。

- **GIVEN** 碎片关卡加载失败，**THEN** 系统回到 `Idle`，叙事状态机无写入，视觉回到现实，HUD 显示错误提示。

- **GIVEN** 系统已处于 `Transitioning_In` 状态时第二个文物触发，**THEN** 第二次调用静默失败，无额外动画或状态变化。

## Open Questions

- **OQ-01**：碎片关卡是否使用 UE5 World Partition 子关卡，还是独立 Level Instance？建议 Level Instance 方案（隔离更清晰），待架构阶段 ADR 确认。
- **OQ-02**：是否为每个时代设置独立的 `AFragmentCharacter` 蓝图类，还是一个通用类 + DataAsset 配置？推荐独立类（便于时代特有动画）。
