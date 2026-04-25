# ADR-011: 记忆碎片场景加载 — Godot ResourceLoader 异步加载

**Status:** Accepted
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)
**Supersedes:** ADR-003（UE5 版本）

---

## Context

记忆碎片系统需要：在保持当前世界场景运行的同时，异步加载一个独立的历史时代场景，
视觉过渡动画播放期间完成加载，加载完成后将玩家转入碎片场景，碎片完成后卸载并返回。

UE5 版使用 `ULevelStreamingDynamic::LoadLevelInstance`。  
Godot 4 等价方案：`ResourceLoader.load_threaded_request()` + 动态 `add_child()`。

**Godot 场景架构特点：**
- Godot 没有 Level Streaming 概念，但可以用 `add_child(packed_scene.instantiate())` 动态加入节点树
- `ResourceLoader.load_threaded_request()` 实现后台异步加载，不阻塞主线程
- 加载完成的 PackedScene 通过 `ResourceLoader.load_threaded_get()` 取出并实例化

---

## Decision

**使用 `FragmentManager` Autoload 管理碎片生命周期，`ResourceLoader` 异步加载场景。**

### 状态机（与 UE5 原型一致，保留 5 状态）

```gdscript
enum FragmentState {
    IDLE,
    TRANSITIONING_IN,   # 视觉过渡 + 场景加载并发
    IN_FRAGMENT,        # 玩家在碎片中
    TRANSITIONING_OUT,  # 反向过渡 + 场景卸载
    ABORTED             # 错误处理
}
```

### 核心实现

```gdscript
# autoload/fragment_manager.gd
extends Node

signal fragment_state_changed(new_state: FragmentState)
signal fragment_completed(tag: StringName, result: Dictionary)

var state: FragmentState = FragmentState.IDLE

# 场景路径映射（等价于 UE5 的 DT_FragmentLevels DataTable）
# 实际从 fragments_table.tres 资源文件读取，此处仅示意
const FRAGMENT_LEVELS: Dictionary = {
    &"Fragment.Sourceholm.WaterGate":  "res://scenes/fragments/sourceholm_watergate.tscn",
    &"Fragment.MirrorCoast.Marriage":  "res://scenes/fragments/mirrorcoast_marriage.tscn",
}

var _current_fragment_scene: Node = null
var _is_animation_complete: bool = false  # 视觉过渡完成标志
var _is_load_complete: bool = false       # 场景加载完成标志

func begin_fragment(tag: StringName) -> void:
    if state != FragmentState.IDLE:
        push_warning("FragmentManager: begin_fragment() 忽略 — 当前状态 %s" % FragmentState.keys()[state])
        return

    var path: String = FRAGMENT_LEVELS.get(tag, "")
    if path.is_empty():
        push_error("FragmentManager: 未知碎片标签 %s" % tag)
        return

    state = FragmentState.TRANSITIONING_IN
    _is_animation_complete = false
    _is_load_complete = false
    fragment_state_changed.emit(state)

    # ── 并发启动两件事 ──────────────────────────────────────────────────────
    # 1. 通知视觉过渡系统开始动画（VisualTransition autoload）
    VisualTransition.begin_transition_to(VisualTransition.Layer.OLD_WORLD, 2.5)

    # 2. 后台异步加载场景
    ResourceLoader.load_threaded_request(path)
    _poll_load_status(path, tag)

func _poll_load_status(path: String, tag: StringName) -> void:
    # 每帧检查加载进度（通过 call_deferred 递归，不阻塞）
    var status = ResourceLoader.load_threaded_get_status(path)
    match status:
        ResourceLoader.THREAD_LOAD_IN_PROGRESS:
            call_deferred("_poll_load_status", path, tag)
        ResourceLoader.THREAD_LOAD_LOADED:
            _on_fragment_scene_loaded(path, tag)
        ResourceLoader.THREAD_LOAD_FAILED:
            _abort()

func _on_fragment_scene_loaded(path: String, tag: StringName) -> void:
    var packed: PackedScene = ResourceLoader.load_threaded_get(path)
    _current_fragment_scene = packed.instantiate()
    # 场景加入树但隐藏（过渡完成前不可见）
    add_child(_current_fragment_scene)
    _current_fragment_scene.visible = false

    _is_load_complete = true
    _try_enter_fragment()

# VisualTransition 动画完成时调用此函数
func _on_transition_animation_complete() -> void:
    _is_animation_complete = true
    _try_enter_fragment()

func _try_enter_fragment() -> void:
    # 两个条件都满足才进入——与 UE5 原型的两标志门逻辑一致
    if not (_is_animation_complete and _is_load_complete):
        return

    _current_fragment_scene.visible = true
    state = FragmentState.IN_FRAGMENT
    fragment_state_changed.emit(state)

func complete_fragment(tag: StringName, result: Dictionary) -> void:
    if state != FragmentState.IN_FRAGMENT:
        return

    # 写入 NSM
    NarrativeState.set_flag(StringName(str(tag) + ".Complete"))
    for score_key in result.get("scores", {}):
        NarrativeState.increment_int(score_key, result["scores"][score_key])

    state = FragmentState.TRANSITIONING_OUT
    fragment_state_changed.emit(state)

    # 反向过渡
    VisualTransition.begin_transition_to(VisualTransition.Layer.PRESENT, 2.5)
    await VisualTransition.transition_completed

    # 卸载场景
    _current_fragment_scene.queue_free()
    _current_fragment_scene = null
    state = FragmentState.IDLE
    fragment_state_changed.emit(state)
    fragment_completed.emit(tag, result)

func _abort() -> void:
    if _current_fragment_scene:
        _current_fragment_scene.queue_free()
        _current_fragment_scene = null
    state = FragmentState.IDLE
    fragment_state_changed.emit(state)
    push_error("FragmentManager: 碎片加载中断，已回滚至 IDLE")
```

