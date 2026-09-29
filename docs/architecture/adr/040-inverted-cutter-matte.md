# ADR-040 — Format-19 inverted cutter matte

Status: experimental HM-12 subset, 2026-09-29.

## Decision

Format 19 adds `invertMatte` to each layer, default false when loading
formats 1–18. It can be true only when a Drawing or Part has a valid cutter
source. The binding and inversion are edited through document commands; a
binding removal clears inversion atomically. First saving an older file
creates a readable source-version backup.

The derived typed graph inserts `InvertMatte` after `MatteFromImage` and
before `ApplyMatte`. It has one Matte input and one Matte output, so graph
port validation and cycle checks remain effective. For source alpha `a`,
inversion yields `255 − a`; `ApplyMatte` multiplies the target's
premultiplied color and alpha by that fractional coverage. Inversion covers
the whole image, including pixels outside the source's ink bounds. The
source still does not paint into the final composite. Display and Write
evaluate the same graph under either composition profile.

## Evidence and limits

Tests compare 128/255 target alpha against 64/255 source alpha: inside
coverage is 32/255 and outside coverage is 96/255. They check typed graph
construction, target ink outside the source image's ink bounds, invalid
inverse-without-source rejection, format-18 migration
and backup, controller undo and native Qt Quick save/reopen. General cutter
shapes and artistic joint recipes remain HM-12 work.
