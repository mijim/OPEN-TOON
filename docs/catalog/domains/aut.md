# AUT — Scripting and automation

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Automate edits → execute commands → batch process → integrate pipeline.

**Module:** `scripting`.

**Entities:** CommandAPI, Script, ScriptContext, BatchJob.

**Relationships:** PRJ, NOD, OUT.

**Main risk:** Executable external code and irreversible changes.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## AUT-001 — Script editing and execution

Create and execute scripts with explicit output, errors and scene context.

**Initial acceptance:** An error identifies the script and location while preserving the last valid state.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## AUT-002 — Script actions in the UI

Assign scripts to identifiable buttons and actions.

**Initial acceptance:** A missing action is disabled with an explanation.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## AUT-003 — Scene and node API

Expose queries and commands for layers, attributes, drawings and connections.

**Initial acceptance:** An API modification updates the same views as a UI operation.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## AUT-004 — Python integration

Support external automation through a versioned API and error contracts.

**Initial acceptance:** An external process can open a copy, query it and render without a UI.

**Scope:** `base` · **Level:** `advanced` · **Status:** `not_started`.

**Specification source:** `proposal`.

## AUT-005 — Sandbox and execution context

Separate script testing and execution policies from the document.

**Initial acceptance:** Opening someone else's scene does not execute scripts automatically.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.

## AUT-006 — CLI conversion tools

Expose validation, conversion and rendering as commands with structured results.

**Initial acceptance:** An invalid file returns a nonzero exit code and readable diagnostics.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Specification source:** `proposal`.
