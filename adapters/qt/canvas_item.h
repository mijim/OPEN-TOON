#pragma once
#include "editor_controller.h"
#include "opentoon/document.h"
#include "raster_brush.h"
#include "revision_render_cache.h"
#include "preview_render_queue.h"
#include <QElapsedTimer>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QPolygonF>
class CanvasItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QVariantMap objectProperties READ objectProperties NOTIFY regionChanged)
    Q_PROPERTY(bool hasRegion READ hasRegion NOTIFY regionChanged)
    Q_PROPERTY(int selectionMedia READ selectionMedia WRITE setSelectionMedia NOTIFY regionChanged)
    Q_PROPERTY(QString regionInfo READ regionInfo NOTIFY regionChanged)
    Q_PROPERTY(EditorController* editor READ editor WRITE setEditor NOTIFY editorChanged)
    Q_PROPERTY(bool motionPathEditing READ motionPathEditing WRITE setMotionPathEditing NOTIFY viewChanged)
    Q_PROPERTY(bool hasVectorClipboard READ hasVectorClipboard NOTIFY regionChanged)
    Q_PROPERTY(bool gridVisible READ gridVisible WRITE setGridVisible NOTIFY viewChanged)
    Q_PROPERTY(bool snapToGrid READ snapToGrid WRITE setSnapToGrid NOTIFY viewChanged)
    Q_PROPERTY(int gridSpacing READ gridSpacing WRITE setGridSpacing NOTIFY viewChanged)
    Q_PROPERTY(bool cameraGuidesVisible READ cameraGuidesVisible WRITE setCameraGuidesVisible NOTIFY viewChanged)
    Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY viewChanged)
    Q_PROPERTY(bool mirrored READ mirrored WRITE setMirrored NOTIFY viewChanged)
    Q_PROPERTY(double rotationAngle READ rotationAngle WRITE setRotationAngle NOTIFY viewChanged)
  public:
    QVariantMap objectProperties() const;
    Q_INVOKABLE void setObjectProperty(QString, double);
    Q_INVOKABLE QPointF handlePosition(int) const;
    bool hasRegion() const { return region_.width > 0 && region_.height > 0; }
    int selectionMedia() const { return int(selectionMedia_); }
    void setSelectionMedia(int);
    QString regionInfo() const;
    Q_INVOKABLE void clearRegion();
    Q_INVOKABLE void selectAllVectors(bool invert = false);
    Q_INVOKABLE bool copyVectorSelection(bool cut = false);
    Q_INVOKABLE bool pasteVectorSelection();
    Q_INVOKABLE bool editVectorSelection(QString operation, double value);
    Q_INVOKABLE bool nudgeVectorSelection(int dx, int dy);
    bool hasVectorClipboard() const { return !vectorClipboard_.strokes.empty(); }
    bool gridVisible() const { return gridVisible_; }
    bool snapToGrid() const { return snapToGrid_; }
    int gridSpacing() const { return gridSpacing_; }
    void setGridVisible(bool);
    void setSnapToGrid(bool);
    void setGridSpacing(int);
    bool cameraGuidesVisible() const { return cameraGuidesVisible_; }
    void setCameraGuidesVisible(bool value) { cameraGuidesVisible_ = value; emit viewChanged(); update(); }

    Q_INVOKABLE void transformRegion(int action, int dx = 0, int dy = 0);
    explicit CanvasItem(QQuickItem* parent = nullptr);
    EditorController* editor() const { return editor_; }
    void setEditor(EditorController*);
    bool motionPathEditing() const { return motionPathEditing_; }
    void setMotionPathEditing(bool);
    Q_INVOKABLE QPointF motionPathPosition(int frame) const;
    double zoom() const { return zoom_; }
    void setZoom(double);
    bool mirrored() const { return mirrored_; }
    void setMirrored(bool);
    double rotationAngle() const { return angle_; }
    void setRotationAngle(double);
    void paint(QPainter*) override;
    Q_INVOKABLE void fit();
    Q_INVOKABLE void smoothSelection();
    Q_INVOKABLE void deleteSelection();
    Q_INVOKABLE void cancelGesture();
    Q_INVOKABLE void capture(QString path);
    bool hasPreparedFrame(int frame);
    Q_INVOKABLE QPointF cameraHandlePosition(int index) const;
  signals:
    void regionChanged();
    void editorChanged();
    void viewChanged();

  protected:
    void hoverMoveEvent(QHoverEvent*) override;
    void hoverEnterEvent(QHoverEvent*) override;
    void hoverLeaveEvent(QHoverEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseUngrabEvent() override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;

  private:
    struct MotionSample {
        opentoon::Frame frame;
        QPointF position;
    };
    bool motionPathEditing_ = false, motionReferenceValid_ = false;
    int motionKey_ = -1;
    QPointF motionReference_, motionPress_;
    opentoon::Id motionReferenceLayer_ = 0;
    std::uint64_t motionReferenceScene_ = 0;
    QTransform motionParentInverse_;
    void resetMotionReference();
    std::vector<MotionSample> motionSamples(const opentoon::Document&, QPointF reference) const;
    int motionPathKeyAt(QPointF) const;
    int motionPathFrameAt(QPointF) const;
    void beginMotionPath(QPointF);
    void previewMotionPath(QPointF);
    void commitMotionPath();
    void paintMotionPath(QPainter*, const opentoon::Document&, const QTransform& itemTransform);
    QPointF hoverPosition_;
    void updateCursor(QPointF);
    opentoon::Id hitVector(QPointF) const;
    void selectAnimationBounds();
    bool handleVisible(int) const;
    void previewPose(QTransform);
    std::optional<opentoon::Document> posePreview_;
    opentoon::Transform sourcePose_, previewPose_;
    opentoon::Drawing pointDrawingPreview_;
    opentoon::PixelRect region_;
    std::vector<opentoon::Id> regionStrokes_;
    opentoon::Drawing transformSource_, transformPreview_;
    QTransform pendingTransform_;
    int transformHandle_ = -2;
    bool transforming_ = false, previewValid_ = false, committing_ = false, shift_ = false;
    QTransform selectionWorld() const;
    void selectStroke(opentoon::Id);
    void setVectorSelection(std::vector<opentoon::Id>);
    void modifyVectorSelection(const std::vector<opentoon::Id>&, int operation);
    void paintVectorSelection(QPainter*, const QTransform& itemTransform);
    opentoon::SelectionMedia activeSelectionMedia() const;
    bool vectorSelection_ = false, subtract_ = false;
    opentoon::VectorBlock vectorClipboard_;
    bool gridVisible_ = false, snapToGrid_ = false;
    bool cameraGuidesVisible_ = false;
    int cameraHandle_ = -1;
    QPointF cameraPress_;
    opentoon::Transform cameraSourcePose_;
    int gridSpacing_ = 40;
    opentoon::Point snapDrawingPoint(opentoon::Point) const;
    opentoon::Point constrainedEndpoint(opentoon::Point) const;
    void paintGrid(QPainter*);
    QPolygonF cameraFrame(const opentoon::Document&) const;
    int cameraHandleAt(QPointF) const;
    void paintCameraGuide(QPainter*, const QTransform&);
    void beginCamera(QPointF);
    void previewCamera(QPointF);
    void commitCamera();
    void schedulePreview(const opentoon::RenderCacheKey&);

    int marqueeOperation_ = 0; // Replace, add, subtract; captured on press.

    void startTransform(int);
    void previewTransform(QTransform);
    void commitTransform();
    opentoon::SelectionMedia selectionMedia_ = opentoon::SelectionMedia::Both;
    bool movingRegion_ = false;
    QPointer<EditorController> editor_;
    opentoon::RevisionRenderCache previewCache_;
    opentoon::PreviewRenderQueue previewQueue_{previewCache_};
    QString previousTool_;
    QPointer<QQuickWindow> filteredWindow_;
    double zoom_ = 1, angle_ = 0;
    bool mirrored_ = false, drawing_ = false, panning_ = false, tablet_ = false;
    QPointF pan_, panStart_, last_, selectionDelta_;
    opentoon::Id selectedStroke_ = 0;
    int selectedPoint_ = -1;
    opentoon::Point pointPreview_;
    std::vector<opentoon::Point> samples_;
    std::unique_ptr<opentoon::RasterBrush> rasterBrush_;
    opentoon::Drawing rasterPreview_;
    QElapsedTimer sampleClock_;
    double tiltX_ = 0, tiltY_ = 0;
    QTransform viewTransform() const;
    QTransform contentTransform() const;
    opentoon::Point localPoint(QPointF, double) const;
    void begin(QPointF, double);
    void move(QPointF, double);
    void end();
};
