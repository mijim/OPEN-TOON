# ADR-049 — Saved Drawing and Part blend modes

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 25 adds a `blendMode` to Drawing and Part layers: Normal, Multiply or
Screen. Formats 1–24 load as Normal; the first upgraded save keeps a readable
source-version backup. The selected layer's mode is an undoable edit in Nodes.
Other layer kinds and unknown modes reject before publication.

The derived graph chooses a typed Composite, Multiply or Screen node for the
layer's ordered paint operation. Blend math uses premultiplied source-over:
non-overlapping contributions retain their source color, and the shared area
uses the selected blend function on straight colors. Alpha stays ordinary
source-over. In the Legacy profile, colors blend in encoded sRGB values; in
the Linear sRGB profile they blend after linearization. A cutter still clips
the foreground before blending, and a painted cutter source uses its own
blend mode at its paint position. Display, Write and PNG export evaluate the
same graph. The legacy direct painter enters that graph only when a non-Normal
mode is present; isolated source rendering resets the mode to Normal to avoid
recursive graph entry.

## Evidence and limits

Fractional-alpha tests compare Normal, Multiply and Screen, both color
profiles, Display/Write, undo/redo, invalid mode rejection and save/reopen.
Format-24 migration checks Normal default and a readable `.pre-v24.bak`.
Native Qt Quick smoke opens the actual blend picker, clicks Multiply on a
painted cutter source, checks changed pixels, undoes and reopens. In a local
three-frame 1920 × 1080 run with 20 original-art layers on an M1 Pro,
Normal measured 1.62 ms/frame in Legacy and 43.32 ms/frame in Linear sRGB;
five alternating Multiply/Screen layers measured 49.36 and 43.59 ms/frame.
The Legacy increase includes entering the full derived graph. These are mean
render times, not p95 control-to-present latency. Other blend modes,
per-group blending and graph-level connections remain HM-12 work.
