# PRJ — Projects, scenes and persistence

[Back to catalog](../README.md)

> Generated from `features.json` and `domains.json`; do not edit manually.

**Workflow:** Create scene → configure → save → recover → package.

**Module:** `project`.

**Entities:** Project, Scene, SceneSettings, AssetRef, Revision.

**Relationships:** None.

**Main risk:** Data loss and broken references.

## Shared contract

Mutations must respect transactions, undo/redo and persistence. Visual aids are not exported. Errors, cancellation and unsupported data must preserve the last valid state. These OPEN-TOON conditions will be specified per operation during implementation.

## PRJ-001 — Create and open scenes

Create local scenes with an explicit name, location and configuration.

**Initial acceptance:** Reopening a new scene preserves its dimensions and duration.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## PRJ-002 — Resolution and aspect ratio

Configure width, height, pixel aspect ratio and custom presets.

**Initial acceptance:** A non-square-pixel preset produces the expected framing.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## PRJ-003 — Frame rate and duration

Define frame rate and length; specify whether changing frame rate preserves frame count or elapsed time.

**Initial acceptance:** Changing from 24 to 25 fps explains the timing effect and can be undone.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## PRJ-004 — Save and save a copy

Persist scene data and resources coherently; saving a copy preserves the original.

**Initial acceptance:** The copy opens from another location without depending on the original directory.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## PRJ-005 — Autosave

Schedule recovery points without blocking drawing or silently replacing a manual save.

**Initial acceptance:** After a forced shutdown, the latest valid recovery point can be restored.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## PRJ-006 — Scene versions

Retain identifiable revisions and open a selected version.

**Initial acceptance:** An earlier revision retains its drawings when a newer revision changes them.

**Scope:** `base` · **Level:** `pro` · **Status:** `partial`.

**Evidence:** `proposal`.

## PRJ-007 — Compact project format

Package and unpack scenes with their resources included.

**Initial acceptance:** Both representations render the same image after reopening.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## PRJ-008 — Project integrity

Detect missing files, invalid references and corrupt components with actionable diagnostics.

**Initial acceptance:** A missing texture appears in a report without crashing the editor.

**Scope:** `base` · **Level:** `core` · **Status:** `partial`.

**Evidence:** `proposal`.

## PRJ-009 — Backup optimization

List backup size and usage before removing redundant resources.

**Initial acceptance:** Resources still referenced by another revision are not deleted.

**Scope:** `base` · **Level:** `pro` · **Status:** `not_started`.

**Evidence:** `proposal`.

## PRJ-010 — Recent scenes and welcome screen

Show recent scenes and actions to create, open or locate a moved file.

**Initial acceptance:** A missing recent entry can be removed without deleting other files.

**Scope:** `base` · **Level:** `core` · **Status:** `not_started`.

**Evidence:** `proposal`.
