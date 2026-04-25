# HUD Design

> **Status**: In Design
> **Author**: User + Claude Code (ux-designer)
> **Last Updated**: 2026-04-25
> **Template**: HUD Design
> **Source GDD**: design/gdd/hud-ui-system.md

---

## HUD Philosophy

**Firewatch model: nearly HUD-free, atmosphere-first.**

The HUD exists to serve one purpose: tell the player what they can do *right now*, in the moment they need to know it. Between those moments, the screen belongs entirely to the world.

There is no health bar. There is no minimap. There is no quest tracker. There are no floating objective markers. The only things that appear on the HUD are things that have no alternative — information that cannot be communicated through the world itself.

> "好的界面是不被注意到的。" — hud-ui-system.md Player Fantasy

**The HUD-free rule in practice:** If a proposed HUD element could be communicated through environment design, NPC dialogue, or player memory — it should not be on the HUD. The HUD is a last resort, not a default channel.

---

## Information Architecture

### Full Information Inventory

All information that game systems have flagged as needing HUD communication (from GDD UI Requirements sections):

| # | Information | Source System |
|---|------------|--------------|
| 1 | "This object can be interacted with, and how" | Exploration/Interaction |
| 2 | "Hold to examine (vs short-press to interact)" | Exploration/Interaction |
| 3 | "A journal entry was just unlocked" | Journal/Chronicle |
| 4 | "A memory fragment is about to begin / is loading" | Memory Fragment |
| 5 | "Gaia's internal thought / hint about a puzzle nearby" | Puzzle System |
| 6 | "Dialogue UI is active" (implicit — the dialogue box IS the HUD element) | Dialogue |
| 7 | "Journal is available (key reminder, optional)" | HUD System |

### Categorization

| Item | Category | Reasoning |
|------|----------|-----------|
| 1 — Interaction available | **Contextual** | Only visible when Gaia is within interaction radius of an interactable |
| 2 — Hold to examine hint | **Contextual** | Appears as part of the interaction hint; same zone |
| 3 — Journal entry unlocked | **Contextual** | Pulse notification: 1.5s, then gone |
| 4 — Fragment loading / entering | **Contextual** | During transition only; disappears immediately after |
| 5 — Gaia's puzzle hint monologue | **Contextual** | After 90s proximity, one-time per InProgress session |
| 6 — Dialogue box | **Must Show** (when dialogue active) | The dialogue UI is itself the HUD element |
| 7 — Journal key reminder | **On Demand / Hidden** | Not shown during normal play; mentioned in first-time tutorial moment only |

**Result:** Zero Must Show elements during exploration. The HUD is purely contextual and on-demand. The screen is clean by default.

---

## Layout Zones

```
╔═══════════════��══════════════════════════════════════════════════════╗
║                                                                      ║
║                                                                      ║
║                         [WORLD]                                      ║
║                                                                      ║
║              ▼▼▼▼ interaction hint zone ▼▼▼▼                        ║
║                  [Interact Hint]                                     ║
║                  appears ~40% down from top                         ║
║                  horizontally centered                               ║
║                                                                      ║
║                                                                      ║
║  [Gaia monologue]                    [Journal notification]          ║
║  left lower third                    right lower corner              ║
║                                                                      ║
╚══════════════════════════════════════════════════════════════════════╝
```

**Zone definitions:**

| Zone | Position | Contents |
|------|----------|---------|
| Interaction Hint Zone | Center-screen, ~35–45% from top | Interact prompt (key + hint text); hold-examine progress ring |
| Monologue Zone | Lower-left, 8% from bottom, 6% from left | Gaia's internal thoughts (puzzle hints, observations) |
| Notification Zone | Lower-right, 8% from bottom, 6% from right | Journal unlock pulse; future: area name watermark (if added) |
| Fragment Prompt Zone | Upper-center, 12% from top | "记忆碎片：[Fragment name]" — appears during transition, fades after 1s |

---

## HUD Elements

### Element 1: Interaction Hint

**Category:** Contextual
**Trigger:** `UInteractionComponent` focus changes — FocusScore selects a new `IInteractable`
**Dismissal:** Focus lost, or interaction completed

