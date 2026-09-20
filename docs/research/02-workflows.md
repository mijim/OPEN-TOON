# Workflows and integrated acceptance

These cross-module scenarios prevent isolated features that cannot finish a shot. They are proposed future tests and have not been executed.

| ID | Role and task | Workflow | Verifiable result |
|---|---|---|---|
| WF-01 | Traditional animator: two-second bouncing ball | Create 1080p/24fps → draw poses → expose on twos → onion skin → save → export PNG | 48 images; correct shared drawings; no visual aids in exports; reopening preserves work |
| WF-02 | Cleanup artist: correct a line | Open rough → Line Art → variable pencil → edit contour → erase → undo | Rough and width preserved; history reverses the complete gesture |
| WF-03 | Colorist: night variant | ID-based palette → Color Art → fill → palette clone → branch override | Only the selected instance changes; another retains the original scheme |
| WF-04 | Cut-out animator: waving gesture | Torso/arm/hand rig → pivots/pegs → keys → hand substitution → easing | No pivot or registration jumps; curves and preview agree |
| WF-05 | Rigger: deformable arm | Arm drawing → rest pose → curve/envelope → influences → kinematic accessory | No significant fixture seam; accessory follows the endpoint; reset restores rest pose |
| WF-06 | Advanced rigger: head turn | Compatible poses → controller grid → interpolation → save template | Each sample recovers its pose; imported controller works without links to the original scene |
| WF-07 | Dialogue animator | Import WAV → waveform → scrub → detect → map mouths → correct → export | Corrections retained; mouth/audio alignment within one frame; report unmapped labels |
| WF-08 | Compositor: parallax shot | Background/midground/character → camera → masks → blur/shadow → image/video Write | Consistent order/alpha; incomplete preview quality visible; complete profile-based output |
| WF-09 | 2D/3D generalist | Import authorized model → units → clip → override → composite with drawings | Matching scale/timing in preview/output; material limits stated |
| WF-10 | Game animator: runtime delivery | Compatible rig → bake unsupported effects → export atlas/skeleton → play in open reference runtime | Registration, timing, pivots and reference poses match; unsupported elements reported |
| WF-11 | Distributed studio | Acquire scene/revision → lock resource → work offline → submit divergent revision | Conflict detected; no revision overwritten without explicit resolution |
| WF-12 | Artist recovering from failure | Edit → autosave → interrupt save → reopen → locate missing texture | Coherent revision recovered; references retained and resource can be relinked |

## Cross-cutting edge cases

| Situation | Required result |
|---|---|
| One drawing exposed 200 times | Editing updates all exposures; duplication deliberately breaks sharing |
| Two swatches with equal RGB | Their identities remain distinct |
| Peg with negative scale | Transforms and pivots follow explicit finite rules |
| Layer locked during a drag | Defined cancellation or completion without partial mutation |
| Empty frame between holds | Distinct from repeating the previous drawing |
| Rational frame-rate change | Audio, markers and keys follow the chosen policy |
| Missing effect plugin | Preserve its block; final rendering does not silently omit it |
| Huge scene | Preview cancellation and lower quality remain available |
| Save while drawing continues | Identify the saved revision; the document may remain modified |
| External file containing scripts | Opening does not execute code automatically |
| Color-space change | Preview and output communicate the applied conversion |
| Lossy import | Report the supported subset and preserve the original according to policy |

Each scenario requires owned fixtures, quantitative criteria where appropriate and a recorded real workflow. The roadmap maps these to capability groups and complete vertical deliveries. WF-10 supplies an OPEN-TOON-specific acceptance scenario for the planned game-export/runtime phase.
