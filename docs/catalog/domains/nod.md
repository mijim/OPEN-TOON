# NOD — Compositing graph

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Connect inputs → group → configure parameters → preview → render.

**Module:** `compositor-graph`.

**Entities:** Node, Port, Edge, NodeGroup, AttributeBinding.

**Relationships:** LYR, ANI.

**Main risk:** Cycles, incorrect invalidation and confusion between layer order and topology.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## NOD-001 — Node view

Create, move, connect, search and delete nodes while navigating the graph.

**Initial acceptance:** Deleting a node reconnects only according to the chosen option.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## NOD-002 — Typed ports

Distinguish image, transform, matte and other data flows.

**Initial acceptance:** Incompatible connections are rejected before evaluation.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM04-ACCEPTANCE.md](../../../docs/implementation/HM04-ACCEPTANCE.md) — typed Image, Transform and Matte graph boundary with rejection tests.

**Remaining scope:** The internal derived graph has typed ports. Editable graph connections and wider port types remain open.

## NOD-003 — Node library

Organize operators by category with search and descriptions.

**Initial acceptance:** Search finds an operator by name and category.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## NOD-004 — Groups and published ports

Encapsulate subgraphs with reusable inputs and outputs.

**Initial acceptance:** Grouping preserves the graph's resulting image.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## NOD-005 — Layer compositing

Combine inputs while preserving order, alpha and depth according to the mode.

**Initial acceptance:** A semitransparent layer produces the expected alpha over a transparent background.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM04-ACCEPTANCE.md](../../../docs/implementation/HM04-ACCEPTANCE.md) — ordered premultiplied Over and alpha-chart evidence.

**Remaining scope:** Ordered Over and alpha composition work. Depth modes and broader compositing operators remain open.

## NOD-006 — Masks and cutters

Clip by matte with defined inversion and alpha handling.

**Initial acceptance:** A partial mask is not treated as binary unless explicitly configured.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM04-ACCEPTANCE.md](../../../docs/implementation/HM04-ACCEPTANCE.md) — internal alpha-matte extraction and multiplication.

**Remaining scope:** Internal alpha mattes work. Editable cutters, inversion controls and wider matte operators remain open.

## NOD-007 — Switches

Select images or transforms using animated attributes.

**Initial acceptance:** Changing the selector avoids evaluating unnecessary branches unless dependencies are shared.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## NOD-008 — Display and Write

Separate preview output from exportable final outputs.

**Initial acceptance:** An alternate Display does not change the configured Write output.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM04-ACCEPTANCE.md](../../../docs/implementation/HM04-ACCEPTANCE.md) — explicit internal Display and Write outputs with matched evaluated pixels.

**Remaining scope:** Display and Write exist in the derived graph. User-configurable alternate outputs remain open.

## NOD-009 — Bypass and cache

Temporarily bypass operators and reuse valid results.

**Initial acceptance:** Editing a dependency invalidates all affected outputs.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Specification source:** `proposal`.

**Implementation evidence:** [docs/implementation/HM04-ACCEPTANCE.md](../../../docs/implementation/HM04-ACCEPTANCE.md) — revision-aware preview cache and affected-descendant invalidation.

**Remaining scope:** Revision-aware cache and invalidation work. User-controlled node bypass remains open.

## NOD-010 — Notes and organization

Add notes and spatial organization without changing the image.

**Initial acceptance:** Moving notes does not invalidate the render.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.
