# UX Spec: Dialogue UI

> **Status**: In Design
> **Author**: User + Claude Code (ux-designer)
> **Last Updated**: 2026-04-25
> **Journey Phase(s)**: Exploration (triggered during NPC interactions throughout Acts 1–3)
> **Template**: UX Spec
> **Source GDD**: design/gdd/dialogue-system.md

---

## Purpose & Player Need

The Dialogue UI is the player's voice in Gaia's world. It presents NPC speech, gives the player meaningful response choices, and records those choices in the narrative state — all without breaking the feeling that this is a real conversation with a real character, not a menu selection.

Two simultaneous player needs:
1. **Read and understand** what an NPC is saying — clearly, comfortably, without distraction
2. **Express Gaia's attitude** by choosing a response — not just advance the plot, but communicate something about who Gaia is and how she feels

The system must be completely invisible when it's working. The player should be thinking about the characters, not noticing the UI.

> "选哪个选项不只是剧情进度的按钮，而是在表达盖亚对这件事的态度" — dialogue-system.md Player Fantasy

---

## Player Context on Arrival

**Trigger**: Player interacts (E key / A button) with an NPC who is in a dialogable state.

**Arrival context**:
- Player was exploring, curious about this NPC
- Camera cuts softly from exploration view to dialogue composition (0.2s blend to NPC 3/4 face angle)
- Movement input is suspended — player's full attention is available
- Emotional state: **attentive, open** — they chose to start this conversation

**Critical expectation to meet**: The first line of NPC dialogue should feel like the NPC reacting to *this* player, at *this* moment — not reading from a script. This is a writing responsibility, but the UI must not undermine it with clinical presentation.

---

## Navigation Position

```
Gameplay (Exploration) ──[E key / A button on NPC]──▶ Dialogue UI
                                                            │
                                                  [Player selects option
                                                   / dialogue ends naturally]
                                                            │
                                                     ◀── Gameplay
```

The dialogue UI is a **temporal overlay** — not a separate screen. The world is still visible behind it. Camera is in dialogue composition during this state.

---

## Entry & Exit Points

### Entry

| Entry Source | Trigger | Player carries this context |
|---|---|---|
| NPC `OnInteract()` | E key / gamepad A on NPC within interaction radius | Current NSM state (dialogue conditions evaluated immediately) |

### Exit

| Exit Destination | Trigger | Notes |
|---|---|---|
| Gameplay (resume) | Yarn script reaches end node | Camera blend back; movement restored; NSM dialogue done flag written |
| Gameplay (resume, aborted) | Fragment trigger fires during dialogue | `EndDialogue(Aborted)` called; partial flag written; NPC will resume on next meeting |

**No player-initiated exit.** The player cannot press Esc to leave dialogue mid-conversation. They must see it through or trigger an in-game abort. This is a deliberate narrative design choice — conversations have weight.

---

## Layout Specification

### Information Hierarchy

1. **What the NPC just said** (the NPC's voice — the center of attention)
2. **Who is speaking** (speaker identity — needed to track multi-NPC conversations)
3. **Player's response options** (Gaia's possible attitudes — visible after NPC line finishes)
4. **The world behind the dialogue** (world stays visible at ~30% opacity under dialogue box)

### Layout Zones

Bottom-screen dialogue box, world visible above:

```
┌──────────────────────────────────────────────────────────────────────┐
│                         [WORLD VISIBLE]                              │
│                                                                      │
│           [NPC face in dialogue composition — camera handles this]   │
│                                                                      │
├──────────────────────────────────────────────────────────────────────┤
│  纳雅                          [Speaker label — top-left, handwriting]│
│ ┌────────────────────────────────────────────────────────────────────┐│
│ │                                                                    ││
│ │  "水渠建成的那年，我三岁。我只记得母亲说，那年水第一次流进了南区。" ││
│ │                                                                    ││
│ └────────────────────────────────────────────────────────────────────┘│
│  ┌────────────────────────────────────────┐                          │
│  │ ① 那南区的人怎么说？                   │                          │
│  │ ② 你母亲还在吗？                       │                          │
│  │ ③ 水渠是谁建的？                       │                          │
│  └────────────────────────────────────────┘                          │
└──────────────────────────────────────────────────────────────────────┘
```

**Dialogue box height**: ~30% of viewport height (occupies bottom third)
**Dialogue box width**: Full width, with 48px horizontal margin on each side
**Background**: Journal cream `#F5EDD6` at 85% opacity — world visible behind
**World dim**: No explicit dimming — the dialogue box color contrast is sufficient

### Component Inventory

