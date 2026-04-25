# ADR-002: Dialogue Middleware — Yarn Spinner for Unreal (with UE5.7 Compatibility Note)

**Status:** Superseded — 引擎迁移至 Godot 4 (2026-04-25) (with implementation-time verification required)
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)

---

## Context

Gaia Chronicles requires a branching dialogue system with:
- Conditional branches based on NSM flags
- Side-effect commands (set flags, increment scores)
- NPC node memory (resume from last conversation point)
- Large dialogue volumes (5 NPCs across 5 areas, each with multi-branch trees)
- Non-linear narrative (player choices affect world state)

Three candidates were evaluated:

| Option | System | Source | Cost | UE5.7 Risk | Notes |
|--------|--------|--------|------|-----------|-------|
| A | Yarn Spinner for Unreal | Open source (YarnSpinner/yarn-spinner-unity ported to UE) | Free | ⚠️ MEDIUM — UE5.7 compat unconfirmed | GDD's chosen option; widely used in indie narrative games |
| B | Articy Draft 3 + UE5 plugin | Commercial (Articy) | ~$149/seat | ✅ LOW — dedicated UE5 support | Heavier tool; cost; overkill for scope |
| C | Custom minimal dialogue tree | Internal | Free | ✅ LOW — we write it | Control; but re-inventing the wheel; maintenance burden |

---

## Decision

**Accept Yarn Spinner for Unreal as the dialogue middleware, with a mandatory
compatibility verification step before the Dialogue System implementation sprint begins.**

**Verification procedure (must be completed before TR-dlg-001 implementation):**
1. Clone the latest `YarnSpinner-Unreal` repository
2. Verify the plugin builds cleanly against UE 5.7 (no engine API errors)
3. Confirm `.yarn` file parsing, `YarnSpinner.Runner` execution, and custom command
   registration work correctly in a minimal test Blueprint
4. If verification passes: proceed with integration per this ADR
5. If Yarn Spinner fails UE 5.7 compat: escalate to ADR-002-fallback (custom minimal
   dialogue tree — see Option C below)

**Fallback — Option C (custom minimal tree)** if Yarn fails:

The game's actual dialogue needs are limited:
- Linear sequences with player choice branches
- Condition checks (`HasFlag`) and writes (`SetFlag`, `IncrementInt`)
- Node ID persistence (`GetInt` / `SetInt`)

A custom `UDialogueTree` data asset with `TArray<FDialogueNode>` (each node containing
text, speaker tag, options array, and `TArray<FYarnCommand>` effects) can replace Yarn
Spinner in approximately one sprint. This is explicitly not the first-choice path but
is a well-understood fallback.

---

## Consequences

**Positive (Yarn Spinner):**
- `.yarn` script format is readable by writers without programming knowledge
- Yarn Spinner handles the state machine of dialogue execution, branching, and
  stopping conditions — no custom parser required
- Yarn's custom command system provides clean integration with NSM via ADR-008's
  injection pattern

**Negative / Mitigations:**
- **UE5.7 compatibility is unverified.** The LLM has no training data on Yarn Spinner
  for Unreal post-UE 5.3. **Mitigation:** Mandatory verification sprint before dialogue
  implementation sprint (see above). Fallback is defined.
- Open-source project maintenance risk: Yarn Spinner for Unreal may be unmaintained.
  **Mitigation:** At verification time, check repo activity (last commit date, open issues).
  If repository is stale (>6 months), switch to fallback immediately.
- Yarn stores dialogue state internally if not overridden. **Risk:** Dual source of truth.
  **Mitigation:** ADR-008 mandates all Yarn variables routed through NSM; Yarn's internal
  variable storage is disabled.

---

## ADR Dependencies

- ADR-001 (NSM): all dialogue state written to NSM, not Yarn's internal storage
- ADR-008 (Yarn→NSM injection): defines the injection pattern for custom Yarn commands

---

## Engine Compatibility

| API | UE Version | Risk | Notes |
|-----|-----------|------|-------|
| Yarn Spinner for Unreal plugin | UE 5.x | ⚠️ MEDIUM | Must verify against UE 5.7 build; fallback defined |
| `UUserWidget` (dialogue UI) | UE 4.x | ✅ LOW | Stable |
| `UWorldSubsystem` (dialogue subsystem host) | UE 4.27+ | ✅ LOW | Stable in 5.7 |

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| TR-dlg-001 | Yarn Spinner for Unreal plugin integration |
| TR-dlg-002 | Custom Yarn commands: set_flag, check_flag, add_ending_score |
| TR-dlg-003 | UDialogueSubsystem: BeginDialogue / EndDialogue lifecycle |
| TR-dlg-004 | Max 4 options; unavailable options hidden |
| TR-dlg-005 | NPC node persistence via NSM int store |
| TR-dlg-006 | Camera state switch on dialogue begin/end |
| TR-dlg-007 | Dialogue abort on fragment transition |
