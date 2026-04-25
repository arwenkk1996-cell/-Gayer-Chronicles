# UX Spec: Journal Interface

> **Status**: In Design
> **Author**: User + Claude Code (ux-designer)
> **Last Updated**: 2026-04-25
> **Journey Phase(s)**: Exploration (available throughout Acts 1–3 during gameplay)
> **Template**: UX Spec
> **Source GDD**: design/gdd/journal-chronicle-system.md

---

## Purpose & Player Need

The Journal Interface is Gaia's private diary — the player's **only tool for tracking narrative progress** and re-reading Gaia's first-person perspective on her discoveries. It serves two needs simultaneously:

1. **Narrative re-reading**: The player revisits fragments they've seen, re-reading Gaia's written thoughts as they accumulate and evolve. When a RevealText fires, the player discovers that Gaia has changed her mind about something.
2. **Soft progress check**: The four time-axis columns show which areas have been explored and which still have gaps, without reducing the game to a checklist.

What this screen deliberately does NOT do: act as a quest tracker, objective marker, or database. The player arrives wanting to re-experience Gaia's thinking, not to manage tasks. The moment the screen feels like a status dashboard, it has failed.

> "翻开日志，不是在查数据库——是在重新翻盖亚的思考。" — journal-chronicle-system.md Player Fantasy

---

## Player Context on Arrival

**First time** (after first journal entry unlocks):
- Player has just seen the HUD notification pulse (羽毛笔 feather-pen glow)
- Curious about what was just recorded
- Possibly slightly interrupted from exploration
- Emotional state: **curious, slightly pleased** — something was worthy of note

**Returning visits** (Acts 2–3):
- Player has accumulated several entries
- Arrives wanting to re-read a specific entry, or to check whether a new discovery added a RevealText supplement
- Emotional state: **reflective, investigative** — tracking Gaia's evolving understanding

**Key design constraint**: The player opens the journal voluntarily, never by game-forced redirect. The interface must respect the player's moment — it pauses time and invites attention, it does not demand it.

---

## Navigation Position

```
Gameplay (Exploration) ──[J key / Gamepad Back]──▶ Journal Interface
                                                          │
                                                  [Esc / J / Back]
                                                          │
                                                   ◀── Gameplay
```

The journal is a **direct overlay on gameplay** — not a separate scene, not a full menu stack. It lives at the same level as the HUD, layered above it. No loading, no level transition.

Relationship to other future screens:
- Accessible from: Gameplay only (not from Main Menu, not from Pause)
- Not accessible during: Fragment transitions, Fragment InProgress, active Dialogue

---

## Entry & Exit Points

### Entry

| Entry Source | Trigger | Player carries this context |
|---|---|---|
| Gameplay exploration | J key (keyboard) | Current world position, recent discoveries |
| Gameplay exploration | Gamepad `Back` / `Select` button | Same |
| HUD notification pulse tap/click (future) | Click on feather-pen notification | May pre-select the newly unlocked entry |

### Exit

| Exit Destination | Trigger | Notes |
|---|---|---|
| Gameplay (resume) | Esc key | 0.3s close animation, game clock resumes |
| Gameplay (resume) | J key (toggle) | Same |
| Gameplay (resume) | Gamepad `Back` / `Select` | Same |

**No one-way exits.** The journal is a pause-and-read overlay — the player always returns to the exact gameplay state they left.

---

## Layout Specification

### Information Hierarchy

In priority order (what the player needs to find fastest):

