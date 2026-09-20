# Documentation map

**Updated: 2026-09-20.** The repository contains specifications, an execution roadmap and an experimental native editor. [Implementation status](implementation/STATUS.md) records the available subsets and open P00–P11 gates.

## Current decisions

| Topic | Direction | Status |
|---|---|---|
| Product | Professional offline desktop 2D editor | Planned |
| Appearance | Minimal black, white and gray; Geist-inspired language | User requirement |
| Language | English across first-party UI, assets, code, help and documentation | User requirement; previous documentation translated |
| Technology | C++20, Qt 6 and Qt Quick/QML | Accepted by user; risky integration paths need spikes |
| Renderer | CPU QPainter reference adapter; Qt RHI/Skia comparison remains open | Experimental |
| Storage | Experimental SQLite snapshot revisions; immutable external resources remain proposed | See ADR-011 and recovery tests |
| Open-source reuse | Prefer proven libraries through tested adapters | User requirement; 28 library/tool entries evaluated in the roadmap |
| Initial license | GPL-3.0-or-later for original contributions | Adopted |
| Implementation | Experimental editor; partial catalog coverage | No complete phase or production release |
| Development roadmap | 23 phases, 69 work packages, complete feature and node ownership mapping | Prepared; estimates subject to evidence |

## Reading routes

**Use or build the application:** [build](implementation/BUILD.md) → [user guide](implementation/USER-GUIDE.md) → [status](implementation/STATUS.md) → [machine-readable evidence](implementation/status.json).

**Planning:** [roadmap](planning/README.md) → [phase table](planning/PHASES.md) → [first backlog](planning/FIRST-STEPS.md) → [execution rules](planning/EXECUTION.md) → [estimates](planning/ESTIMATES.md).

**Dependencies:** [open-source register](planning/LIBRARIES.md), including upstream sources, intended use, benchmark gates, licensing investigations and fallback choices. Actual experimental dependency adoption is recorded in [implementation dependencies](implementation/DEPENDENCIES.md).

**Product research:** [scope](research/01-scope-and-method.md) → [catalog](catalog/README.md) → [workflows](research/02-workflows.md) → [risks](research/03-risks-and-decisions.md). The research and canonical catalogs are available in English.

**Engineering:** [technology selection](architecture/01-technology-selection.md) → [system architecture](architecture/02-system-design.md) → [document model](architecture/03-document-model.md) → [quality and spikes](architecture/04-quality-and-spikes.md) → [ADRs](architecture/adr/README.md).

**Design:** [visual system](design/01-design-system.md) and [English language policy](design/02-language-policy.md).

## Retrieve context for AI-assisted work

| Need | Query or document |
|---|---|
| Where a feature completes | `python3 scripts/roadmap.py feature DEF-005` |
| Phase outcome, dependencies and gates | `python3 scripts/roadmap.py phase P13` |
| Exact feature behavior | `python3 scripts/catalog.py show DEF-005` |
| Domain context | `python3 scripts/catalog.py domain DEF` |
| Dependency decision | `python3 scripts/roadmap.py library LIB-LIBIGL` |
| Operator specification owner | `planning/node-assignments.json` |
| Create an implementation issue | Feature criteria + phase + contract + `planning/FIRST-STEPS.md` template |
| Quality and language requirements | `catalog/nonfunctional.json` — 27 proposed requirements |
| Validate documentary consistency | `python3 scripts/validate_docs.py` |

`core`, `pro`, `advanced`, `optional` and `legacy` describe capability complexity/use; they are not paid editions or implementation states. Primary completion assignments and node specification ownership are distinct. An audit must use working scenes and evidence, not menu or inventory counts.
