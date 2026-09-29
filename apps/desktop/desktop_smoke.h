#pragma once
#include <QStringList>
class QGuiApplication;
class QQmlApplicationEngine;
class EditorController;
void scheduleWorkspaceSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                            QQmlApplicationEngine& engine);
void scheduleMiloSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                       QQmlApplicationEngine& engine);
void scheduleIntegratedSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                             QQmlApplicationEngine& engine);
void scheduleCompositionSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                              QQmlApplicationEngine& engine);
void scheduleAudioSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                        QQmlApplicationEngine& engine);
void scheduleAnimatorSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                           QQmlApplicationEngine& engine);
void scheduleMeshBenchmark(const QStringList& args, QGuiApplication& app, EditorController& editor,
                           QQmlApplicationEngine& engine);
void scheduleEditorSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                         QQmlApplicationEngine& engine);
