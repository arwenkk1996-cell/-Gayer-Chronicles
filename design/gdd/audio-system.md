# Audio System

> **Status**: Designed
> **Author**: User + Claude Code (audio-director, sound-designer)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Atmosphere — the emotional layer beneath everything visible

## Overview

音频系统管理盖亚编年史中所有声音的播放、分层与状态切换。系统分为三个子域：**环境音（Ambience）**、**音乐（Music）**和**音效（SFX）**。核心设计是以时间层为驱动的**动态音乐分层系统**：四个时代各有独立的音乐色彩，记忆碎片进入/退出时音乐平滑过渡，不是硬切而是像调音台慢慢推拨子。系统依赖 **Unreal Engine 5 的 MetaSounds** 实现程序化音效合成，以及 **UE5 Sound Cue / SoundClass / SoundMix** 管理整体音量与 Ducking 逻辑。

## Player Fantasy

进入记忆碎片的瞬间，世界的声音没有"关掉再开"——它像是透过水听到的，现在的声音渐渐远去，古老的声音渐渐浮现。翻开日志时，一滴轻微的墨水声；解开谜题时，不是游戏音效，而是石头真的移动的声音，空气的振动。**盖亚所在的时代不同，空气本身的质感不同。** 玩家应该能闭上眼睛，靠声音分辨自己处于哪个时代。

## Detailed Design

### Core Rules

**四时代音乐特征：**

| 时代 | 乐器主调 | 情绪色彩 | 节奏 | BPM 参考 |
|------|----------|----------|------|----------|
| 现在（Present） | 大提琴 + 钢鼓 + 轻风声 | 孤独中的好奇，带一丝暖意 | 漂浮，不定拍 | 无节拍（Ambient）|
| 旧世界（OldWorld） | 长笛 + 拨弦 + 木块 | 文明繁荣的记忆，微微失落 | 轻快，3/4 拍 | 72 BPM |
| 逃亡（Exodus） | 低弦张力 + 金属打击 + 失真人声 | 绝望与意志的对峙 | 紧迫，不规则 | 90–110 BPM（动态）|
| 建国（Founding） | 合唱（女声 a cappella）+ 弦乐 | 悲中有壮，失去中诞生 | 庄严，4/4 拍 | 60 BPM |

**动态音乐分层（Layered Music System）：**

每个时代的音乐由 3–4 条独立音轨（Stem）构成：
- **Base Stem**：始终播放（环境底纹）
- **Exploration Stem**：探索时渐入，碎片/对话时渐出
- **Tension Stem**：谜题 InProgress 时渐入，解决时渐出
- **Cinematic Stem**：特殊叙事时刻专用（Sequencer 触发）

时代切换时，当前时代所有 Stem 以 `CrossfadeDuration` 渐出，目标时代 Base Stem 渐入，其他 Stem 跟随游戏状态决定是否渐入。

**Sound Classes 层级：**

```
Master
├── Music
│   ├── Ambient (Base Stem)
│   ├── Exploration (Exploration Stem)
│   ├── Tension (Tension Stem)
│   └── Cinematic (Cinematic Stem)
├── SFX
│   ├── Environment (脚步、风声、水声)
│   ├── Interaction (交互音效、谜题音效)
│   └── UI (界面反馈音效)
└── Voice
    └── Gaia (盖亚独白)
```

**Ducking 规则（SoundMix Modifier）：**

| 触发条件 | 被降音的类 | 降至 | 恢复时间 |
|----------|------------|------|----------|
| 对话开始 | Music.Exploration | 30% | 2.0 秒 |
| 对话结束 | Music.Exploration | 100% | 2.0 秒 |
| 盖亚独白触发 | Music (全部) | 70% | 1.0 秒 |
| 记忆碎片过渡 | SFX.Environment | 0%（立即静音）| — |

### 环境音（Ambience）设计

每个地点有专属环境音层，由 `UAudioComponent` 实例管理，随关卡加载而激活。环境音不受时代切换影响（现实层环境音持续），碎片内环境音随碎片关卡加载。

