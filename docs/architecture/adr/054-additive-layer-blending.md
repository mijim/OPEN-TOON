# ADR-054 — Additive layer blend

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 28 extends Drawing and Part `blendMode` with Add. The ordered typed
graph represents it as an image Add node with two image inputs. In overlapping
pixels, the straight-color blend function is `min(1, foreground + background)`
per channel. The existing premultiplied source-over formula handles partial
coverage and retains ordinary source-over alpha. Non-overlapping pixels keep
their source color. Legacy composition applies the blend in encoded sRGB;
Linear sRGB composition applies it after linearization. Cutters, opacity,
Display, Write and PNG export evaluate the same graph.

The Nodes picker offers Add after Normal, Multiply and Screen. Changing it is
one undoable document command. Formats 25–27 retain their recorded modes;
format-27 files claiming Add reject. The first format-28 save keeps a readable
`.pre-v27.bak`. The direct painter enters the typed graph when any layer uses
Add, as it does for other non-Normal modes.

## Evidence and limits

Fractional-alpha tests compare Add with Screen in both color profiles and
check Display/Write parity, undo/redo and save/reopen. A format-27 migration
test checks existing Screen, backup and invalid legacy Add. Native Qt Quick
smoke opens the blend picker, clicks Add on a painted cutter source, checks
brightened pixels, undoes and reopens. A local 20-layer original-art 1080p
three-frame run on the M1 Pro measured Add at 50.93 ms/frame in Legacy and
45.22 ms/frame in Linear sRGB. The same run measured Multiply/Screen at
45.79/45.18 ms and Normal at 1.84/42.99 ms, respectively. These are means
of three frames with a warm renderer, not p95 interaction measurements.
General editable graph connections and group blending remain open.