| Component | Type | Content | Interactive? | Notes |
|-----------|------|---------|--------------|-------|
| Speaker label | Text | NPC name | No | Playfair Display (hand-writing variant), 16pt, `#3D2B1F`; top-left of dialogue box |
| Dialogue box background | Container | Parchment texture at 85% opacity | No | `#F5EDD6`, subtle paper texture, rounded corners 4px |
| NPC line text | Static text | Current Yarn dialogue line | No | Playfair Display 18pt, `#3D2B1F`; appears line by line or instantly per player preference |
| Advance prompt | Icon | "[Space / A]" icon | No | Subtle, appears after line fully displayed; hidden when options are showing |
| Option list container | Container | Options panel | No | Below NPC text box, slightly inset |
| Option item | Button | Player response text | Yes | Max 4 items; 14pt Playfair; hover/focus state: Matriarchal Teal `#4AADAA` left bar + faint background |
| Option number hint | Text | "①②③④" | No | Keyboard shortcut hint; 12pt; shown for KB/Mouse only |

### ASCII Wireframe

```
╔══════════════════════════════════════════════════════════════════════╗
║  [World visible: NPC face at 3/4 angle in dialogue composition]     ║
║                                                                      ║
║                                                                      ║
╠══════════════════════════════════════════════════════════════════════╣
║ 纳雅                                                                 ║
║ ┌──────────────────────────────────────────────────────────────────┐ ║
║ │                                                                  │ ║
║ │  "水渠建成的那年，我三岁。我只记得母亲说，那年水第一次流进       │ ║
║ │   了南区。"                                     [Space/A → ]    │ ║
║ │                                                                  │ ║
║ └──────────────────────────────────────────────────────────────────┘ ║
║                                                                      ║
║  ▌① 那南区的人怎么说？           ← active/hovered option (teal bar) ║
║    ② 你母亲还在吗？                                                  ║
║    ③ 水渠是谁建的？                                                  ║
║                                                                      ║
╚══════════════════════════════════════════════════════════════════════╝
```

---

## States & Variants

| State / Variant | Trigger | What Changes |
|-----------------|---------|--------------|
| **NPC speaking — no options yet** | Yarn line being delivered | Options hidden; advance prompt visible after line complete |
| **Options showing** | Yarn script reaches a player-choice node | Options panel slides in; advance prompt hidden |
| **Single option (auto-advance)** | Only one valid option exists (others hidden by condition) | Option panel shows one item; OR auto-selects after 0.3s delay if it is the "再见" fallback |
| **Loading next line** | Player selected option; Yarn advancing | Brief flash of empty NPC box (< 0.1s); then next line appears |
| **Closing** | Dialogue reaches end node | 0.3s box fade out; camera blend back |
| **Aborted (fragment trigger)** | Fragment fires during dialogue | Immediate dismiss, no animation |
| **Keyboard input mode** | Last input: keyboard | Number key hints ①②③ visible |
| **Gamepad input mode** | Last input: gamepad | Number hints hidden; D-pad up/down highlights; A to confirm |

---

## Interaction Map

*Input methods: Keyboard/Mouse (primary) + Gamepad (partial). No touch.*

| Component | Mouse/Keyboard Action | Gamepad Action | Immediate Feedback | Outcome |
|-----------|----------------------|----------------|-------------------|---------|
| Advance NPC line | Space / Enter / Left-click anywhere in dialogue box | A / Cross | None (instant) | Next Yarn line loads; or options appear |
| Select option (by number) | 1, 2, 3, 4 keys | — | Option highlights briefly | Option selected; Yarn advances |
| Select option (by navigation) | Click option item | D-pad Up/Down + A/Cross | Teal left-bar appears on focused option | Option selected; Yarn advances |
| Hover/focus option | Mouse move over option | D-pad Up/Down | Teal left accent bar; faint `#4AADAA` background | Visual focus feedback |

**No Esc / Back exit.** Player cannot close dialogue with cancel button.

**Gamepad specifics:**
- Dialogue box does not consume left thumbstick (Gaia cannot move)
- A button: advance NPC line when no options; confirm selection when options showing
- D-pad Up/Down: navigate between options when options showing
- B/Circle: no action (cannot exit dialogue)

---

## Events Fired

| Player Action | Event Fired | Payload |
|---|---|---|
| Dialogue begins | `Dialogue.Started(NPCTag, NodeID)` | `NPCTag`, `NodeID` |
| Option selected | `Dialogue.OptionSelected(NPCTag, OptionIndex, OptionText)` | analytics |
| Dialogue completes | `Dialogue.Completed(NPCTag)` | `NPCTag`, `timestamp` |
| Dialogue aborted | `Dialogue.Aborted(NPCTag)` | `NPCTag`, reason |

**State-modifying actions** (handled by `UDialogueSubsystem`, not UI):
- `<<set_flag X>>` commands → NSM.SetFlag
- `<<add_ending_score X N>>` → NSM.IncrementInt
- `Dialogue.NPC.NodeID.Done` flag → written on complete

---

## Transitions & Animations

