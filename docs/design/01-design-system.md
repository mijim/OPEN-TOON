# Visual design and editing experience

## Direction

Black, white and gray; restrained typography, thin lines, flat surfaces and precise controls. Use [Geist's visual language](https://vercel.com/geist/stack), [color hierarchy](https://vercel.com/geist/colors) and [typography](https://vercel.com/geist/typography) as references. Build original animation-editor components; do not copy a dashboard layout literally or assume public documentation grants rights to every brand asset.

The aesthetic applies to the interface. **User artwork retains its full color**, and palettes display actual colors. The canvas supports white, neutral gray or transparent backgrounds. Animation aids may use configurable colors, but default to patterns, opacity and symbols readable in monochrome.

All first-party text follows the [English language policy](02-language-policy.md). This applies to tools, errors, accessibility labels, presets and documentation, independently of the language of the artwork or dialogue.

## Proposed tokens

These are OPEN-TOON values inspired by the reference, not official Vercel tokens.

| Token | Light | Dark | Usage |
|---|---|---|---|
| `surface.base` | `#FFFFFF` | `#0A0A0A` | Window |
| `surface.panel` | `#FAFAFA` | `#111111` | Panels and bars |
| `surface.hover` | `#F0F0F0` | `#222222` | Hover |
| `surface.selected` | `#E8E8E8` | `#303030` | Selection with an additional indicator |
| `text.primary` | `#171717` | `#EDEDED` | Primary text |
| `text.secondary` | `#666666` | `#A3A3A3` | Secondary text |
| `border.subtle` | `#E5E5E5` | `#2A2A2A` | Decorative separators |
| `border.control` | `#767676` | `#777777` | Essential control boundaries |
| `focus.ring` | `#171717` | `#FFFFFF` | Double ring separated from the background |
| `action.primary.bg` | `#171717` | `#EDEDED` | Primary action background |
| `action.primary.fg` | `#FFFFFF` | `#0A0A0A` | Primary action text |

Decorative separators are insufficient to communicate focus or essential boundaries. Disabled styling is not reused for readable secondary text. Errors include an icon, explanation and recovery action; red alone never identifies them.

Proposed typography: Geist Sans for UI and Geist Mono for timecode, frames and aligned values. [Geist uses SIL OFL 1.1](https://github.com/vercel/geist-font/blob/main/OFL.txt); retain the license when bundling it. Allow an offline system-font fallback. Sizes: 12 px for secondary metadata, 13–14 px for controls and 16–20 px for headings, with UI scaling. Avoid oversized headings in the workspace.

Base spacing is 4 px, with a 4/8/12/16/24 scale. Control corner radii are 4–6 px, without oversized cards or decorative shadows. Rows are 28–32 px in compact mode and 36–40 px in comfortable mode. Interactive targets are at least 24×24 logical pixels where appropriate, expandable for pen/touch; icons may be smaller inside them.

## Tool iconography

The compact tool strip uses original 22 × 22 monochrome line drawings inside 34 × 28
buttons. A cursor arrow, transform handles with a pose diamond, dashed marquee,
freehand lasso, endpoint-marked line, pencil, bristled raster brush, eraser, shape
outlines, pouring color and editable curve nodes express the actual actions. Do not
use font glyphs whose appearance changes across operating systems. The selected
tool reverses icon contrast against its light button background; tooltips and Qt
accessibility names retain the full English tool name and shortcut. The illustration
never replaces hover, pressed, focus or unavailable feedback.

## Layout

```text
┌ Project / scene ── workspace ── save status ── command search ── export ┐
├ Active tool / size / mode / contextual controls ───────────────────────┤
│       │                                            │                  │
│ Tools │          Camera / Drawing viewport         │ Inspector        │
│       │          canvas occupies the main area     │ Tool properties  │
│       │                                            │ Color / Library  │
├───────┴────────────────────────────────────────────┴──────────────────┤
│ Timeline / Xsheet / Nodes       transport · frame · range · FPS       │
│ tracks, exposures or graph; adjustable height                         │
├ Useful status: selection, coordinates, zoom, preview quality ─────────┤
```

Start with a 48 px tool strip, a 260–340 px inspector and a timeline area around one quarter of the height. These are initial values, not rigid constraints. The viewport receives the remaining area. Do not display every rigging, drawing and compositing tool simultaneously.

| Workspace | Center | Bottom | Inspector |
|---|---|---|---|
| Drawing | Drawing/Camera | Short timeline or Xsheet | Brush, palette, onion skin |
| Animation | Camera | Timeline and curves | Transform, exposure, substitutions |
| Rigging | Camera with controls | Hierarchy/nodes | Pivots, rest pose, influences |
| Compositing | Camera preview | Node graph | Parameters, masks, output |
| Review | Large image | Transport and notes | Quality, comparison, export |

Initial evaluation minimum: 1280×800 with collapsible panels; recommended workspace 1920×1080 or higher. Essential actions remain accessible through menus and panel collapse at 200% scale. Multi-monitor layouts store relative geometry and recover off-screen windows. Supporting external drawing tablets does not imply an iPad/Android application.

## Component behavior

| Component | Interaction contract |
|---|---|
| Tool button | Selection shown by background and mark; tooltip includes name and shortcut |
| Numeric field | Explicit unit, direct entry, optional dragging, reset, animated state |
| Animated property | Distinguish base value, current key and interpolation through shape/symbol and text |
| Timeline | Clearly distinguish empty cells, holds, new drawings, continuous and held keys |
| Xsheet | Numbering, rectangular selection, keyboard editing and fixed headers |
| Node | Name, type, ports distinguished by shape/label and error states |
| Save status | Distinct modified, saving, saved, recovery-available and error states |
| Progress | Operation, measurable progress when available, cancellation and result; no fictional progress bar |
| Import dialog | Preview, relevant options and conversion-loss report |
| Command search | Names, synonyms and shortcuts; no remote service required |

A click changes selection; a double-click enters editing where conventional. Drag gestures always have a field, menu or keyboard alternative. Numeric scrubbing previews values and supports cancellation. Hover is never the sole route to an essential function.

## Accessibility and localization architecture

Use [WCAG 2.2](https://www.w3.org/TR/WCAG22/) as a verifiable guide for contrast, focus and pointer actions without claiming native-app certification. Target 4.5:1 normal-text contrast, distinct focus and names/roles through Qt accessibility. Artistic geometry cannot be fully reduced to standard controls, but its operations and properties need accessible alternatives.

Contextual shortcuts detect conflicts and support Spanish keyboards, IMEs and remapping. First-party strings are English and localization-ready; do not concatenate messages in ways that prevent future pluralization. Display numbers/units according to an explicit locale policy where appropriate, while files use stable language-independent representations. Non-English OS settings do not silently change the initial English product language.

## Future visual validation

UI scenes include an empty project, loaded drawing, 300 layers, long names, multi-selection, missing resource, partial render, dark mode and 200% scaling. Check density, clipping, legibility and canvas/UI separation. UI transitions are brief and reducible; never add visual easing to the playhead that changes perceived timing.

Future product screenshots must come from the actual application. This document defines visual direction; it is not a functional mockup or a completed-interface claim.
