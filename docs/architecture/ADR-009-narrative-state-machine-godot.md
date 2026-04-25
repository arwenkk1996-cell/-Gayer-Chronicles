# ADR-009: 叙事状态机 — Godot Autoload 单例

**Status:** Accepted
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)
**Supersedes:** ADR-001（UE5 版本）

---

## Context

叙事状态机（NSM）是全游戏最核心的数据层：对话选择、碎片完成、结局得分全部写入这里，
跨场景切换需要持久存在。

Godot 4 的对应方案是 **Autoload 单例**——在 Project Settings > Autoload 注册的节点，
在整个游戏生命周期内只有一个实例，无需手动传递引用，任何脚本均可通过 `NarrativeState.xxx` 直接访问。

ADR-001（UE5）使用 `UGameInstanceSubsystem + FGameplayTagContainer`。  
Godot 等价方案：`Autoload Node + Dictionary + StringName`。

---

## Decision

**使用 `NarrativeState` Autoload 节点作为叙事状态机宿主。**

### 数据结构

```gdscript
# autoload/narrative_state.gd

extends Node

# 标志存储：StringName → bool
# 等价于 UE5 的 FGameplayTagContainer
var _flags: Dictionary = {}

# 整数存储：StringName → int
# 等价于 UE5 的 TMap<FGameplayTag, int32>
var _ints: Dictionary = {}

# 信号（等价于 UE5 的 FOnFlagSet / FOnIntChanged 委托）
signal flag_set(tag: StringName)
signal int_changed(tag: StringName, new_value: int)
```

### 核心 API

```gdscript
# 写入标志
func set_flag(tag: StringName) -> void:
    _flags[tag] = true
    flag_set.emit(tag)

# 读取标志
func has_flag(tag: StringName) -> bool:
    return _flags.get(tag, false)

# 清除标志（用于测试；游戏中不调用）
func clear_flag(tag: StringName) -> void:
    _flags.erase(tag)

# 整数增量
func increment_int(tag: StringName, delta: int) -> void:
    _ints[tag] = _ints.get(tag, 0) + delta
    int_changed.emit(tag, _ints[tag])

# 读取整数
func get_int(tag: StringName) -> int:
    return _ints.get(tag, 0)

# 序列化（存档用）
func export_state() -> Dictionary:
    return { "flags": _flags.duplicate(), "ints": _ints.duplicate() }

# 反序列化（读档用）
func import_state(data: Dictionary) -> void:
    _flags = data.get("flags", {})
    _ints = data.get("ints", {})
```

### 标签命名规范

与 ADR-001 保持一致，使用点分层级 StringName：

| 类型 | 格式 | 示例 |
|------|------|------|
| 碎片完成 | `Fragment.<区域>.<名称>.Complete` | `&"Fragment.Sourceholm.WaterGate.Complete"` |
| 对话完成 | `Dialogue.<NPC标签>.<节点ID>.Done` | `&"Dialogue.Naya.FirstMeeting.Done"` |
| 结局得分 | `Ending.Score.<走向>` | `&"Ending.Score.Restore"` |
| NPC 进度 | `NPC.<标签>.LastNodeID` | `&"NPC.Naya.LastNodeID"` |

**规范**：所有标签以 `&"..."` 字面量形式声明，禁止动态拼接字符串作为标签。

### 注册方式

`Project Settings > Autoload > 添加 autoload/narrative_state.gd，名称 NarrativeState`

调用示例：
```gdscript
# 从任意脚本读/写 NSM
NarrativeState.set_flag(&"Fragment.WaterGate.Complete")
if NarrativeState.has_flag(&"Fragment.WaterGate.Complete"):
    show_reveal_text()
```

---

## Consequences

**Positive:**
- Autoload 天生跨场景持久（无需 DontDestroyOnLoad 或 GameInstance 体系）
- `NarrativeState.xxx` 全局可访问，无需依赖注入
- 信号机制替代 UE5 委托，连接更简洁
- Dictionary + StringName 在 Godot 4 性能良好，适合本项目规模

**Negative / Mitigations:**
- 全局单例增加耦合风险。**缓解**：严格要求只有 `FragmentManager`、`DialogueRunner`、
  `SaveManager` 三个系统**写入** NSM；其他系统只**读取**。
- StringName 无编译期类型检查。**缓解**：在 `narrative_state.gd` 顶部维护一个
  `const Tags` 内部类，集中声明所有合法标签名（等价于 UE5 的 NarrativeTags.h）。

```gdscript
# narrative_state.gd 顶部——所有标签集中声明
class Tags:
    const FRAGMENT_WATERGATE_COMPLETE = &"Fragment.Sourceholm.WaterGate.Complete"
    const DIALOGUE_NAYA_FIRST_DONE    = &"Dialogue.Naya.FirstMeeting.Done"
    const ENDING_SCORE_RESTORE        = &"Ending.Score.Restore"
    # ... 全部标签在此注册
```

---

## ADR Dependencies

- ADR-012（存档系统）：调用 `NarrativeState.export_state()` / `import_state()` 序列化

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-nsm-001 | 叙事状态机跨场景持久化 |
| TR-nsm-002 | SetFlag / HasFlag / IncrementInt / GetInt API |
| TR-nsm-003 | 状态变更信号通知订阅方 |
| TR-nsm-004 | 状态可序列化（存档/读档） |
