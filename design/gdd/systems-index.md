# Systems Index: Gaia Chronicles

> **Status**: Approved
> **Created**: 2026-04-25
> **Last Updated**: 2026-04-25
> **Source Concept**: design/gdd/game-concept.md

---

## Overview

Gaia Chronicles is a third-person narrative puzzle adventure. The mechanical scope centers
on exploration-driven world traversal, a core "Memory Fragment" time-layer system that lets
players inhabit other eras through artifact interaction, and a dialogue-driven narrative state
machine that accumulates toward a three-ending finale. The game does not have combat. Systems
exist in service of curiosity, discovery, and choice. The core loop is:
**Explore ruin → Touch artifact → Enter fragment → Return with new understanding → Ruin changes → Progress.**

---

## Systems Enumeration

| # | System Name | Category | Priority | Status | Design Doc | Depends On |
|---|-------------|----------|----------|--------|------------|------------|
| 1 | Narrative State Machine | Core | MVP | Not Started | — | — |
| 2 | Exploration / Interaction System | Gameplay | MVP | Not Started | — | Narrative State Machine, Character Controller |
| 3 | Memory Fragment System | Gameplay | MVP | Not Started | — | Interaction System, Visual Transition System, Narrative State Machine |
| 4 | Visual Transition System | Core | MVP | Not Started | — | Narrative State Machine |
| 5 | Character Controller | Core | MVP | Not Started | — | — |
| 6 | Camera System | Core | MVP | Not Started | — | Character Controller |
| 7 | Save / Load System | Persistence | MVP | Not Started | — | Narrative State Machine |
| 8 | Dialogue System | Narrative | Vertical Slice | Not Started | — | Interaction System, Narrative State Machine |
| 9 | Puzzle System | Gameplay | Vertical Slice | Not Started | — | Memory Fragment System, Interaction System |
| 10 | Journal / Chronicle System | Narrative | Vertical Slice | Not Started | — | Memory Fragment System, Dialogue System, Artifact System |
| 11 | HUD / UI System | UI | Vertical Slice | Not Started | — | All gameplay systems |
| 12 | Audio System | Audio | Vertical Slice | Not Started | — | Visual Transition System |
| 13 | NPC AI System (inferred) | Gameplay | Alpha | Not Started | — | Character Controller, Dialogue System |
| 14 | Artifact / Item System (inferred) | Gameplay | Alpha | Not Started | — | Interaction System, Narrative State Machine |
| 15 | World Map System (inferred) | UI | Alpha | Not Started | — | Narrative State Machine, Level Streaming |
| 16 | Cinematic / Staging System (inferred) | Narrative | Alpha | Not Started | — | Narrative State Machine, Dialogue System |
| 17 | Level Streaming System (inferred) | Core | Alpha | Not Started | — | — |
| 18 | Localization System (inferred) | Meta | Full Vision | Not Started | — | Dialogue System, Journal System, HUD/UI |

---

## Categories

| Category | Description |
|----------|-------------|
| **Core** | Foundation systems everything depends on |
| **Gameplay** | Systems that define what the player does |
| **Narrative** | Story, dialogue, journal, and lore delivery |
| **Persistence** | Save state and continuity |
| **UI** | Player-facing information displays |
| **Audio** | Sound, music, and ambient audio |
| **Meta** | Polish, accessibility, localization |

---

## Priority Tiers

| Tier | Definition | Target Milestone |
|------|------------|------------------|
| **MVP** | Required for the core loop to function. Can we test "is the Memory Fragment mechanic compelling?" | First playable prototype |
| **Vertical Slice** | Required for one complete, polished ruin (Sourceholm). Full experience from arrival to fragment reveal to journal entry. | Vertical slice / demo |
| **Alpha** | All features present in rough form. All 5 ruin areas reachable. | Alpha milestone |
| **Full Vision** | Content-complete, localized, polished. | Beta / Release |

---

## Dependency Map

### Foundation Layer (no dependencies)
1. **Character Controller** — movement is the base of all player agency
2. **Narrative State Machine** — global story tracking; every system reads or writes to it
3. **Level Streaming System** — UE5 World Partition; enables multi-area world without memory overload

### Core Layer (depends on foundation)
1. **Camera System** — depends on: Character Controller
2. **Exploration / Interaction System** — depends on: Character Controller, Narrative State Machine
3. **Visual Transition System** — depends on: Narrative State Machine (state drives time-layer switch)
4. **Save / Load System** — depends on: Narrative State Machine (serializes its state)

### Feature Layer (depends on core)
1. **Memory Fragment System** — depends on: Interaction System, Visual Transition System, Narrative State Machine
2. **Dialogue System** — depends on: Interaction System, Narrative State Machine
3. **NPC AI System** — depends on: Character Controller, Dialogue System