1. **The text of the selected entry** (Gaia's voice — the reason for opening the journal)
2. **Which time axis the entry belongs to** (orientation)
3. **RevealText supplement** (new perspective — appears below main text if triggered)
4. **Other available entries** (navigation — what else has been discovered)
5. **Unfilled positions on each axis** (ghost nodes — sense of progress without enumeration)

### Layout Zones

Double-page book layout, consistent with Art Bible Section 7 ("UI as Gaia's diary"):

```
┌──────────────────────────────────────────────────────────────────────────────┐
│  [PAGE CURL DECORATION]          盖亚的日志                  [关闭 Esc/J]    │
├─────────────────────────────────────┬──────────────────────────────────────── │
│           LEFT PAGE                 │              RIGHT PAGE                 │
│  Time Axis Navigation               │  Entry Detail                           │
│                                     │                                         │
│  Four vertical columns,             │  Title (Playfair Display, 20pt)        │
│  labelled at top                    │  Era badge                              │
│  Nodes: solid circles = unlocked    │  ─────────────────────────             │
│  Ghost rings = adjacent-only hint   │  Body text (Playfair, 18pt)            │
│                                     │  (scrollable if long)                   │
│  Colors per era (see table below)   │                                         │
│                                     │  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─       │
│                                     │  补记 (RevealText, darker ink)          │
│                                     │  (appears only when bHasReveal = true)  │
│                                     │                                         │
└─────────────────────────────────────┴─────────────────────────────────────────┘
```

**Zone dimensions (relative, responsive):**
- Left page: 35% of total width
- Right page: 65% of total width
- Total height: 80% of viewport height (centered, with journal-frame decoration)
- Minimum supported resolution: 1280×720 — all text at minimum sizes must remain readable

### Time Axis Era Colors

| Era / Layer | Column Label | Node Color | Text Color |
|-------------|-------------|------------|-----------|
| 建国 (Founding) | 建国 | Warm Yellow `#D4A843` | `#3D2B1F` |
| 旧世界 (OldWorld) | 旧世界 | Faded Brown `#8B6E4E` | `#3D2B1F` |
| 逃亡 (Exodus) | 逃亡 | Cool Gray `#7A8FA0` | `#3D2B1F` |
| 现在 (Present) | 现在 | Warm Orange `#C4873A` (Ruin Ochre) | `#3D2B1F` |

All colors from Art Bible Section 4 (四时代色调).

### Component Inventory

**Left Page:**

| Component | Type | Content | Interactive? | Notes |
|-----------|------|---------|--------------|-------|
| Era column labels | Static text label | Era name (short, ≤4 chars) | No | Playfair 14pt, top of each column |
| Timeline axis line | Decorative | Thin vertical line | No | Drawn in era color |
| Unlocked entry node | Button/selectable | Solid filled circle | Yes | Click/select → load entry on right page |
| Ghost hint node | Decorative | Faint dashed ring | No | Shown only if adjacent unlocked entries exist |
| Active/selected node indicator | State overlay | Slightly larger ring around selected node | No | Shows which entry is currently on right page |

**Right Page:**

| Component | Type | Content | Interactive? | Notes |
|-----------|------|---------|--------------|-------|
| Entry title | Static text | `FJournalEntry.Title` | No | Playfair Display 20pt, bold |
| Era badge | Colored chip | Era name | No | Background = era color, small 12pt |
| Divider rule | Decorative | Thin horizontal line | No | Ink-drawn style |
| Body text | Scrollable text | `FJournalEntry.BodyText` | Scroll only | Playfair Display 18pt, `#3D2B1F` |
| Reveal divider | Decorative | Dashed rule | No | Visible only when bHasReveal = true |
| Reveal text | Scrollable text | `FJournalEntry.RevealText` | Scroll only | Playfair 18pt, slightly darker `#2A1A0F` |
| Empty state placeholder | Static text | "选择左侧的记录来阅读" | No | Shown when no entry is selected |

**Frame / Global:**

| Component | Type | Content | Interactive? | Notes |
|-----------|------|---------|--------------|-------|
| Journal book frame | Decorative | Parchment texture, page curl corner | No | Art Bible `#F5EDD6` background |
| Close hint | Text label | "[Esc] 关闭" (keyboard) / "[Back] 关闭" (gamepad) | No | Adaptive to input device |
| Page divider seam | Decorative | Center binding shadow | No | Separates left/right pages |

### ASCII Wireframe

```
╔═══════════════════════════════════════════════════════════════════════════════╗
║  ∿ 盖亚的日志                                              [Esc] 关闭  ∿    ║
╠═══════════════════════════════════╦═══════════════════════════════════════════╣
║                                   ║                                           ║
║  建国   旧世界   逃亡   现在      ║  水渠石碑                                ║
║  ──┬──  ──┬──  ──┬──  ──┬──     ║  ╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌     ║
║    │       │       │       │      ║  [现在]                                   ║
║    ●       │       │       │      ║                                           ║
║    │       │       │       │      ║  "水渠比我想象的要古老得多。石碑上       ║
║    │       ●       │       │      ║   的刻痕——某种符文，或者日历？——         ║
║    │       │       │       │      ║   让我想起纳雅说过的话。也许这里         ║
║    │       ●       │       │      ║   不只是一个蓄水池。"                    ║
║    │       │       │       │      ║                                           ║
║    │       │       ●       │      ║   …                                       ║
║    │       │       │       │      ║                                           ║
║   ◌        │       │       ●     ║  ╌ ╌ ╌ ╌ ╌ ╌ 补记 ╌ ╌ ╌ ╌ ╌ ╌ ╌ ╌     ║
║    │       │       │       │      ║                                           ║
║    │       │       │       ●     ║  "原来这个水渠的设计来自于……              ║
║    │       │       │       │      ║   那时候的人比我们想象的走得更远。"      ║
║    ●       │       │       │      ║   ↕ (可滚动)                             ║
║            │       │       │      ║                                           ║
╚═══════════════════════════════════╩═══════════════════════════════════════════╝
```

Legend: `●` = unlocked entry node, `◌` = ghost hint (adjacent only), `[现在]` = era badge

---

## States & Variants

| State / Variant | Trigger | What Changes |
|-----------------|---------|--------------|
| **Default — entry selected** | Any unlocked node is selected | Right page shows entry title + body |
| **Empty right page** | Journal opens and no entry has been selected yet; OR first time player opens journal | Right page shows placeholder text "选择左侧的记录来阅读" |
| **Entry with RevealText visible** | `bHasReveal = true` AND reveal condition already triggered | Dashed rule + RevealText section appears below body |
| **Entry without RevealText** | `bHasReveal = false` OR reveal not yet triggered | No dashed rule, no RevealText section |
| **New entry highlight** | Entry was unlocked in this session (within last 60s) | Node has golden glow `#E8B84B` that fades over 1.5s on first view |
| **RevealText first view** | Player opens an entry whose RevealText just triggered | RevealText fades in 0.5s after body text is displayed |
| **Single axis populated** | Early game — only Present axis has entries | Only one axis has nodes; others are empty lines |
| **Keyboard navigation mode** | Last input device was keyboard | Close hint shows "[Esc] 关闭" |
| **Gamepad navigation mode** | Last input device was gamepad | Close hint shows "[Back] 关闭"; D-pad navigation indicators visible |
| **Scrollable right page** | Body text + RevealText exceeds page height | Scroll indicator (subtle) appears; mouse wheel / stick scroll active |

---

## Interaction Map

*Input methods: Keyboard/Mouse (primary) + Gamepad (partial). No touch.*

| Component | Mouse/Keyboard Action | Gamepad Action | Immediate Feedback | Outcome |
|-----------|----------------------|----------------|-------------------|---------|
| Unlocked entry node | Left-click | D-pad navigate to node + A/Cross | Node ring highlight brightens; right page cross-fades (0.15s) | Right page shows selected entry |
| Navigate between axes | Tab (left → right through axes) | D-pad Left / Right | Focus moves to adjacent axis column | Active axis highlighted with thin line underline |
| Navigate between nodes (within axis) | Arrow Up / Arrow Down | D-pad Up / Down | Node highlight advances | Next/previous node in same axis focused |
| Scroll right page | Mouse wheel; Page Up/Down | Right stick Y-axis | Content scrolls smoothly | More body text or RevealText revealed |
| Close journal | Esc; J key | Back / Select | 0.3s book-close animation | Journal closes; game clock resumes |
| Ghost hint node | Not interactive | Not interactive | None | Decorative only |
| Open journal (from gameplay) | J key | Back / Select | 0.3s book-open animation; game pauses | Journal opens; empty or last-selected entry shown |

**Gamepad navigation rules:**
- D-pad Left/Right: move between axes (wraps L→R)
- D-pad Up/Down: move between nodes within current axis (wraps top/bottom)
- A/Cross: confirm selection (load entry on right page)
- B/Circle or Back: close journal
- Right stick: scroll right page text
- No analog movement — all navigation is discrete (node to node)

**Keyboard focus order (Tab sequence):**
建国 axis nodes (top to bottom) → 旧世界 axis nodes → 逃亡 axis nodes → 现在 axis nodes → Close button

---

## Events Fired

| Player Action | Event Fired | Payload / Data |
|---|---|---|
| Open journal | `Journal.Opened` | `timestamp` |
| Select entry node | `Journal.EntryViewed(EntryID)` | `EntryID`, `TimeLayer`, `bHasReveal` |
| View RevealText for first time | `Journal.RevealTextFirstViewed(EntryID)` | `EntryID` — analytics only, no state change |
| Close journal | `Journal.Closed` | `timestamp`, `entriesViewedThisSession: int` |

**State-modifying actions:**
None — the journal is read-only. All state (unlock status, RevealText trigger) is managed by `UJournalSubsystem` + NSM. The UI only reads.

---

## Transitions & Animations

| Moment | Animation | Duration | Reduced Motion Alternative |
|--------|-----------|----------|---------------------------|
| Journal opens | Book-flip from right (pages unfurl from right edge) | 0.3s | Instant fade-in (opacity 0→1) |
| Journal closes | Book-flip back to right edge | 0.3s | Instant fade-out |
| Node selected | Right page cross-fade (old content fades out, new fades in) | 0.15s | Instant swap |
| New entry node glow | `#E8B84B` radial glow fades out | 1.5s from first entry view | Skip glow |
| RevealText first view | Fade in from 0 opacity, 0.5s delay after body text loads | 0.5s | Instant appear |
| Entry scroll | Smooth CSS-style inertia scroll | Natural | Instant jump-scroll |

**Motion sickness note:** The book-flip animation moves across ~30% of screen width. For players with motion sensitivity, a reduced-motion toggle (system preference honored on PC, or in-game accessibility setting) should disable the flip and use a fade instead.

---

## Data Requirements

| Data | Source System | Read / Write | Notes |
|------|--------------|--------------|-------|
| All unlocked `FJournalEntry` records | `UJournalSubsystem::GetEntriesForLayer(ETimeLayer)` | Read | Called on open; one call per axis |
| Total entries per layer (for axis position) | `UJournalSubsystem` | Read | `TotalEntriesInLayer` used in axis position formula |
| `bHasReveal` per entry | `FJournalEntry.bHasReveal` | Read | Controls RevealText section visibility |
| Entry body text (`BodyText`, `RevealText`) | `FJournalEntry` | Read | FText, goes through localization |
| Last selected entry (to restore state on reopen) | Local widget state (not persisted) | Read/Write (session only) | Not saved to disk |
| Input device type (keyboard vs gamepad) | `APlayerController::GetLastInputDeviceType()` | Read | For close hint icon switching |

**No persistent UI state.** The journal does not write to NSM or save data. It is purely a display consumer.

---

## Accessibility

*Accessibility tier: not yet formally defined. The following meets WCAG AA as a baseline.*

| Requirement | Implementation |
|-------------|---------------|
| Keyboard navigation | Tab moves between all interactive nodes + close button; focus ring always visible |
| Gamepad navigation | Full D-pad navigation; all nodes reachable without analog stick |
| Text contrast | `#3D2B1F` (dark brown) on `#F5EDD6` (cream) = ~9:1 contrast ratio (exceeds WCAG AA) |
| RevealText contrast | `#2A1A0F` on `#F5EDD6` = ~12:1 contrast ratio |
| Color independence | Era columns labeled with text names; era colors are supplemental, not the only identifier |
| Minimum font size | Body text: 18pt; Labels: 14pt — meets minimum readable requirements |
| Focus indicators | All selectable nodes must have a visible focus ring when navigated via keyboard or gamepad |
| Screen reader | Entry body text is `FText` and readable; era labels are text, not image-only |
| Reduced motion | Book-flip animation replaced with fade when system reduced-motion is active |
| Scroll affordance | Scroll indicator visible when content overflows; not hidden entirely |

**Open accessibility question (OQ-UX-JOUR-01):** Should the journal have a text-size scaling option (e.g., 150% or 200% text size for low-vision players)? This would require the right page layout to be fully dynamic-height. Recommend flagging for Full Vision milestone.

---

## Localization Considerations

| Element | Concern | Priority |
|---------|---------|----------|
| Body text (100–300 characters Chinese) | German/French translations may expand by 40–80% in character count | 🔴 HIGH — right page must be scrollable for all languages |
| Era column labels (≤4 Chinese characters) | "建国" = 2 chars; equivalent may be longer in other languages | 🟡 MEDIUM — test with "Founding Era" (English) to verify column width holds |
| Entry title (≤10 characters Chinese) | "水渠石碑" = 4 chars; English equivalent "The Waterway Stele" = 18 chars | 🟡 MEDIUM — title must allow 2-line wrap, not single-line clamp |
| Close hint label | "[Esc] 关闭" is short; all languages short | 🟢 LOW |
| RevealText supplemental | Same concern as body text | 🔴 HIGH — must scroll |
| "选择左侧的记录来阅读" (placeholder) | ~20 chars; German equivalent ~30 chars | 🟢 LOW — single line with generous width |

**Rule:** Right page must never clip text — it must always provide a scrollable container. No fixed-height text clipping.

---

## Acceptance Criteria

- [ ] **Performance**: Journal opens (first frame visible) within 150ms of J key press from any point in gameplay exploration.
- [ ] **Navigation**: All unlocked journal entries are visible as solid nodes on their correct time-axis column (verified against NSM flag state).
- [ ] **Selection**: Clicking or gamepad-selecting an entry node loads the correct entry body text on the right page within 150ms, with a visible 0.15s cross-fade.
- [ ] **RevealText**: When `bHasReveal = true` and reveal condition is met, the RevealText section (with dashed divider) appears below the body text. When `bHasReveal = false`, no RevealText section is shown.
- [ ] **Close behavior**: Pressing Esc, J, or gamepad Back closes the journal within 0.3s, game clock resumes, and Gaia can move again within 0.1s of the close animation completing.
- [ ] **Game pause**: All timers and gameplay events are paused while the journal is open (including puzzle hint timers, auto-save triggers).
- [ ] **Keyboard navigation**: Tab key navigates through all unlocked nodes and the close button in the defined focus order. Focus ring is visible on each focused element.
- [ ] **Gamepad navigation**: D-pad navigates between axes (Left/Right) and between nodes within an axis (Up/Down). A/Cross selects. B/Circle or Back closes.
- [ ] **Contrast**: Body text (#3D2B1F on #F5EDD6) measures ≥ 4.5:1 contrast ratio (WCAG AA) at all specified font sizes.
- [ ] **Empty state**: First time the journal opens (no entry yet selected), the right page shows the placeholder text instead of blank space.
- [ ] **Ghost nodes**: Adjacent-only ghost hint rings (◌) appear only when the player has unlocked at least one entry on a given axis AND there are undiscovered entries immediately adjacent (one position above or below). Ghost nodes are not interactive.
- [ ] **Input icon adaptation**: Close hint shows keyboard icon (`[Esc]`) when last input was keyboard; shows gamepad icon (`[Back]`) when last input was gamepad.

---

## Open Questions

- **OQ-UX-JOUR-01**: Text-size scaling accessibility option (Full Vision milestone consideration — see Accessibility section).
- **OQ-UX-JOUR-02**: Should the journal remember the last-selected entry across sessions, or always open to the most recently unlocked entry? Currently: opens to most recently unlocked (simplest). No persistent UI state in save data.
- **OQ-UX-JOUR-03**: When all entries in a layer are unlocked, should there be a visual celebration state (e.g., a subtle golden shimmer on the completed axis)? Not in GDD spec — flag for future narrative reward design.
- **OQ-UX-JOUR-04**: Does the player journey from Main Menu need to eventually link to a "resume / journal preview" state? Out of scope for this spec — Main Menu UX spec should address this.
