# ADR-014: 视觉过渡系统 — Godot WorldEnvironment + 全局着色器参数

**Status:** Accepted
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)
**Supersedes:** ADR-007（UE5 版本）

---

## Context

四个时代层需要截然不同的视觉风格，记忆碎片进入/退出时需要 2.5s 平滑过渡。

UE5 版使用 4 个 `APostProcessVolume`（全局无界）+ `UMaterialParameterCollection`。  
Godot 4 等价方案：`WorldEnvironment` 节点（每个场景一个）+ `RenderingServer` 全局着色器参数。

---

## Decision

**使用 `VisualTransition` Autoload 管理时代层视觉状态，通过插值 `WorldEnvironment`
属性 + `RenderingServer.global_shader_parameter_set()` 实现跨时代视觉切换。**

### 各时代参数表（等价于 UE5 ADR-007 的 MPC 值）

| 时代层 | Saturation | ColorAdjust (Hue Shift) | GrainAmount | Vignette | 色温感知 |
|--------|-----------|------------------------|-------------|----------|---------|
| Present | 1.0 | 0.0 | 0.0 | 0.1 | 暖橙，5500K |
| OldWorld | 0.45 | +5°（偏暖棕）| 0.3 | 0.35 | 发黄，4000K |
| Exodus | 0.15 | -8°（偏冷蓝）| 0.5 | 0.70 | 灰蓝，7000K |
| Founding | 0.70 | +3°（暖黄）| 0.0 | 0.25 | 烛光，3000K |

### 核心实现

```gdscript
# autoload/visual_transition.gd
extends Node

signal transition_completed

enum Layer { PRESENT, OLD_WORLD, EXODUS, FOUNDING }

# 各时代参数（Saturation, GrainAmount, Vignette）
const LAYER_PARAMS: Dictionary = {
    Layer.PRESENT:   { "sat": 1.00, "grain": 0.00, "vignette": 0.10, "tint": Color(1.00, 0.97, 0.90) },
    Layer.OLD_WORLD: { "sat": 0.45, "grain": 0.30, "vignette": 0.35, "tint": Color(1.00, 0.94, 0.80) },
    Layer.EXODUS:    { "sat": 0.15, "grain": 0.50, "vignette": 0.70, "tint": Color(0.88, 0.92, 1.00) },
    Layer.FOUNDING:  { "sat": 0.70, "grain": 0.00, "vignette": 0.25, "tint": Color(1.00, 0.95, 0.82) },
}

var current_layer: Layer = Layer.PRESENT
var _tween: Tween = null

# 当前插值中间值（供碎片管理器读取动画进度）
var transition_alpha: float = 0.0

func begin_transition_to(target: Layer, duration: float) -> void:
    if _tween:
        _tween.kill()

    var source_params = LAYER_PARAMS[current_layer]
    var target_params = LAYER_PARAMS[target]

    _tween = create_tween()
    # TRANS_CUBIC + EASE_IN_OUT ≈ SmoothStep（等价于 UE5 FMath::SmoothStep）
    _tween.set_trans(Tween.TRANS_CUBIC)
    _tween.set_ease(Tween.EASE_IN_OUT)

    # 插值 Saturation
    _tween.tween_method(
        func(v): _apply_saturation(v),
        source_params["sat"], target_params["sat"], duration
    )
    # 插值 Vignette
    _tween.parallel().tween_method(
        func(v): _apply_vignette(v),
        source_params["vignette"], target_params["vignette"], duration
    )
    # 插值 Tint（通过全局着色器参数传给所有材质）
    _tween.parallel().tween_method(
        func(v): _apply_tint(v),
        source_params["tint"], target_params["tint"], duration
    )
    # 插值 GrainAmount
    _tween.parallel().tween_method(
        func(v): _apply_grain(v),
        source_params["grain"], target_params["grain"], duration
    )
    # 追踪 alpha（供 FragmentManager 的两标志门使用）
    _tween.parallel().tween_method(
        func(v): transition_alpha = v,
        0.0, 1.0, duration
    )

    await _tween.finished
    current_layer = target
    transition_alpha = 1.0
    transition_completed.emit()

# ── 应用函数（操作当前场景的 WorldEnvironment）────────────────────────────

func _get_env() -> Environment:
    var we = get_viewport().get_camera_3d()
    # 实际实现：从场景树找到 WorldEnvironment 节点
    var world_env = get_tree().get_first_node_in_group("world_environment")
    if world_env:
        return world_env.environment
    return null

func _apply_saturation(value: float) -> void:
    var env = _get_env()
    if env:
        env.adjustment_enabled = true
        env.adjustment_saturation = value

func _apply_vignette(value: float) -> void:
    # Godot 4.3+ 原生 vignette 参数
    # 如不可用，退化为全局着色器参数 "VignetteAmount"
    RenderingServer.global_shader_parameter_set("VignetteAmount", value)

func _apply_tint(value: Color) -> void:
    # 通过全局着色器参数传递色调给所有场景材质
    # 材质中用 global_param("ColorTint") 采样
    RenderingServer.global_shader_parameter_set("ColorTint", value)

func _apply_grain(value: float) -> void:
    RenderingServer.global_shader_parameter_set("GrainAmount", value)
```

