#include "desktop_smoke.h"
#include "editor_controller.h"
#include "native_smoke_support.h"
#include "opentoon/audio.h"
#include <QElapsedTimer>
#include <QFile>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QThread>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

void scheduleAudioSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                        QQmlApplicationEngine& engine) {
    if (args.contains("--hm10-smoke")) {
        scheduleNativeSmoke(app, [&] {
            if (engine.rootObjects().isEmpty())
                throw std::runtime_error("No QML window for HM-10 smoke.");
            auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            if (!window || !window->findChild<QQuickItem*>("timelineCanvas"))
                throw std::runtime_error("The audio timeline has no native canvas.");
            editor.newScene();
            editor.setWorkspaceMode("Rig");
            QTemporaryDir directory;
            if (!directory.isValid())
                throw std::runtime_error("Cannot create HM-10 WAV fixture.");
            QByteArray bytes;
            auto u16 = [&](quint16 value) {
                bytes.append(char(value & 255));
                bytes.append(char(value >> 8));
            };
            auto u32 = [&](quint32 value) {
                u16(value & 65535);
                u16(value >> 16);
            };
            bytes.append("RIFF", 4);
            u32(36 + 48000 * 2);
            bytes.append("WAVEfmt ", 8);
            u32(16);
            u16(1);
            u16(1);
            u32(48000);
            u32(96000);
            u16(2);
            u16(16);
            bytes.append("data", 4);
            u32(48000 * 2);
            for (int sample = 0; sample < 48000; ++sample)
                u16(sample == 2002 ? 32767 : (sample % 80 < 40 ? 9000 : quint16(-9000)));
            QFile source(directory.filePath("original-cue.wav"));
            if (!source.open(QIODevice::WriteOnly) || source.write(bytes) != bytes.size())
                throw std::runtime_error("Cannot write HM-10 WAV fixture.");
            source.close();
            const auto baseline = editor.document();
            if (!editor.importAudio(QUrl::fromLocalFile(source.fileName())) ||
                editor.audioClips().size() != 1)
                throw std::runtime_error("Native WAV import did not create a clip.");
            const auto clip = editor.audioClips().front().toMap().value("id").toInt();
            editor.setFrame(24);
            QCoreApplication::processEvents();
            const auto findDuplicateItem = [](auto&& self, QQuickItem* parent,
                                              const QString& name) -> QQuickItem* {
                if (!parent)
                    return nullptr;
                if (parent->objectName() == name)
                    return parent;
                for (auto* child : parent->childItems())
                    if (auto* match = self(self, child, name))
                        return match;
                return nullptr;
            };
            auto* duplicateButton =
                findDuplicateItem(findDuplicateItem, window->contentItem(), "audioDuplicateClip");
            if (!duplicateButton || !duplicateButton->isVisible())
                throw std::runtime_error("Audio duplicate button is unavailable.");
            const auto duplicatePoint = duplicateButton->mapToScene(
                QPointF(duplicateButton->width() / 2, duplicateButton->height() / 2));
            QMouseEvent duplicatePress(QEvent::MouseButtonPress, duplicatePoint,
                                       window->mapToGlobal(duplicatePoint.toPoint()), Qt::LeftButton,
                                       Qt::LeftButton, Qt::NoModifier);
            QMouseEvent duplicateRelease(QEvent::MouseButtonRelease, duplicatePoint,
                                         window->mapToGlobal(duplicatePoint.toPoint()), Qt::LeftButton,
                                         Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &duplicatePress);
            QCoreApplication::sendEvent(window, &duplicateRelease);
            QCoreApplication::processEvents();
            if (editor.audioClips().size() != 2 || editor.document().audioAssets.size() != 1 ||
                editor.audioClips().back().toMap().value("start").toInt() != 24)
                throw std::runtime_error("Native audio duplication did not reuse the source.");
            editor.undo();
            if (editor.audioClips().size() != 1)
                throw std::runtime_error("Native audio duplication did not undo in one step.");
            editor.setFrame(12);
            QCoreApplication::processEvents();
            const auto beforeSplit = opentoon::AudioMixPlan(editor.document(), 48000).renderBlock(0, 48000);
            auto* splitButton = findDuplicateItem(findDuplicateItem, window->contentItem(), "audioSplitClip");
            if (!splitButton || !splitButton->isVisible() || !splitButton->property("enabled").toBool())
                throw std::runtime_error("Audio split button is unavailable at an interior frame.");
            const auto splitPoint =
                splitButton->mapToScene(QPointF(splitButton->width() / 2, splitButton->height() / 2));
            QMouseEvent splitPress(QEvent::MouseButtonPress, splitPoint,
                                   window->mapToGlobal(splitPoint.toPoint()), Qt::LeftButton, Qt::LeftButton,
                                   Qt::NoModifier);
            QMouseEvent splitRelease(QEvent::MouseButtonRelease, splitPoint,
                                     window->mapToGlobal(splitPoint.toPoint()), Qt::LeftButton, Qt::NoButton,
                                     Qt::NoModifier);
            QCoreApplication::sendEvent(window, &splitPress);
            QCoreApplication::sendEvent(window, &splitRelease);
            QCoreApplication::processEvents();
            if (editor.audioClips().size() != 2 ||
                editor.audioClips().back().toMap().value("start").toInt() != 12 ||
                opentoon::AudioMixPlan(editor.document(), 48000).renderBlock(0, 48000) != beforeSplit)
                throw std::runtime_error("Native audio split changed the canonical mix.");
            editor.undo();
            if (editor.audioClips().size() != 1)
                throw std::runtime_error("Native audio split did not undo in one step.");
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            auto* muteButton = findDuplicateItem(findDuplicateItem, window->contentItem(), "audioMuteClip");
            if (!muteButton || !muteButton->isVisible())
                throw std::runtime_error("Audio mute button is unavailable.");
            const auto mutePoint =
                muteButton->mapToScene(QPointF(muteButton->width() / 2, muteButton->height() / 2));
            QMouseEvent mutePress(QEvent::MouseButtonPress, mutePoint,
                                  window->mapToGlobal(mutePoint.toPoint()), Qt::LeftButton, Qt::LeftButton,
                                  Qt::NoModifier);
            QMouseEvent muteRelease(QEvent::MouseButtonRelease, mutePoint,
                                    window->mapToGlobal(mutePoint.toPoint()), Qt::LeftButton, Qt::NoButton,
                                    Qt::NoModifier);
            QCoreApplication::sendEvent(window, &mutePress);
            QCoreApplication::sendEvent(window, &muteRelease);
            QCoreApplication::processEvents();
            if (!editor.audioClips().front().toMap().value("muted").toBool())
                throw std::runtime_error("Native audio mute button did not silence its clip.");
            if (!window->grabWindow().save("build/hm10-muted-smoke.png"))
                throw std::runtime_error("Cannot capture the muted audio timeline.");
            editor.undo();
            if (editor.audioClips().front().toMap().value("muted").toBool())
                throw std::runtime_error("Native audio mute did not undo in one step.");
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            auto* soloButton = findDuplicateItem(findDuplicateItem, window->contentItem(), "audioSoloClip");
            if (!soloButton || !soloButton->isVisible())
                throw std::runtime_error("Audio solo button is unavailable.");
            const auto soloPoint =
                soloButton->mapToScene(QPointF(soloButton->width() / 2, soloButton->height() / 2));
            QMouseEvent soloPress(QEvent::MouseButtonPress, soloPoint,
                                  window->mapToGlobal(soloPoint.toPoint()), Qt::LeftButton, Qt::LeftButton,
                                  Qt::NoModifier);
            QMouseEvent soloRelease(QEvent::MouseButtonRelease, soloPoint,
                                    window->mapToGlobal(soloPoint.toPoint()), Qt::LeftButton, Qt::NoButton,
                                    Qt::NoModifier);
            QCoreApplication::sendEvent(window, &soloPress);
            QCoreApplication::sendEvent(window, &soloRelease);
            QCoreApplication::processEvents();
            if (!editor.audioClips().front().toMap().value("solo").toBool())
                throw std::runtime_error("Native audio solo button did not isolate its clip.");
            if (!window->grabWindow().save("build/hm10-solo-smoke.png"))
                throw std::runtime_error("Cannot capture the soloed audio timeline.");
            editor.undo();
            if (editor.audioClips().front().toMap().value("solo").toBool())
                throw std::runtime_error("Native audio solo did not undo in one step.");
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            auto* balanceInput = findDuplicateItem(findDuplicateItem, window->contentItem(), "audioBalance");
            if (!balanceInput || !balanceInput->isVisible())
                throw std::runtime_error("Audio balance input is unavailable.");
            const auto balancePoint =
                balanceInput->mapToScene(QPointF(balanceInput->width() / 2, balanceInput->height() / 2));
            QMouseEvent balancePress(QEvent::MouseButtonPress, balancePoint,
                                     window->mapToGlobal(balancePoint.toPoint()), Qt::LeftButton,
                                     Qt::LeftButton, Qt::NoModifier);
            QMouseEvent balanceRelease(QEvent::MouseButtonRelease, balancePoint,
                                       window->mapToGlobal(balancePoint.toPoint()), Qt::LeftButton,
                                       Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &balancePress);
            QCoreApplication::sendEvent(window, &balanceRelease);
            balanceInput->forceActiveFocus();
            if (!QMetaObject::invokeMethod(balanceInput, "selectAll"))
                throw std::runtime_error("Audio balance input could not select its current value.");
            QKeyEvent balanceMinus(QEvent::KeyPress, Qt::Key_Minus, Qt::NoModifier, "-");
            QKeyEvent balanceOne(QEvent::KeyPress, Qt::Key_1, Qt::NoModifier, "1");
            QKeyEvent balanceEnter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &balanceMinus);
            QCoreApplication::sendEvent(window, &balanceOne);
            QCoreApplication::sendEvent(window, &balanceEnter);
            QCoreApplication::processEvents();
            if (editor.audioClips().front().toMap().value("balance").toDouble() != -1 ||
                opentoon::AudioMixPlan(editor.document(), 48000).renderBlock(2002, 1)[1] != 0)
                throw std::runtime_error(
                    "Native audio balance field did not isolate the left channel: text=" +
                    balanceInput->property("text").toString().toStdString() + ", value=" +
                    std::to_string(editor.audioClips().front().toMap().value("balance").toDouble()));
            if (!window->grabWindow().save("build/hm10-balance-smoke.png"))
                throw std::runtime_error("Cannot capture the balanced audio control.");
            editor.undo();
            if (editor.audioClips().front().toMap().value("balance").toDouble() != 0)
                throw std::runtime_error("Native audio balance did not undo in one step.");
            editor.setFrame(0);
            const auto peaks = editor.audioWaveform(clip, 0, 3);
            if (peaks.size() != 3 || peaks[1].toDouble() < 0.99)
                throw std::runtime_error("Native waveform cue is not sample aligned.");
            QCoreApplication::processEvents();
            if (!window->grabWindow().save("build/hm10-audio-smoke.png"))
                throw std::runtime_error("Cannot capture the audio timeline.");
            qputenv("OPENTOON_TEST_NULL_AUDIO_BACKEND", "1");
            editor.togglePlayback();
            QElapsedTimer playbackWait;
            playbackWait.start();
            while (editor.frame() == 0 && playbackWait.elapsed() < 1000) {
                QCoreApplication::processEvents();
                QThread::msleep(5);
            }
            if (!editor.playing() || editor.frame() == 0)
                throw std::runtime_error("Audio device did not advance the native playhead.");
            editor.setFrame(18);
            if (editor.frame() != 18)
                throw std::runtime_error("Native audio playhead seek failed.");
            editor.togglePlayback();
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            auto* loopButton = window->findChild<QQuickItem*>("playbackLoopButton");
            if (!loopButton || !loopButton->isVisible() || !editor.loopPlayback())
                throw std::runtime_error("Playback loop control is unavailable.");
            const auto loopPoint =
                loopButton->mapToScene(QPointF(loopButton->width() / 2, loopButton->height() / 2));
            QMouseEvent loopPress(QEvent::MouseButtonPress, loopPoint,
                                  window->mapToGlobal(loopPoint.toPoint()), Qt::LeftButton, Qt::LeftButton,
                                  Qt::NoModifier);
            QMouseEvent loopRelease(QEvent::MouseButtonRelease, loopPoint,
                                    window->mapToGlobal(loopPoint.toPoint()), Qt::LeftButton, Qt::NoButton,
                                    Qt::NoModifier);
            QCoreApplication::sendEvent(window, &loopPress);
            QCoreApplication::sendEvent(window, &loopRelease);
            QCoreApplication::processEvents();
            if (editor.loopPlayback())
                throw std::runtime_error("Native loop control did not select play once.");
            const auto transportScene = editor.document();
            editor.setFrame(editor.duration() - 1);
            editor.togglePlayback();
            playbackWait.restart();
            while (editor.playing() && playbackWait.elapsed() < 1000) {
                QCoreApplication::processEvents();
                QThread::msleep(5);
            }
            if (editor.playing() || editor.frame() != editor.duration() - 1 ||
                editor.document() != transportScene)
                throw std::runtime_error("Play once did not stop at the final frame.");
            editor.selectTimelineRange(4, 6, 0, 0);
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            auto* rangeButton = window->findChild<QQuickItem*>("playSelectedRangeButton");
            if (!rangeButton || !rangeButton->isVisible())
                throw std::runtime_error("Selected-range playback control is unavailable.");
            const auto rangePoint =
                rangeButton->mapToScene(QPointF(rangeButton->width() / 2, rangeButton->height() / 2));
            QMouseEvent rangePress(QEvent::MouseButtonPress, rangePoint,
                                   window->mapToGlobal(rangePoint.toPoint()), Qt::LeftButton, Qt::LeftButton,
                                   Qt::NoModifier);
            QMouseEvent rangeRelease(QEvent::MouseButtonRelease, rangePoint,
                                     window->mapToGlobal(rangePoint.toPoint()), Qt::LeftButton, Qt::NoButton,
                                     Qt::NoModifier);
            QCoreApplication::sendEvent(window, &rangePress);
            QCoreApplication::sendEvent(window, &rangeRelease);
            QCoreApplication::processEvents();
            if (!editor.playSelectedRange() || editor.rangeStart() != 4 || editor.rangeEnd() != 7)
                throw std::runtime_error("Native Range control did not use the selected frames.");
            editor.setFrame(0);
            editor.togglePlayback();
            if (editor.frame() != 4)
                throw std::runtime_error("Range playback did not start at its first frame.");
            playbackWait.restart();
            while (editor.playing() && playbackWait.elapsed() < 1000) {
                QCoreApplication::processEvents();
                QThread::msleep(5);
            }
            if (editor.playing() || editor.frame() != 6 || editor.document() != transportScene)
                throw std::runtime_error("Selected-range playback missed its final frame.");
            editor.setPlaySelectedRange(false);
            editor.selectTimelineRange(0, 0, 0, 0);
            qunsetenv("OPENTOON_TEST_NULL_AUDIO_BACKEND");
            if (editor.playbackDiagnostics().value("callbacks").toULongLong() == 0)
                throw std::runtime_error("Native audio callback diagnostics are empty.");
            editor.setFrame(0);
            auto* timelineInput = window->findChild<QQuickItem*>("timelineInput");
            if (!timelineInput)
                throw std::runtime_error("Native audio drag target is missing.");
            const auto rowY = 30 + editor.layers().size() * 34 + 17;
            const auto startPoint = timelineInput->mapToScene(QPointF(11, rowY));
            const auto endPoint = timelineInput->mapToScene(QPointF(4 * 22 + 11, rowY));
            const auto sendDrag = [&](QEvent::Type type, QPointF point, Qt::MouseButton button,
                                      Qt::MouseButtons held) {
                QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()), button, held,
                                  Qt::NoModifier);
                QCoreApplication::sendEvent(window, &event);
                QCoreApplication::processEvents();
            };
            sendDrag(QEvent::MouseButtonPress, startPoint, Qt::LeftButton, Qt::LeftButton);
            sendDrag(QEvent::MouseMove, endPoint, Qt::NoButton, Qt::LeftButton);
            sendDrag(QEvent::MouseButtonRelease, endPoint, Qt::LeftButton, Qt::NoButton);
            if (editor.audioClips().front().toMap().value("start").toInt() != 4)
                throw std::runtime_error("Dragging the native waveform did not move its clip.");
            editor.undo();
            if (editor.audioClips().front().toMap().value("start").toInt() != 0)
                throw std::runtime_error("Native waveform drag did not undo atomically.");
            const auto rulerStart = timelineInput->mapToScene(QPointF(11, 15));
            const auto rulerEnd = timelineInput->mapToScene(QPointF(2 * 22 + 11, 15));
            sendDrag(QEvent::MouseButtonPress, rulerStart, Qt::LeftButton, Qt::LeftButton);
            if (!editor.audioScrubbing())
                throw std::runtime_error("Native timeline did not start audio scrubbing.");
            sendDrag(QEvent::MouseMove, rulerEnd, Qt::NoButton, Qt::LeftButton);
            if (editor.frame() != 2)
                throw std::runtime_error("Native audio scrub did not follow the marked frame.");
            sendDrag(QEvent::MouseButtonRelease, rulerEnd, Qt::LeftButton, Qt::NoButton);
            if (editor.audioScrubbing())
                throw std::runtime_error("Native audio scrub did not stop on release.");
            if (!editor.setAudioClipRepeats(clip, 2) ||
                editor.audioWaveform(clip, 25, 1).front().toDouble() < 0.99)
                throw std::runtime_error("Repeated native waveform lost its second cue.");
            const auto output = directory.filePath("mix.wav");
            editor.exportAudio(QUrl::fromLocalFile(output));
            QElapsedTimer timeout;
            timeout.start();
            while (editor.exporting() && timeout.elapsed() < 15000) {
                QCoreApplication::processEvents();
                QThread::msleep(1);
            }
            QFile rendered(output);
            if (editor.exporting() || !rendered.open(QIODevice::ReadOnly) ||
                rendered.size() != 44 + 96000 * 4)
                throw std::runtime_error("Native PCM WAV export length is incorrect.");
            const auto mix = rendered.readAll();
            if (quint8(mix[44 + 2002 * 4]) != 255 || quint8(mix[44 + 2002 * 4 + 1]) != 127 ||
                mix.mid(44 + 50002 * 4, 4) != mix.mid(44 + 2002 * 4, 4))
                throw std::runtime_error("Native PCM WAV cue shifted during export.");
            const auto selectedOutput = directory.filePath("selected.wav");
            editor.exportAudioRange(QUrl::fromLocalFile(selectedOutput), 1, 3);
            timeout.restart();
            while (editor.exporting() && timeout.elapsed() < 15000) {
                QCoreApplication::processEvents();
                QThread::msleep(1);
            }
            QFile selected(selectedOutput);
            if (editor.exporting() || !selected.open(QIODevice::ReadOnly) ||
                selected.size() != 44 + 4000 * 4 ||
                selected.readAll().mid(44) != mix.mid(44 + 2000 * 4, 4000 * 4))
                throw std::runtime_error("Native selected WAV range shifted or changed duration.");
            const auto beforeFadeWindow = window->grabWindow();
            const auto findVisualItem = [](auto&& self, QQuickItem* parent,
                                           const QString& name) -> QQuickItem* {
                if (!parent)
                    return nullptr;
                if (parent->objectName() == name)
                    return parent;
                for (auto* child : parent->childItems())
                    if (auto* found = self(self, child, name))
                        return found;
                return nullptr;
            };
            const auto* fadeInControl =
                findVisualItem(findVisualItem, window->contentItem(), "audioFadeInSamples");
            const auto* fadeOutControl =
                findVisualItem(findVisualItem, window->contentItem(), "audioFadeOutSamples");
            if (!fadeInControl || !fadeOutControl)
                throw std::runtime_error("Native audio fade controls are unavailable.");
            if (!editor.setAudioClipFades(clip, 4000, 4000))
                throw std::runtime_error("Native audio fades were rejected.");
            if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 4000)
                throw std::runtime_error("Native audio fade did not reach the document.");
            const auto afterFadeWindow = window->grabWindow();
            if (!afterFadeWindow.save("build/hm10-fade-smoke.png"))
                throw std::runtime_error("Cannot capture the native fade guide.");
            const auto rowTop = timelineInput->mapToScene(QPointF(0, 30 + editor.layers().size() * 34));
            const auto scale = afterFadeWindow.devicePixelRatio();
            const int left = int(std::floor(rowTop.x() * scale));
            const int top = int(std::floor(rowTop.y() * scale));
            int changedPixels = 0;
            for (int y = top; y < top + int(34 * scale) && y < afterFadeWindow.height(); ++y)
                for (int x = left; x < left + int(44 * scale) && x < afterFadeWindow.width(); ++x)
                    changedPixels += beforeFadeWindow.pixel(x, y) != afterFadeWindow.pixel(x, y);
            if (changedPixels < 5)
                throw std::runtime_error("The audio fade guide did not appear on the timeline.");
            const auto fadeHandle = timelineInput->mapToScene(QPointF(44, rowY - 11));
            const auto fadeDragged = timelineInput->mapToScene(QPointF(66, rowY - 11));
            sendDrag(QEvent::MouseButtonPress, fadeHandle, Qt::LeftButton, Qt::LeftButton);
            sendDrag(QEvent::MouseMove, fadeDragged, Qt::NoButton, Qt::LeftButton);
            sendDrag(QEvent::MouseButtonRelease, fadeDragged, Qt::LeftButton, Qt::NoButton);
            if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 6000)
                throw std::runtime_error("Dragging the native fade handle did not adjust samples.");
            editor.undo();
            if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 4000)
                throw std::runtime_error("Native fade-handle drag did not undo in one step.");
            editor.redo();
            if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 6000)
                throw std::runtime_error("Native fade-handle drag did not redo.");
            editor.undo();
            const auto fadeCancelled = timelineInput->mapToScene(QPointF(88, rowY - 11));
            sendDrag(QEvent::MouseButtonPress, fadeHandle, Qt::LeftButton, Qt::LeftButton);
            sendDrag(QEvent::MouseMove, fadeCancelled, Qt::NoButton, Qt::LeftButton);
            QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &escape);
            QCoreApplication::processEvents();
            sendDrag(QEvent::MouseButtonRelease, fadeCancelled, Qt::LeftButton, Qt::NoButton);
            if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 4000)
                throw std::runtime_error("Escape did not cancel the native fade-handle drag.");
            const auto fadeOutHandle = timelineInput->mapToScene(QPointF(1012, rowY - 11));
            const auto fadeOutDragged = timelineInput->mapToScene(QPointF(990, rowY - 11));
            sendDrag(QEvent::MouseButtonPress, fadeOutHandle, Qt::LeftButton, Qt::LeftButton);
            sendDrag(QEvent::MouseMove, fadeOutDragged, Qt::NoButton, Qt::LeftButton);
            sendDrag(QEvent::MouseButtonRelease, fadeOutDragged, Qt::LeftButton, Qt::NoButton);
            if (editor.audioClips().front().toMap().value("fadeOutSamples").toInt() != 6000)
                throw std::runtime_error("Dragging the native fade-out handle did not adjust samples.");
            editor.undo();
            if (editor.audioClips().front().toMap().value("fadeOutSamples").toInt() != 4000)
                throw std::runtime_error("Native fade-out handle did not undo in one step.");
            editor.redo();
            if (editor.audioClips().front().toMap().value("fadeOutSamples").toInt() != 6000)
                throw std::runtime_error("Native fade-out handle did not redo.");
            editor.undo();
            editor.undo();
            if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 0)
                throw std::runtime_error("Native audio fade undo failed.");
            editor.redo();
            const auto fadedOutput = directory.filePath("faded.wav");
            editor.exportAudio(QUrl::fromLocalFile(fadedOutput));
            timeout.restart();
            while (editor.exporting() && timeout.elapsed() < 15000) {
                QCoreApplication::processEvents();
                QThread::msleep(1);
            }
            QFile fadedFile(fadedOutput);
            if (editor.exporting() || !fadedFile.open(QIODevice::ReadOnly))
                throw std::runtime_error("Native faded WAV export failed.");
            const auto fadedMix = fadedFile.readAll();
            if (fadedMix.size() != mix.size() || fadedMix.mid(44, 4) != QByteArray(4, '\0') ||
                fadedMix.mid(44 + 2002 * 4, 4) == mix.mid(44 + 2002 * 4, 4) ||
                fadedMix.mid(44 + 50002 * 4, 4) != mix.mid(44 + 50002 * 4, 4))
                throw std::runtime_error("Native fade changed the repeated cue or its endpoints.");
            editor.undo(); // Fades.
            editor.undo(); // Repeat count.
            (void)window->grabWindow();
            const auto beforeEdgeTrim =
                opentoon::AudioMixPlan(editor.document(), 48000).renderBlock(0, 48000);
            const auto rightTrimHandle = timelineInput->mapToScene(QPointF(24 * 22, rowY));
            const auto rightTrimTarget = timelineInput->mapToScene(QPointF(22 * 22, rowY));
            sendDrag(QEvent::MouseButtonPress, rightTrimHandle, Qt::LeftButton, Qt::LeftButton);
            sendDrag(QEvent::MouseMove, rightTrimTarget, Qt::NoButton, Qt::LeftButton);
            sendDrag(QEvent::MouseButtonRelease, rightTrimTarget, Qt::LeftButton, Qt::NoButton);
            if (editor.audioClips().front().toMap().value("outSample").toLongLong() != 44000)
                throw std::runtime_error("Dragging the native right trim edge did not change its sample.");
            if (!window->grabWindow().save("build/hm10-edge-trim-smoke.png"))
                throw std::runtime_error("Cannot capture the trimmed audio timeline.");
            const auto afterEdgeTrim = opentoon::AudioMixPlan(editor.document(), 48000).renderBlock(0, 48000);
            if (!std::equal(beforeEdgeTrim.begin(), beforeEdgeTrim.begin() + 44000 * 2,
                            afterEdgeTrim.begin()) ||
                !std::all_of(afterEdgeTrim.begin() + 44000 * 2, afterEdgeTrim.end(),
                             [](auto sample) { return sample == 0; }))
                throw std::runtime_error("Native right trim changed surviving PCM samples.");
            editor.undo();
            if (editor.audioClips().front().toMap().value("outSample").toLongLong() != 48000)
                throw std::runtime_error("Native right trim did not undo in one step.");
            (void)window->grabWindow();
            sendDrag(QEvent::MouseButtonPress, rightTrimHandle, Qt::LeftButton, Qt::LeftButton);
            sendDrag(QEvent::MouseMove, rightTrimTarget, Qt::NoButton, Qt::LeftButton);
            QKeyEvent trimEscape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &trimEscape);
            QCoreApplication::processEvents();
            sendDrag(QEvent::MouseButtonRelease, rightTrimTarget, Qt::LeftButton, Qt::NoButton);
            if (editor.audioClips().front().toMap().value("outSample").toLongLong() != 48000)
                throw std::runtime_error("Escape did not cancel the native edge trim.");
            (void)window->grabWindow();
            const auto leftTrimHandle = timelineInput->mapToScene(QPointF(1, rowY));
            const auto leftTrimTarget = timelineInput->mapToScene(QPointF(2 * 22, rowY));
            sendDrag(QEvent::MouseButtonPress, leftTrimHandle, Qt::LeftButton, Qt::LeftButton);
            sendDrag(QEvent::MouseMove, leftTrimTarget, Qt::NoButton, Qt::LeftButton);
            sendDrag(QEvent::MouseButtonRelease, leftTrimTarget, Qt::LeftButton, Qt::NoButton);
            if (editor.audioClips().front().toMap().value("start").toInt() != 2 ||
                editor.audioClips().front().toMap().value("inSample").toLongLong() != 4000)
                throw std::runtime_error("Dragging the native left trim edge did not retain source timing.");
            editor.undo();
            if (editor.audioClips().front().toMap().value("start").toInt() != 0 ||
                editor.audioClips().front().toMap().value("inSample").toLongLong() != 0)
                throw std::runtime_error("Native left trim did not undo in one step.");
            editor.undo();
            if (editor.document() != baseline)
                throw std::runtime_error("WAV import did not undo atomically.");
            std::cout
                << "HM-10 audio smoke passed: native PCM16 import, shared-source clip duplication/undo, "
                   "exact split/undo, mute/undo, solo/undo, balance/undo, cue waveform, "
                   "device-clock playhead/seek, native play-once and selected-range transport, waveform and "
                   "edge-trim drags/undo, audio scrub/repeat, source-sample fades, exact full and "
                   "selected-range WAV export, timeline screenshot and atomic undo.\n";
        });
    }
}
