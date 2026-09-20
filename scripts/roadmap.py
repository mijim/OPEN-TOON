#!/usr/bin/env python3
"""Query and render the development roadmap using only the Python standard library."""
from __future__ import annotations
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / 'docs/planning'


def read(name):
    return json.loads((PLAN / name).read_text(encoding='utf-8'))


def documents():
    plan = read('roadmap.json')
    libraries = read('libraries.json')['libraries']
    catalog = json.loads((ROOT / 'docs/catalog/features.json').read_text())['features']
    features = {f['id']: f for f in catalog}
    files = {}
    index = ['# Phase index', '', '> Generated from `roadmap.json`. Phase status is separate from feature completion; see [implementation evidence](../implementation/STATUS.md).', '',
             '| Phase | Outcome / milestone | Depends on | Features | Engineer-weeks |', '|---|---|---|---:|---:|']
    for p in plan['phases']:
        low, high = p['effort_engineer_weeks']
        deps = ', '.join(p['depends_on']) or 'None'
        index.append(f"| [{p['id']} — {p['title']}](phases/{p['id']}.md) | {p['milestone']} | {deps} | {len(p['feature_ids'])} | {low}–{high} |")
        lines = [f"# {p['id']} — {p['title']}", '', '[Roadmap](../README.md) · [All phases](../PHASES.md)', '',
                 '> Generated from `roadmap.json`; edit the canonical JSON and regenerate.', '', p['outcome'], '',
                 f"**Status:** {p['status']}. **Dependencies:** {deps}. **Effort:** {low}–{high} focused engineer-weeks.", '',
                 f"**Milestone:** {p['milestone']}.", '', '## Work packages', '']
        for w in p['work_packages']:
            lines += [f"### {w['id']}", '', w['description'], '']
        lines += ['## Exit criteria', ''] + [f'- {a}' for a in p['acceptance']]
        lines += ['', 'Apply the [shared Definition of Done](../EXECUTION.md) as well; a visual control alone never completes a feature.', '',
                  '## Deliverables', ''] + [f'- {d}' for d in p['deliverables']]
        lines += ['', '## Quality and risks', '', f"**Fixtures:** {', '.join(p['fixtures'])}. Use only the supported subset at this phase and record its parameters.", '',
                  f"**Quality requirements:** {', '.join(p['quality_requirements'])}; see [the quality catalog](../../catalog/nonfunctional.json).", '',
                  *p['risks'], '', '## Feature completion targets', '',
                  'IDs below identify primary completion targets. Prerequisite subsets may ship earlier; completion requires the catalog acceptance criteria and the phase evidence. The catalog and phase specifications are maintained in English.', '']
        for fid in p['feature_ids']:
            f = features[fid]
            lines.append(f"- [{fid}](../../catalog/domains/{f['domain'].lower()}.md) — module `{f['owner_module']}`.")
        if not p['feature_ids']:
            lines.append('This phase provides cross-cutting feasibility, release or audit evidence; no additional capability is counted.')
        libs = [lib for lib in libraries if lib['adoption_phase'] == p['id']]
        lines += ['', '## Reuse evaluation', '']
        lines += [f"- `{lib['id']}` — {lib['name']}: {lib['status']}." for lib in libs] or ['Use the adapters already adopted; avoid a new dependency without an evidence-backed decision.']
        lines += ['', 'See the [dependency register](../LIBRARIES.md) for upstream sources, boundaries, evaluation and fallback. Adoption phases do not forbid earlier spikes.', '']
        files[PLAN / 'phases' / f"{p['id']}.md"] = '\n'.join(lines)
    totals = [sum(p['effort_engineer_weeks'][i] for p in plan['phases']) for i in (0, 1)]
    index += ['', f"Total planning envelope: **{totals[0]}–{totals[1]} focused engineer-weeks** including optional branches. See [estimation assumptions](ESTIMATES.md); this is not a calendar commitment.", '']
    files[PLAN / 'PHASES.md'] = '\n'.join(index)
    lines = ['# Open-source dependency register', '', '> Generated from `libraries.json`. Actual experimental adoption is recorded below; see [build evidence](../implementation/DEPENDENCIES.md).', '',
             'The user approved C++20 + Qt 6/QML and the reuse of proven, efficient open-source libraries. `experimental_adopted` means used in the bounded prototype, with production gates still open. `selected_for_plan` means intended adoption subject to pinned-version checks; `candidate_requires_spike` means no adoption decision yet. Upstream maturity does not prove performance in OPEN-TOON.', '',
             'Record exact source revision, package checksum, build flags, enabled modules, license files, transitive dependencies and benchmark evidence when adopting a library. Each adapter must have one accountable owner. Model/brush/font/fixture licenses are separate from library licenses.', '']
    for lib in libraries:
        lines += [f"## {lib['id']} — {lib['name']}", '', f"**Phase:** {lib['adoption_phase']}. **Decision:** `{lib['status']}`.", '',
                  lib['purpose'], '', f"**Boundary:** {lib['integration_boundary']}", '', f"**Evaluation:** {lib['evaluation']}", '',
                  f"**Fallback:** {lib['fallback']}", '', f"**License investigation:** {lib['license_notes']}.", '',
                  f"**Primary source:** [{lib['name']} upstream]({lib['upstream_url']}).", '']
    lines += ['## Additional primary references', '',
              '- [Earcut algorithm and its limitations](https://github.com/mapbox/earcut.hpp).',
              '- [KDDockWidgets Qt Quick notes](https://github.com/KDAB/KDDockWidgets/blob/main/README-QtQuick.md).',
              '- [Rhubarb licensing](https://github.com/DanielSWolf/rhubarb-lip-sync/blob/master/LICENSE.md).',
              '- [Qt RHI compatibility](https://doc.qt.io/qt-6/qrhi.html).', '',
              'Before adoption, verify recent release activity, unresolved platform blockers, security advisories, supported build tools and whether the actual workload benefits. No unpinned dependency is approved for a release merely because it appears in this register.', '']
    files[PLAN / 'LIBRARIES.md'] = '\n'.join(lines)
    return files


