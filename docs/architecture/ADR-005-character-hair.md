# ADR-005: Gaia Hair Rendering — Hair Cards with Groom Upgrade Path

**Status:** Superseded — 引擎迁移至 Godot 4 (2026-04-25)
**Date:** 2026-04-25
**Author:** Claude Code (technical-director)

---

## Context

Gaia is a Maine Coon — a long-haired cat breed with a distinctive thick mane, ear tufts,
and bushy tail. Her hair is a significant part of her visual identity (Art Bible Section 5).

Two rendering approaches are available in UE 5.7:

| Option | System | Quality | Performance | Risk |
|--------|--------|---------|-------------|------|
| A | **UE5 Groom** (`UGroomComponent`) — full strand-based hair simulation | Photorealistic, physically accurate | Expensive: ~2-4ms GPU on high-end PC; potentially prohibitive on 4 GB VRAM console target | Medium — Groom is production-ready UE5 but performance is hardware-dependent |
| B | **Hair Cards** — static mesh strips with alpha-masked hair texture + anisotropic shader | Very good (75-85% of Groom quality at character distances) | Cheap: ~0.3ms GPU; well within budget | Low — proven technique, hardware-independent |

**Performance context (from technical-preferences.md):**
- PC target: 60fps, 16.6ms frame budget
- Console target: 30fps, 33.3ms frame budget, 4 GB VRAM

A Maine Coon Groom with full fur simulation would require:
- High hair density: ~50,000 strands (realistic long cat fur)
- Multiple Groom LODs: LOD0 at game distance, LOD1 at medium, LOD2 at far
- Estimated cost at LOD0: 2–4 ms GPU on current-gen GPU (RTX 3070 class)

On 4 GB VRAM console target, Groom memory overhead for dense long fur may risk
the memory ceiling, especially combined with Lumen + Nanite + post-process.

---

## Decision

**Use hair cards for the Vertical Slice and Alpha milestones. Evaluate Groom upgrade
at or before the Full Vision milestone after hardware benchmarking.**

**Hair card specification (to Art team):**
- 3–5 hair card layers (mane, body fur, tail, ear tufts, bib)
- Alpha-masked diffuse + normal maps (2K resolution per region)
- Substrate material with anisotropic specular (hair sheen) and subsurface scattering
- Physics: `UPhysicsAsset` on hair card mesh for gentle cloth-like movement
  (not full strand sim — just 2–3 bone jiggle per region)
- Color matching: Art Bible silver-gray tabby mackerel markings

**Groom upgrade conditions (any one triggers evaluation):**
- Console target VRAM budget confirmed > 5 GB (expanded target hardware)
- Benchmark shows Groom fits within 1.5ms GPU at LOD0 on reference console hardware
- Hair cards assessed as visually insufficient after art review at Alpha milestone

---

## Consequences

**Positive:**
- Hair cards are within VRAM and GPU budget on all target platforms from day one
- Art team can iterate faster on hair cards (standard mesh workflow) than Groom
  (requires Groom asset authoring tools)
- No engine-version risk: hair cards use standard Substrate/Static Mesh pipeline

**Negative / Mitigations:**
- Hair cards lack real-time strand physics; Gaia's fur won't respond dynamically to
  wind or movement. **Mitigation:** `UPhysicsAsset` jiggle on 2–3 bones per region
  provides a convincing approximation. Camera is never close enough to discern individual
  strands in this game.
- Upgrade path requires re-importing hair as Groom asset at Full Vision.
  **Mitigation:** Art Bible locks down the reference silhouette now; Groom artist can
  match the established look. Not a visual regression.

---

## ADR Dependencies

- None. Orthogonal to all narrative/gameplay systems.

---

## Engine Compatibility

| API | UE Version | Risk | Notes |
|-----|-----------|------|-------|
| `UGroomComponent` | UE 5.0+ | ✅ MEDIUM | Production-ready; NOT used in this ADR |
| Static Mesh hair cards | UE 4.x | ✅ LOW | Standard mesh pipeline |
| Substrate anisotropic hair shading | ⚠️ UE 5.7 | HIGH | Production-ready 5.7; verify Substrate Hair node vs Slab node for hair cards |
| `UPhysicsAsset` bone jiggle | UE 4.x | ✅ LOW | Stable |

---

## GDD Requirements Addressed

| TR ID | Requirement |
|-------|-------------|
| (Art Bible) | Gaia visual spec: silver-gray Maine Coon, LOD0–LOD2 rules, within 4GB VRAM budget |
