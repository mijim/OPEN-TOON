#include "editor_controller.h"
#include <QPainter>
#include <QPdfWriter>
#include <QSaveFile>
#include <stdexcept>
using namespace opentoon;
std::vector<Id> EditorController::validRangeLayers() const {
    std::vector<Id> result;
    for (auto id : rangeLayers_)
        if (std::any_of(document().layers.begin(), document().layers.end(),
                        [=](auto& l) { return l.id == id; }))
            result.push_back(id);
    if (result.empty() && layer_)
        result.push_back(layer_);
    return result;
}
QVariantList EditorController::selectedLayers() const {
    QVariantList result;
    for (auto id : validRangeLayers())
        result.push_back(int(id));
    return result;
}
QVariantList EditorController::markers() const {
    QVariantList result;
    for (const auto& marker : document().markers)
        result.push_back(QVariantMap{{"frame", marker.frame}, {"name", QString::fromStdString(marker.name)}});
    return result;
}
void EditorController::selectTimelineRange(int firstFrame, int lastFrame, int firstRow, int lastRow) {
    if (document().layers.empty())
        return;
    rangeStart_ = std::clamp(std::min(firstFrame, lastFrame), 0, duration() - 1);
    rangeEnd_ = int(std::clamp(std::int64_t(std::max(firstFrame, lastFrame)) + 1,
                               std::int64_t(rangeStart_ + 1), std::int64_t(duration())));
    firstRow = std::clamp(firstRow, 0, int(document().layers.size()) - 1);
    lastRow = std::clamp(lastRow, 0, int(document().layers.size()) - 1);
    rangeLayers_.clear();
    for (int row = std::min(firstRow, lastRow); row <= std::max(firstRow, lastRow); ++row)
        rangeLayers_.push_back(document().layers[document().layers.size() - 1 - row].id);
    layer_ = rangeLayers_.front();
    setFrame(lastFrame);
    emit selectionChanged();
    emit rangeChanged();
    emit changed();
}
void EditorController::copyTimelineRange() {
    try {
        clipboard_ = copyRange(document(), validRangeLayers(), rangeStart_, std::min(rangeEnd_, duration()));
        clipboardGeneration_ = sceneGeneration_;
        emit rangeChanged();
        report(QString("Copied %1 frames from %2 layers")
                   .arg(clipboard_.duration)
                   .arg(clipboard_.tracks.size()));
    } catch (const std::exception& e) {
        report(QString::fromUtf8(e.what()));
    }
}
void EditorController::pasteTimelineRange(int content, bool insert) {
    if (!hasClipboard() || content < 0 || content > 3)
        return;
    auto targets = validRangeLayers();
    if (targets.size() != clipboard_.tracks.size()) {
        targets.clear();
        auto found = std::find_if(document().layers.rbegin(), document().layers.rend(),
                                  [&](auto& l) { return l.id == layer_; });
        while (found != document().layers.rend() && targets.size() < clipboard_.tracks.size())
            targets.push_back((found++)->id);
    }
    edit("Paste timeline range", [&](Document& d) {
        pasteRange(d, targets, frame_, clipboard_, static_cast<PasteContent>(content), insert,
                   clipboardGeneration_ == sceneGeneration_);
    });
}
void EditorController::clearTimelineRange(bool keys) {
    edit("Clear selected range", [&](Document& d) {
        clearRange(d, validRangeLayers(), rangeStart_, std::min(rangeEnd_, duration()), keys);
    });
}
void EditorController::repeatTimelineRange(int repeats) {
    edit("Repeat selected range", [&](Document& d) {
        repeatRange(d, validRangeLayers(), rangeStart_, std::min(rangeEnd_, duration()), repeats);
    });
}
void EditorController::retimeTimelineRange(int frames) {
    if (edit("Retime selected range", [&](Document& d) {
            retimeRange(d, validRangeLayers(), rangeStart_, std::min(rangeEnd_, duration()), frames);
        })) {
        rangeEnd_ = rangeStart_ + frames;
        emit rangeChanged();
    }
}
void EditorController::timeSelectedDrawings(int step) {
    edit("Set drawing timing", [&](Document& d) {
        timeDrawings(d, validRangeLayers(), rangeStart_, std::min(rangeEnd_, duration()), step);
    });
}
void EditorController::moveTimelineRange(int destination, bool insert) {
    const auto length = std::min(rangeEnd_, duration()) - rangeStart_;
    if (edit("Move selected range", [&](Document& d) {
            const auto ids = validRangeLayers();
            const auto clip = copyRange(d, ids, rangeStart_, std::min(rangeEnd_, duration()));
            clearRange(d, ids, rangeStart_, std::min(rangeEnd_, duration()), true);
            pasteRange(d, ids, destination, clip, PasteContent::All, insert);
        })) {
        rangeStart_ = destination;
        rangeEnd_ = destination + length;
        setFrame(destination);
        emit rangeChanged();
    }
}
void EditorController::setSceneMarker(QString name) {
    edit("Set scene marker", [&](Document& d) { setMarker(d, frame_, name.toStdString()); });
}
void EditorController::exportXsheet(QUrl url) {
    if (!url.isLocalFile())
        return;
    const auto pages = ((std::int64_t(duration()) + 39) / 40) * ((document().layers.size() + 4) / 5);
    if (pages > 200) {
        report("Xsheet exceeds the 200-page export limit. Export a shorter scene.");
        return;
    }
    QSaveFile file(url.toLocalFile());
    if (!file.open(QIODevice::WriteOnly)) {
        report("Cannot create Xsheet PDF.");
        return;
    }
    bool ok = true;
    {
        QPdfWriter pdf(&file);
        pdf.setPageSize(QPageSize(QPageSize::A4));
        pdf.setResolution(96);
        pdf.setTitle(sceneName() + " — Xsheet");
        QPainter p(&pdf);
        constexpr int rows = 40, columns = 5;
        int page = 0;
        for (int firstLayer = 0; ok && firstLayer < int(document().layers.size()); firstLayer += columns) {
            for (int firstFrame = 0; firstFrame < duration(); firstFrame += rows) {
                if (page++ && !pdf.newPage()) {
                    ok = false;
                    break;
                }
                p.setPen(Qt::black);
                p.setFont(QFont("sans-serif", 12));
                p.drawText(10, 24,
                           sceneName() + QString(" | %1/%2 fps | Page %3")
                                             .arg(fpsNumerator())
                                             .arg(fpsDenominator())
                                             .arg(page));
                const int cellWidth = (pdf.width() - 60) / columns;
                p.setFont(QFont("sans-serif", 8));
                for (int c = 0; c < columns && firstLayer + c < int(document().layers.size()); ++c) {
                    const auto& layer = document().layers[firstLayer + c];
                    p.drawText(QRect(60 + c * cellWidth, 38, cellWidth, 30), Qt::TextWordWrap,
                               QString::fromStdString(layer.name));
                    for (int r = 0; r < rows && firstFrame + r < duration(); ++r) {
                        int y = 72 + r * 20;
                        p.drawText(5, y + 14, QString::number(firstFrame + r + 1));
                        p.drawRect(60 + c * cellWidth, y, cellWidth, 20);
                        if (auto* drawing = document().drawingAt(layer.id, firstFrame + r))
                            p.drawText(64 + c * cellWidth, y + 14, QString::fromStdString(drawing->name));
                    }
                }
            }
        }
        ok = p.end() && ok;
    }
    if (ok && file.commit())
        report("Xsheet PDF exported");
    else {
        file.cancelWriting();
        report("Xsheet PDF export failed.");
    }
}
