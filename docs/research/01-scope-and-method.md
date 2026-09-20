# Scope and methodology

## What we will build

OPEN-TOON aims to cover vector and bitmap drawing, painting, exposure, traditional animation, cut-out rigging, deformation, curves, cameras, compositing, sound, 3D integration, libraries and export. The catalog also includes studio workflows, game delivery and optional assistance.

## Product boundaries

| Area | Treatment |
|---|---|
| Production management | Separate studio extension with revisions and locks |
| Auxiliary tools | Specialized workflows integrated as workspaces or standalone utilities |
| Game SDK | Future export profile without a proprietary dependency |
| Generative assistance | Optional capabilities through interchangeable providers; not a core dependency |

## Time baseline

The analysis is versioned with the repository and must be reviewed when product or architecture decisions change.

## Research process

1. Agree on product scope with the owner.
2. Classify families into **26 domains** and define **282 OPEN-TOON capabilities** with initial behavior and acceptance criteria.
3. Record the node inventory separately. Some entries are families rather than operators; they are not added to the capability count as unique requirements.
4. Compare technology alternatives using primary documentation from the relevant open-source projects.
5. Propose architecture, document model, visual language, workflows and quality gates for the [development plan](../planning/README.md).

The descriptions are OPEN-TOON's own specifications.

## Evidence levels

| Label | Meaning | Does not mean |
|---|---|---|
| `proposal` | Proposed OPEN-TOON capability | Implementation or functional verification |
| `analysis_draft` | Proposed requirement prepared for review | Final algorithm design |
| `not_started` | No application implementation | A working control exists |

## Deliberate distinctions

OPEN-TOON is a provisional name and should remain clearly distinguishable from [OpenToonz](https://opentoonz.github.io/e/), an existing project also considered in the technical evaluation.
