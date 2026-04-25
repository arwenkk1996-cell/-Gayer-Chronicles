# Technical Preferences

<!-- Populated by /setup-engine. Updated as the user makes decisions throughout development. -->
<!-- All agents reference this file for project-specific standards and conventions. -->

## Engine & Language

- **Engine**: Unreal Engine 5.7
- **Language**: C++ (primary), Blueprint (gameplay prototyping)
- **Rendering**: Lumen (global illumination), Nanite (virtualized geometry)
- **Physics**: Chaos Physics

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

- **Classes**: Prefixed PascalCase (`A` for Actor, `U` for UObject, `F` for struct, `E` for enum)
- **Variables**: PascalCase (e.g., `MoveSpeed`, `CurrentHealth`)
- **Booleans**: `b` prefix (e.g., `bIsAlive`, `bCanInteract`)
- **Functions**: PascalCase (e.g., `TakeDamage()`, `OnInteract()`)
- **Files**: Match class without prefix (e.g., `PlayerController.h`, `PlayerController.cpp`)
- **Blueprint assets**: BP_ prefix (e.g., `BP_GaiaCharacter`)
- **Constants**: UPPER_SNAKE_CASE

## Performance Budgets

- **Target Framerate**: 60fps (PC), 30fps (Console target)
- **Frame Budget**: 16.6ms (PC), 33.3ms (Console)
- **Draw Calls**: < 2000 per frame
- **Memory Ceiling**: 8 GB VRAM (PC high), 4 GB (PC low / Console target)

## Testing

- **Framework**: Unreal Automation Framework (headless with `-nullrhi`)
- **Minimum Coverage**: Core gameplay systems and narrative state machine
- **Required Tests**: Timeline state transitions, puzzle completion logic, memory fragment triggers

## Forbidden Patterns

<!-- Add patterns that should never appear in this project's codebase -->
- No hardcoded narrative strings in C++ — all text must go through the localization table
- No Tick() for logic that can be event-driven — use delegates and timers
- No Blueprint-only systems for core narrative state — must have C++ backing class

## Allowed Libraries / Addons

<!-- Add approved third-party dependencies here -->
- [None configured yet — add as dependencies are approved]

## Architecture Decisions Log

<!-- Quick reference linking to full ADRs in docs/architecture/ -->
- [No ADRs yet — use /architecture-decision to create one]

## Engine Specialists

- **Primary**: unreal-specialist
- **Language/Code Specialist**: ue-blueprint-specialist (Blueprint graphs) or unreal-specialist (C++)
- **Shader Specialist**: unreal-specialist (no dedicated shader specialist — primary covers materials)
- **UI Specialist**: ue-umg-specialist (UMG widgets, CommonUI, input routing, widget styling)
- **Additional Specialists**: ue-gas-specialist (Gameplay Ability System, attributes, gameplay effects), ue-replication-specialist (property replication, RPCs — if multiplayer added later)
- **Routing Notes**: Invoke primary for C++ architecture and broad engine decisions. Invoke Blueprint specialist for Blueprint graph architecture and BP/C++ boundary design. Invoke GAS specialist for all ability and attribute code (interaction system, memory fragment triggers). Invoke UMG specialist for all UI implementation.

### File Extension Routing

| File Extension / Type | Specialist to Spawn |
|-----------------------|---------------------|
| Game code (.cpp, .h files) | unreal-specialist |
| Shader / material files (.usf, .ush, Material assets) | unreal-specialist |
| UI / screen files (.umg, UMG Widget Blueprints) | ue-umg-specialist |
| Scene / level files (.umap, .uasset) | unreal-specialist |
| Native extension / plugin files (.uplugin, modules) | unreal-specialist |
| Blueprint graphs (.uasset BP classes) | ue-blueprint-specialist |
| Ability / attribute / GAS files | ue-gas-specialist |
| General architecture review | unreal-specialist |
