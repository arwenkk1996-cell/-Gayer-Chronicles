# Character Controller

> **Status**: Designed
> **Author**: User + Claude Code (gameplay-programmer, ue-blueprint-specialist)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Foundation — Gaia's physical presence in the world

## Overview

角色控制器定义盖亚在世界中的移动方式和物理存在感。这是一个叙事探索游戏，没有战斗，移动设计的核心目标是**流畅、有质感、不成为焦点**——玩家应该感觉到盖亚是一个有重量的生命体，但永远不应该为移动本身感到挫败。系统基于 UE5 `ACharacter` + `UCharacterMovementComponent`，通过 Enhanced Input System 处理输入，支持行走、奔跑、轻微攀爬（遗迹矮墙）和互动位移（慢走查看物件）。

## Player Fantasy

盖亚走路有自己的节奏——不是游戏里那种"没有重力的滑行"，而是真实的步伐重量。奔跑时长毛飘起来，急停时身体有个惯性的弯曲。她爬上废墟矮墙时会用手先撑一下。这些细节不应该让玩家觉得"哦操作好流畅"，而应该让他们觉得"这个角色是真实的"，然后忘记它。

## Detailed Design

### Core Rules

**移动模式：**

| 模式 | 触发条件 | 移速 | 说明 |
|------|----------|------|------|
| `Walk` | 默认 / 轻推摇杆 | 280 cm/s | 日常探索，可触发检视 |
| `Run` | 长按 Shift / 满推摇杆 | 520 cm/s | 快速移动，不可同时检视 |
| `Examine Walk` | 进入检视目标附近 | 160 cm/s | 自动降速，镜头轻微前推 |
| `Vault` | 面向矮墙（< 120cm）+ 移动输入 | — | 自动翻越，动画驱动位移 |
| `Crouch` | 无（本游戏不需要潜行） | — | 不实现 |

**输入映射（Enhanced Input）：**
- `IA_Move`：WASD / 左摇杆 → `AddMovementInput`
- `IA_Look`：鼠标移动 / 右摇杆 → `AddControllerYawInput` / `AddControllerPitchInput`
- `IA_Run`：Shift 持续 / 摇杆满推自动 → 切换 `Run` 模式
- `IA_Interact`：F / 手柄 A → 传递给交互系统

**角色朝向：**
- 移动时角色朝向与移动方向对齐（`bOrientRotationToMovement = true`）
- 摄像机独立旋转，不绑定角色朝向（第三人称标准）

**Vault（轻翻越）：**
- 条件：前方 60cm 内存在高度 40–120cm 的静态障碍，且上方空间 > 180cm
- 触发：移动输入持续 + 贴近障碍时自动触发，无需额外按键
- 实现：Montage 动画 + Root Motion 位移，过程中禁用移动输入（0.8 秒）

### States and Transitions

| 状态 | 转换条件 |
|------|----------|
| `Walk` | 有移动输入 |
| `Run` | Walk 中 + Shift/满推 |
| `Idle` | 无移动输入 |
| `Vaulting` | 贴近可翻越障碍 + 移动输入（0.8s 后自动回 Walk/Idle） |
| `Interacting` | 交互系统接管（0.3–2.0s，动画驱动） |
| `InFragment` | 记忆碎片系统接管，本控制器挂起 |

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 探索/交互系统 | 提供数据 | `GetActorForwardVector()`, `GetActorLocation()` — 焦点检测用 |
| 记忆碎片系统 | 被接管 | `PlayerController->Possess(FragmentCharacter)` 时本控制器挂起 |
| 摄像机系统 | 提供 Anchor | 弹簧臂挂载在盖亚 `CameraAttachPoint` 组件上 |
| 音频系统 | 驱动 | 步伐音效由移动状态和地表材质共同触发（Footstep Notify） |

## Formulas

### 公式 1：移速插值（加速/减速）

`CurrentSpeed = Lerp(CurrentSpeed, TargetSpeed, AccelRate × DeltaTime)`

| 变量 | 类型 | 范围 | 说明 |
|------|------|------|------|
| `TargetSpeed` | float (cm/s) | 0 / 280 / 520 / 160 | 当前模式目标速度 |
| `AccelRate` | float | 8.0–12.0 | 加速率；越高越响应，越低越柔和 |
| `CurrentSpeed` | float (cm/s) | 0–520 | 实际速度 |

**示例：** 从 Walk(280) 切换到 Run(520)，AccelRate=10，0.1s 后速度约 304 cm/s（感觉有明显加速过程）。

### 公式 2：Vault 可行性检测

`CanVault = ObstacleHeight >= 40cm AND ObstacleHeight <= 120cm AND ClearanceAbove >= 180cm AND ForwardClear(60cm)`

## Edge Cases

- **如果 Vault 动画播放中玩家松开移动键**：Root Motion 继续执行完毕，不可中断（0.8s 内）。
- **如果 InFragment 状态中角色控制器收到移动输入**：全部忽略（控制器已转移至碎片角色）。
- **如果同时满足 Vault 条件和交互条件（障碍旁有可交互物）**：交互优先，Vault 不触发。
- **如果移动到场景边界碰撞墙**：`UCharacterMovementComponent` 默认行为（滑步），无特殊处理。

## Dependencies

**上游依赖：** 无（Foundation 层）

**下游依赖（依赖本系统的）：** 探索/交互系统、摄像机系统、记忆碎片系统、音频系统。

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| Walk 速度 | 280 cm/s | 220–350 | 探索节奏感 |
| Run 速度 | 520 cm/s | 420–650 | 奔跑的爽快感 |
| 加速率 AccelRate | 10.0 | 6–16 | 移动响应性 vs 惯性质感 |
| Vault 最大高度 | 120 cm | 80–150 | 翻越能力范围 |
| 鼠标灵敏度默认值 | 0.8 | 设置菜单可调 | — |

## Visual/Audio Requirements

- Idle / Walk / Run 各一套 AnimBlendSpace（由动画蓝图处理）
- Vault Montage：翻越矮墙动画，含 Root Motion
- 步伐音效通过 Animation Notify 触发，地表材质标签决定音效变体（石头/木头/沙土/草地）
- 奔跑时发丝/毛发物理模拟（UE5 Chaos Hair 或 Groom 组件，性能待验证）

## UI Requirements

无直接 UI 需求。移速状态不显示于 HUD。

## Acceptance Criteria

- **GIVEN** 玩家按下 W 键，**WHEN** 角色从 Idle 状态，**THEN** 角色以 Walk 速度朝摄像机前方移动，AnimBlendSpace 播放行走动画。
- **GIVEN** 玩家长按 Shift + W，**THEN** 速度在 AccelRate 约束下插值到 Run(520 cm/s)。
- **GIVEN** 玩家面向 80cm 高障碍并持续移动，**WHEN** 距离 < 60cm，**THEN** Vault Montage 触发，Root Motion 完成翻越，0.8s 内不接受移动输入。
- **GIVEN** 记忆碎片系统接管控制器，**WHEN** 玩家输入移动，**THEN** 盖亚不移动，碎片角色接收输入。

## Open Questions

- **OQ-01**：Groom（毛发模拟）的性能开销在 UE5.7 中是否可接受？需要在 PC 低配设置下测试。若开销过高，回退到静态毛发 mesh。