**现实层典型环境音栈（以 Sourceholm 为例）：**
- 远处风声（Loop，-18 dB）
- 残破建筑共鸣（偶发，MetaSounds 随机间隔 10–30s）
- 水流滴落声（近水源处，`ATriggerBox` 范围内渐入）
- 虫鸣/鸟声（仅白天时段，时间系统触发——若无时间系统则设计层固定状态）

**碎片内环境音（以旧世界 Sourceholm 为例）：**
- 人声嘈杂（市场远景），-24 dB，立体声
- 流水（水渠运转），较现实层更响亮
- 金属工具声（偶发）

### SFX 索引

| 触发 | 音效描述 | 实现 |
|------|----------|------|
| 步行（石地） | 石板脚步，带轻微空腔共鸣 | MetaSounds，4 种随机变体 |
| 步行（土地） | 沙土脚步，干燥质感 | MetaSounds，4 种随机变体 |
| 交互（触碰文物） | 轻微石头摩擦 + 共鸣泛音 | 1 种，播完即止 |
| 碎片触发 | 低频共鸣泛音，渐强 1.5s | MetaSounds，与过渡动画同步 |
| 谜题 InProgress | 细小激活嗡鸣（Loop） | MetaSounds，解决时打断 |
| 谜题 Solved | 三音阶上行共鸣（非音效，是音乐片段）| 固定 SoundCue，0.8s |
| 谜题错误操作 | 软性阻尼声，低沉 | 1 种 |
| 日志解锁 | 极轻微墨水滴落声 | 1 种，-28 dB |
| 日志开启/关闭 | 翻页纸张声 | 1 种 |
| 对话开始 | 无（静默过渡）| — |
| Vault 动作 | 布料摩擦 + 落地重音 | 2 步 SFX，RootMotion 同步 |

### States and Transitions

音频系统以 `ETimeLayer` + `EAudioGameState` 的组合驱动：

```
EAudioGameState：
Exploring → InDialogue
Exploring → InFragment
Exploring → InPuzzle
Exploring → InCinematic
```

每种状态组合对应一套 SoundMix Snapshot，由 `UAudioSubsystem` 在收到各系统事件时切换。

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 视觉过渡系统 | 订阅 | `FOnTimeLayerChanged(ETimeLayer)` → 触发音乐时代切换 |
| 记忆碎片系统 | 订阅 | `FOnFragmentStateChanged` → 环境音切换、SFX 触发 |
| 对话系统 | 订阅 | `FOnDialogueStateChanged` → Music Ducking |
| 谜题系统 | 订阅 | `FOnPuzzleStateChanged(EPuzzleState)` → Tension Stem 渐入/出 |
| HUD/UI 系统 | 被触发 | 日志开关、通知弹出时播放 SFX |
| 关卡流送系统 | 订阅 | 区域加载时注册/注销该区域的环境音 Component |

## Formulas

### 公式 1：Stem 音量插值

`StemVolume(t) = Lerp(SourceVolume, TargetVolume, t / CrossfadeDuration)`

| 变量 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `SourceVolume` | float [0,1] | 当前音量 | 切换前的音量 |
| `TargetVolume` | float [0,1] | 0 或 1 | 渐出目标=0，渐入目标=1 |
| `CrossfadeDuration` | float (秒) | 2.5 | 时代切换淡变时长 |

线性插值（非 SmoothStep）——音量线性变化听感更自然，避免 Ease In-Out 在音量上产生"先慢后慢"的拖沓感。

### 公式 2：随机环境音间隔

`NextAmbientEventTime = CurrentTime + RandomFloat(MinInterval, MaxInterval)`

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `MinInterval` | 10.0 秒 | 最短静默间隔 |
| `MaxInterval` | 30.0 秒 | 最长静默间隔 |

MetaSounds Random node 实现，每次触发后重新采样下一次时间。

## Edge Cases

