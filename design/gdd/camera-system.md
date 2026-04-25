# Camera System

> **Status**: Designed
> **Author**: User + Claude Code (gameplay-programmer)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Core — the player's eye in the world

## Overview

摄像机系统管理盖亚编年史中玩家的视角。采用经典第三人称弹簧臂（Spring Arm）跟随摄像机，配合场景感知的避障和遗迹探索时的特殊镜头行为（狭窄通道自动拉近、检视物件时轻微推近）。碎片过渡期间配合视觉过渡系统使用特殊镜头状态。不使用锁定瞄准（无战斗），摄像机完全服务于叙事探索体验。

## Player Fantasy

摄像机是玻璃——玩家应该感觉不到它的存在。它跟着盖亚，但不粘着她；它在狭窄空间里自动调整，但不让人察觉到"哦它在调整"。唯一允许玩家感知到摄像机的时刻是检视重要物件时那个轻微的推近，那是游戏在说：**看这里，这很重要**。

## Detailed Design

### Core Rules

**默认跟随状态：**
- 弹簧臂长度：350 cm
- 俯仰角范围：-20°（仰视）to +60°（俯视）
- 水平旋转：360°（无限制）
- 碰撞检测：开启，碰到墙时弹簧臂缩短（最短 80 cm），恢复时平滑插值回原长

**特殊镜头状态：**

| 状态 | 触发条件 | 臂长 | 镜头变化 |
|------|----------|------|----------|
| `Default` | 正常探索 | 350 cm | 标准跟随 |
| `Narrow` | 通道宽度 < 200 cm | 自动缩至 180 cm | 过肩感增强 |
| `Examine` | 进入检视（长按交互键） | 推近至 280 cm | 轻微前推 + 微小 FOV 收窄（75°→65°） |
| `Fragment` | 碎片进行中 | 由碎片关卡配置 | 可设置固定摄像机或自由跟随 |
| `Cinematic` | 过场触发 | 由 Level Sequence 接管 | 完全由 Sequencer 控制 |

**Examine 推近：**
- 触发：交互系统进入 `Interacting` 状态且目标类型为 `AMemoryArtifact`
- 推近过渡时长：0.4 秒（Ease Out）
- 退出：交互结束后 0.6 秒平滑返回 Default 臂长

### States and Transitions

| 状态 | 转换 |
|------|------|
| `Default` → `Narrow` | 场景检测到狭窄通道 |
| `Default` → `Examine` | 交互系统通知文物交互开始 |
| `Default/Narrow/Examine` → `Fragment` | 碎片系统接管 |
| `Fragment` → `Default` | 碎片结束 |
| 任意 → `Cinematic` | Level Sequence 启动（优先级最高） |

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 角色控制器 | 挂载 | 弹簧臂附加于盖亚 `CameraAttachPoint` |
| 探索/交互系统 | 监听 | 检视开始/结束通知 → 切换 Examine 状态 |
| 记忆碎片系统 | 被控制 | `SetCameraState(Fragment)` |

## Formulas

### 公式 1：弹簧臂避障缩短

`ArmLength = Max(MinArmLength, TargetArmLength - ObstacleProtrusion)`

| 变量 | 类型 | 范围 | 说明 |
|------|------|------|------|
| `TargetArmLength` | float (cm) | 80–350 | 当前状态目标臂长 |
| `ObstacleProtrusion` | float (cm) | 0–350 | 臂到障碍的侵入量 |
| `MinArmLength` | float (cm) | 80 | 臂最短不低于此值（防止穿模） |

## Edge Cases

- **如果弹簧臂被夹在两面墙之间**：使用最短臂长（80 cm），优先保证不穿模，允许镜头贴近角色背部。
- **如果 `Cinematic` 状态与 `Fragment` 同时触发**：`Cinematic` 优先，Level Sequence 接管。
- **如果 `Examine` 状态中玩家快速移动离开文物**：立即中断推近，0.3 秒内平滑返回 `Default`。

## Dependencies

**上游依赖：** 角色控制器（挂载点）

**下游依赖：** 无

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| 默认臂长 | 350 cm | 250–450 | 角色在画面中的大小 |
| 最短臂长 | 80 cm | 60–120 | 贴墙时的最近距离 |
| 默认 FOV | 75° | 65°–90° | 视野宽度，影响空间感 |
| Examine FOV | 65° | 55°–75° | 检视时的轻微推近感 |
| 垂直俯仰上限 | +60° | +45°–+75° | 俯视角范围 |

## Visual/Audio Requirements

无独立视觉/音频资产需求。摄像机运动本身不产生音效。

## UI Requirements

无。

## Acceptance Criteria

- **GIVEN** 盖亚站在开阔场景中，**WHEN** 玩家移动鼠标，**THEN** 摄像机围绕盖亚旋转，臂长维持 350 cm。
- **GIVEN** 弹簧臂与墙壁碰撞，**THEN** 臂长缩短至不穿模位置（最短 80 cm），恢复时平滑插值。
- **GIVEN** 交互系统通知检视开始，**THEN** 0.4 秒内臂长平滑推近至 280 cm，FOV 收至 65°。
- **GIVEN** 检视结束，**THEN** 0.6 秒内臂长和 FOV 平滑返回 Default 值。
- **GIVEN** 场景通道宽度 < 200 cm，**THEN** 臂长自动缩至 180 cm，过肩感增强。

## Open Questions

无。摄像机系统设计直接，无待决问题。
