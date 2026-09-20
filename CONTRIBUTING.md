# Contributing

OPEN-TOON is in the specification and development-planning stage. Useful contributions currently include requirement corrections, reusable-library evaluations, redistributable fixtures and bounded technical experiments. There is no stable application SDK or plugin API yet.

Read [AGENTS.md](AGENTS.md), the [roadmap](docs/planning/README.md) and the relevant phase before changing scope. Describe the problem, feature IDs, expected behavior, evidence and limitations. Keep existing IDs and regenerate Markdown views after editing canonical records.

```sh
python3 scripts/catalog.py render
python3 scripts/roadmap.py render
python3 scripts/validate_docs.py
```

Use English for new code, comments, documentation, UI strings and built-in assets. Preserve multilingual user content. The previous research has been translated; future revisions must retain stable IDs and technical meaning.

Original contributions are offered under GPL-3.0-or-later. Identify the provenance and license of external resources; do not copy proprietary manuals, commercial assets or client scenes. New dependencies require pinned-version, license, maintenance and workload evidence. Future code should use small APIs, observable changes and meaningful behavior tests proportional to the risk.

A planning assignment does not mark a feature implemented. Update implementation status only with working document behavior and the evidence required by the [Definition of Done](docs/planning/EXECUTION.md).