- **如果时代切换在上一次切换的 Crossfade 尚未完成时触发**：立即打断上一次 Crossfade，从当前插值位置开始向新目标切换（不从头重来，避免"闪断"）。
- **如果碎片被 Aborted（强制退出）**：跳过正常渐出，在 0.5s 内快速淡切回 Present 音乐，SFX 立即停止。
- **如果玩家在对话中进入谜题（理论上不可能，但防御性处理）**：Dialogue Ducking 优先于 Puzzle Tension Stem 渐入，对话结束后 Tension Stem 正常渐入。
- **如果脚步音效的地面材质信息缺失（关卡蓝图未配置 Physical Material）**：回退至默认"石地"脚步音效，不崩溃。
- **如果 MetaSounds 资产加载失败**：静默失败（不播放），`UE_LOG` 记录错误，不影响游戏流程。

## Dependencies

**上游依赖：** 视觉过渡系统（时代信号）、记忆碎片系统、对话系统、谜题系统

**下游依赖：** 无（音频是最终输出层）

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| 时代切换 Crossfade 时长 | 2.5 秒 | 1.5–4.0 秒 | 过渡感 vs 响应感 |
| 对话 Ducking 降至 | 30% | 20–50% | 对话可读性 vs 音乐存在感 |
| 谜题 Tension Stem 渐入时长 | 1.5 秒 | 0.5–3.0 秒 | 紧张感建立速度 |
| 随机环境音最小间隔 | 10 秒 | 5–20 秒 | 随机感密度 |
| 全局 Master 音量 | 1.0 | — | 玩家可在设置中调整 |
| 音乐音量（相对 Master）| 0.8 | — | 玩家可调 |
| SFX 音量（相对 Master）| 1.0 | — | 玩家可调 |

## Visual/Audio Requirements

（本系统本身即音频层，以下为约束性要求）

- 所有音乐 Stem 资产格式：OGG Vorbis，44.1kHz，16bit（内存/质量平衡）
- 脚步音效：至少 4 变体/地面类型（防止重复感）
- MetaSounds 程序化节点用于：脚步随机化、环境音随机间隔、碎片触发共鸣合成
- 音频预算（垂直切片阶段）：同时激活 Sound Source ≤ 32（PC），≤ 16（主机目标）
- 所有 Spatial Sound 使用 UE5 Attenuation Settings：LinearRolloff，MaxDistance 1500cm（室外），600cm（室内）

## Acceptance Criteria

- **GIVEN** 游戏从 Present 切换到 OldWorld 时代，**THEN** Present Base Stem 在 2.5 秒内淡出，OldWorld Base Stem 在 2.5 秒内淡入，无硬切点。
- **GIVEN** 对话开始，**THEN** Music.Exploration 音量在 2.0 秒内降至 30%；对话结束后 2.0 秒内恢复至 100%。
- **GIVEN** 玩家进入谜题 InProgress，**THEN** Tension Stem 在 1.5 秒内渐入；谜题 Solved 后 Tension Stem 立即淡出并播放三音阶 Solved SFX。
- **GIVEN** 盖亚在石地行走，**THEN** 每步播放 4 种随机变体之一，无连续两步相同（RNG 防重复）。
- **GIVEN** MetaSounds 资产不存在，**THEN** 游戏不崩溃，控制台有警告日志，静默跳过播放。
- **GIVEN** 玩家调整设置中音乐音量为 0，**THEN** 所有 Music SoundClass 静音，SFX 不受影响。

## Open Questions

- **OQ-01**：逃亡层（Exodus）的"失真人声"是否需要真人录音？还是 MetaSounds 合成？建议垂直切片阶段用合成占位，Alpha 阶段评估录音成本。
- **OQ-02**：是否需要动态音乐响应盖亚与 NSM 状态（如结局倾向值高时音乐色调变化）？当前设计不包含此功能，可作为 Full Vision 阶段 Feature Flag。
- **OQ-03**：本地化阶段若有配音 VO（OQ-01 in dialogue-system.md），Voice SoundClass 需要重新评估 Ducking 逻辑。
