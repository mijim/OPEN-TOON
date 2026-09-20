# Risks, differences and open decisions

## Risks that determine effort

| ID | Risk | Consequence | Response and closure condition |
|---|---|---|---|
| R-01 | Professional-suite scope | Many subsystems and specialties; feature counts do not estimate duration | Plan complete workflows with explicit staffing/capacity and revisable ranges |
| R-02 | Inadequate vector strokes and fills | Attractive UI remains unusable for drawing | SP-01/SP-04 and animator evaluation |
| R-03 | Complex Qt RHI or Skia integration | Version coupling and GPU differences | Isolated adapter, SP-02 and build matrix |
| R-04 | Inconsistent database and asset saves | Lost work | Commit protocol, fault injection and immutable revisions |
| R-05 | Advanced deformation | Artifacts and expensive algorithms | Validate curve/envelope before weighted/shape-aware deformation |
| R-06 | Inconsistent color and alpha | Preview and render disagree | Explicit pipeline and compositing fixtures |
| R-07 | Overpromised format compatibility | Destructive import and frustration | Format profiles with loss reports |
| R-08 | Dependency licenses | Difficult binary redistribution | Review each module/build and preserve notices |
| R-09 | Name resembles OpenToonz | Confused identity and expectations | Provisional name and review before release |
| R-10 | Unstable plugin or script | Crashes, improper access or corrupt data | No automatic execution; limits, adapters and appropriate isolation |
| R-11 | Premature collaboration features | Work diverted from the local tool | Revisions and packages first; separate studio service |
| R-12 | Provider-dependent AI | Costs and loss of offline operation | Optional features; complete editor without an AI service |
| R-13 | Incomplete capability catalog | Missing operators or variants | Human review by domain and catalog updates |
| R-14 | Counts interpreted as progress | False impression of a finished product | Preserve `not_started` and distinguish capabilities from nodes |

## Resolved or proposed decisions

| ID | Decision | Status |
|---|---|---|
| D-02 | Minimal black/white interface close to Vercel's visual language | Confirmed by user |
| D-03 | Public GitHub repository under `mijim` | Confirmed by user |
| D-04 | Research before planning; implementation afterward | Confirmed by user; plan now prepared |
| D-05 | Local desktop editor without a mandatory account | Recommended direction |
| D-06 | C++20 / Qt Quick | Accepted direction; integration evidence still required |
| D-08 | GPL-3.0-or-later for original contributions | Initial license applied |
| D-09 | Exact graphics backend, dependencies and versions | Open until the spikes |
| D-10 | English product, code and documentation | Confirmed by user |
| D-11 | Reuse proven, efficient open-source libraries | Confirmed direction; individual adoption gates apply |

## Information that improves estimates

Available people, C++/Qt/graphics experience, budget, real operating systems/tablets, traditional-versus-cut-out priorities and the first required production workflow will improve the schedule. These do not prevent completing the analysis or plan. Until supplied, [estimates](../planning/ESTIMATES.md) use explicit assumptions and risk ranges rather than fictional dates.

The analysis proposes an owned core but has not yet audited OpenToonz code. It also contains no measured tablet latency, rendering performance or device compatibility. These are specific uncertainties recorded in the spikes, not a reason to keep researching indefinitely before producing a useful tool.

## Details to resolve per capability

Before implementation, specify parameters, defaults, limits, units, supported formats, transactions, cancellation cases and tests.

## Maintaining the research

Scope changes preserve existing IDs, update canonical records and regenerate Markdown views. Translation preserves behavior and acceptance criteria rather than merely translating labels.