| Property | Value |
|----------|-------|
| Position | Center screen, 38% from top (above natural focal point) |
| Icon | Key icon (adaptive: E key or A button) + progress ring for hold actions |
| Text | `IInteractable::GetInteractHintText()` — max 12 Chinese characters |
| Font | Playfair Display 16pt, `#F5EDD6` (light on dark world) or `#3D2B1F` (dark on light environment) — auto-contrast |
| Animation in | Fade 0.15s from transparent |
| Animation out | Fade 0.15s to transparent |
| Hold examine ring | Arc progress 0–0.5s; encircles key icon; Matriarchal Teal `#4AADAA` |

**Input icon adaptation:**
- Keyboard/Mouse mode: shows keyboard key glyph (E)
- Gamepad mode: shows gamepad face button glyph (A/Cross)
- Switches within 0.2s of last detected input device change

**Auto-contrast note:** In dark scenes (Exodus layer, interior ruins), hint text is light cream on a subtle dark scrim. In light scenes (Founding layer, exterior), hint text is dark brown. Driven by `UHUDSubsystem` sampling the pixel luminance behind the hint zone (or set per level by level designer).

### Element 2: Journal Unlock Notification (Pulse)

**Category:** Contextual
**Trigger:** `UJournalSubsystem::FOnEntryUnlocked` → `UHUDSubsystem::QueueNotification()`
**Duration:** 1.5s total; queue interval 0.3s between consecutive notifications

| Property | Value |
|----------|-------|
| Position | Lower-right corner; right-aligned; 8% margin from bottom and right |
| Icon | 羽毛笔 (feather-pen) glyph, Memory Gold `#E8B84B` |
| Text | Entry short title (≤10 Chinese characters) |
| Font | Playfair Display 14pt, `#3D2B1F` |
| Animation in | Slide in from right (32px) + fade, 0.2s |
| Hold | 1.0s static |
| Animation out | Fade out, 0.3s |
| Sound | Extremely subtle ink-drop sound (audio-system.md defines this) |

**Queue behavior:** Multiple unlocks within the same frame are queued and displayed with 0.3s delay between each. Maximum queue size: 5 notifications. If queue exceeds 5, oldest (not newest) are dropped.

### Element 3: Gaia Monologue / Puzzle Hint

**Category:** Contextual
**Trigger:** Puzzle system fires hint after 90s proximity + `PuzzleState == InProgress`
**Duration:** Displayed for 3s then fades; not repeatable in same InProgress session

| Property | Value |
|----------|-------|
| Position | Lower-left corner; left-aligned; 8% margin from bottom and left |
| Style | Italic Playfair Display 16pt; `#3D2B1F` (dark) or `#F5EDD6` (light) per scene luminance |
| Max lines | 2 lines maximum (~30 Chinese characters) |
| Animation in | Fade in, 0.3s |
| Hold | 3.0s |
| Animation out | Fade out, 0.5s |
| Background | None — no box or scrim; text floats over world |
| Sound | None |

**Tone note:** Gaia's monologue is in first-person, present tense. It reads as Gaia thinking aloud to herself, not as a game system giving a hint. Example: "如果那个刻符对应的是季节顺序……" — not "提示：按顺序触碰符文".

### Element 4: Fragment Entry Prompt

**Category:** Contextual
**Trigger:** `UMemoryFragmentSubsystem` state changes to `Transitioning_In`
**Duration:** Duration of transition animation (~2.5s); fades 1s after `InFragment` state is reached

| Property | Value |
|----------|-------|
| Position | Upper-center; 12% from top; horizontally centered |
| Content | "记忆碎片：[Fragment short name]" |
| Font | Playfair Display 18pt; `#E8B84B` Memory Gold |
| Animation in | Fade in 0.5s (concurrent with visual transition) |
| Animation out | Fade out 1.0s (after InFragment begins) |
| Background | None |

---

## Dynamic Behaviors

