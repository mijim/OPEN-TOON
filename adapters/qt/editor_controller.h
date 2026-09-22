#pragma once
#include "opentoon/drawing_selection.h"
#include "opentoon/key_block.h"
#include "opentoon/session.h"
#include "opentoon/timeline.h"
#include "opentoon/vector_edit.h"
#include <QColor>
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <atomic>
#include <thread>
class EditorController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool animateMode READ animateMode WRITE setAnimateMode NOTIFY animationModeChanged)
    Q_PROPERTY(bool autoKey READ autoKey WRITE setAutoKey NOTIFY animationModeChanged)
    Q_PROPERTY(QVariantList animationKeys READ animationKeys NOTIFY changed)
    Q_PROPERTY(QVariantList selectedPoseFrames READ selectedPoseFrames NOTIFY keySelectionChanged)
    Q_PROPERTY(bool hasPoseClipboard READ hasPoseClipboard NOTIFY keySelectionChanged)
    Q_PROPERTY(QString keyState READ keyState NOTIFY frameChanged)
    Q_PROPERTY(int rangeStart READ rangeStart NOTIFY rangeChanged)
    Q_PROPERTY(int rangeEnd READ rangeEnd NOTIFY rangeChanged)
    Q_PROPERTY(QVariantList selectedLayers READ selectedLayers NOTIFY rangeChanged)
    Q_PROPERTY(QVariantList markers READ markers NOTIFY changed)
    Q_PROPERTY(bool hasClipboard READ hasClipboard NOTIFY rangeChanged)
    Q_PROPERTY(bool hasCopiedTransform READ hasCopiedTransform NOTIFY poseClipboardChanged)
    Q_PROPERTY(QString sceneName READ sceneName NOTIFY changed)
    Q_PROPERTY(QString projectPath READ projectPath NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY changed)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY changed)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY changed)
    Q_PROPERTY(QVariantList layers READ layers NOTIFY changed)
    Q_PROPERTY(QVariantList palette READ palette NOTIFY changed)
    Q_PROPERTY(QVariantList revisions READ revisions NOTIFY changed)
    Q_PROPERTY(int frame READ frame WRITE setFrame NOTIFY frameChanged)
    Q_PROPERTY(int duration READ duration NOTIFY changed)
    Q_PROPERTY(int sceneWidth READ sceneWidth NOTIFY changed)
    Q_PROPERTY(int sceneHeight READ sceneHeight NOTIFY changed)
    Q_PROPERTY(double fps READ fps NOTIFY changed)
    Q_PROPERTY(int fpsNumerator READ fpsNumerator NOTIFY changed)
    Q_PROPERTY(int fpsDenominator READ fpsDenominator NOTIFY changed)
    Q_PROPERTY(int selectedLayer READ selectedLayer WRITE setSelectedLayer NOTIFY selectionChanged)
    Q_PROPERTY(int selectedSwatch READ selectedSwatch WRITE setSelectedSwatch NOTIFY selectionChanged)
    Q_PROPERTY(bool savingRecovery READ savingRecovery NOTIFY recoveryChanged)
    Q_PROPERTY(QString tool READ tool WRITE setTool NOTIFY toolChanged)
    Q_PROPERTY(double brushOpacity READ brushOpacity WRITE setBrushOpacity NOTIFY toolChanged)
    Q_PROPERTY(double brushSize READ brushSize WRITE setBrushSize NOTIFY toolChanged)
    Q_PROPERTY(bool onionSkin READ onionSkin WRITE setOnionSkin NOTIFY toolChanged)
    Q_PROPERTY(bool filled READ filled WRITE setFilled NOTIFY toolChanged)
    Q_PROPERTY(int artLayer READ artLayer WRITE setArtLayer NOTIFY toolChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playbackChanged)
    Q_PROPERTY(bool exporting READ exporting NOTIFY exportChanged)
    Q_PROPERTY(double exportProgress READ exportProgress NOTIFY exportChanged)
    Q_PROPERTY(QString recoveryPath READ recoveryPath CONSTANT)
    Q_PROPERTY(QVariantMap transform READ transform NOTIFY changed)
  public:
    explicit EditorController(QObject* parent = nullptr);
    ~EditorController() override;
    const opentoon::Document& document() const { return session_.document(); }
    std::uint64_t sceneGeneration() const { return sceneGeneration_; }
    bool animateMode() const { return animateMode_; }
    bool autoKey() const { return autoKey_; }
    void setAnimateMode(bool);
    void setAutoKey(bool);
    QVariantList animationKeys() const;
    QString keyState() const;
    QVariantList selectedPoseFrames() const;
    bool hasPoseClipboard() const { return !poseClipboard_.keys.empty(); }
    Q_INVOKABLE void selectPoseKey(int frame, bool extend = false, bool toggle = false);
    Q_INVOKABLE void selectPoseRange(int first, int last, bool additive = false);
    Q_INVOKABLE void clearPoseSelection();
    Q_INVOKABLE void copyPoseKeys();
    Q_INVOKABLE bool pastePoseKeys();
    Q_INVOKABLE bool moveSelectedPoseKeys(int offset, bool duplicate = false);
    Q_INVOKABLE bool stretchSelectedPoseKeys(int last);
    Q_INVOKABLE bool deleteSelectedPoseKeys();
    Q_INVOKABLE bool interpolateSelectedPoseKeys(int preset);
    Q_INVOKABLE bool repeatSelectedPoseKeys(int copies);
    bool editVectors(const std::vector<opentoon::Id>&, QString operation, double value);
    bool pasteVectorBlock(const opentoon::VectorBlock&, std::vector<opentoon::Id>& result);

    Q_INVOKABLE QVariantList curveSamples(QString channel, int samples = 400) const;
    Q_INVOKABLE void nextKey(int direction);
    Q_INVOKABLE bool updateKey(int source, int destination, QString channel, double value, int interpolation);
    Q_INVOKABLE void retimeSelectedKeys(int destination, int length);
    int rangeStart() const { return rangeStart_; }
    int rangeEnd() const { return rangeEnd_; }
    QVariantList selectedLayers() const;
    QVariantList markers() const;
    bool hasClipboard() const { return clipboard_.duration > 0; }
    Q_INVOKABLE void selectTimelineRange(int firstFrame, int lastFrame, int firstRow, int lastRow);
    Q_INVOKABLE void copyTimelineRange();
    Q_INVOKABLE void pasteTimelineRange(int content = 0, bool insert = false);
    Q_INVOKABLE void clearTimelineRange(bool keys = true);
    Q_INVOKABLE void repeatTimelineRange(int repeats);
    Q_INVOKABLE void retimeTimelineRange(int frames);
    Q_INVOKABLE void timeSelectedDrawings(int step);
    Q_INVOKABLE void moveTimelineRange(int destination, bool insert = false);
    Q_INVOKABLE void setSceneMarker(QString name);
    Q_INVOKABLE void exportXsheet(QUrl);
    QString sceneName() const;
    QString projectPath() const { return path_; }
    QString status() const { return status_; }
    bool modified() const { return session_.modified(); }
    bool canUndo() const { return session_.canUndo(); }
    bool canRedo() const { return session_.canRedo(); }
    QVariantList layers() const;
    QVariantList palette() const;
    QVariantList revisions() const;
    QVariantMap transform() const;
    int frame() const { return frame_; }
    int duration() const { return document().duration; }
    int sceneWidth() const { return document().width; }
    int sceneHeight() const { return document().height; }
    int fpsNumerator() const { return document().rate.numerator; }
    int fpsDenominator() const { return document().rate.denominator; }
    double fps() const { return double(document().rate.numerator) / document().rate.denominator; }
    int selectedLayer() const { return static_cast<int>(layer_); }
    int selectedSwatch() const { return static_cast<int>(swatch_); }
    QString tool() const { return tool_; }
    double brushOpacity() const { return brushOpacity_; }
    void setBrushOpacity(double);
    bool transformDrawingRegion(opentoon::PixelRect, opentoon::SelectionMedia,
                                const std::vector<opentoon::Id>&, opentoon::SelectionTransform);
    bool setStrokeProperty(opentoon::Id, QString, double);
    bool editDrawingRegion(opentoon::PixelRect, opentoon::SelectionMedia, opentoon::SelectionAction,
                           int dx = 0, int dy = 0, const std::vector<opentoon::Id>* selectedIds = nullptr,
                           std::vector<opentoon::Id>* resultingIds = nullptr);
    double brushSize() const { return brushSize_; }
    bool onionSkin() const { return onion_; }
    bool filled() const { return filled_; }
    int artLayer() const { return artLayer_; }
    bool playing() const { return playTimer_.isActive(); }
    bool exporting() const { return exporting_; }
    double exportProgress() const { return exportProgress_; }
    QString recoveryPath() const { return previousRecovery_; }
    void setFrame(int);
    void setSelectedLayer(int);
    void setSelectedSwatch(int);
    void setTool(QString);
    void setBrushSize(double);
    void setOnionSkin(bool);
    void setFilled(bool);
    void setArtLayer(int);
    bool savingRecovery() const { return savingRecovery_; }
    Q_INVOKABLE void compactProject(int retain);
    void commitRaster(opentoon::RasterImage);
    void commitStroke(std::vector<opentoon::Point>);
    void eraseGesture(std::vector<opentoon::Point>);
    void translateStroke(opentoon::Id, double, double);
    void recolorStroke(opentoon::Id);
    bool insertPoint(opentoon::Id, int segment, double fraction);
    bool deletePoint(opentoon::Id, int point);
    void movePoint(opentoon::Id, int, opentoon::Point);
    void smoothStroke(opentoon::Id);
    void deleteStroke(opentoon::Id);
    Q_INVOKABLE void newScene();
    Q_INVOKABLE void loadDemo();
    Q_INVOKABLE bool openProject(QUrl);
    Q_INVOKABLE bool saveProject(QUrl url = {});
    Q_INVOKABLE void restoreRevision(int);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void addLayer();
    Q_INVOKABLE void removeLayer();
    Q_INVOKABLE void duplicateLayer(bool linked = false);
    Q_INVOKABLE void renameLayer(int, QString);
    Q_INVOKABLE void toggleLayer(int, QString);
    Q_INVOKABLE void moveLayer(int);
    Q_INVOKABLE void setParent(int);
    Q_INVOKABLE void newDrawing(bool duplicate = false);
    Q_INVOKABLE void holdDrawing(int);
    Q_INVOKABLE void clearExposure();
    Q_INVOKABLE void insertFrames(int);
    Q_INVOKABLE void removeFrames(int);
    Q_INVOKABLE void nextDrawing(int);
    Q_INVOKABLE void addSwatch(QColor);
    Q_INVOKABLE void setSwatchColor(int, QColor);
    Q_INVOKABLE void setScene(QString, int, int, int, int, int);
    Q_INVOKABLE void setTransform(QString, double);
    bool hasCopiedTransform() const { return transformClipboard_.has_value(); }
    Q_INVOKABLE void copyTransformPose();
    Q_INVOKABLE bool pasteTransformPose(int mode);
    Q_INVOKABLE bool resetTransformPose();
    bool commitPose(const opentoon::Transform&);
    bool setPoseKeyPosition(int frame, double x, double y);
    Q_INVOKABLE bool movePoseKey(int source, int destination);
    Q_INVOKABLE bool setPoseCurveHandles(int frame, double x1, double y1, double x2, double y2);
    Q_INVOKABLE bool setCurveHandles(int frame, QString channel, double x1, double y1, double x2, double y2);
    Q_INVOKABLE bool addCurveKey(int frame, QString channel, double value);
    Q_INVOKABLE void addKey(int interpolation = 0);
    Q_INVOKABLE void deleteKey();
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void importImage(QUrl);
    Q_INVOKABLE bool importParts(QVariantList urls);
    Q_INVOKABLE bool importImageSequence(QVariantList urls);
    Q_INVOKABLE void exportFrames(QUrl);
    Q_INVOKABLE void cancelExport();
    Q_INVOKABLE void recover();
    Q_INVOKABLE void autosave();
    Q_INVOKABLE void report(QString message);
  signals:
    void keySelectionChanged();
    void poseClipboardChanged();
    void animationModeChanged();
    void rangeChanged();
    void changed();
    void frameChanged();
    void selectionChanged();
    void toolChanged();
    void statusChanged();
    void playbackChanged();
    void exportChanged();
    void recoveryChanged();

  private:
    bool importImageBatch(QVariantList urls, bool sequence);
    opentoon::Session session_;
    std::vector<opentoon::Frame> poseSelection_;
    opentoon::Id poseSelectionLayer_ = 0;
    int poseSelectionAnchor_ = -1;
    opentoon::KeyBlock poseClipboard_;
    std::optional<opentoon::Transform> transformClipboard_;
    void reconcilePoseSelection();
    void setPoseSelection(std::vector<opentoon::Frame>);
    bool retimePoseSelection(int first, int last, bool duplicate);
    opentoon::ExposureClipboard clipboard_;
    std::vector<opentoon::Id> rangeLayers_;
    int rangeStart_ = 0, rangeEnd_ = 1;
    std::uint64_t sceneGeneration_ = 0, clipboardGeneration_ = 0;
    std::vector<opentoon::Id> validRangeLayers() const;
    opentoon::Id layer_ = 0, swatch_ = 0;
    int frame_ = 0, artLayer_ = 2;
    QString tool_ = "Pencil", path_, status_ = "Ready", recovery_, previousRecovery_;
    double brushSize_ = 5, brushOpacity_ = 1;
    bool animateMode_ = false, autoKey_ = false;
    bool onion_ = true, filled_ = false;
    QTimer playTimer_, autosaveTimer_;
    QElapsedTimer playClock_;
    int playStart_ = 0;
    std::int64_t diskRevision_ = -1;
    bool exporting_ = false;
    double exportProgress_ = 0;
    std::atomic_bool cancelExport_{false};
    std::thread exportThread_, recoveryThread_;
    bool savingRecovery_ = false;
    bool edit(const std::string&, const std::function<void(opentoon::Document&)>&);
    void resetSelection();
    void stopPlayback();
};