def validate():
    errors = []
    plan = read('roadmap.json')
    phases = plan['phases']
    phase_map = {p['id']: p for p in phases}
    assignments = plan['feature_assignments']
    catalog = json.loads((ROOT / 'docs/catalog/features.json').read_text())['features']
    feature_ids = {f['id'] for f in catalog}
    nfr_ids = {n['id'] for n in json.loads((ROOT / 'docs/catalog/nonfunctional.json').read_text())['requirements']}
    assigned = [a['feature_id'] for a in assignments]
    if len(phase_map) != len(phases):
        errors.append('Duplicate phase IDs')
    if len(assigned) != len(set(assigned)) or set(assigned) != feature_ids:
        errors.append('Every catalog feature must have exactly one primary phase assignment')
    flattened = [f for p in phases for f in p['feature_ids']]
    if len(flattened) != len(set(flattened)) or set(flattened) != feature_ids:
        errors.append('Phase feature lists do not cover the catalog exactly once')
    assignment_map = {a['feature_id']: a['completion_phase'] for a in assignments}
    work_ids = []
    visited = set()
    def visit(pid, active):
        if pid in active:
            errors.append('Phase dependency cycle: ' + ' -> '.join([*active, pid])); return
        if pid not in phase_map:
            errors.append(f'Unknown phase dependency: {pid}'); return
        if pid in visited:
            return
        for dep in phase_map[pid]['depends_on']:
            visit(dep, [*active, pid])
        visited.add(pid)
    for p in phases:
        visit(p['id'], [])
        if p['status'] not in {'planned', 'in_progress', 'complete'} or not p['outcome'] or not p['acceptance'] or not p['deliverables']:
            errors.append(f"Incomplete planning fields: {p['id']}")
        lo, hi = p['effort_engineer_weeks']
        if not 0 < lo <= hi:
            errors.append(f"Invalid estimate: {p['id']}")
        if set(p['quality_requirements']) - nfr_ids:
            errors.append(f"Unknown quality requirement: {p['id']}")
        for fid in p['feature_ids']:
            if assignment_map.get(fid) != p['id']:
                errors.append(f'Assignment mismatch: {fid}')
        work_ids += [w['id'] for w in p['work_packages']]
    if len(work_ids) != len(set(work_ids)):
        errors.append('Duplicate work package IDs')
    libraries = read('libraries.json')['libraries']
    if len({lib['id'] for lib in libraries}) != len(libraries):
        errors.append('Duplicate library IDs')
    for lib in libraries:
        if lib['adoption_phase'] not in phase_map or not lib['upstream_url'].startswith('https://'):
            errors.append(f"Invalid library entry: {lib['id']}")
        if lib['status'] not in {'selected_for_plan', 'candidate_requires_spike', 'experimental_adopted'}:
            errors.append(f"Invalid library decision: {lib['id']}")
    nodes = json.loads((ROOT / 'docs/catalog/node-reference.json').read_text())['entries']
    node_plan = read('node-assignments.json')['assignments']
    if len(node_plan) != len(nodes) or {n['node_id'] for n in node_plan} != {n['id'] for n in nodes}:
        errors.append('Every node inventory entry needs one specification owner phase')
    for n in node_plan:
        if n['specification_phase'] not in phase_map:
            errors.append(f"Unknown node phase: {n['node_id']}")
    if plan['product_language'] != 'en':
        errors.append('English product language is required')
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    for name in ('stats', 'render', 'check'):
        sub.add_parser(name)
    for name in ('phase', 'feature', 'library'):
        sub.add_parser(name).add_argument('id')
    args = parser.parse_args()
    plan = read('roadmap.json')
    if args.command == 'render':
        for path, content in documents().items():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding='utf-8')
        print(f'Generated {len(documents())} roadmap views.')
    elif args.command == 'check':
        errors = validate()
        for path, content in documents().items():
            if not path.exists() or path.read_text(encoding='utf-8') != content:
                errors.append(f'Stale roadmap view: {path.relative_to(ROOT)}')
        if errors:
            raise SystemExit('\n'.join(errors))
        print('Roadmap coverage, dependencies, library decisions and generated views valid.')
    elif args.command == 'stats':
        print(json.dumps({'phases': len(plan['phases']), 'work_packages':sum(len(p['work_packages']) for p in plan['phases']), 'assigned_features':len(plan['feature_assignments']), 'libraries':len(read('libraries.json')['libraries'])}, indent=2))
    else:
        if args.command == 'phase':
            result = next((p for p in plan['phases'] if p['id'] == args.id.upper()), None)
        elif args.command == 'library':
            result = next((lib for lib in read('libraries.json')['libraries'] if lib['id'] == args.id.upper()), None)
        else:
            result = next((a for a in plan['feature_assignments'] if a['feature_id'] == args.id.upper()), None)
        if result is None:
            raise SystemExit(f'Unknown {args.command} ID: {args.id}')
        print(json.dumps(result, ensure_ascii=False, indent=2))

if __name__ == '__main__':
    main()
