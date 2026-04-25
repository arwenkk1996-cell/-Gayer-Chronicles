# Visual Transition System

> **Status**: Designed
> **Author**: User + Claude Code (unreal-specialist, technical-artist)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Core — the artistic language of time-layer switching

## Overview

视觉过渡系统管理盖亚编年史中四个时代之间的美术状态切换。它接收来自记忆碎片系统的切换指令，在指定时长内通过后处理配置的插值将当前视觉风格过渡至目标时代的风格，并在碎片结束后返回现实层。系统以 UE5 Post Process Volume 混合 + Level Sequence 驱动，不持有游戏逻辑，只负责"让世界看起来在哪个时代"。

## Player Fantasy

时代的切换不应该是技术性的——它应该感觉像**记忆本身的质感**：不完整，带着岁月的磨损，颜色像被水泡过的照片。进入旧世界时，饱和度退去的方式让人想起翻到了某个不想翻到的旧相册。进入逃亡路上的黑白灰，是那种无从逃脱的压迫感。每个时代的视觉语言在说一件事：**这里发生过的事和你想象的不一样**。

## Detailed Design

### Core Rules

**四时代视觉配置：**

| 时代 | 枚举值 | 色调方案 | 后处理关键参数 |
|------|--------|----------|----------------|
| 现在（盖亚层） | `ETimeLayer::Present` | 全彩，暖橙+青绿，Lumen 全开 | Saturation=1.0, Contrast=1.05, Temperature=6500K, Bloom=0.8 |
| 旧世界末期 | `ETimeLayer::OldWorld` | 褪色暖褐，类旧照片 | Saturation=0.45, Contrast=1.2, Temperature=4200K, Vignette=0.5, FilmGrain=0.3 |
| 逃亡之路 | `ETimeLayer::Exodus` | 高对比灰蓝，灰烬质感 | Saturation=0.15, Contrast=1.45, Temperature=7800K, Vignette=0.7, FilmGrain=0.5 |
| 建国初代 | `ETimeLayer::Founding` | 手工感暖黄，粗糙纹理感 | Saturation=0.7, Contrast=0.95, Temperature=5000K, VignetteRounded=0.3 |

**切换流程：**

1. `BeginTransitionTo(ETimeLayer Target, float Duration)` 被调用
2. 系统找到当前激活的 Post Process Volume（`PPV_GaiaPresent`）和目标配置（`PPV_[Target]`）
3. 在 `Duration` 秒内通过 `APostProcessVolume::BlendWeight` 插值（Ease In-Out 曲线）
4. 过渡中途播放 Level Sequence 中的附加效果（屏幕空间扰动、短暂过曝等）
5. 过渡完成时广播 `FOnTransitionComplete(ETimeLayer)`

**双向过渡均通过此接口：**
- 进入碎片：`BeginTransitionTo(FragmentTimeLayer, 2.5s)`
- 离开碎片：`BeginTransitionTo(Present, 2.0s)`

### States and Transitions

| 状态 | 说明 |
|------|------|
| `Stable(Layer)` | 当前稳定显示某时代，无过渡进行 |
| `Transitioning(From, To)` | 过渡动画进行中，不接受新的切换请求 |

过渡进行中若收到新的 `BeginTransitionTo()` 调用，静默排队（队列最多 1 个），当前过渡完成后执行。

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 记忆碎片系统 | 被控制 | `BeginTransitionTo(ETimeLayer, float Duration)` |
| 音频系统 | 通知 | `FOnTransitionComplete` — 音频系统监听以切换时代音乐 |
| 叙事状态机 | 无直接接口 | 由记忆碎片系统中转 |

## Formulas

### 公式 1：BlendWeight 插值

`BlendWeight(t) = SmoothStep(0, 1, t / Duration)`

`SmoothStep(a, b, x) = clamp((x-a)/(b-a), 0, 1)² × (3 - 2×clamp(...))`

| 变量 | 类型 | 范围 | 说明 |
|------|------|------|------|
| `t` | float (秒) | 0–Duration | 当前已过渡时间 |
| `Duration` | float (秒) | 1.0–4.0 | 过渡总时长 |
| `BlendWeight` | float | 0.0–1.0 | 目标 PPV 的混合权重 |

**输出范围：** 0（源时代）→ 1（目标时代），Ease In-Out 曲线  
**示例：** Duration=2.5s，t=1.25s → BlendWeight=0.5（半程）

## Edge Cases

- **如果过渡进行中收到新切换请求**：排队，当前完成后执行。队列溢出（>1）时替换队列中的待执行项。
- **如果 Duration 传入 0**：立即切换，不播放动画（用于游戏开始时的初始化）。
- **如果目标 Layer 与当前 Layer 相同**：静默忽略，不触发任何动画。
- **如果 Level Sequence 资源缺失**：跳过附加效果，BlendWeight 插值正常执行，后处理切换不受影响。

## Dependencies

**上游依赖：**
- 记忆碎片系统（控制方）
- UE5 Post Process Volume + Level Sequence（引擎原生，非游戏系统）

**下游依赖：**
- 音频系统（监听 `FOnTransitionComplete` 切换音乐）

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| 进入过渡时长 | 2.5 秒 | 1.5–4.0 秒 | 仪式感 vs 节奏 |
| 返回过渡时长 | 2.0 秒 | 1.0–3.0 秒 | 略短于进入，强化突然性 |
| 各时代 Saturation | 见上表 | ±0.1 | 时代视觉辨识度 |
| FilmGrain 强度 | 见上表 | 0.0–0.8 | 历史感质感，过强影响辨识度 |

## Visual/Audio Requirements

详见 Detailed Design 四时代色调表。各时代的完整 Art Bible 规范在 `/art-bible` 阶段定义。

## UI Requirements

过渡进行中 HUD 淡出（透明度 0），过渡完成后 HUD 淡入。具体淡出时机：`BeginTransitionTo()` 调用后 0.2 秒开始，完成于过渡动画中点。

## Acceptance Criteria

- **GIVEN** 系统处于 `Stable(Present)`，**WHEN** `BeginTransitionTo(OldWorld, 2.5)` 调用，**THEN** `Transitioning(Present→OldWorld)` 开始，2.5 秒后 `Stable(OldWorld)` 且 `FOnTransitionComplete` 广播。
- **GIVEN** 系统处于 `Transitioning` 状态，**WHEN** 新的 `BeginTransitionTo()` 调用，**THEN** 当前过渡完成后执行新请求，不中断当前动画。
- **GIVEN** Duration=0，**WHEN** `BeginTransitionTo(Exodus, 0)` 调用，**THEN** 立即切换到 Exodus 视觉，无动画，`FOnTransitionComplete` 立即广播。
- **GIVEN** 目标与当前相同（`BeginTransitionTo(Present)` 时已在 Present），**THEN** 无响应，无事件广播。

## Open Questions

- **OQ-01**：Substrate Material System（UE5.7 新特性）是否影响各时代的材质配置？需在 Art Bible + 架构阶段验证。
- **OQ-02**：建国初代的"手工感"是否通过 FilmGrain 模拟，还是需要专用 Post Process 材质？待 Art Bible 阶段确认。
