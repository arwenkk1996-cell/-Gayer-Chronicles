# Technical Preferences

<!-- Populated by /setup-engine. Updated as the user makes decisions throughout development. -->
<!-- All agents reference this file for project-specific standards and conventions. -->

## Engine & Language

- **Engine**: Godot 4.4 (稳定版)
- **Language**: GDScript (primary), C# (performance-critical systems only)
- **Rendering**: Forward+ renderer (主要场景); Compatibility renderer (低端设备 fallback)
- **Physics**: Godot Physics (内置)
- **Previous Engine**: ~~Unreal Engine 5.7~~ — 迁移至 Godot 4（2026-04-25，原因：开发设备为 MateBook 13 2021 核显）

## Input & Platform

<!-- Written by /setup-engine. Read by /ux-design, /ux-review, /test-setup, /team-ui, and /dev-story -->
<!-- to scope interaction specs, test helpers, and implementation to the correct input methods. -->

- **Target Platforms**: PC (primary), Console (future)
- **Input Methods**: Keyboard/Mouse, Gamepad
- **Primary Input**: Keyboard/Mouse
- **Gamepad Support**: Partial (recommended for controller players)
- **Touch Support**: None
- **Platform Notes**: All UI must support both mouse and d-pad navigation. No hover-only interactions.

## Naming Conventions

- **节点类 / 场景类**: PascalCase（如 `GaiaCharacter`、`MemoryArtifact`）
- **变量**: snake_case（如 `move_speed`、`current_state`）
- **常量**: UPPER_SNAKE_CASE（如 `HINT_TRIGGER_SECONDS`）
- **函数**: snake_case（如 `begin_fragment()`、`on_interact()`）
- **信号**: 过去式 snake_case（如 `fragment_completed`、`flag_set`）
- **布尔变量**: `is_` 或 `has_` 前缀（如 `is_saving`、`has_reveal`）
- **场景文件**: snake_case（如 `gaia_character.tscn`、`memory_artifact.tscn`）
- **脚本文件**: 与节点同名（如 `gaia_character.gd`）
- **Autoload 单例**: PascalCase（如 `NarrativeState`、`FragmentManager`）
- **常量标签（StringName）**: 点分层级（如 `&"Fragment.WaterGate.Complete"`）

## Performance Budgets

- **目标帧率**: 60fps（PC，含 MateBook 13 级别集显）
- **帧预算**: 16.6ms（PC）
- **Draw Calls**: < 1000（Forward+ 渲染器下，比 UE5 宽松）
- **内存上限**: 2GB RAM（MateBook 13 8GB 内存，留足系统余量）
- **纹理规格**: 2K 最大（集显无专用 VRAM，纹理共享系统内存）
- **粒子上限**: GPUParticles3D，单场景 ≤ 3 个活跃粒子系统

## Testing

- **Framework**: Godot 内置 GUT（Godot Unit Testing）框架
- **运行方式**: `godot --headless -s addons/gut/gut_cmdln.gd`
- **最低覆盖**: 叙事状态机（NarrativeState autoload）、记忆碎片状态机
- **必须有测试的系统**: 时间轴状态转换、谜题完成逻辑、碎片触发守卫条件

## Forbidden Patterns

- 禁止在 GDScript 中硬编码叙事文本——所有玩家可见文字走 `TranslationServer` / `.po` 文件
- 禁止在 `_process()` 中写可以用信号替代的逻辑——用信号和 `call_deferred()` 驱动事件
- 禁止直接修改 NarrativeState 以外的地方写叙事标志——状态写入只经过 `NarrativeState` autoload
- 禁止在碎片进行中（`FragmentState != IDLE`）触发存档
- 禁止在场景切换期间调用 `get_tree().change_scene_to_*()`——使用 FragmentManager 的统一加载流程

## Allowed Libraries / Addons

| 插件 | 用途 | 状态 |
|------|------|------|
| [Dialogue Manager (Nathan Hoad)](https://github.com/nathanhoad/godot_dialogue_manager) | 对话脚本解析与执行（替代 Yarn Spinner） | ✅ 批准使用 |
| [GUT (Godot Unit Testing)](https://github.com/bitwes/Gut) | 单元测试框架 | ✅ 批准使用 |
| [Phantom Camera](https://phantom-camera.dev/) | 镜头状态管理（替代 SpringArmComponent 状态机） | ⚠️ 待评估 |

## Architecture Decisions Log

- ADR-001 ~ ADR-008: ~~UE5 版（已被引擎迁移取代）~~ 状态：Superseded
- ADR-009: Narrative State Machine — Godot Autoload
- ADR-010: Dialogue System — Dialogue Manager Plugin
- ADR-011: Memory Fragment Scene Loading — Godot Async ResourceLoader
- ADR-012: Save System — Godot Resource + FileAccess async
- ADR-013: Camera System — SpringArm3D + Camera3D
- ADR-014: Visual Transition — WorldEnvironment + GlobalShaderParameters
- ADR-015: Character Controller — CharacterBody3D
- ADR-016: HUD/UI System — CanvasLayer + Control

## Engine Specialists

- **Primary**: unreal-specialist → 迁移后改用通用 `general-purpose` 处理 Godot 架构问题
- **UI 专项**: 参考 Godot Control 节点体系，UX 规格文档不变
- **GDScript 审查**: 使用 `code-review` skill

### 文件类型路由

| 文件类型 | 处理方式 |
|----------|---------|
| `.gd` 脚本 | general-purpose / code-review |
| `.tscn` 场景 | general-purpose |
| `.gdshader` 着色器 | general-purpose |
| `.tres` / `.res` 资源 | general-purpose |
| 架构评审 | general-purpose (architecture 角色) |