| Moment | Animation | Duration | Reduced Motion Alternative |
|--------|-----------|----------|---------------------------|
| Dialogue opens | Dialogue box fades in from bottom (slides up 24px + fade) | 0.2s | Instant fade-in |
| Camera to dialogue composition | `SetViewTargetWithBlend` to NPC cam | 0.2s | — (camera system, not UI) |
| Options appear | Options panel slides in (24px from bottom + fade) | 0.15s | Instant appear |
| Option hovered | Left teal bar slides in from left | 0.1s | Instant |
| Option selected | Brief option flash (opacity 0.8 → 1.0) | 0.1s | — |
| Dialogue closes | Dialogue box fades out | 0.3s | Instant fade |
| Camera return | Blend back to Default camera | 0.6s | — (camera system) |

---

## Data Requirements

| Data | Source System | Read / Write | Notes |
|------|--------------|--------------|-------|
| Current NPC dialogue line | `UDialogueSubsystem` via `FOnDialogueLineReady(FDialogueLineData)` | Read | FText; localized |
| Speaker name | `FDialogueLineData.SpeakerTag` → lookup display name | Read | FText |
| Available options (filtered) | `FDialogueLineData.Options[]` | Read | Only valid options (hidden ones already excluded by Yarn subsystem) |
| Input device type | `APlayerController` | Read | For number hint visibility |

**Important:** Options are pre-filtered by `UDialogueSubsystem` — unavailable options are never passed to the widget. The UI never receives "greyed out" options. This is a core design commitment (dialogue-system.md).

---

## Accessibility

| Requirement | Implementation |
|-------------|---------------|
| Keyboard-only navigation | Space/Enter advance; number keys 1–4 for options; full keyboard access |
| Gamepad navigation | D-pad Up/Down between options; A to advance/select; complete gamepad access |
| Text contrast | `#3D2B1F` on `#F5EDD6` (85% opacity box on world background) — contrast ≥ 4.5:1 at 85% opacity |
| Color independence | Options identified by number AND text AND position — not color alone |
| Font size | 18pt NPC text; 14pt option text — minimum readable |
| Focus indicators | Teal left bar + background on focused option; visible in both KB and gamepad modes |
| No time pressure | No timeout; player reads at their own pace |
| Screen reader | FText pipeline; all dialogue text is machine-readable |

---

## Localization Considerations

| Element | Concern | Priority |
|---------|---------|----------|
| NPC dialogue line (100–400 chars Chinese) | German/French expansion may overflow two-line estimate | 🔴 HIGH — dialogue box must allow 3–4 lines of text; never clip |
| Option text (≤20 Chinese characters) | "那南区的人怎么说？" = 9 chars; English "What did the people in the south district say?" = 46 chars | 🔴 HIGH — options may need to wrap to 2 lines; option button height must be dynamic |
| Speaker name | Short; low risk | 🟢 LOW |
| Number hints ①②③ | Universal symbols; no translation needed | 🟢 LOW |

**Rule:** All text containers must be dynamic-height with a maximum (scroll if overflow). No fixed-height clipping.

---

## Acceptance Criteria

- [ ] **Trigger**: Interacting with an NPC who has dialogue triggers the dialogue box to appear within 150ms (first line visible).
- [ ] **Options**: Only valid options (per Yarn condition evaluation) appear in the options list. No greyed-out or hidden-but-present options.
- [ ] **Max options**: Never more than 4 options visible simultaneously.
- [ ] **Selection**: Pressing 1–4 (keyboard) or D-pad + A (gamepad) selects the correct option and advances the Yarn script within 100ms.
- [ ] **No exit**: Pressing Esc, B/Circle, or Back during dialogue does NOT close the dialogue UI.
- [ ] **Close behavior**: When dialogue ends naturally, box fades out within 0.3s, camera returns within 0.6s, movement input is restored.
- [ ] **NSM writes**: After dialogue ends with Complete reason, `Dialogue.NPC.NodeID.Done` flag is present in NSM.
- [ ] **Repeat visit**: Second interaction with same NPC starts from `NPC.Tag.LastNodeID` (not from `DefaultStart`).
- [ ] **Gamepad contrast**: Focused option has a visible teal left-bar when navigated via D-pad (not just mouse hover).
- [ ] **Font contrast**: NPC text (#3D2B1F) on dialogue box (#F5EDD6 at 85% opacity) meets ≥ 4.5:1 ratio.

---

## Open Questions

- **OQ-UX-DLG-01**: Should the player be able to "review" the previous NPC line? The GDD has this as `OQ-02` in dialogue-system.md. Currently: no. If added, a "scroll up" gesture or Back-button-on-first-option behavior would show the previous line. Flag for post-vertical-slice consideration.
- **OQ-UX-DLG-02**: When Gaia is speaking (future — if Gaia lines are added), should her text appear in the same box with a different speaker label, or should there be a small secondary element on the other side of the screen? Currently: same box, different label.
