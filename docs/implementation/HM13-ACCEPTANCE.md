# HM-13 bounded output-camera acceptance

The HM-13 one-camera orthographic contract is accepted on the macOS development
profile. It delivers these observable behaviors:

1. Add an explicit output camera from the Scene menu without moving artwork.
2. Keep the shot to one active output camera and reject a second camera.
3. Select a dedicated Camera tool with an illustrative icon and contextual cursor.
4. Drag inside the camera frame to pan the shot directly.
5. Hold Shift while panning to constrain movement to one axis.
6. Drag the rotation handle to rotate the output view.
7. Hold Shift while rotating to snap to 15-degree increments.
8. Drag a frame corner to zoom the output view.
9. Set zoom numerically in the compact camera bar or selected-camera Properties.
10. Reset the camera to the scene center and unit zoom.
11. Show the output frame and safe-area guides without writing them into pixels.
12. Keep viewport pan, fit and zoom independent of output framing.
13. Animate camera pose with the existing keyframe and curve evaluator.
14. Use the same evaluated camera in legacy and linear Display/Write output.
15. Preserve drawing-local editing coordinates when the output camera moves.
16. Reopen format-7 projects with the same camera pose and rendered pixels.
17. Export PNG frames using the same camera pose as preview and reopened scenes.
18. Undo/redo direct camera gestures as atomic document edits.
19. Delete and undo camera deletion without leaving a stale UI selection.
20. Reject locked-camera edits and invalid, singular or unsafe zoom keys.

The native `--smoke-test` exercises direct pan/rotate/zoom, undo/redo, guide
isolation, viewport navigation, save/reopen and camera deletion. Render tests
compare projected and unprojected pixels, animated keys, serialization and
invalid-camera rejection. The export integration test compares PNGs at two
camera keys with the matching SceneRenderer output after project reopening.
Format-6 migration tests verify the backup and newly stored camera.

These are 20 bounded user-facing behaviors, not 20 completed catalog
capabilities. HM-04's playback/scrub and color/alpha closure is additional
contract work from this turn. The full P06/P10 phases remain open.
The camera remains orthographic and single-output; multiplane, perspective and
switching cameras are later work.
