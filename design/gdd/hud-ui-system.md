# HUD / UI System

> **Status**: Designed
> **Author**: User + Claude Code (ue-umg-specialist, ux-designer)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Presentation — information without intrusion

## Overview

HUD/UI 系统是盖亚编年史中所有玩家界面元素的统一管理层。系统分为两层：**HUD 层**（持续叠加在游戏视口上的非打断式信息）和**全屏 UI 层**（日志界面、对话框等占据注意力的界面）。设计原则是"你感受到它的时候它已经不在了"——HUD 只在需要时出现，不占空间，不打断沉浸。所有 UI 组件以 UMG（Unreal Motion Graphics）实现，统一使用日志米纸质调色板和盖亚日记本视觉语言。

## Player Fantasy

好的界面是不被注意到的。玩家走进遗迹，视线里没有血条、没有小地图、没有漂浮标记——只有世界。当盖亚靠近某个可交互物时，一个小小的提示轻轻出现；当她解开一个谜题，右下角有什么东西闪了一下；当玩家想翻日志时，游戏让位于一本打开的日记。**界面的存在感和故事的密度成反比——叙事越深，界面越退。**

## Detailed Design

### Core Rules

**HUD 元素清单（常驻层，默认隐藏，按需显示）：**

| 元素 | 位置 | 触发条件 | 消失条件 |
|------|------|----------|----------|
| 交互提示 | 屏幕中央偏下 | 盖亚对焦可交互物 | 焦点移开或交互完成 |
| 检视文本 | 屏幕左下，竖排 | 长按触发检视动作 | 松开按键或 2 秒后自动淡出 |
| 日志解锁通知 | 屏幕右下 | `FOnFlagSet` → 新日志条目 | 1.5 秒后自动淡出 |
| 碎片进入提示 | 屏幕上方居中 | 记忆碎片过渡开始 | 过渡完成后 1 秒淡出 |
| 对话 UI | 屏幕下方 30% | `BeginDialogue()` | `EndDialogue()` |
| 盖亚独白（提示） | 屏幕左下，斜体 | 谜题提示触发 | 3 秒后自动淡出 |

**HUD 全体隐藏规则：**
- 记忆碎片过渡动画播放期间（`Transitioning_In` / `Transitioning_Out`）：全 HUD 隐藏
- 进入过场动画（Sequencer 接管）：全 HUD 隐藏
- 玩家打开日志界面时：HUD 层整体不可见（日志 UI 独占）

**交互提示设计：**
```
[交互键图标]  [提示文本]
  (E / X)     "触碰水渠石碑"
```
- 图标自动切换：键盘/鼠标模式显示按键图标，手柄模式显示手柄键位图标（监听 `APlayerController::GetLastInputDeviceType()`）
- 长按进度环：若操作为"长按检视"，在图标周围显示 0–0.5 秒弧形进度圆
- 字体：Playfair Display 类，16pt，深棕色 `#3D2B1F`

**日志解锁通知（脉冲通知）：**
- 羽毛笔图标 + 短标题，右对齐浮现
- 动画：从右侧滑入 0.2s → 停留 1.0s → 淡出 0.3s（总 1.5s）
- 多条同时解锁时按 0.3s 间隔队列显示（与日志系统解锁间隔同步）

**对话 UI 结构：**
```
┌────────────────────────────────────────────────────┐
│ 纳雅                                                │  ← 说话者标签（手写字体，左上）
│                                                    │
│  "水渠建成的那年，我三岁。我只记得母亲说，那年     │  ← 台词文本（Playfair Display, 18pt）
│   水第一次流进了南区。"                             │
│                                                    │
│  ① 那南区的人怎么说？                               │  ← 选项列表（母系青高亮悬停项）
│  ② 你母亲还在吗？                                  │
│  ③ 水渠是谁建的？                                  │
└────────────────────────────────────────────────────┘
```
- 台词框：日志米 `#F5EDD6` 底色，深棕边框，轻微纸张纹理
- 透明度：背景 85%，确保世界可见
- 选项悬停：当前选项背景变为母系青 `#4AADAA` 10% 淡填充 + 左侧竖线

### Full-Screen UI: 日志界面

参见 `design/gdd/journal-chronicle-system.md` 的 UI Requirements。

HUD/UI 系统负责：
- 响应 `J` / 手柄 `Back` 键，播放翻书进入动画（0.3s）
- `SetGamePaused(true)` 在日志打开时暂停游戏时钟
- 翻书离开动画 → `SetGamePaused(false)`

### Input Mode 管理

| 状态 | Input Mode | 鼠标可见 |
|------|------------|----------|
| 正常探索 | `GameOnly` | 否 |
| 对话进行中 | `GameAndUI` | 是（可点击选项）|
| 日志打开 | `UIOnly` | 是 |
| 过场动画 | `GameOnly`（Sequencer 控制）| 否 |

`APlayerController::SetInputMode()` + `bShowMouseCursor` 由本系统统一管理，其他系统通过 `UHUDSubsystem::RequestInputMode(EHUDInputMode)` 请求，由 HUD 系统裁决最终状态（优先级：Cinematic > UIOnly > GameAndUI > GameOnly）。

### States and Transitions

```
Exploration → [BeginDialogue] → Dialogue → [EndDialogue] → Exploration
Exploration → [OpenJournal]  → Journal  → [CloseJournal] → Exploration
Exploration → [FragmentBegin] → FragmentTransition → Fragment → FragmentTransition → Exploration
Any → [Sequencer] → Cinematic → [SequencerEnd] → (previous state)
```

