#!/usr/bin/env python3
"""Validate catalog references, dependency graph, generated views and local links."""
from __future__ import annotations
import json
from pathlib import Path
import re
import sys
from urllib.parse import unquote
from catalog import ROOT, read, documents
from roadmap import validate as validate_roadmap, documents as roadmap_documents

errors = []
features = read('features.json')['features']
domains = read('domains.json')['domains']
nodes = read('node-reference.json')['entries']
nfr = read('nonfunctional.json')['requirements']


def unique(items, kind):
    ids = [item['id'] for item in items]
    if len(ids) != len(set(ids)):
        errors.append(f'Duplicate {kind} IDs')
    return set(ids)


feature_ids = unique(features, 'feature')
domain_ids = unique(domains, 'domain')
unique(nodes, 'node')
unique(nfr, 'nonfunctional')
required = {'id', 'domain', 'title', 'requirement', 'acceptance_criteria', 'evidence', 'implementation_status', 'scope', 'capability_level', 'owner_module', 'depends_on_domains', 'specification_status'}
for f in features:
    if not required <= f.keys():
        errors.append(f"Missing fields in {f['id']}")
    if not re.fullmatch(r'[A-Z]{2,3}-\d{3}', f['id']):
        errors.append(f"Invalid ID: {f['id']}")
    if f['domain'] not in domain_ids or not f['id'].startswith(f['domain'] + '-'):
        errors.append(f"Invalid domain: {f['id']}")
    if not f['acceptance_criteria'] or any(not str(x).strip() for x in f['acceptance_criteria']):
        errors.append(f"Missing acceptance: {f['id']}")
    if f['implementation_status'] not in {'not_started', 'in_progress', 'partial', 'implemented', 'verified'}:
        errors.append(f"Invalid implementation status: {f['id']}")
    if f['scope'] not in {'base', 'studio_extension', 'optional_extension', 'legacy_candidate'}:
        errors.append(f"Invalid scope: {f['id']}")
    if f['capability_level'] not in {'core', 'pro', 'advanced', 'optional', 'legacy'}:
        errors.append(f"Invalid level: {f['id']}")
    if set(f['depends_on_domains']) - domain_ids:
        errors.append(f"Unknown dependency: {f['id']}")

graph = {d['id']: d['depends_on'] for d in domains}
visited = set()

def visit(key, active):
    if key in active:
        errors.append('Domain dependency cycle: ' + ' -> '.join([*active, key]))
        return
    if key in visited:
        return
    if key not in graph:
        errors.append(f'Unknown domain dependency: {key}')
        return
    for dep in graph[key]:
        visit(dep, [*active, key])
    visited.add(key)

for domain in graph:
    visit(domain, [])
errors.extend(validate_roadmap())
for path, text in {**documents(), **roadmap_documents()}.items():
    if not path.exists() or path.read_text(encoding='utf-8') != text:
        errors.append(f'Stale generated view: {path.relative_to(ROOT)}')
for path in [ROOT / 'README.md', ROOT / 'AGENTS.md', ROOT / 'CONTRIBUTING.md', *(ROOT / 'docs').rglob('*.md')]:
    for link in re.findall(r'\]\(([^)]+)\)', path.read_text(encoding='utf-8')):
        if link.startswith(('http://', 'https://', 'mailto:', '#')):
            continue
        target = unquote(link.split('#', 1)[0]).strip('<>')
        if target and not (path.parent / target).exists():
            errors.append(f'Broken local link: {path.relative_to(ROOT)} -> {target}')
if errors:
    print('\n'.join(errors), file=sys.stderr)
    raise SystemExit(1)
print(f'OK: {len(features)} features, {len(domains)} domains, {len(nodes)} node entries, {len(nfr)} quality requirements; generated views and local links valid.')
