#pragma once
#include "editor_controller.h"
#include "opentoon/document.h"
#include "raster_brush.h"
#include <QElapsedTimer>
#include <QPointer>
#include <QQuickPaintedItem>
class CanvasItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool hasRegion READ hasRegion NOTIFY regionChanged)
    Q_PROPERTY(int selectionMedia READ selectionMedia WRITE setSelectionMedia NOTIFY regionChanged)
    Q_PROPERTY(QString regionInfo READ regionInfo NOTIFY regionChanged)
    Q_PROPERTY(EditorController* editor READ editor WRITE setEditor NOTIFY editorChanged)
    Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY viewChanged)
    Q_PROPERTY(bool mirrored READ mirrored WRITE setMirrored NOTIFY viewChanged)
    Q_PROPERTY(double rotationAngle READ rotationAngle WRITE setRotationAngle NOTIFY viewChanged)
  public:
    bool hasRegion() const { return region_.width > 0 && region_.height > 0; }
    int selectionMedia() const { return int(selectionMedia_); }
    void setSelectionMedia(int);
    QString regionInfo() const;
    Q_INVOKABLE void clearRegion();
    Q_INVOKABLE void transformRegion(int action, int dx = 0, int dy = 0);
    explicit CanvasItem(QQuickItem* parent = nullptr);
    EditorController* editor() const { return editor_; }
    void setEditor(EditorController*);
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
  signals:
    void regionChanged();
    void editorChanged();
    void viewChanged();

  protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseUngrabEvent() override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;

  private:
    opentoon::PixelRect region_;
    opentoon::SelectionMedia selectionMedia_ = opentoon::SelectionMedia::Both;
    bool movingRegion_ = false;
    QPointer<EditorController> editor_;
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
    opentoon::Point localPoint(QPointF, double) const;
    void begin(QPointF, double);
    void move(QPointF, double);
    void end();
};