### Feature+ Layer (depends on feature)
1. **Puzzle System** — depends on: Memory Fragment System, Interaction System
2. **Artifact / Item System** — depends on: Interaction System, Narrative State Machine
3. **Journal / Chronicle System** — depends on: Memory Fragment System, Dialogue System, Artifact System
4. **World Map System** — depends on: Narrative State Machine, Level Streaming System

### Presentation Layer (depends on features)
1. **HUD / UI System** — depends on: all gameplay systems
2. **Audio System** — depends on: Visual Transition System (layer switch drives music change)
3. **Cinematic / Staging System** — depends on: Narrative State Machine, Dialogue System

### Polish Layer
1. **Localization System** — depends on: Dialogue System, Journal System, HUD/UI

---

## Recommended Design Order

| Order | System | Priority | Layer | Lead Agent | Est. Effort |
|-------|--------|----------|-------|-----------|-------------|
| 1 | Narrative State Machine | MVP | Foundation | narrative-director + game-designer | M |
| 2 | Exploration / Interaction System | MVP | Core | game-designer + ue-blueprint-specialist | M |
| 3 | Memory Fragment System | MVP | Feature | narrative-director + game-designer + unreal-specialist | L |
| 4 | Visual Transition System | MVP | Core | unreal-specialist + technical-artist | M |
| 5 | Character Controller | MVP | Foundation | gameplay-programmer + ue-blueprint-specialist | M |
| 6 | Camera System | MVP | Core | gameplay-programmer | S |
| 7 | Save / Load System | MVP | Core | unreal-specialist | S |
| 8 | Dialogue System | Vertical Slice | Feature | narrative-director + ue-umg-specialist | M |
| 9 | Puzzle System | Vertical Slice | Feature | game-designer + level-designer | M |
| 10 | Journal / Chronicle System | Vertical Slice | Feature+ | narrative-director + ue-umg-specialist | M |
| 11 | HUD / UI System | Vertical Slice | Presentation | ue-umg-specialist + ux-designer | M |
| 12 | Audio System | Vertical Slice | Presentation | audio-director + sound-designer | M |
| 13 | NPC AI System | Alpha | Feature | gameplay-programmer + ai-programmer | M |
| 14 | Artifact / Item System | Alpha | Feature | game-designer | S |
| 15 | World Map System | Alpha | Feature+ | ue-umg-specialist + level-designer | S |
| 16 | Cinematic / Staging System | Alpha | Presentation | narrative-director + unreal-specialist | M |
| 17 | Level Streaming System | Alpha | Foundation | unreal-specialist | M |
| 18 | Localization System | Full Vision | Polish | localization-lead | L |

*Effort: S = 1 session, M = 2–3 sessions, L = 4+ sessions*

---

## Circular Dependencies

- **None detected.** The dependency graph is acyclic. The Narrative State Machine is the
  central hub — all systems read from or write to it, but none of them require each other
  to define its own structure.

---

## High-Risk Systems

| System | Risk Type | Risk Description | Mitigation |
|--------|-----------|-----------------|------------|
| **Memory Fragment System** | Design + Technical | Core differentiator — if it doesn't *feel* right (the transition, the temporal disorientation, the return moment), the entire game is undermined. UE5 visual transition with Lumen is unproven at this fidelity. | Prototype this first, before any other system is built. `/prototype memory-fragment-transition` |
| **Narrative State Machine** | Scope | Narrative games have state explosion risk — the number of flags needed to track story progress can spiral. If under-designed, save files bloat and bugs become untraceable. | Design with a strict serialization contract from day one. All state in one central class. |
| **Visual Transition System** | Technical | 4 distinct visual styles in UE5.7 (Lumen settings, post-process profiles, material overrides) that must transition smoothly without frame hitches during Memory Fragment entry/exit. | Prototype independently early. Use UE5 Level Sequence + post-process blend volumes. |
| **Dialogue System** | Scope | Narrative games need large, flexible dialogue trees. If we build too simple a system, late-game branching breaks it. If too complex, it's a project in itself. | Use a proven dialogue middleware (Articy Draft or Yarn Spinner for UE5) rather than building from scratch. ADR needed. |

---

## Progress Tracker

| Metric | Count |
|--------|-------|
| Total systems identified | 18 |
| Design docs started | 0 |
| Design docs reviewed | 0 |
| Design docs approved | 0 |
| MVP systems designed | 0 / 7 |
| Vertical Slice systems designed | 0 / 5 |

---

## Next Steps

- [ ] `/design-system narrative-state-machine` — first and most critical GDD
- [ ] `/design-system memory-fragment-system` — prototype candidate (high-risk, design early)
- [ ] `/prototype memory-fragment-transition` — validate the core mechanic before building anything else
- [ ] Run `/design-review` on each completed GDD
- [ ] Run `/gate-check pre-production` when all 7 MVP GDDs are approved
