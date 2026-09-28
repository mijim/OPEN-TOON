#pragma once
class EditorController;
class CanvasItem;
class QQuickWindow;
class QString;
void meshSmoke(EditorController&, CanvasItem&, QQuickWindow&);
void meshInteractionBenchmark(EditorController&, CanvasItem&, QQuickWindow&, const QString& project);
