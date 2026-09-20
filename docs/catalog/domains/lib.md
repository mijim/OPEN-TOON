# LIB — Libraries, symbols and reuse

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Create asset → catalog → reuse → update.

**Module:** `asset-library`.

**Entities:** AssetLibrary, Template, Symbol, AssetVersion.

**Relationships:** PRJ, RIG.

**Main risk:** Assets that appear independent but share data.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## LIB-001 — Local libraries

Organize assets in folders with search, previews and refresh.

**Initial acceptance:** Moving the catalog does not lose packaged references.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.

## LIB-002 — Scene and rig templates

Save selections with required drawings, palettes, nodes and animation.

**Initial acceptance:** A template imports correctly into an empty scene.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## LIB-003 — Reusable symbols

Encapsulate elements with their own content and transforms.

**Initial acceptance:** Editing a symbol updates its instances according to the linking mode.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## LIB-004 — Import templates

Apply copy, linking and conflict rules when reusing assets.

**Initial acceptance:** Two imports can be independent when duplication is selected.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## LIB-005 — Paste special

Choose whether to import animation, drawings, palettes or structure.

**Initial acceptance:** Pasting only keys does not overwrite the receiving rig.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## LIB-006 — Library thumbnails

Generate refreshable previews without blocking editing.

**Initial acceptance:** Modifying an asset invalidates its thumbnail.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## LIB-007 — Template editing

Edit a library resource while distinguishing its source from instances.

**Initial acceptance:** Saving changes identifies which resource is modified.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.
