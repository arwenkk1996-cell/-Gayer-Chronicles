# Save / Load System

> **Status**: Designed
> **Author**: User + Claude Code (unreal-specialist)
> **Last Updated**: 2026-04-25
> **Implements Pillar**: Foundation — narrative persistence across sessions

## Overview

存档/读档系统负责将叙事状态机的完整状态持久化到磁盘，并在读档时恢复。对于叙事游戏，存档即等于"盖亚经历过的一切"——所有碎片状态、对话选择、结局得分都必须忠实保存。系统采用 UE5 原生 `USaveGame` + `UGameplayStatics::AsyncSaveGameToSlot()` 异步存档，支持单存档槽位（后续可扩展）。存档自动触发（区域切换、碎片完成）以及手动触发（暂停菜单）。

## Player Fantasy

存档对玩家是透明的——它悄悄发生，在正确的时机，不打断体验。玩家离开游戏再回来，盖亚还在她离开的地方，记着她看过的一切。这种持续感是叙事游戏沉浸感的基础。

## Detailed Design

### Core Rules

**存档数据结构 `FGaiasSaveData`：**
```
FGaiasSaveData {
    FGameplayTagContainer  FlagStore        // 叙事状态机 Flag 镜像
    TMap<FGameplayTag, int32> IntStore      // 叙事状态机 Int 镜像
    FVector                GaiaLocation     // 盖亚当前位置
    FRotator               GaiaRotation     // 朝向
    FName                  CurrentLevelName // 当前关卡名
    FDateTime              SaveTimestamp    // 存档时间（显示用）
    int32                  SaveVersion      // 版本号，用于向后兼容
}
```

**自动存档触发点：**
1. 记忆碎片完成（`Fragment.Complete` 写入后）
2. 区域切换（World Partition 流送新区域加载完成后）
3. 对话树结束（含选择的对话完成后）

**手动存档：** 暂停菜单 → "存档" 按钮（任意时刻可用，碎片进行中除外）

**异步存档流程：**
1. `RequestSave()` → 检查 `UMemoryFragmentSubsystem::State != InFragment`
2. 调用 `UNarrativeStateSubsystem::ExportState()` 获取当前状态快照
3. 追加盖亚位置/朝向/关卡名
4. `UGameplayStatics::AsyncSaveGameToSlot("GaiaSlot_0", 0, Callback)`
5. 存档期间 HUD 显示存档图标（不阻塞游戏）
6. 回调完成时 HUD 存档图标消失

**读档流程：**
1. 游戏启动 / 主菜单"继续" → `AsyncLoadGameFromSlot("GaiaSlot_0", 0, Callback)`
2. 回调中调用 `UNarrativeStateSubsystem::ImportState(SaveData)`
3. 传送盖亚到 `SaveData.GaiaLocation`，加载对应关卡
4. 完成后进入游戏

### States and Transitions

| 状态 | 说明 |
|------|------|
| `Idle` | 无存档操作进行 |
| `Saving` | 异步写盘中（不阻塞游戏） |
| `Loading` | 异步读盘 + 恢复状态（阻塞在加载界面） |

### Interactions with Other Systems

| 系统 | 方向 | 接口 |
|------|------|------|
| 叙事状态机 | 读+写 | `ExportState()` / `ImportState()` |
| 记忆碎片系统 | 查询 | 存档前检查 `State != InFragment` |
| HUD/UI 系统 | 推送 | 存档开始/完成通知 → 存档图标显示/隐藏 |
| 角色控制器 | 读+写 | 读取/恢复盖亚位置和朝向 |

## Formulas

### 公式 1：存档版本向后兼容检查

`IsCompatible = (SaveVersion >= MinSupportedVersion) AND (SaveVersion <= CurrentVersion)`

| 变量 | 类型 | 当前值 | 说明 |
|------|------|--------|------|
| `SaveVersion` | int32 | 文件中读取 | 存档写入时的版本 |
| `CurrentVersion` | int32 | 1（初始版本） | 当前游戏版本 |
| `MinSupportedVersion` | int32 | 1 | 最老可兼容版本 |

不兼容时：清空存档，以新游戏状态启动，显示"存档版本不兼容"提示。

## Edge Cases

- **如果碎片进行中触发自动存档**：请求被拦截，不执行存档；碎片完成后的自动存档点会覆盖补充。
- **如果磁盘空间不足导致写盘失败**：HUD 显示"存档失败，磁盘空间不足"，游戏继续运行，不崩溃。存档状态回到 `Idle`。
- **如果存档文件损坏（反序列化失败）**：以空状态启动（等同于新游戏），显示"存档异常，已重置"，不崩溃。
- **如果存档版本不兼容**：同上，显示"存档版本不兼容，已重置"。
- **如果玩家在 `Saving` 状态中强制关闭游戏**：下次读档时若文件损坏走容错流程；若文件完整正常读取。异步存档的原子性由 UE5 `USaveGame` 保证（写临时文件后改名）。
- **如果不存在存档文件**：主菜单"继续"按钮不可用；仅显示"新游戏"。

## Dependencies

**上游依赖：**
- 叙事状态机（数据来源）
- 记忆碎片系统（状态锁定查询）
- 角色控制器（位置数据）

**下游依赖：** HUD/UI 系统（存档提示）

## Tuning Knobs

| 调节项 | 默认值 | 安全范围 | 影响 |
|--------|--------|----------|------|
| 存档槽位数量 | 1 | 1–3 | 当前单槽，扩展多槽需 UI 支持 |
| 存档图标显示时长 | 1.5 秒（写盘完成后再显示 0.5s） | — | 反馈可见性 |
| `MinSupportedVersion` | 1 | 只增不减 | 向后兼容范围 |

## Visual/Audio Requirements

- 存档图标：HUD 角落小图标，淡入淡出（0.2s），无音效（不打断沉浸）
- 读档：全屏过渡（黑屏淡入），期间播放加载动画

## UI Requirements

- 暂停菜单：手动存档按钮（碎片进行中变灰不可用）
- 主菜单："继续"按钮（无存档时不可用）+ 显示上次存档时间戳

## Acceptance Criteria

- **GIVEN** 碎片完成事件触发，**WHEN** `RequestSave()` 被调用，**THEN** 异步存档启动，HUD 存档图标出现，叙事状态机完整状态写入磁盘。
- **GIVEN** 碎片进行中，**WHEN** `RequestSave()` 被调用，**THEN** 请求被拦截，不写盘，不显示图标。
- **GIVEN** 存档成功后重启游戏并读档，**THEN** 叙事状态机恢复至存档前状态（所有 Flag 和 Int 一致），盖亚出现在存档位置。
- **GIVEN** 存档文件损坏，**WHEN** 尝试读档，**THEN** 以空状态启动，显示"存档异常"提示，不崩溃。
- **GIVEN** 磁盘写入失败，**THEN** 显示"存档失败"提示，游戏继续运行。

## Open Questions

- **OQ-01**：是否需要支持 Steam Cloud 存档同步？若是，需要在 `AsyncSaveGameToSlot` 外层加 Steamworks 集成层。暂缓至发行阶段评估。
