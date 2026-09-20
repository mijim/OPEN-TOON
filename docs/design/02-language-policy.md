# English product language

**User requirement, 2026-09-20:** the interface and the entire product must be in English. This requirement applies to every delivery phase and is tracked as `NFR-027`.

All first-party menus, tool names, buttons, tooltips, placeholders, dialogs, notifications, errors, keyboard shortcut descriptions, accessibility names, preset/template names, installer text, help and public product documentation use English. Code identifiers, comments and API documentation also use English. Release notes, contribution guides and new project planning documents follow the same rule.

Use clear, consistent animation terminology: **Drawing**, **Exposure**, **Keyframe**, **Layer**, **Peg**, **Palette**, **Swatch**, **Onion Skin**, **Deformer**, **Node**, **Scene** and **Render**. Distinguish a stored drawing from its exposure over time; distinguish timeline frames from audio samples. Keep this glossary in the command registry and help documentation as features arrive.

Use centralized Qt translatable string mechanisms and named interpolation arguments. The initial product language remains English even on a Spanish operating system. Keep architecture ready for future localization without adding other shipped UI languages unless the product scope changes. Do not build sentences by joining translated fragments.

English product language does not restrict artists' content: Unicode layer names, filenames, text drawings, captions and dialogue round-trip unchanged. Imported proper names and third-party attributions retain their provenance. Numeric editing may honor an explicit input locale, but saved files and API interchange use documented locale-independent representations. Test decimal separators and shortcuts under non-English OS settings.

The previous Spanish research, Markdown documents and canonical catalogs were translated in this planning update, preserving stable feature/node IDs and behavior. P01 verifies terminology against the implemented command registry and packaged UI; translation is no longer deferred to that phase.

Verification combines a string/preset inventory, English review of packaged surfaces, keyboard/screen-reader checks and multilingual-content fixtures. Regex or dictionary checks can flag omissions but cannot establish correct terminology or usable error messages alone. English conversation with contributors is the repository default; the current owner can continue discussing the project in Spanish.
