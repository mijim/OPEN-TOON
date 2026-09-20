# Working on OPEN-TOON

## Current context

- The user explicitly authorized implementation through P11. An experimental native editor now exists. Read `docs/implementation/STATUS.md` and `status.json` for working behavior, evidence and remaining gates. Continue toward P11 without calling this prototype a completed phase or production release. Mouse input is required; the owner has no physical tablet available yet.
- The user accepted C++20 + Qt 6/QML as the technical direction and explicitly requested proven, efficient open-source library reuse. Candidate libraries still require the documented adoption evidence.
- **English is mandatory** for all first-party UI, menus, tooltips, messages, accessibility labels, built-in assets, help, code identifiers/comments and public documentation. User content remains multilingual. See `docs/design/02-language-policy.md` and `NFR-027`.
- All project documentation and canonical catalog prose are now English. Preserve that language in future work; the owner may continue the conversation in Spanish.
- The interface is minimalist black, white and gray, with a Geist-inspired visual language and its own identity.
- The repository is public at `mijim/OPEN-TOON`. Preserve concurrent user edits and do not restore intentionally removed research references.

## Read only the relevant context

1. Start with `docs/INDEX.md` and `docs/planning/README.md`.
2. Query a feature assignment with `python3 scripts/roadmap.py feature DEF-005`, then its phase and catalog requirement.
3. Use `python3 scripts/catalog.py show DEF-005` or `domain DEF` to retrieve behavior and acceptance.
4. Review the owning architecture contract and the library register before adding dependencies.
5. Node inventory entries may describe families. Their specification owner must expand operators and parameters before implementation; never count entries as implemented effects.

- Keep the official `README.md` concise and professional. Link detailed coverage and limitations from implementation records; do not restore the removed mascot or long experimental feature inventory. The owner superseded the earlier playful-README policy.

## Maintain the planning records

- Canonical catalog files: `features.json`, `domains.json`, `node-reference.json`, `nonfunctional.json` under `docs/catalog/`. Preserve stable IDs.
- Canonical planning files: `roadmap.json`, `libraries.json`, `node-assignments.json` under `docs/planning/`.
- Regenerate with `python3 scripts/catalog.py render` and `python3 scripts/roadmap.py render`; validate with `python3 scripts/validate_docs.py`.
- Each feature has exactly one primary completion phase. Earlier subsets stay partial; optional deferrals remain unimplemented with an explicit decision.
- Each new capability needs scope, observable behavior, acceptance and an owning module. Update phase coverage and estimates when scope changes.
- Technology, serialization, time, color and extension-contract changes need an ADR update.
- A button, mockup or implementation-mirroring test does not complete a capability. Record real behavior and verification evidence separately from the plan.

## Interaction requirements

- Curves defaults to All motion for the selected layer. Its legend edits a highlighted curve in place; Link easing defaults to all pose channels for handle drags. Individual channels are optional precision views. Clear removes both exposures and animation keys in the selected range, with atomic undo.
- Keep controls compact and visually minimal throughout the interface. Curves should devote most panel height to the graph; numeric controls are secondary. The timeline/curve panel must resize by dragging its visible workspace separator.
- Editing and transforms must be direct: canvas bounding boxes with drag handles and live previews. Do not put selection transforms or timing tools in separate dialogs.
- Timing and curves stay in the main workspace. Properties describes the explicitly selected canvas object/region or layer, with a clear empty state; it must not silently edit a different target.

## Architecture

- Keep the C++20 domain independent of QML/widgets, filesystem, network and proprietary SDKs. UI calls application use cases; adapters implement ports.
- Route all document mutations through transactional commands with undo/redo. View state, selection and hover have separate ownership.
- Use one rational time model across exposures, curves, audio and render. Define interval boundaries and coordinate spaces explicitly.
- Keep Qt's UI scene graph separate from the animation document and compositor graph.
- Use immutable evaluation snapshots and revision-aware cache/job publication. Prevent stale jobs from overwriting newer edits.
- Prefer small capability modules, explicit APIs, RAII, visible ownership and validation at boundaries. Avoid speculative abstractions or empty frameworks.
- Do not add mandatory remote services, microservices, a second equivalent engine or a technology rewrite without a recorded need and decision.
- Adopt libraries through narrow adapters with pinned revisions, workload benchmarks, license inventory and a documented fallback. Upstream maturity alone does not establish our performance.

## Verification and publication

- Current owner policy: implement coherent functionality blocks, then run meaningful macOS tests at the end and fix failures iteratively. Do not rebuild or test after every small edit.
- Develop and test coherent blocks on macOS. The owner requested a downloadable macOS preview for experimental.10; include its dependencies, notices, source correspondence and verification in the release. Linux/Windows builds and the full native CI matrix remain manual future qualification work, not every push or source tag.

- Follow `docs/planning/EXECUTION.md`; choose meaningful invariant, recovery, image, input or artistic workflow checks for the change.
- Use synthetic or redistributable fixtures with recorded provenance. Do not publish private third-party projects.
- Keep credentials, personal paths, research caches and downloaded proprietary manuals out of commits. Do not stage unrelated local tour files.
- Original contributions are GPL-3.0-or-later. Check actual Qt/FFmpeg/plugin/model modules and shipped assets individually.
- New work branches use `codex/` unless the user explicitly requests another name.