| Behavior | Trigger | Effect |
|----------|---------|--------|
| **Full HUD hide** | `UMemoryFragmentSubsystem` → `Transitioning_In` or `Transitioning_Out` | All HUD elements instantly hidden (opacity → 0); interaction hints, notifications frozen |
| **Full HUD hide** | Sequencer / cinematic takeover | Same — all hidden |
| **HUD restore** | Fragment returns to `Idle` (gameplay resumed) | HUD elements re-enable; focus state re-evaluated immediately |
| **Dialogue overlay** | `UDialogueSubsystem::BeginDialogue()` | Interaction hint hidden; notification queue frozen (resumes after dialogue) |
| **Journal overlay** | `UHUDSubsystem::OpenJournal()` | All HUD elements hidden; journal replaces HUD layer entirely |
| **Input mode switch** | Player switches from keyboard to gamepad (or vice versa) | Interaction hint icon and close hint labels update within 0.2s |

---

## Platform & Input Variants

| Platform / Input | Variation |
|-----------------|-----------|
| Keyboard/Mouse | Interaction hint shows keyboard key glyphs; number hints ①②③ in dialogue |
| Gamepad | Interaction hint shows gamepad button glyphs; D-pad navigation indicators in journal |
| Console (future) | Safe Zone padding applied via `GetSafeZone()` to all edge-positioned elements (notification zone, monologue zone) |
| 1280×720 (minimum) | All elements scale; minimum font size 14pt maintained at all resolutions |
| 3840×2160 (4K) | All elements use percentage-based anchors; no pixel-locked positions |

---

## Accessibility

| Requirement | Implementation |
|-------------|---------------|
| Interaction hint contrast | Auto-contrast: dark text on light scenes, light text on dark scenes — minimum 4.5:1 in all cases |
| Color independence | Icon + text combination — never icon alone; key identity not conveyed by color only |
| Font sizes | Minimum 14pt on all HUD elements at 1280×720 |
| Reduced motion | Interaction hint appears instantly (no fade) when reduced motion is active; notification slides are instant |
| Notification accessibility | Journal unlock notification is purely informational — missing it does not block any action; player can always check the journal |
| Monologue accessibility | Gaia's puzzle hint text is also available via the interaction system (examining the puzzle element gives similar hint) — redundant path |

---

## Open Questions

- **OQ-HUD-01**: Should there be a very subtle "area name" watermark (like Firewatch area labels) when Gaia enters a new ruin zone? Would strengthen sense of place without being intrusive. Currently not in GDD — flag for narrative-director review.
- **OQ-HUD-02**: Interaction hint auto-contrast (sampling pixel luminance behind hint zone) adds implementation complexity. Simpler alternative: use a minimal text shadow / outline that works on both light and dark backgrounds. Recommend implementation team decide at story level.
- **OQ-HUD-03**: Should Gaia's monologue always appear in the lower-left, or should it appear near the puzzle element in world space (3D UI via `UWidgetComponent`)? World-space would feel more diegetic. Tradeoff: harder to read, potential occlusion by geometry. Currently: screen-space lower-left.

---

## Acceptance Criteria

- [ ] **Clean screen**: During normal exploration with no nearby interactables and no recent journal unlocks, the HUD is fully transparent — no visible elements except the world.
- [ ] **Interaction hint appears**: When Gaia is within 180cm of an `IInteractable` object and it has the highest FocusScore, the interaction hint appears within 150ms.
- [ ] **Hint icon adapts**: When the player last used keyboard input, the hint shows keyboard key icon; when gamepad, shows gamepad button icon. Icon updates within 0.2s of switching devices.
- [ ] **Hold ring**: For hold-examine actions, a teal arc progress ring encircles the key icon and fills over 0.5s; interaction triggers at completion.
- [ ] **Notification queue**: When 3 journal entries unlock within 1 second, they appear as 3 separate notifications, each 0.3s after the last (not simultaneously stacked).
- [ ] **HUD full-hide on fragment**: When Memory Fragment transition begins, all HUD elements disappear immediately (before visual transition animation begins).
- [ ] **HUD restore after fragment**: After returning from a fragment, HUD elements re-enable within 0.5s; interaction hints re-evaluate correctly.
- [ ] **Monologue timing**: Gaia's puzzle hint monologue appears no earlier than 90 seconds after `PuzzleState = InProgress` begins, and no more than once per InProgress session.
- [ ] **No persistent elements**: No health bars, no quest markers, no minimaps, no objective counters appear during exploration under any circumstances.
- [ ] **Safe Zone compliance**: All edge-positioned HUD elements (notification zone, monologue zone) respect the console Safe Zone margins on constrained screen areas.