HUD 层本身无状态机——它是事件驱动的 widget 集合，响应各系统推送的数据。

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 探索/交互系统 | 推送 | `FOnFocusChanged(IInteractable*)` → 显示/隐藏交互提示 |
| 对话系统 | 推送 | `FDialogueLineData`（台词+说话者+选项列表）→ 更新对话 UI |
| 日志系统 | 推送 | `FOnJournalEntryUnlocked(FJournalEntry)` → 脉冲通知队列 |
| 记忆碎片系统 | 推送 | `FOnFragmentStateChanged(EFragmentState)` → HUD 全隐或碎片提示 |
| 谜题系统 | 推送 | 盖亚独白文本 → 独白 HUD 元素 |
| 角色控制器 | 查询 | `GetLastInputDeviceType()` → 图标模式切换 |
| 过场/Sequencer | 控制 | 接管时广播 HUD 全隐事件 |

## Formulas

### 公式 1：通知队列弹出时机

`NextNotifyTime = LastNotifyTime + NotifyInterval`

| 变量 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `LastNotifyTime` | float (GameTime) | — | 上一条通知的显示时间戳 |
| `NotifyInterval` | float (秒) | 0.3 | 两条通知之间的最小间隔 |

队列为 FIFO，`Tick` 中检查 `CurrentTime >= NextNotifyTime` 时弹出下一条。

### 公式 2：Input Mode 优先级裁决

`ActiveInputMode = Max(RequestedModes, by priority)`

优先级：Cinematic(4) > UIOnly(3) > GameAndUI(2) > GameOnly(1)

多个系统同时请求不同模式时取最高优先级，最低请求解除时回退至次高仍在请求的模式。

## Edge Cases

- **如果脉冲通知队列积压超过 5 条**：超出部分丢弃最早入队的通知，保留最新 5 条（避免过渡游戏后打开游戏有大量通知堆叠）。
- **如果对话 UI 激活时收到日志解锁通知**：通知进入队列，对话结束后开始弹出（对话期间不显示其他 HUD 元素）。
- **如果玩家在对话中切换输入设备（接手柄/拔手柄）**：0.2s 后重新检测，图标平滑切换（不中断对话）。
- **如果日志界面打开时游戏失去焦点（Alt+Tab）**：`SetGamePaused` 状态不变，窗口重新获焦后 UI 状态保持。
- **如果过场动画中 Sequencer 提前结束（中断播放）**：`UHUDSubsystem` 监听 `OnLevelSequenceFinished`，立即恢复上一个 Input Mode 状态。

## Dependencies

**上游依赖：** 探索/交互系统、对话系统、日志系统、记忆碎片系统、谜题系统

**下游依赖：** 无（HUD/UI 是最终呈现层）

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| 通知显示总时长 | 1.5 秒 | 1.0–3.0 秒 | 玩家阅读时间 |
| 通知队列间隔 | 0.3 秒 | 0.1–1.0 秒 | 通知密度感 |
| 交互提示淡入时长 | 0.15 秒 | 0.1–0.3 秒 | 响应感 |
| 盖亚独白显示时长 | 3 秒 | 2–5 秒 | 提示可读性 |
| 对话框背景透明度 | 85% | 70–95% | 沉浸感 vs 可读性 |

## Visual/Audio Requirements

- 所有 UI 颜色严格遵循 Art Bible Section 4（日志米底色 `#F5EDD6`，深棕文字 `#3D2B1F`，母系青高亮 `#4AADAA`）
- 字体：标题/说话者标签使用 Playfair Display 类衬线字体；正文使用 16–18pt；选项使用 14pt
- 所有 Widget 动画时长 ≤ 0.3s（过渡感；不拖沓）
- 音效：交互提示出现时无声（静默出现）；日志通知伴随极轻微墨水滴落声（来自日志系统定义）；对话 UI 开启/关闭时页面翻动声

## UI Requirements

- HUD 层使用 `UUserWidget` + `ZOrder` 管理层级（HUD < Dialog < Journal < Cinematic overlay）
- 所有文本走 `FText` + 本地化管道（为后续 Localization System 预留）
- 交互提示、通知等短效元素使用 Widget Pool（`UUserWidgetPool`）避免频繁 GC
- 分辨率支持：1280×720 至 3840×2160（Anchors 百分比布局，无硬编码像素坐标）
- Safe Zone：支持主机 Safe Zone（`GetSafeZone()` padding 应用于所有边缘 HUD 元素）

## Acceptance Criteria

- **GIVEN** 盖亚靠近可交互物，**THEN** 0.15 秒内交互提示从透明淡入显示，显示正确的键位图标和提示文本。
- **GIVEN** 玩家使用手柄，**THEN** 交互提示显示手柄按键图标（而非键盘图标）；切换输入设备后 0.2 秒内图标更新。
- **GIVEN** 日志条目解锁，**THEN** 右下角羽毛笔通知在 1.5 秒内完成滑入-停留-淡出完整动画。
- **GIVEN** 同一帧 3 个日志条目解锁，**THEN** 3 条通知以 0.3 秒间隔依次显示，不同时叠加。
- **GIVEN** 玩家按 `J`，**THEN** 0.3 秒翻书动画后日志界面打开，游戏时钟暂停，鼠标可见。
- **GIVEN** 记忆碎片过渡开始，**THEN** 全 HUD 元素立即隐藏（包括任何进行中的通知动画）。
- **GIVEN** 对话进行中，**THEN** 交互提示、通知等其他 HUD 元素不显示。

## Open Questions

- **OQ-01**：是否需要"无障碍模式"——大字体、高对比度、色盲辅助色？设计层先预留文本走 FText，颜色走主题变量，实现层在 Full Vision 阶段处理。
- **OQ-02**：手柄震动反馈是否纳入本系统管理？建议由各触发系统（谜题完成、碎片进入）直接调用 `UGameplayStatics::PlayForceFeedback()`，HUD 系统不介入。
