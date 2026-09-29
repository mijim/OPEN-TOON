# Current work — Harmony Moment

**Current execution guide; status snapshot as of 2026-09-30.** This page is a route into the [canonical delivery contracts](roadmap.json) and [recorded implementation evidence](../implementation/status.json), not another source of truth. Re-run `python3 scripts/roadmap.py next` before taking work; if this snapshot differs, use the canonical records and reconcile this page.

Read in order: **this page → [implementation status](../implementation/STATUS.md) → [next-task guide](FIRST-STEPS.md) → [the specific slice](HARMONY-MOMENT.md) → [full roadmap and references](README.md).**

The immediate objective is a complete, reliable character-animation **Harmony Moment**, from artwork intake through rigging, deformation, controls, animation and audio, composition and camera to preview, save/reopen and export. This is a bounded workflow milestone; the full P00–P22 roadmap remains intact.

**Open critical-path slice: [HM-06](HARMONY-MOMENT.md#hm-06).** HM-05 has accepted bounded [rest/UV and renderer evidence](../implementation/HM05-ACCEPTANCE.md). The [working HM-06 subset](../implementation/HM06-PROGRESS.md) saves bone/curve controls, weights and keys, supports canvas drags, continuous limbs and explicit matching of compatible substitution poses. The 480-frame visual shot passes continuity, native input and reopen checks, including regenerated contour-bound limbs, sampled artwork edges and original headwear in both views. Further extreme-bend and complete-reference review remain before acceptance. Its remaining review is open.

**In-progress slice: [HM-07](HARMONY-MOMENT.md#hm-07).** Format-12 named masked poses capture, apply and blend selected channels and substitutions with undo/reopen evidence. Format-15 publication exposes grouped views, per-Part drawing choices and poses in an Animator dashboard and compact camera viewport panel over the same document. Compatible in-project pose transfer maps unique roles and drawing names; paired left/right roles can create mirrored poses. [The tested subset](../implementation/HM07-PROGRESS.md) leaves general controls, broader retargeting and complete workspace presets open.

**In-progress slice: [HM-10](HARMONY-MOMENT.md#hm-10).** Format-16 original PCM16 WAV assets, format-17 sample-contiguous repeats and format-22 source-sample fades and format-23 saved clip mute, format-26 per-clip solo and format-27 stereo balance, undoable clip placement/trim/gain and source-sharing duplication and exact 48 kHz single-pass clip splitting and direct frame-edge trimming including native waveform drag, indexed sample-aligned timeline waveforms, band-limited rate conversion, deterministic full/selected-range offline WAV mix/export, bounded miniaudio device preview with a loop/play-once toggle and timeline fragment scrub pass local tests. An original 480-frame study joins the connected toon, nine mouth changes, eye cutter and exact synthetic audio cues with save/reopen and native Qt Quick checks. [The audio subset](../implementation/HM10-PROGRESS.md) includes two completed silent ten-minute CoreAudio scheduling probes at 24 and 24000/1001 fps. Hardware presentation latency/underruns and audible scrub-quality qualification plus broader rate-ratio quality remain.

**Active slice: [HM-12](HARMONY-MOMENT.md#hm-12).** HM-03 character identity and HM-04 typed graph runtime have accepted bounded evidence. Format-18 cutter bindings, format-19 fractional inside/outside coverage, format-20 persistent cutter bypass, format-21 visible cutter sources, format-24 opacity bypass and format-25 Normal/Multiply/Screen and format-28 Add blending, format-29 blend bypass, format-30 composite bypass and format-31 contiguous groups with whole-group movement and format-32 group bypass, direct front/behind Drawing/Part-card order dragging, Alt-drag cutter binding, derived node-owner selection, direct card bypass, alternate image/matte Display, node-scoped cache, name/kind search and a supported operator palette and closed rig-copy group remapping, explicit source deletion, in-scene group duplication, direct edge-member edits, character graph/matte dependency collection, private cutter copies and manual editable joint patches with direct Control+Alt card drag pass tests and native smoke in the derived Nodes workspace with animated Opacity nodes and clicked output previews. The complete node editor and automatic part-overlap repair remain open.

**Current gate:** HM-06 has animated bone/curve evaluation, bounded per-substitution binding, editable elbow influence and a refreshed original character fixture; broader extreme-bend quality and artistic interaction remain unaccepted. HM-07 and HM-10 remain working subsets. The owner resumed work after the earlier cutoff. Continue HM-12 graph grouping and part-overlap work after checking its current contract.

**Animation fixture:** the owner's [Milo source and 17-Part continuous
study](../implementation/HM06-MILO.md) is available for later animation work.
The current priority is editor functionality. Keep Clockwork projects as
regression fixtures. Milo's broader artistic approval and complete animated
shot are still open.

**Accepted HM-03 evidence:** the owner delegated review to the implementer. The original 19-part import/assembly/view journey, reference-pixel comparison, saved/reopened substitutions, independent copy, undo, negative/nonuniform reparenting and visual screenshot inspection are recorded in [HM03-ACCEPTANCE](../implementation/HM03-ACCEPTANCE.md). This bounded contract does not complete P08 or the independent second-animator HM-15 review.

## Recorded dependency snapshot

Statuses and hard prerequisites below come from `roadmap.json`; acceptance and phase limits come from [implementation status](../implementation/STATUS.md). `planned` is not `ready`.

| Slice | Recorded state | Hard prerequisite contracts | Operational gate |
|---|---|---|---|
| [HM-03](HARMONY-MOMENT.md#hm-03) | Bounded contract accepted | HM-01, HM-02 accepted | P08 owning phase remains open. |
| [HM-04](HARMONY-MOMENT.md#hm-04) | Bounded contract accepted | HM-01 accepted | P10 owning phase remains open. |
| [HM-05](HARMONY-MOMENT.md#hm-05) | Bounded contract accepted | HM-03, HM-04 accepted | P09 owning phase remains open. |
| [HM-06](HARMONY-MOMENT.md#hm-06) | In progress | HM-05 accepted | Extreme-bend quality and interaction budgets remain. |
| [HM-07](HARMONY-MOMENT.md#hm-07) | In progress | HM-03, HM-04 accepted | Published grouped dashboard and viewport controls, compatible transfer and paired-role mirroring pass; general bindings and workspace presets remain. |
| [HM-08](HARMONY-MOMENT.md#hm-08) | Planned | HM-03, HM-07 | Wait for both contracts. |
| [HM-09](HARMONY-MOMENT.md#hm-09) | Planned | HM-06, HM-07, HM-08 | Wait for all three contracts. |
| [HM-10](HARMONY-MOMENT.md#hm-10) | In progress | HM-01 accepted | PCM16 import, saved repeats/fades, direct clip drag, indexed waveform, offline mix/export, device preview and fragment scrub pass; hardware sync remains. |
| [HM-11](HARMONY-MOMENT.md#hm-11) | Planned | HM-03 accepted, HM-10 pending | Wait for HM-10. |
| [HM-12](HARMONY-MOMENT.md#hm-12) | In progress | HM-04, HM-03 accepted | Matte binding, derived node previews, front/behind Drawing/Part-card order dragging, contiguous named groups with whole-group movement and bypass, Alt-drag cutter binding, saved blend/composite bypass, derived node-owner selection, direct card bypass, alternate image/matte Display, node-scoped cache, name/kind search and a supported operator palette and closed rig-copy group remapping, explicit source deletion, in-scene group duplication, direct edge-member edits, character graph/matte dependency collection, private cutter copies and manual editable joint patches with direct Control+Alt card drag pass as a subset; full graph workflow remains open. |
| [HM-13](HARMONY-MOMENT.md#hm-13) | Bounded contract accepted | HM-04 accepted | P06 owning phase remains open. |
| [HM-14](HARMONY-MOMENT.md#hm-14) | Planned | HM-10, HM-12, HM-13 | Wait for HM-10 and HM-12 acceptance. |
| [HM-15](HARMONY-MOMENT.md#hm-15) | Planned | HM-09, HM-11, HM-14 | Wait for the three joining contracts. |

**Stop rule:** before implementing a consumer slice, inspect `requires` and accepted evidence for every prerequisite. If any required contract is unaccepted, stop consumer work and finish that prerequisite or choose a truly eligible independent slice. An isolated feasibility spike may inform a decision, but it does not make its consumer ready. Keep at most two bounded slices active with separate owners.

## Consult and validate

```sh
python3 scripts/roadmap.py next             # Current eligible slices and instruction
python3 scripts/roadmap.py slice HM-05      # One contract, prerequisites and acceptance
python3 scripts/roadmap.py phase P09        # Owning phase and exit criteria
python3 scripts/roadmap.py feature DEF-008  # Primary completion assignment
python3 scripts/catalog.py show DEF-008    # Feature behavior and acceptance
python3 scripts/roadmap.py stats            # Plan counts and effort envelope
python3 scripts/validate_docs.py           # Documentary and generated-view consistency
```

For observed behavior and verification, consult [STATUS.md](../implementation/STATUS.md), [status.json](../implementation/status.json) and the linked acceptance records. The historical backlog and planning rationale are reference material, not a competing task queue.