### 场景文件结构

```
res://scenes/fragments/
  sourceholm_watergate_oldworld.tscn   # 水闸石碑·旧世界层
  sourceholm_watergate_founding.tscn   # 水闸石碑·建国层
  mirrorcoast_marriage_road.tscn       # 婚路·旧世界层
```

### 场景路径映射表

```
res://data/fragments_table.tres        # Resource 文件，Dictionary 格式，替代 UE5 的 DataTable
```

---

## Consequences

**Positive:**
- `ResourceLoader.load_threaded_request()` 是 Godot 4 官方推荐的异步加载 API，稳定
- 两标志门逻辑（`_is_animation_complete + _is_load_complete`）与 UE5 原型完全一致，
  已验证无竞态条件（见 `prototypes/memory-fragment-transition/REPORT.md`）
- 场景文件完全独立（`.tscn`），美术可单独编辑每个时代场景

**Negative / Mitigations:**
- Godot 的轮询加载状态需要 `call_deferred` 递归，略显冗长。
  **缓解**：Godot 4.3+ 支持 `await ResourceLoader.load_threaded_request_completed`
  信号（需版本确认）；若不支持，当前轮询方式可靠性已足够。
- 碎片场景加载时间受设备限制。**缓解**：MateBook 13 NVMe SSD 读速约 1GB/s，
  小场景（<50MB）加载预计 < 0.5s，在 2.5s 过渡动画内完全透明。

---

## ⚠️ 实现时需验证

- [ ] Godot 4.4 中 `ResourceLoader.load_threaded_get_status()` 的准确轮询方式
- [ ] 异步加载的场景是否自动处理其内部资源（纹理、材质）的引用计数
- [ ] `queue_free()` 是否充分释放碎片场景内存（在 Godot 内存分析器中验证）

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-frag-001 | FragmentManager 管理碎片生命周期 |
| TR-frag-002 | AMemoryArtifact 触发 begin_fragment() |
| TR-frag-003 | 异步场景加载，视觉过渡期间完成 |
| TR-frag-004 | 进入/退出碎片时切换角色控制权 |
| TR-frag-006 | 错误中断回滚至 IDLE |
