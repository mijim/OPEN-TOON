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


def slice_schedule(plan):
    """Dependency-only labor lower bounds; no staffing or calendar promise."""
    slices = {s['id']: s for s in plan['delivery_slices']}
    finish = {}
    paths = {}
    def visit(sid):
        if sid in finish:
            return
        item = slices[sid]
        parents = [r['slice'] for r in item['requires']]
        for parent in parents:
            visit(parent)
        finish[sid], paths[sid] = [], []
        for bound in (0, 1):
            parent = max(parents, key=lambda p: finish[p][bound]) if parents else None
            finish[sid].append((finish[parent][bound] if parent else 0) + item['effort_engineer_weeks'][bound])
            paths[sid].append((paths[parent][bound] if parent else []) + [sid])
    for sid in slices:
        visit(sid)
    terminal = plan['harmony_moment']['terminal_slice']
    return {'total_engineer_weeks': [sum(s['effort_engineer_weeks'][i] for s in slices.values()) for i in (0, 1)],
            'dependency_only_chain_weeks': finish[terminal], 'limiting_paths': paths[terminal]}


def documents():
    plan = read('roadmap.json')
    libraries = read('libraries.json')['libraries']
    catalog = json.loads((ROOT / 'docs/catalog/features.json').read_text())['features']
    features = {f['id']: f for f in catalog}
    files = {}
    index = ['# Phase index', '', '> Generated from `roadmap.json`. Phase status is separate from feature completion; see [implementation evidence](../implementation/STATUS.md).', '',
             'Implementation order is governed by the [Harmony Moment contract graph](HARMONY-MOMENT.md). Phase dependencies below concern full epic completion, not permission to start a bounded subset. Phase IDs are stable scope owners, not a serial schedule.', '',
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
        owned = [s for s in plan['delivery_slices'] if s['owner_phase'] == p['id']]
        lines += ['## Delivery priority', '',
                  'Use contract-level prerequisites for early delivery. Full phase exit criteria and catalog acceptance remain unchanged unless explicitly revised below.', '']
        lines += [f"- [{s['id']} — {s['title']}](../HARMONY-MOMENT.md#{s['id'].lower()}): {s['scope']}" for s in owned] or ['No critical-path slice is owned here; retained work follows the Harmony Moment unless a measured blocker requires it.']
        lines += ['', '## Exit criteria', ''] + [f'- {a}' for a in p['acceptance']]
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
                  f"**Delivery:** {lib['delivery_priority']}; slices: {', '.join(lib['harmony_moment_slices']) or 'none required for HM'}.", '',
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
    milestone = plan['harmony_moment']
    schedule = slice_schedule(plan)
    lines = ['# Harmony Moment delivery plan', '', '> Generated from `roadmap.json`; edit the canonical JSON and regenerate.', '',
             '[Rationale and code audit](REORIENTATION.md) · [Next tasks](FIRST-STEPS.md) · [Full phases](PHASES.md)', '',
             milestone['title'] + '. **Status: ' + milestone['status'] + '.** No new feature is implemented by this plan.', '',
             '## Acceptance profile', '']
    lines += [f'- **{key.replace("_", " ").capitalize()}:** {value}' for key, value in milestone['profile'].items()]
    lines += ['', '## Scheduling rules', '', plan['execution_policy']['phase_dependency_semantics'], '',
              plan['execution_policy']['entry_rule'], '', plan['execution_policy']['staffing'], '',
              '## Contract dependency graph', '', '```mermaid', 'flowchart TD']
    for s in plan['delivery_slices']:
        lines.append(f'  {s["id"].replace("-", "")}["{s["id"]} {s["title"]}"]')
        for r in s['requires']:
            lines.append(f'  {r["slice"].replace("-", "")} --> {s["id"].replace("-", "")}')
    lines += ['```', '', '## Work and ordering', '',
              f"Remaining bounded work: **{schedule['total_engineer_weeks'][0]}–{schedule['total_engineer_weeks'][1]} engineer-weeks**, low confidence. This is included within phase scope, not added to its full-program envelope.", '',
              'Dependency-only longest chains under the lower/upper estimates (unlimited staffing, no resource contention; **not delivery dates**):', '']
    for i, name in enumerate(('Lower', 'Upper')):
        lines.append(f"- {name}: {' → '.join(schedule['limiting_paths'][i])}; {schedule['dependency_only_chain_weeks'][i]} serial engineer-weeks.")
    lines += ['', '| Slice | Owner | Hard prerequisites | Remaining engineer-weeks |', '|---|---|---|---:|']
    for s in plan['delivery_slices']:
        lines.append(f"| [{s['id']}](#{s['id'].lower()}) — {s['title']} | {s['owner_phase']} / {s['owner_module']} | {', '.join(r['slice'] for r in s['requires']) or 'None'} | {'–'.join(map(str,s['effort_engineer_weeks']))} |")
    for s in plan['delivery_slices']:
        lines += ['', f"## {s['id']}", '', f"**{s['title']}** — `{s['status']}`; owner `{s['owner_module']}`, work package `{s['work_package']}`.", '',
                  s['scope'], '', '**Required delivered contracts:**', '']
        lines += [f"- {r['slice']}: {r['contract']}" for r in s['requires']] or ['- No earlier slice. Reuse evidence is an input to review, not a passed gate.']
        lines += ['', '**Acceptance:**', ''] + [f'- {a}' for a in s['acceptance']]
        lines += ['', '**Explicitly outside this slice:**', ''] + [f'- {a}' for a in s['excluded']]
        lines += ['', f"**Catalog subsets:** {', '.join(s['feature_ids'])}.", '',
                  f"**Library boundaries:** {', '.join(s['library_ids']) or 'existing adapters; no new library required by this slice'}.", '',
                  '**Existing evidence to inspect (not slice completion):**', '']
        lines += [f'- [{ref}](../../{ref})' for ref in s['reuse_evidence']] or ['- New subsystem; no implementation claimed.']
    lines += ['', '## Claims excluded from this milestone', ''] + [f'- {v}' for v in milestone['not_claimed']]
    lines += ['', '## Retained follow-on work', '',
              'Each catalog feature still has exactly one primary completion phase. A slice only schedules its declared subset; it does not weaken the full feature acceptance.', '',
              '| Feature | Primary phase | Priority | Reason |', '|---|---|---|---|']
    for a in plan['feature_assignments']:
        if a['delivery_priority'] == 'next_after_harmony_moment':
            lines.append(f"| {a['feature_id']} — {features[a['feature_id']]['title']} | {a['completion_phase']} | next | {a['scheduling_note']} |")
    lines += ['', 'All other retained requirements and their deferral notes are queryable with `roadmap.py feature ID`. The full catalog and long-term phases remain authoritative.', '']
    files[PLAN / 'HARMONY-MOMENT.md'] = '\n'.join(lines)
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
    if plan['schema_version'] != 2:
        errors.append('Roadmap schema 2 is required for contract-level delivery slices')
    slices = plan.get('delivery_slices', [])
    slice_map = {s['id']: s for s in slices}
    if not slices or len(slices) != len(slice_map):
        errors.append('Delivery slice IDs must be nonempty and unique')
    lib_map = {lib['id']: lib for lib in libraries}
    coverage = {fid: [] for fid in feature_ids}
    seen = set()
    def visit_slice(sid, active):
        if sid in active:
            errors.append('Delivery dependency cycle: ' + ' -> '.join([*active, sid])); return
        if sid not in slice_map:
            errors.append(f'Unknown delivery prerequisite: {sid}'); return
        if sid in seen:
            return
        for edge in slice_map[sid]['requires']:
            visit_slice(edge['slice'], [*active, sid])
        seen.add(sid)
    for s in slices:
        visit_slice(s['id'], [])
        phase = phase_map.get(s['owner_phase'])
        if not phase or s['work_package'] not in {w['id'] for w in phase['work_packages']}:
            errors.append(f"Invalid slice work-package owner: {s['id']}")
        if s['status'] not in {'planned', 'in_progress', 'complete'} or not all(s.get(k) for k in ('title', 'scope', 'owner_module', 'acceptance', 'excluded', 'feature_ids')):
            errors.append(f"Incomplete delivery slice: {s['id']}")
        lo, hi = s['effort_engineer_weeks']
        if not 0 < lo <= hi:
            errors.append(f"Invalid delivery estimate: {s['id']}")
        if len(s['feature_ids']) != len(set(s['feature_ids'])) or set(s['feature_ids']) - feature_ids:
            errors.append(f"Unknown/duplicate slice feature: {s['id']}")
        for fid in s['feature_ids']:
            if fid in coverage:
                coverage[fid].append(s['id'])
        edges = [r['slice'] for r in s['requires']]
        if len(edges) != len(set(edges)) or any(not r.get('contract', '').strip() for r in s['requires']):
            errors.append(f"Missing/duplicate prerequisite contract: {s['id']}")
        if set(s['library_ids']) - lib_map.keys():
            errors.append(f"Unknown slice library: {s['id']}")
        for path in s['reuse_evidence']:
            if not (ROOT / path).exists():
                errors.append(f"Missing reuse evidence: {s['id']} -> {path}")
        if s['status'] == 'complete':
            if not s.get('completion_evidence') or any(slice_map.get(r, {}).get('status') != 'complete' for r in edges):
                errors.append(f"Unproven delivery completion: {s['id']}")
    for a in assignments:
        sid_list = a.get('delivery_slices', [])
        if sorted(sid_list) != sorted(coverage.get(a['feature_id'], [])):
            errors.append(f"Delivery assignment mismatch: {a['feature_id']}")
        if a.get('delivery_priority') not in {'harmony_moment_subset', 'next_after_harmony_moment', 'long_term'} or not a.get('scheduling_note'):
            errors.append(f"Missing delivery disposition: {a['feature_id']}")
        if bool(sid_list) != (a.get('delivery_priority') == 'harmony_moment_subset'):
            errors.append(f"Contradictory delivery priority: {a['feature_id']}")
    for lib in libraries:
        expected = {s['id'] for s in slices if lib['id'] in s['library_ids']}
        if set(lib.get('harmony_moment_slices', [])) != expected:
            errors.append(f"Library delivery mismatch: {lib['id']}")
        if lib.get('delivery_priority') != ('harmony_moment_boundary' if expected else 'after_harmony_moment'):
            errors.append(f"Library delivery priority mismatch: {lib['id']}")
    for node in node_plan:
        sid = node.get('delivery_slice')
        if sid and (sid not in slice_map or node['specification_phase'] != slice_map[sid]['owner_phase']):
            errors.append(f"Invalid node delivery owner: {node['node_id']}")
        if node.get('delivery_priority') != ('harmony_moment_subset' if sid else 'long_term'):
            errors.append(f"Node delivery priority mismatch: {node['node_id']}")
    milestone = plan.get('harmony_moment', {})
    required = milestone.get('required_slices', [])
    terminal = milestone.get('terminal_slice')
    if milestone.get('status') not in {'planned', 'in_progress', 'complete'} or not milestone.get('profile'):
        errors.append('Missing milestone status/profile')
    if len(required) != len(set(required)) or set(required) != set(slice_map) or terminal not in slice_map:
        errors.append('Harmony Moment must include every declared delivery slice and a valid terminal gate')
    ancestors = set()
    def collect(sid):
        if sid in ancestors or sid not in slice_map:
            return
        ancestors.add(sid)
        for edge in slice_map[sid]['requires']:
            collect(edge['slice'])
    collect(terminal)
    if ancestors != set(required):
        errors.append('Harmony Moment terminal gate does not depend on every required slice')
    if milestone.get('status') == 'complete' and any(s['status'] != 'complete' for s in slices):
        errors.append('Harmony Moment cannot complete before all delivery slices')
    order = plan.get('execution_policy', {}).get('priority_order', [])
    if len(order) != len(set(order)) or set(order) != set(slice_map):
        errors.append('Delivery priority order must list every slice once')
    else:
        positions = {sid: i for i, sid in enumerate(order)}
        for s in slices:
            if any(positions.get(r['slice'], -1) >= positions[s['id']] for r in s['requires']):
                errors.append(f"Delivery priority precedes prerequisite: {s['id']}")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    for name in ('stats', 'render', 'check', 'next'):
        sub.add_parser(name)
    for name in ('phase', 'feature', 'library', 'slice', 'milestone'):
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
        print(json.dumps({'phases': len(plan['phases']), 'work_packages':sum(len(p['work_packages']) for p in plan['phases']), 'assigned_features':len(plan['feature_assignments']), 'libraries':len(read('libraries.json')['libraries']), 'delivery_slices':len(plan['delivery_slices']), **slice_schedule(plan)}, indent=2))
    elif args.command == 'next':
        slices = {s['id']: s for s in plan['delivery_slices']}
        eligible = [sid for sid in plan['execution_policy']['priority_order'] if slices[sid]['status'] != 'complete'
                    and all(slices[r['slice']]['status'] == 'complete' for r in slices[sid]['requires'])]
        print(json.dumps({'eligible_after_contract_evidence': eligible, 'rule': plan['execution_policy']['entry_rule'],
                          'current_instruction': plan['execution_policy']['current_instruction']}, indent=2))
    else:
        if args.command == 'phase':
            result = next((p for p in plan['phases'] if p['id'] == args.id.upper()), None)
        elif args.command == 'library':
            result = next((lib for lib in read('libraries.json')['libraries'] if lib['id'] == args.id.upper()), None)
        elif args.command == 'slice':
            result = next((s for s in plan['delivery_slices'] if s['id'] == args.id.upper()), None)
        elif args.command == 'milestone':
            result = plan['harmony_moment'] if args.id.upper() == plan['harmony_moment']['id'] else None
        else:
            result = next((a for a in plan['feature_assignments'] if a['feature_id'] == args.id.upper()), None)
        if result is None:
            raise SystemExit(f'Unknown {args.command} ID: {args.id}')
        print(json.dumps(result, ensure_ascii=False, indent=2))

if __name__ == '__main__':
    main()
