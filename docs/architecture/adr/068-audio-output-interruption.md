# ADR-068 — Audio output interruption

Status: adopted for the HM-10 experimental subset, 2026-09-30.

When miniaudio reports an interruption or reroute, stop the output device
before reading its final submitted sample cursor. Move the visible playhead
to the frame containing that sample, bounded by the active half-open playback
range, then stop transport and report the event. Preserve the final sample and
callback counters for diagnostics. No document command, scene revision or
media rewrite occurs. A later Play creates a fresh output device from the
current immutable snapshot; automatic device replacement is not part of this
decision.

The UI timer owns this response, outside the audio callback. The callback
continues to use atomics and its prebuilt mix plan without document locks or
allocations. Format and rational frame-to-sample contracts do not change.

A 24000/1001 fps controller test uses miniaudio's null backend, advances its
sample cursor and invokes the same interruption handler as the timer. It
checks exact final-frame mapping, stopped transport, retained diagnostics and
unchanged document, revision and selection. Real CoreAudio unplug/reroute,
device replacement, audible continuity and output latency still need hardware
qualification before HM-10 acceptance.
