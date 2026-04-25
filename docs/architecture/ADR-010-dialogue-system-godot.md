# ADR-010: 对话系统 — Dialogue Manager Plugin (Nathan Hoad)

**Status:** Accepted
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)
**Supersedes:** ADR-002（UE5 版本，Yarn Spinner for Unreal）

---

## Context

游戏需要：分支对话树、条件可见选项（基于 NSM 标志）、对话完成后写入 NSM。

UE5 版使用 Yarn Spinner for Unreal（有 UE5.7 兼容性风险）。  
Godot 4 最成熟的等价方案是 **Dialogue Manager**（Nathan Hoad），
Godot 社区叙事游戏事实标准插件，GitHub 12k+ stars，持续维护。

---

## Decision

**使用 Dialogue Manager 插件处理对话脚本解析和执行。**
**禁用插件内置变量存储；所有状态通过 `NarrativeState` autoload 读写。**

### 安装

```
Project > AssetLib > 搜索 "Dialogue Manager" > 安装
或从 https://github.com/nathanhoad/godot_dialogue_manager 手动安装
```

### 对话文件格式（`.dialogue`）

```dialogue
# naya_first_meeting.dialogue
# 等价于 UE5 的 .yarn 文件

~ start

Naya: 水渠建成的那年，我三岁。我只记得母亲说，那年水第一次流进了南区。

=> player_choice

~ player_choice

- 那南区的人怎么说？ => ask_south
- 你母亲还在吗？ => ask_mother
- 水渠是谁建的？ [if NarrativeState.has_flag(&"Fragment.WaterGate.Complete")] => ask_builder

~ ask_south
Naya: 他们说那是奇迹。现在他们的孙辈每天走过水渠，已经不觉得了。
do NarrativeState.set_flag(&"Dialogue.Naya.SouthDistrict.Done")
=> END

~ ask_mother
Naya: 她走了很多年了。但她的手——干活的手——我还记得。
do NarrativeState.set_flag(&"Dialogue.Naya.Mother.Done")
=> END

~ ask_builder
Naya: 你知道建造者的名字？你去过那里——那块石碑……
do NarrativeState.set_flag(&"Dialogue.Naya.BuilderKnown.Done")
do NarrativeState.increment_int(&"Ending.Score.Restore", 2)
=> END
```

### 对话触发

```gdscript
# 在 NPC 的 on_interact() 函数中：
func on_interact(_gaia: Node) -> void:
    var start_node = "start"
    # 续接上次对话（等价于 UE5 的 LastNodeID 机制）
    if NarrativeState.has_flag(&"Dialogue.Naya.FirstMeeting.Done"):
        start_node = NarrativeState.get_int(&"NPC.Naya.LastNodeID_AsInt")  # 简化处理
    DialogueManager.show_dialogue_balloon(
        load("res://dialogue/naya_first_meeting.dialogue"),
        start_node
    )
```

### 自定义对话气泡 UI（连接至 `dialogue-ui.md` UX 规格）

```gdscript
# 实现自定义 balloon 场景，替换 Dialogue Manager 的默认 UI
# 场景：res://ui/dialogue_balloon.tscn
# 绑定：Project Settings > Plugins > Dialogue Manager > Custom Balloon Path
```

### NSM 集成——禁用插件变量存储

Dialogue Manager 支持 `do` 命令调用任意 GDScript 函数，
配合 `if` 条件支持调用任意 GDScript 表达式：

```gdscript
# 条件判断直接调用 NarrativeState：
[if NarrativeState.has_flag(&"Fragment.WaterGate.Complete")]

# 状态写入通过 do 命令：
do NarrativeState.set_flag(&"Dialogue.Naya.Done")
do NarrativeState.increment_int(&"Ending.Score.Restore", 2)
```

**不使用** Dialogue Manager 的内置变量（`{variable_name}` 语法）——
与 ADR-002 对 Yarn Spinner 的决策一致：双数据源必然造成存档不一致。

---

## Consequences

**Positive:**
- Dialogue Manager 是 Godot 社区最成熟的对话插件，文档完善
- `.dialogue` 格式比 Yarn 更简洁，易于非程序员编写
- `do` 命令 + `if` 条件表达式直接调用 GDScript，NSM 集成无缝
- 支持自定义 UI balloon——可完全实现 `dialogue-ui.md` 规格

**Negative / Mitigations:**
- 插件版本更新可能引入 API 变化。**缓解**：锁定版本到 `v3.x`，只在有充分理由时升级。
- `.dialogue` 语法与 Yarn 不同，前期设计的对话脚本草稿需要改写格式。
  **缓解**：格式很简单，迁移成本低于两小时。

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-dlg-001 | 分支对话树，条件可见选项 |
| TR-dlg-002 | 对话完成写入 NSM |
| TR-dlg-003 | 条件选项基于 NSM 标志过滤 |
| TR-dlg-004 | 对话状态不在对话系统内部存储 |