### 材质端采样（场景材质需要的配置）

```gdscript
# 在所有环境材质（.gdshader）中添加全局参数采样：
global uniform vec3 ColorTint = vec3(1.0, 1.0, 1.0);
global uniform float GrainAmount = 0.0;
global uniform float VignetteAmount = 0.1;

# 在 fragment shader 中应用：
ALBEDO *= ColorTint;
```

### 全局着色器参数初始化

在项目初始化时（`autoload/visual_transition.gd` 的 `_ready()`）注册：

```gdscript
func _ready() -> void:
    RenderingServer.global_shader_parameter_add("ColorTint",
        RenderingServer.GLOBAL_VAR_TYPE_COLOR, Color.WHITE)
    RenderingServer.global_shader_parameter_add("GrainAmount",
        RenderingServer.GLOBAL_VAR_TYPE_FLOAT, 0.0)
    RenderingServer.global_shader_parameter_add("VignetteAmount",
        RenderingServer.GLOBAL_VAR_TYPE_FLOAT, 0.1)
```

---

## Consequences

**Positive:**
- `Tween.TRANS_CUBIC + EASE_IN_OUT` 产生的曲线与 `FMath::SmoothStep` 视觉效果等价
- `Environment.adjustment_saturation` 是 Godot 官方文档明确的运行时可修改属性
- `RenderingServer.global_shader_parameter_set()` 单次调用更新所有引用该参数的材质，
  等价于 UE5 的 `UMaterialParameterCollection`
- 过渡参数（sat/vignette/tint/grain）完全数据驱动，美术可在不修改代码的情况下调整

**Negative / Mitigations:**
- `RenderingServer.global_shader_parameter_add()` 是 Godot 4.1+ API，需确认版本。
  **缓解**：项目锁定 Godot 4.4，此 API 已稳定。
- Vignette 效果在 Godot 中非内置 PPV 参数，需通过全局着色器参数 + 后处理 quad 实现。
  **缓解**：使用 `SubViewportContainer` + 简单全屏 shader 实现 vignette，成本低。
- Fragment 场景有自己的 `WorldEnvironment`（时代独立视觉），与 VisualTransition 的
  过渡参数可能冲突。**缓解**：碎片场景的 `WorldEnvironment` 设为 `enabled = false`，
  只依赖 VisualTransition 的全局参数。

---

## ⚠️ 实现时需验证

- [ ] `Environment.adjustment_saturation` 在 Forward+ 渲染器下的实际效果
- [ ] `RenderingServer.global_shader_parameter_add()` 的调用时机（必须在任何材质渲染前）
- [ ] Vignette 实现方案：内置 vs 全屏 quad shader

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-vis-001 | 四时代独立视觉配置 |
| TR-vis-002 | begin_transition_to(Layer, duration) API |
| TR-vis-003 | SmoothStep 等效插值曲线 |
| TR-vis-004 | 过渡期间材质饱和度跟随变化 |
