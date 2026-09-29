#include "editor_controller.h"
#include "opentoon/rigging.h"
#include "opentoon/character_pose.h"
#include "opentoon/audio.h"
#include "project_store.h"
#include "scene_renderer.h"
#include "audio_wav_writer.h"
#include "audio_device.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QColorSpace>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRect>
#include <QSettings>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <array>
#include <cstring>
#include <map>

TEST_CASE("Composition node presentation follows cutter edits and undo") {
    EditorController editor;
    editor.newScene();
    const auto target = editor.document().layers.front().id;
    editor.addLayer();
    const auto source = editor.selectedLayer();
    editor.setSelectedLayer(int(target));
    REQUIRE(editor.setLayerMatte(source));
    const auto nodes = editor.compositionNodes();
    REQUIRE(nodes.size() == 8);
    REQUIRE(std::count_if(nodes.begin(), nodes.end(), [](const QVariant& item) {
        return item.toMap().value("kind").toString() == "Apply matte";
    }) == 1);
    REQUIRE(nodes.back().toMap().value("kind").toString() == "Write");
    REQUIRE(nodes.front().toMap().value("kind").toString() == "Background");
    REQUIRE(std::count_if(nodes.begin(), nodes.end(), [&](const QVariant& item) {
        const auto node = item.toMap();
        return node.value("kind").toString() == "Composite" &&
               node.value("layer").toInt() == int(target) &&
               node.value("name").toString() == QString::fromStdString(editor.document().layer(target).name);
    }) == 1);
    REQUIRE(editor.setMatteInverted(true));
    REQUIRE(editor.compositionNodes().size() == 9);
    REQUIRE(editor.document().layer(target).invertMatte);
    editor.undo();
    REQUIRE(editor.compositionNodes().size() == 8);
    REQUIRE_FALSE(editor.document().layer(target).invertMatte);
    editor.undo();
    REQUIRE(editor.compositionNodes().size() == 7);
}
TEST_CASE("Editor copies an existing cutter privately and reopens its exact pixels") {
    auto document = opentoon::makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    const auto target = document.layers.front().id;
    document.editableDrawing(target, 0).image = opentoon::ImageAsset{1, 1, {255, 0, 0, 255}};
    auto source = document.layers.front();
    source.id = document.allocateId();
    source.name = "Joint silhouette";
    auto drawing = document.drawings.at(source.exposures.front().drawing);
    drawing.id = document.allocateId();
    drawing.image = opentoon::ImageAsset{1, 1, {0, 0, 255, 128}};
    document.drawings.emplace(drawing.id, drawing);
    source.exposures.front().drawing = drawing.id;
    document.layers.push_back(source);
    document.layer(target).matte = source.id;
    document.validate();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = QUrl::fromLocalFile(directory.filePath("private-cutter.otoon"));
    REQUIRE(opentoon::ProjectStore::save(
        std::filesystem::path(path.toLocalFile().toStdString()), document) > 0);
    EditorController editor;
    REQUIRE(editor.openProject(path));
    editor.setSelectedLayer(int(target));
    const auto pixels = opentoon::SceneRenderer::render(editor.document(), 0);
    REQUIRE(editor.copyPrivateCutter(int(source.id)));
    const auto copied = editor.document().layer(target).matte;
    REQUIRE(copied != source.id);
    REQUIRE(editor.document().layer(source.id).compositeBypassed);
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0) == pixels);
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().layer(target).matte == copied);
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0) == pixels);
}
TEST_CASE("Direct drawing reorder changes pixels atomically and survives reopen") {
    auto document = opentoon::makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    const auto red = document.layers.front().id;
    document.editableDrawing(red, 0).image = opentoon::ImageAsset{1, 1, {255, 0, 0, 255}};
    auto append = [&](const char* name, std::array<std::uint8_t, 4> pixel) {
        auto layer = document.layers.front();
        layer.id = document.allocateId();
        layer.name = name;
        auto drawing = document.drawings.at(layer.exposures.front().drawing);
        drawing.id = document.allocateId();
        drawing.image = opentoon::ImageAsset{1, 1, {pixel[0], pixel[1], pixel[2], pixel[3]}};
        document.drawings.emplace(drawing.id, drawing);
        layer.exposures.front().drawing = drawing.id;
        document.layers.push_back(layer);
        return layer.id;
    };
    const auto green = append("Green", {0, 255, 0, 255});
    const auto blue = append("Blue", {0, 0, 255, 255});
    document.validate();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = QUrl::fromLocalFile(directory.filePath("reorder.otoon"));
    REQUIRE(opentoon::ProjectStore::save(std::filesystem::path(path.toLocalFile().toStdString()),
                                         document) > 0);
    EditorController editor;
    REQUIRE(editor.openProject(path));
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE_FALSE(editor.moveDrawingAfter(int(red), int(red)));
    REQUIRE_FALSE(editor.moveDrawingAfter(999999, int(blue)));
    REQUIRE(editor.moveDrawingAfter(int(red), int(blue)));
    REQUIRE(editor.document().layers.back().id == red);
    REQUIRE(qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE_FALSE(editor.moveDrawingAfter(int(red), int(blue)));
    editor.undo();
    REQUIRE(editor.document().layers.back().id == blue);
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.redo();
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().layers.back().id == red);
    REQUIRE(qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.toggleLayer(int(green), "locked");
    REQUIRE_FALSE(editor.moveDrawingAfter(int(green), int(red)));
    REQUIRE(editor.document().layers.back().id == red);
    editor.toggleLayer(int(red), "locked");
    REQUIRE_FALSE(editor.moveDrawingAfter(int(blue), int(red)));
    REQUIRE(editor.document().layers.back().id == red);
    REQUIRE_FALSE(editor.moveDrawingBefore(int(blue), int(red)));
    editor.toggleLayer(int(green), "locked");
    editor.toggleLayer(int(red), "locked");
    REQUIRE_FALSE(editor.moveDrawingBefore(int(red), int(red)));
    REQUIRE_FALSE(editor.moveDrawingBefore(999999, int(blue)));
    REQUIRE(editor.moveDrawingBefore(int(red), int(blue)));
    REQUIRE(editor.document().layers[1].id == red);
    REQUIRE(editor.document().layers.back().id == blue);
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE_FALSE(editor.moveDrawingBefore(int(red), int(blue)));
    editor.undo();
    REQUIRE(editor.document().layers.back().id == red);
    REQUIRE(qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.redo();
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().layers[1].id == red);
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
}
TEST_CASE("A Part crosses behind and in front of the torso with stable saved pixels") {
    auto document = opentoon::makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    const auto torso = document.layers.front().id;
    document.editableDrawing(torso, 0).image = opentoon::ImageAsset{1, 1, {255, 0, 0, 255}};
    auto arm = document.layers.front();
    arm.id = document.allocateId();
    arm.name = "Arm";
    const auto armId = arm.id;
    auto armDrawing = document.drawings.at(arm.exposures.front().drawing);
    armDrawing.id = document.allocateId();
    armDrawing.image = opentoon::ImageAsset{1, 1, {0, 0, 255, 255}};
    document.drawings.emplace(armDrawing.id, armDrawing);
    arm.exposures.front().drawing = armDrawing.id;
    const auto root = opentoon::makeCharacter(document, torso, "Character");
    document.layers.push_back(arm);
    opentoon::attachDrawingAsPart(document, armId, root, "Arm");
    document.validate();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = QUrl::fromLocalFile(directory.filePath("part-overlap.otoon"));
    REQUIRE(opentoon::ProjectStore::save(std::filesystem::path(path.toLocalFile().toStdString()),
                                         document) > 0);
    EditorController editor;
    REQUIRE(editor.openProject(path));
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE(editor.moveDrawingBefore(int(armId), int(torso)));
    REQUIRE(editor.document().layer(armId).parent == root);
    REQUIRE(qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.undo();
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.redo();
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().layer(armId).parent == root);
    REQUIRE(qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE(editor.moveDrawingAfter(int(armId), int(torso)));
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE(editor.moveDrawingBefore(int(armId), int(torso)));
    REQUIRE(qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.setSelectedLayer(int(torso));
    REQUIRE(editor.createJointPatch(int(armId)));
    const auto patch = opentoon::Id(editor.selectedLayer());
    REQUIRE(patch != armId);
    REQUIRE(editor.document().layer(patch).role == "Arm patch");
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.undo();
    REQUIRE(qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.redo();
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().layer(patch).parent == root);
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
}
TEST_CASE("Composite grouping is one undoable edit with stable ports and reopened pixels") {
    auto document = opentoon::makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    const auto red = document.layers.front().id;
    document.editableDrawing(red, 0).image = opentoon::ImageAsset{1, 1, {255, 0, 0, 255}};
    auto blue = document.layers.front();
    blue.id = document.allocateId();
    blue.name = "Blue";
    auto blueDrawing = document.drawings.at(blue.exposures.front().drawing);
    blueDrawing.id = document.allocateId();
    blueDrawing.image = opentoon::ImageAsset{1, 1, {0, 0, 255, 255}};
    document.drawings.emplace(blueDrawing.id, blueDrawing);
    blue.exposures.front().drawing = blueDrawing.id;
    document.layers.push_back(blue);
    auto green = blue;
    green.id = document.allocateId();
    green.name = "Green";
    auto greenDrawing = blueDrawing;
    greenDrawing.id = document.allocateId();
    greenDrawing.image = opentoon::ImageAsset{1, 1, {0, 255, 0, 255}};
    document.drawings.emplace(greenDrawing.id, greenDrawing);
    green.exposures.front().drawing = greenDrawing.id;
    document.layers.push_back(green);
    document.validate();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = QUrl::fromLocalFile(directory.filePath("group.otoon"));
    REQUIRE(opentoon::ProjectStore::save(std::filesystem::path(path.toLocalFile().toStdString()),
                                         document) > 0);
    EditorController editor;
    REQUIRE(editor.openProject(path));
    const auto original = opentoon::SceneRenderer::render(editor.document(), 0);
    REQUIRE(editor.groupDrawings(int(red), int(blue.id)));
    REQUIRE(editor.document().compositeGroups.size() == 1);
    const auto groupId = editor.document().compositeGroups.front().id;
    REQUIRE(editor.document().compositeGroups.front().members ==
            std::vector<opentoon::Id>{red, blue.id});
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0) == original);
    REQUIRE_FALSE(editor.moveCompositeGroup(int(groupId), int(red), false));
    REQUIRE(editor.moveCompositeGroup(int(groupId), int(green.id), false));
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE(editor.document().compositeGroups.front().members ==
            std::vector<opentoon::Id>{red, blue.id});
    editor.undo();
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0) == original);
    editor.redo();
    REQUIRE(editor.moveCompositeGroup(int(groupId), int(green.id), true));
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0) == original);
    REQUIRE_FALSE(editor.groupDrawings(int(red), int(blue.id)));
    REQUIRE_FALSE(editor.moveDrawingBefore(int(blue.id), int(red)));
    REQUIRE(editor.document().compositeGroups.front().members.front() == red);
    editor.undo();
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.undo();
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0) == original);
    editor.undo();
    REQUIRE(editor.document().compositeGroups.empty());
    editor.redo();
    REQUIRE(editor.document().compositeGroups.front().id == groupId);
    REQUIRE(editor.renameCompositeGroup(int(groupId), "Body"));
    REQUIRE(editor.document().compositeGroups.front().name == "Body");
    REQUIRE_FALSE(editor.renameCompositeGroup(int(groupId), ""));
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().compositeGroups.front().name == "Body");
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0) == original);
    REQUIRE(editor.moveCompositeGroup(int(groupId), int(green.id), false));
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().compositeGroups.front().members ==
            std::vector<opentoon::Id>{red, blue.id});
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE(editor.setCompositeGroupBypassed(int(groupId), true));
    REQUIRE(qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().compositeGroups.front().bypassed);
    REQUIRE(qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    REQUIRE(editor.setCompositeGroupBypassed(int(groupId), false));
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.undo();
    REQUIRE(editor.document().compositeGroups.front().bypassed);
    editor.redo();
    REQUIRE_FALSE(editor.document().compositeGroups.front().bypassed);
    REQUIRE(editor.ungroupDrawings(int(groupId)));
    REQUIRE(editor.document().compositeGroups.empty());
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.undo();
    REQUIRE(editor.document().compositeGroups.front().id == groupId);
    editor.redo();
    REQUIRE(editor.document().compositeGroups.empty());
    editor.addLayer();
    const auto yellow = opentoon::Id(editor.selectedLayer());
    REQUIRE(editor.moveDrawingAfter(int(yellow), int(green.id)));
    REQUIRE(editor.groupDrawings(int(green.id), int(yellow)));
    const auto rearGroup = editor.document().compositeGroups.front().id;
    REQUIRE(editor.groupDrawings(int(red), int(blue.id)));
    const auto frontGroup = editor.document().compositeGroups.back().id;
    REQUIRE(editor.moveCompositeGroup(int(frontGroup), int(green.id), true));
    REQUIRE(editor.document().layers.front().id == red);
    REQUIRE(editor.document().layers[1].id == blue.id);
    REQUIRE(editor.document().layers[2].id == green.id);
    REQUIRE(editor.moveCompositeGroup(int(frontGroup), int(yellow), false));
    REQUIRE(editor.document().layers.front().id == green.id);
    REQUIRE(editor.document().layers[1].id == yellow);
    editor.toggleLayer(int(green.id), "locked");
    REQUIRE_FALSE(editor.moveCompositeGroup(int(frontGroup), int(yellow), true));
    REQUIRE(editor.document().compositeGroups.front().id == rearGroup);
    REQUIRE(editor.duplicateCompositeGroup(int(frontGroup)));
    REQUIRE(editor.document().compositeGroups.size() == 3);
    const auto copiedGroup = editor.document().compositeGroups.back();
    REQUIRE(copiedGroup.id != frontGroup);
    REQUIRE(opentoon::Id(editor.selectedLayer()) == copiedGroup.members.front());
    REQUIRE(editor.document().drawingAt(copiedGroup.members.front(), 0)->id !=
            editor.document().drawingAt(red, 0)->id);
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().compositeGroups.back() == copiedGroup);
    REQUIRE(editor.document().drawingAt(copiedGroup.members.front(), 0)->id !=
            editor.document().drawingAt(red, 0)->id);
}
TEST_CASE("Composite group edges can be edited without changing unbypassed pixels") {
    auto document = opentoon::makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    const auto first = document.layers.front().id;
    document.editableDrawing(first, 0).image = opentoon::ImageAsset{1, 1, {255, 0, 0, 255}};
    const auto append = [&](std::string name, std::vector<std::uint8_t> pixels) {
        auto layer = document.layers.front();
        layer.id = document.allocateId();
        layer.name = std::move(name);
        auto drawing = document.drawings.at(layer.exposures.front().drawing);
        drawing.id = document.allocateId();
        drawing.image = opentoon::ImageAsset{1, 1, std::move(pixels)};
        document.drawings.emplace(drawing.id, drawing);
        layer.exposures.front().drawing = drawing.id;
        document.layers.push_back(layer);
        return layer.id;
    };
    const auto second = append("Second", {0, 255, 0, 255});
    const auto third = append("Third", {0, 0, 255, 255});
    const auto fourth = append("Fourth", {255, 255, 0, 255});
    const auto fifth = append("Fifth", {255, 0, 255, 255});
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = QUrl::fromLocalFile(directory.filePath("members.otoon"));
    REQUIRE(opentoon::ProjectStore::save(
        std::filesystem::path(path.toLocalFile().toStdString()), document) > 0);
    EditorController editor;
    REQUIRE(editor.openProject(path));
    const auto original = opentoon::SceneRenderer::render(editor.document(), 0);
    REQUIRE(editor.groupDrawings(int(second), int(third)));
    const auto group = editor.document().compositeGroups.front().id;
    REQUIRE_FALSE(editor.toggleCompositeGroupMember(int(group), int(fifth)));
    REQUIRE(editor.toggleCompositeGroupMember(int(group), int(first)));
    REQUIRE(editor.toggleCompositeGroupMember(int(group), int(fourth)));
    REQUIRE(editor.document().compositeGroups.front().members ==
            std::vector<opentoon::Id>{first, second, third, fourth});
    REQUIRE_FALSE(editor.toggleCompositeGroupMember(int(group), int(second)));
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0) == original);
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().compositeGroups.front().members.size() == 4);
    REQUIRE(editor.toggleCompositeGroupMember(int(group), int(first)));
    REQUIRE(editor.document().compositeGroups.front().members.front() == second);
    editor.undo();
    REQUIRE(editor.document().compositeGroups.front().members.front() == first);
    editor.redo();
    REQUIRE(editor.toggleCompositeGroupMember(int(group), int(fourth)));
    REQUIRE(editor.toggleCompositeGroupMember(int(group), int(second)));
    REQUIRE(editor.document().compositeGroups.empty());
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0) == original);
}
TEST_CASE("Deleting a composition source uses the chosen reference policy atomically") {
    auto document = opentoon::makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    const auto source = document.layers.front().id;
    document.editableDrawing(source, 0).image =
        opentoon::ImageAsset{1, 1, {255, 0, 0, 128}};
    const auto append = [&](std::string name, std::vector<std::uint8_t> pixels) {
        auto layer = document.layers.front();
        layer.id = document.allocateId();
        layer.name = std::move(name);
        auto drawing = document.drawings.at(layer.exposures.front().drawing);
        drawing.id = document.allocateId();
        drawing.image = opentoon::ImageAsset{1, 1, std::move(pixels)};
        document.drawings.emplace(drawing.id, drawing);
        layer.exposures.front().drawing = drawing.id;
        document.layers.push_back(layer);
        return layer.id;
    };
    const auto middle = append("Middle", {0, 255, 0, 255});
    const auto target = append("Target", {0, 0, 255, 255});
    document.layer(target).matte = source;
    document.layer(target).invertMatte = true;
    const auto groupId = document.allocateId();
    document.compositeGroups.push_back({groupId, "Source and middle",
                                        {source, middle}});
    document.validate();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = QUrl::fromLocalFile(directory.filePath("delete-source.otoon"));
    REQUIRE(opentoon::ProjectStore::save(std::filesystem::path(path.toLocalFile().toStdString()),
                                         document) > 0);
    EditorController editor;
    REQUIRE(editor.openProject(path));
    const auto baseline = editor.document();
    const auto original = opentoon::SceneRenderer::render(editor.document(), 0);
    REQUIRE_FALSE(editor.deleteCompositionSource(int(source), false));
    REQUIRE(editor.document() == baseline);
    editor.toggleLayer(int(target), "locked");
    const auto locked = editor.document();
    REQUIRE_FALSE(editor.deleteCompositionSource(int(source), true));
    REQUIRE(editor.document() == locked);
    editor.toggleLayer(int(target), "locked");
    REQUIRE(editor.deleteCompositionSource(int(source), true));
    REQUIRE(editor.document().layers.size() == 2);
    REQUIRE(editor.document().compositeGroups.empty());
    REQUIRE(editor.document().layer(target).matte == 0);
    REQUIRE_FALSE(editor.document().layer(target).invertMatte);
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
    editor.undo();
    REQUIRE(editor.document() == baseline);
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0) == original);
    editor.redo();
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().compositeGroups.empty());
    REQUIRE(editor.document().layer(target).matte == 0);
    REQUIRE(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) == 255);
}
TEST_CASE("Deleting a Part source disconnects external cutters and group boundaries") {
    auto document = opentoon::makeDocument();
    const auto body = document.layers.front().id;
    const auto root = opentoon::makeCharacter(document, body, "Hero");
    opentoon::Layer cutter;
    cutter.id = document.allocateId();
    const auto cutterId = cutter.id;
    cutter.name = "Cutter";
    document.layers.push_back(cutter);
    opentoon::attachDrawingAsPart(document, cutterId, root, "Cutter");
    opentoon::Layer outside;
    outside.id = document.allocateId();
    const auto outsideId = outside.id;
    outside.name = "Outside target";
    outside.matte = cutterId;
    document.layers.push_back(outside);
    const auto groupId = document.allocateId();
    document.compositeGroups.push_back({groupId, "Body joint", {body, cutterId}});
    document.validate();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = QUrl::fromLocalFile(directory.filePath("delete-part-source.otoon"));
    REQUIRE(opentoon::ProjectStore::save(std::filesystem::path(path.toLocalFile().toStdString()),
                                         document) > 0);
    EditorController editor;
    REQUIRE(editor.openProject(path));
    const auto baseline = editor.document();
    REQUIRE_FALSE(editor.deleteCompositionSource(int(cutterId), false));
    REQUIRE(editor.document() == baseline);
    REQUIRE(editor.deleteCompositionSource(int(cutterId), true));
    REQUIRE(editor.document().compositeGroups.empty());
    REQUIRE(editor.document().layer(outsideId).matte == 0);
    REQUIRE(editor.document().layer(body).parent == root);
    REQUIRE_THROWS(editor.document().layer(cutterId));
    editor.undo();
    REQUIRE(editor.document() == baseline);
    editor.redo();
    REQUIRE(editor.saveProject({}));
    REQUIRE(editor.openProject(path));
    REQUIRE(editor.document().compositeGroups.empty());
    REQUIRE(editor.document().layer(outsideId).matte == 0);
}
namespace {
const QString partFixture = QStringLiteral(OPENTOON_SOURCE_DIR "/tests/fixtures/harmony-moment/parts/");
QVariantList paths(std::initializer_list<QString> values) {
    QVariantList result;
    for (const auto& value : values)
        result.push_back(QUrl::fromLocalFile(value));
    return result;
}
void waitForExport(EditorController& editor) {
    QElapsedTimer timeout;
    timeout.start();
    while (editor.exporting() && timeout.elapsed() < 30000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    REQUIRE_FALSE(editor.exporting());
}
QJsonObject manifest(const QDir& root, QString folder) {
    QFile file(root.filePath(folder + "/manifest.json"));
    REQUIRE(file.open(QIODevice::ReadOnly));
    return QJsonDocument::fromJson(file.readAll()).object();
}
QByteArray audioCueWav() {
    QByteArray bytes;
    auto u16 = [&](quint16 value) { bytes.append(char(value & 255)); bytes.append(char(value >> 8)); };
    auto u32 = [&](quint32 value) { u16(value & 65535); u16(value >> 16); };
    bytes.append("RIFF", 4); u32(36 + 48000 * 2); bytes.append("WAVEfmt ", 8);
    u32(16); u16(1); u16(1); u32(48000); u32(96000); u16(2); u16(16);
    bytes.append("data", 4); u32(48000 * 2);
    for (int sample = 0; sample < 48000; ++sample)
        u16(sample == 2002 ? 32767 : 0);
    return bytes;
}
} // namespace
TEST_CASE("Editor imports a WAV cue, draws exact frame peaks, edits and reopens") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = directory.filePath("cue.wav");
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(audioCueWav()) == 44 + 48000 * 2);
    file.close();
    EditorController editor;
    editor.newScene();
    editor.setFrame(0);
    const auto before = editor.document();
    const auto url = QUrl::fromLocalFile(path);
    REQUIRE(editor.importAudio(url));
    REQUIRE(editor.audioClips().size() == 1);
    const int clip = editor.audioClips().front().toMap().value("id").toInt();
    const auto peaks = editor.audioWaveform(clip, 0, 3);
    REQUIRE(peaks.size() == 3);
    REQUIRE(peaks[0].toDouble() == 0);
    REQUIRE(peaks[1].toDouble() > 0.99);
    REQUIRE(peaks[2].toDouble() == 0);
    REQUIRE(editor.moveAudioClip(clip, 8));
    REQUIRE(editor.trimAudioClip(clip, 2002, 4000));
    REQUIRE(editor.setAudioClipGain(clip, 0.5));
    REQUIRE(editor.audioWaveform(clip, 8, 1).front().toDouble() > 0.49);
    const auto edited = editor.document();
    REQUIRE(editor.saveProject(QUrl::fromLocalFile(directory.filePath("audio.otoon"))));
    EditorController reopened;
    REQUIRE(reopened.openProject(QUrl::fromLocalFile(directory.filePath("audio.otoon"))));
    REQUIRE(reopened.document() == edited);
    REQUIRE(reopened.audioWaveform(clip, 8, 1).front().toDouble() > 0.49);
    editor.undo(); // Gain.
    REQUIRE(editor.audioClips().front().toMap().value("gain").toDouble() == 1);
    REQUIRE_FALSE(editor.importAudio(QUrl::fromLocalFile(directory.filePath("missing.wav"))));
    REQUIRE(editor.document().audioAssets.size() == 1);
    REQUIRE(editor.removeAudioClip(clip));
    REQUIRE(editor.document().audioClips.empty());
    editor.undo();
    REQUIRE(editor.document().audioClips.size() == 1);
    REQUIRE(before.audioAssets.empty());
}
TEST_CASE("Waveform cache rebuilds when a new scene reuses an audio asset ID") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto first = directory.filePath("first.wav");
    QFile source(first);
    REQUIRE(source.open(QIODevice::WriteOnly));
    REQUIRE(source.write(audioCueWav()) == 44 + 48000 * 2);
    source.close();
    EditorController editor;
    editor.newScene();
    REQUIRE(editor.importAudio(QUrl::fromLocalFile(first)));
    const auto firstClip = editor.audioClips().front().toMap().value("id").toInt();
    REQUIRE(editor.audioWaveform(firstClip, 0, 3)[1].toDouble() > 0.99);
    auto movedCue = audioCueWav();
    movedCue[44 + 2002 * 2] = 0;
    movedCue[44 + 2002 * 2 + 1] = 0;
    movedCue[44 + 5000 * 2] = char(255);
    movedCue[44 + 5000 * 2 + 1] = char(127);
    const auto second = directory.filePath("second.wav");
    source.setFileName(second);
    REQUIRE(source.open(QIODevice::WriteOnly));
    REQUIRE(source.write(movedCue) == movedCue.size());
    source.close();
    editor.newScene();
    REQUIRE(editor.importAudio(QUrl::fromLocalFile(second)));
    const auto secondClip = editor.audioClips().front().toMap().value("id").toInt();
    REQUIRE(secondClip == firstClip);
    const auto peaks = editor.audioWaveform(secondClip, 0, 3);
    REQUIRE(peaks[1].toDouble() == 0);
    REQUIRE(peaks[2].toDouble() > 0.99);
}
TEST_CASE("PCM WAV mix export has exact rational length and cancellation keeps the destination") {
    EditorController editor;
    editor.newScene();
    QBuffer output;
    REQUIRE(output.open(QIODevice::ReadWrite));
    const auto integerResult = opentoon::writeAudioWav(editor.document(), output);
    REQUIRE(integerResult.sampleFrames == 96000);
    REQUIRE(output.data().size() == 44 + 96000 * 4);
    REQUIRE(output.data().left(4) == "RIFF");
    REQUIRE(output.data().mid(8, 4) == "WAVE");
    auto fractional = editor.document();
    fractional.rate = {24000, 1001};
    QBuffer fractionalOutput;
    REQUIRE(fractionalOutput.open(QIODevice::ReadWrite));
    const auto fractionalResult = opentoon::writeAudioWav(fractional, fractionalOutput);
    REQUIRE(fractionalResult.sampleFrames == 96096);
    REQUIRE(fractionalOutput.data().size() == 44 + 96096 * 4);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto target = directory.filePath("existing.wav");
    QFile prior(target);
    REQUIRE(prior.open(QIODevice::WriteOnly));
    REQUIRE(prior.write("keep", 4) == 4);
    prior.close();
    {
        QSaveFile replacement(target);
        REQUIRE(replacement.open(QIODevice::WriteOnly));
        REQUIRE_THROWS_AS(opentoon::writeAudioWav(editor.document(), replacement,
                                                  [] { return true; }),
                          opentoon::AudioExportCancelled);
    }
    REQUIRE(prior.open(QIODevice::ReadOnly));
    REQUIRE(prior.readAll() == "keep");
}
TEST_CASE("Editor exports a reopened PCM cue at its exact output sample") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto input = directory.filePath("cue.wav");
    QFile source(input);
    REQUIRE(source.open(QIODevice::WriteOnly));
    REQUIRE(source.write(audioCueWav()) == 44 + 48000 * 2);
    source.close();
    EditorController editor;
    editor.newScene();
    REQUIRE(editor.importAudio(QUrl::fromLocalFile(input)));
    const auto clip = editor.audioClips().front().toMap().value("id").toInt();
    REQUIRE(editor.setAudioClipRepeats(clip, 2));
    REQUIRE(editor.audioWaveform(clip, 25, 1).front().toDouble() > 0.99);
    editor.undo();
    REQUIRE(editor.audioClips().front().toMap().value("repeats").toInt() == 1);
    editor.redo();
    const auto project = QUrl::fromLocalFile(directory.filePath("sound.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    const auto firstPath = directory.filePath("mix-one.wav");
    const auto secondPath = directory.filePath("mix-two.wav");
    editor.exportAudio(QUrl::fromLocalFile(firstPath));
    waitForExport(editor);
    reopened.exportAudio(QUrl::fromLocalFile(secondPath));
    waitForExport(reopened);
    QFile first(firstPath), second(secondPath);
    REQUIRE(first.open(QIODevice::ReadOnly));
    REQUIRE(second.open(QIODevice::ReadOnly));
    const auto bytes = first.readAll();
    REQUIRE(bytes == second.readAll());
    REQUIRE(bytes.size() == 44 + 96000 * 4);
    const auto atCue = 44 + 2002 * 4;
    REQUIRE(quint8(bytes[atCue]) == 255);
    REQUIRE(quint8(bytes[atCue + 1]) == 127);
    REQUIRE(bytes.mid(atCue, 2) == bytes.mid(atCue + 2, 2));
    const auto repeatedCue = 44 + 50002 * 4;
    REQUIRE(bytes.mid(atCue, 4) == bytes.mid(repeatedCue, 4));
    const auto rangePath = directory.filePath("range.wav");
    reopened.exportAudioRange(QUrl::fromLocalFile(rangePath), 1, 3);
    waitForExport(reopened);
    QFile rangeFile(rangePath);
    REQUIRE(rangeFile.open(QIODevice::ReadOnly));
    const auto rangeBytes = rangeFile.readAll();
    REQUIRE(rangeBytes.size() == 44 + 4000 * 4);
    REQUIRE(rangeBytes.mid(44) == bytes.mid(44 + 2000 * 4, 4000 * 4));
    const auto invalidPath = directory.filePath("invalid-range.wav");
    reopened.exportAudioRange(QUrl::fromLocalFile(invalidPath), 3, 3);
    REQUIRE_FALSE(QFile::exists(invalidPath));
    auto fractional = editor.document();
    fractional.rate = {24000, 1001};
    fractional.validate();
    QBuffer fractionalFull, fractionalRange;
    REQUIRE(fractionalFull.open(QIODevice::WriteOnly));
    REQUIRE(fractionalRange.open(QIODevice::WriteOnly));
    (void)opentoon::writeAudioWav(fractional, fractionalFull);
    const auto result = opentoon::writeAudioWavRange(fractional, fractionalRange, 10, 20);
    const auto firstSample = fractional.rate.sampleAt(10, 48000);
    const auto count = fractional.rate.sampleAt(20, 48000) - firstSample;
    REQUIRE(result.sampleFrames == count);
    REQUIRE(fractionalRange.data().mid(44) == fractionalFull.data().mid(44 + firstSample * 4,
                                                                       count * 4));
    QBuffer rejected;
    REQUIRE(rejected.open(QIODevice::WriteOnly));
    REQUIRE_THROWS(opentoon::writeAudioWavRange(fractional, rejected, 20, 10));
    REQUIRE(rejected.data().isEmpty());
}
TEST_CASE("Null audio device advances, seeks and stops against one immutable scene") {
    EditorController editor;
    editor.newScene();
    const auto scene = editor.snapshot();
    opentoon::AudioDevice device(scene, true);
    device.start(5);
    QElapsedTimer timeout;
    timeout.start();
    while (device.currentSample() <= scene->rate.sampleAt(5, 48000) &&
           timeout.elapsed() < 1000)
        QThread::msleep(5);
    REQUIRE(device.running());
    REQUIRE(device.currentSample() > scene->rate.sampleAt(5, 48000));
    REQUIRE(device.stats().callbacks > 0);
    device.seek(25);
    REQUIRE(device.currentSample() >= scene->rate.sampleAt(25, 48000));
    REQUIRE(device.currentFrame() >= 25);
    device.stop();
    REQUIRE_FALSE(device.running());
    const auto stoppedAt = device.currentSample();
    QThread::msleep(30);
    REQUIRE(device.currentSample() == stoppedAt);
    REQUIRE(device.stats().maximumCallbackNanoseconds > 0);
    device.scrub(10);
    const auto scrubStart = scene->rate.sampleAt(10, 48000);
    timeout.restart();
    while (device.currentSample() == scrubStart && timeout.elapsed() < 1000)
        QThread::msleep(5);
    REQUIRE(device.currentSample() > scrubStart);
    QThread::msleep(120);
    REQUIRE(device.currentSample() == scrubStart + 3840);
    device.scrub(12);
    REQUIRE(device.currentSample() >= scene->rate.sampleAt(12, 48000));
    device.stop();
    device.setLooping(false);
    device.start(scene->duration - 1);
    timeout.restart();
    while (!device.finished() && timeout.elapsed() < 1000)
        QThread::msleep(5);
    REQUIRE(device.finished());
    REQUIRE(device.currentSample() == scene->rate.sampleAt(scene->duration, 48000));
    REQUIRE(device.currentFrame() == scene->duration - 1);
    device.stop();
    device.setLooping(true);
    device.start(scene->duration - 1);
    const auto lastFrameSample = scene->rate.sampleAt(scene->duration - 1, 48000);
    timeout.restart();
    while (device.currentSample() >= lastFrameSample && timeout.elapsed() < 1000)
        QThread::msleep(5);
    REQUIRE(device.currentSample() < lastFrameSample);
    REQUIRE_FALSE(device.finished());
    device.stop();
    REQUIRE(editor.snapshot() == scene);
    if (qEnvironmentVariableIsSet("OPENTOON_TEST_HOST_AUDIO")) {
        opentoon::AudioDevice host(scene);
        REQUIRE_FALSE(host.running());
    }
}
TEST_CASE("Editor audio preview follows the device cursor and stops before an edit") {
    qputenv("OPENTOON_TEST_NULL_AUDIO_BACKEND", "1");
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto input = directory.filePath("preview.wav");
    QFile source(input);
    REQUIRE(source.open(QIODevice::WriteOnly));
    REQUIRE(source.write(audioCueWav()) == 44 + 48000 * 2);
    source.close();
    EditorController editor;
    editor.newScene();
    REQUIRE(editor.importAudio(QUrl::fromLocalFile(input)));
    editor.togglePlayback();
    REQUIRE(editor.playing());
    QElapsedTimer timeout;
    timeout.start();
    while (editor.frame() == 0 && timeout.elapsed() < 1000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    REQUIRE(editor.frame() > 0);
    editor.setFrame(18);
    REQUIRE(editor.frame() == 18);
    editor.setAudioClipGain(editor.audioClips().front().toMap().value("id").toInt(), 0.5);
    REQUIRE_FALSE(editor.playing());
    REQUIRE(editor.playbackDiagnostics().value("callbacks").toULongLong() > 0);
    REQUIRE(editor.playbackDiagnostics().contains("skippedPlayheadFrames"));
    REQUIRE(editor.document().audioClips.front().gain == 0.5);
    const auto beforeTransport = editor.document();
    editor.setLoopPlayback(false);
    editor.setFrame(editor.duration() - 1);
    editor.togglePlayback();
    timeout.restart();
    while (editor.playing() && timeout.elapsed() < 1000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    REQUIRE_FALSE(editor.playing());
    REQUIRE(editor.frame() == editor.duration() - 1);
    REQUIRE(editor.document() == beforeTransport);
    editor.setLoopPlayback(true);
    editor.togglePlayback();
    timeout.restart();
    while (editor.frame() == editor.duration() - 1 && timeout.elapsed() < 1000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    REQUIRE(editor.playing());
    REQUIRE(editor.frame() < editor.duration() - 1);
    editor.togglePlayback();
    qunsetenv("OPENTOON_TEST_NULL_AUDIO_BACKEND");
}
TEST_CASE("Silent transport can stop once or loop without editing the scene") {
    EditorController editor;
    editor.newScene();
    const auto before = editor.document();
    const auto last = editor.duration() - 1;
    QElapsedTimer timeout;
    editor.setLoopPlayback(false);
    editor.setFrame(last);
    editor.togglePlayback();
    timeout.start();
    while (editor.playing() && timeout.elapsed() < 1000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    REQUIRE_FALSE(editor.playing());
    REQUIRE(editor.frame() == last);
    REQUIRE(editor.document() == before);
    editor.setLoopPlayback(true);
    editor.togglePlayback();
    timeout.restart();
    while (editor.frame() == last && timeout.elapsed() < 1000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    REQUIRE(editor.playing());
    REQUIRE(editor.frame() < last);
    editor.togglePlayback();
    REQUIRE(editor.document() == before);
}
TEST_CASE("Workspace layout preferences reopen and reset without changing the scene") {
    QSettings settings;
    settings.remove("layout");
    settings.remove("workspaceMode");
    EditorController editor;
    editor.newScene();
    editor.setFrame(7);
    const auto before = editor.document();
    const auto revision = editor.documentRevision();
    const auto selected = editor.selectedLayer();
    editor.setWorkspaceMode("Animator");
    editor.setBottomPanelTab("Nodes");
    editor.setBottomPanelHeight(410);
    editor.setTimelineCellWidth(42);
    editor.setTimingToolsVisible(true);
    editor.setBottomPanelTab("Unsupported");
    REQUIRE(editor.bottomPanelTab() == "Nodes");
    REQUIRE(editor.document() == before);
    REQUIRE(editor.documentRevision() == revision);
    REQUIRE(editor.frame() == 7);
    REQUIRE(editor.selectedLayer() == selected);
    QSettings().sync();
    EditorController reopened;
    REQUIRE(reopened.workspaceMode() == "Animator");
    REQUIRE(reopened.bottomPanelTab() == "Nodes");
    REQUIRE(reopened.bottomPanelHeight() == 410);
    REQUIRE(reopened.timelineCellWidth() == 42);
    REQUIRE(reopened.timingToolsVisible());
    editor.resetWorkspaceLayout();
    REQUIRE(editor.document() == before);
    REQUIRE(editor.documentRevision() == revision);
    REQUIRE(editor.frame() == 7);
    REQUIRE(editor.selectedLayer() == selected);
    EditorController reset;
    REQUIRE(reset.workspaceMode() == "Rig");
    REQUIRE(reset.bottomPanelTab() == "Timeline");
    REQUIRE(reset.bottomPanelHeight() == 280);
    REQUIRE(reset.timelineCellWidth() == 22);
    REQUIRE_FALSE(reset.timingToolsVisible());
    settings.setValue("layout/bottomTab", "Missing");
    settings.setValue("layout/bottomHeight", -300);
    settings.setValue("layout/timelineCell", "invalid");
    settings.sync();
    EditorController sanitized;
    REQUIRE(sanitized.bottomPanelTab() == "Timeline");
    REQUIRE(sanitized.bottomPanelHeight() == 140);
    REQUIRE(sanitized.timelineCellWidth() == 22);
    settings.remove("layout");
    settings.remove("workspaceMode");
}
TEST_CASE("Selected frame playback uses exact sample limits and leaves document untouched") {
    EditorController editor;
    editor.newScene();
    const auto scene = editor.snapshot();
    opentoon::AudioDevice device(scene, true);
    REQUIRE_THROWS(device.setPlaybackRange(8, 5));
    REQUIRE_THROWS(device.setPlaybackRange(0, scene->duration + 1));
    device.setPlaybackRange(5, 8);
    REQUIRE_THROWS(device.seek(4));
    device.setLooping(false);
    device.start(7);
    REQUIRE_THROWS(device.setPlaybackRange(6, 9));
    QElapsedTimer timeout;
    timeout.start();
    while (!device.finished() && timeout.elapsed() < 1000)
        QThread::msleep(5);
    REQUIRE(device.finished());
    REQUIRE(device.currentSample() == scene->rate.sampleAt(8, 48000));
    REQUIRE(device.currentFrame() == 7);
    device.stop();
    device.setLooping(true);
    device.start(7);
    timeout.restart();
    while (device.currentSample() >= scene->rate.sampleAt(7, 48000) &&
           timeout.elapsed() < 1000)
        QThread::msleep(5);
    REQUIRE(device.currentSample() >= scene->rate.sampleAt(5, 48000));
    REQUIRE(device.currentSample() < scene->rate.sampleAt(7, 48000));
    device.stop();
    auto fractional = std::make_shared<opentoon::Document>(*scene);
    fractional->rate = {24000, 1001};
    fractional->validate();
    opentoon::AudioDevice fractionalDevice(fractional, true);
    fractionalDevice.setPlaybackRange(10, 13);
    fractionalDevice.setLooping(false);
    fractionalDevice.start(12);
    timeout.restart();
    while (!fractionalDevice.finished() && timeout.elapsed() < 1000)
        QThread::msleep(5);
    REQUIRE(fractionalDevice.finished());
    REQUIRE(fractionalDevice.currentSample() == fractional->rate.sampleAt(13, 48000));
    fractionalDevice.stop();

    const auto before = editor.document();
    editor.selectTimelineRange(5, 7, 0, 0);
    editor.setPlaySelectedRange(true);
    editor.setLoopPlayback(false);
    editor.setFrame(0);
    editor.togglePlayback();
    REQUIRE(editor.frame() == 5);
    timeout.restart();
    while (editor.playing() && timeout.elapsed() < 1000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    REQUIRE_FALSE(editor.playing());
    REQUIRE(editor.frame() == 7);
    REQUIRE(editor.document() == before);
    editor.setLoopPlayback(true);
    editor.togglePlayback();
    timeout.restart();
    while (editor.frame() == 7 && timeout.elapsed() < 1000) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    REQUIRE(editor.playing());
    REQUIRE(editor.frame() >= 5);
    REQUIRE(editor.frame() <= 7);
    editor.setFrame(0);
    REQUIRE_FALSE(editor.playing());
    REQUIRE(editor.frame() == 0);
    REQUIRE(editor.document() == before);
    editor.newScene();
    REQUIRE_FALSE(editor.playSelectedRange());
}
TEST_CASE("Editor scrubs short audio fragments while traversing frames") {
    qputenv("OPENTOON_TEST_NULL_AUDIO_BACKEND", "1");
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = directory.filePath("scrub.wav");
    QFile source(path);
    REQUIRE(source.open(QIODevice::WriteOnly));
    REQUIRE(source.write(audioCueWav()) == 44 + 48000 * 2);
    source.close();
    EditorController editor;
    editor.newScene();
    REQUIRE(editor.importAudio(QUrl::fromLocalFile(path)));
    const auto before = editor.document();
    editor.setFrame(1);
    editor.beginAudioScrub();
    REQUIRE(editor.audioScrubbing());
    REQUIRE_FALSE(editor.playing());
    editor.setFrame(5);
    REQUIRE(editor.frame() == 5);
    editor.endAudioScrub();
    REQUIRE_FALSE(editor.audioScrubbing());
    REQUIRE(editor.document() == before);
    qunsetenv("OPENTOON_TEST_NULL_AUDIO_BACKEND");
}
TEST_CASE("Registered PNG parts preserve a shared canvas and undo as one edit") {
    EditorController editor;
    editor.newScene();
    editor.setFrame(7);
    const auto before = editor.document();
    const auto originalPalette = before.palette;
    REQUIRE(editor.importParts(paths({partFixture + "hand_right__open.png",
                                      partFixture + "torso__base.png",
                                      partFixture + "head__front.png"})));
    const auto imported = editor.document();
    REQUIRE(imported.layers.size() == before.layers.size() + 3);
    REQUIRE(imported.drawings.size() == before.drawings.size() + 3);
    REQUIRE(imported.palette == originalPalette);
    REQUIRE(imported.layers[before.layers.size()].name == "hand_right__open");
    REQUIRE(imported.layers[before.layers.size() + 1].name == "head__front");
    REQUIRE(imported.layers[before.layers.size() + 2].name == "torso__base");
    for (std::size_t index = before.layers.size(); index < imported.layers.size(); ++index) {
        const auto& layer = imported.layers[index];
        REQUIRE(layer.transform.x == 832);
        REQUIRE(layer.transform.y == 412);
        REQUIRE(layer.exposures.size() == 1);
        REQUIRE(layer.exposures.front().start == 7);
        REQUIRE(layer.exposures.front().end == imported.duration);
        const auto& asset = *imported.drawings.at(layer.exposures.front().drawing).image;
        REQUIRE(asset.width == 256);
        const auto source = QImage(partFixture + QString::fromStdString(layer.name) + ".png")
                                .convertToFormat(QImage::Format_RGBA8888);
        REQUIRE_FALSE(source.isNull());
        for (int y = 0; y < source.height(); ++y)
            REQUIRE(std::memcmp(asset.rgba.data() + y * asset.width * 4,
                                source.constScanLine(y), asset.width * 4) == 0);
    }
    const auto rendered = opentoon::SceneRenderer::render(imported, 7);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto project = QUrl::fromLocalFile(directory.filePath("registered-parts.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == imported);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 7) == rendered);
    editor.undo();
    REQUIRE(editor.document() == before);
    editor.redo();
    REQUIRE(editor.document() == imported);
}
TEST_CASE("Editor captures a selected Part pose, applies its mask, undoes and reopens") {
    EditorController editor;
    editor.newScene();
    editor.setWorkspaceMode("Rig");
    REQUIRE(editor.importParts(paths({partFixture + "hand_right__open.png",
                                      partFixture + "torso__base.png"})));
    const int torso = editor.selectedLayer();
    editor.makeCharacter();
    REQUIRE(editor.characterId() > 0);
    editor.attachUnparentedDrawings();
    const int hand = int(std::find_if(editor.document().layers.begin(), editor.document().layers.end(),
                                     [](const auto& layer) { return layer.name == "hand_right__open"; })->id);
    editor.setSelectedLayer(hand);
    editor.setTransform("x", 50);
    editor.setTransform("rotation", 20);
    editor.captureSelectedCharacterPose(opentoon::PoseChannels::PositionX, false);
    REQUIRE(editor.characterPoses().size() == 1);
    const int poseId = editor.selectedCharacterPose();
    editor.setSelectedLayer(torso);
    editor.setSelectedPartInCharacterPose(opentoon::PoseChannels::Rotation);
    REQUIRE(editor.characterPoses().front().toMap().value("parts").toInt() == 2);
    editor.removeSelectedPartFromCharacterPose();
    REQUIRE(editor.characterPoses().front().toMap().value("parts").toInt() == 1);
    editor.setSelectedLayer(hand);
    editor.setTransform("x", 80);
    editor.setTransform("rotation", 70);
    editor.setFrame(8);
    const auto before = editor.document();
    editor.applySelectedCharacterPose();
    REQUIRE(opentoon::evaluateTransform(editor.document().layer(hand), 8).x == 50);
    REQUIRE(opentoon::evaluateTransform(editor.document().layer(hand), 8).rotation == 70);
    editor.undo();
    REQUIRE(editor.document() == before);
    editor.redo();
    editor.setFrame(16);
    editor.setAnimateMode(true);
    editor.setAutoKey(true);
    editor.setTransform("x", 80);
    REQUIRE(opentoon::evaluateTransform(editor.document().layer(hand), 16).x == 80);
    const auto beforeBlend = editor.document();
    editor.beginSelectedCharacterPoseBlend();
    REQUIRE(editor.updateSelectedCharacterPoseBlend(.25));
    REQUIRE(opentoon::evaluateTransform(editor.document().layer(hand), 16).x == 72.5);
    REQUIRE(editor.updateSelectedCharacterPoseBlend(.75));
    REQUIRE(editor.updateSelectedCharacterPoseBlend(1));
    editor.endSelectedCharacterPoseBlend();
    REQUIRE(opentoon::evaluateTransform(editor.document().layer(hand), 16).x == 50);
    editor.undo();
    REQUIRE(editor.document() == beforeBlend);
    editor.redo();
    editor.setSelectedCharacterPosePublished(true);
    editor.captureCharacterView();
    editor.setSelectedViewPublished(true);
    const auto selected = editor.selectedLayer();
    const auto frame = editor.frame();
    const auto pixels = opentoon::SceneRenderer::render(editor.document(), frame, {320, 180});
    editor.setWorkspaceMode("Animator");
    REQUIRE(editor.selectedLayer() == selected);
    REQUIRE(editor.frame() == frame);
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), frame, {320, 180}) == pixels);
    REQUIRE(editor.characterPoses().front().toMap().value("published").toBool());
    REQUIRE(editor.characterViews().front().toMap().value("published").toBool());
    editor.setWorkspaceMode("Rig");
    editor.setSelectedLayer(hand);
    const int originalDrawing = editor.selectedSubstitution();
    editor.createSubstitution(true);
    const int alternateDrawing = editor.selectedSubstitution();
    editor.renameSubstitution(alternateDrawing, "Alternate hand");
    editor.setSelectedSubstitutionPublished(true);
    editor.selectSubstitution(originalDrawing);
    editor.setSelectedSubstitutionPublished(true);
    editor.setWorkspaceMode("Animator");
    REQUIRE(editor.publishedCharacterSubstitutions().size() == 1);
    REQUIRE(editor.publishedCharacterSubstitutions().front().toMap().value("options").toList().size() == 2);
    const auto beforeDrawingSwitch = editor.document();
    REQUIRE(editor.applyPublishedSubstitution(hand, alternateDrawing));
    REQUIRE(editor.document().drawingAt(hand, 16)->id == opentoon::Id(alternateDrawing));
    REQUIRE(editor.document().layer(torso) == beforeDrawingSwitch.layer(torso));
    REQUIRE(editor.document().layer(hand).keys == beforeDrawingSwitch.layer(hand).keys);
    editor.undo();
    REQUIRE(editor.document() == beforeDrawingSwitch);
    editor.redo();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto project = QUrl::fromLocalFile(directory.filePath("named-pose.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    reopened.setSelectedLayer(torso);
    REQUIRE(reopened.characterPoses().size() == 1);
    REQUIRE(reopened.characterPoses().front().toMap().value("id").toInt() == poseId);
    REQUIRE(reopened.document() == editor.document());
}
TEST_CASE("Editor copies a named pose to another character and reopens independent mapping") {
    EditorController editor;
    editor.newScene();
    editor.setWorkspaceMode("Rig");
    REQUIRE(editor.importParts(paths({partFixture + "hand_right__open.png"})));
    const int sourcePart = editor.selectedLayer();
    editor.makeCharacter();
    const int source = editor.characterId();
    editor.setSelectedLayer(sourcePart);
    editor.setTransform("x", 64);
    editor.captureSelectedCharacterPose(opentoon::PoseChannels::PositionX |
                                        opentoon::PoseChannels::Drawing, false);
    const int sourcePose = editor.selectedCharacterPose();
    editor.setSelectedLayer(source);
    editor.duplicateCharacter();
    const int target = editor.characterId();
    REQUIRE(target != source);
    REQUIRE(editor.poseTransferTargets().size() == 1);
    editor.removeSelectedCharacterPose();
    REQUIRE(editor.characterPoses().empty());
    editor.setSelectedLayer(source);
    editor.selectCharacterPose(sourcePose);
    const auto baseline = editor.document();
    REQUIRE(editor.transferSelectedCharacterPose(target));
    REQUIRE(editor.characterId() == target);
    REQUIRE(editor.characterPoses().size() == 1);
    const int transferred = editor.selectedCharacterPose();
    REQUIRE(transferred != sourcePose);
    const int targetPart = editor.characterPoses().front().toMap().value("entries").toList()
                               .front().toMap().value("part").toInt();
    REQUIRE(targetPart != sourcePart);
    REQUIRE(editor.document().layer(source) == baseline.layer(source));
    editor.undo();
    REQUIRE(editor.document() == baseline);
    editor.redo();
    REQUIRE(editor.document().layer(target).poses.front().id == opentoon::Id(transferred));
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = QUrl::fromLocalFile(directory.filePath("transferred-pose.otoon"));
    REQUIRE(editor.saveProject(path));
    EditorController reopened;
    REQUIRE(reopened.openProject(path));
    REQUIRE(reopened.document() == editor.document());
    REQUIRE(reopened.document().layer(target).poses.front().parts.front().part == opentoon::Id(targetPart));
}
TEST_CASE("Animator switches published control groups without changing the scene") {
    EditorController editor;
    editor.newScene();
    editor.setWorkspaceMode("Rig");
    REQUIRE(editor.importParts(paths({partFixture + "hand_right__open.png"})));
    const int part = editor.selectedLayer();
    editor.makeCharacter();
    const int root = editor.characterId();
    editor.captureCharacterView();
    const int stageView = editor.selectedView();
    editor.setSelectedViewPublished(true);
    editor.setSelectedViewControlGroup("Stage");
    editor.captureSelectedCharacterPose(opentoon::PoseChannels::PositionX, true);
    const int bodyPose = editor.selectedCharacterPose();
    editor.setSelectedCharacterPosePublished(true);
    editor.setSelectedCharacterPoseControlGroup("Body");
    editor.setSelectedLayer(part);
    const int original = editor.selectedSubstitution();
    editor.setSelectedSubstitutionPublished(true);
    editor.setSelectedSubstitutionControlGroup("Face");
    editor.createSubstitution(true);
    const int faceDrawing = editor.selectedSubstitution();
    editor.setSelectedSubstitutionPublished(true);
    editor.setSelectedSubstitutionControlGroup("Face");
    editor.selectSubstitution(original);
    editor.setSelectedLayer(root);
    REQUIRE(editor.characterControlGroups().size() == 3);
    const auto before = editor.document();
    const auto pixels = opentoon::SceneRenderer::render(before, 0, {320, 180});
    editor.setWorkspaceMode("Animator");
    editor.setSelectedControlGroup("Face");
    REQUIRE(editor.selectedControlGroup() == "Face");
    REQUIRE(editor.selectedCharacterPose() == 0);
    editor.selectCharacterPose(bodyPose);
    REQUIRE(editor.selectedCharacterPose() == 0);
    const auto face = editor.publishedCharacterSubstitutions();
    REQUIRE(face.size() == 1);
    REQUIRE(face.front().toMap().value("group").toString() == "Face");
    editor.selectView(stageView);
    editor.applyCharacterView();
    REQUIRE(editor.document() == before);
    editor.setSelectedControlGroup("Body");
    REQUIRE(editor.selectedCharacterPose() > 0);
    REQUIRE(!editor.applyPublishedSubstitution(part, faceDrawing));
    REQUIRE(editor.document() == before);
    editor.setSelectedControlGroup("Stage");
    REQUIRE(editor.selectedCharacterPose() == 0);
    REQUIRE(editor.selectedLayer() == root);
    REQUIRE(editor.frame() == 0);
    REQUIRE(editor.document() == before);
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0, {320, 180}) == pixels);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = QUrl::fromLocalFile(directory.filePath("grouped-controls.otoon"));
    REQUIRE(editor.saveProject(path));
    EditorController reopened;
    REQUIRE(reopened.openProject(path));
    REQUIRE(reopened.document() == before);
}
TEST_CASE("Composition profile edits are undoable and persist through project save") {
    EditorController editor;
    editor.newScene();
    REQUIRE(editor.compositionProfile() == 0);
    editor.setCompositionProfile(1);
    REQUIRE(editor.compositionProfile() == 1);
    editor.undo();
    REQUIRE(editor.compositionProfile() == 0);
    editor.redo();
    REQUIRE(editor.compositionProfile() == 1);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto path = std::filesystem::path(directory.filePath("composition.otoon").toStdString());
    REQUIRE(opentoon::ProjectStore::save(path, editor.document()) > 0);
    REQUIRE(opentoon::ProjectStore::load(path).document.composition ==
            opentoon::CompositionProfile::LinearSrgb);
}

TEST_CASE("Character inspector actions build a saved rigid rig with held substitutions") {
    EditorController editor;
    editor.newScene();
    REQUIRE(editor.importParts(paths({partFixture + "torso__base.png", partFixture + "head__front.png"})));
    const auto head = editor.selectedLayer();
    const auto body = int(editor.document().layers[1].id);
    const auto before = opentoon::SceneRenderer::render(editor.document(), 0, {320, 180});
    editor.makeCharacter();
    REQUIRE(editor.document().layer(head).kind == opentoon::LayerKind::Part);
    const auto character = editor.document().layer(head).parent;
    editor.addPeg();
    REQUIRE(editor.document().layer(head).parent != character);
    editor.setPartRole("Head");
    editor.centerRestPivot();
    REQUIRE(editor.document().layer(head).transform.pivotX == 128);
    REQUIRE(editor.document().layer(head).transform.pivotY == 128);
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0, {320, 180}) == before);
    editor.setSelectedLayer(body);
    editor.setParent(int(character));
    REQUIRE(editor.document().layer(body).kind == opentoon::LayerKind::Part);
    REQUIRE(editor.substitutions().size() == 1);
    const auto original = editor.selectedSubstitution();
    editor.createSubstitution(true);
    const auto open = editor.selectedSubstitution();
    REQUIRE(open != original);
    editor.renameSubstitution(open, "Turned head");
    REQUIRE(editor.substitutions().back().toMap().value("name").toString() == "Turned head");
    editor.setFrame(12);
    editor.selectSubstitution(original);
    REQUIRE(editor.selectedSubstitution() == original);
    REQUIRE(editor.document().drawingAt(body, 11)->id == open);
    editor.removeSubstitution(original);
    REQUIRE(editor.selectedSubstitution() == open);
    const auto rigged = editor.document();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto project = QUrl::fromLocalFile(directory.filePath("rig.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == rigged);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 0, {320, 180}) ==
            opentoon::SceneRenderer::render(rigged, 0, {320, 180}));
    editor.undo();
    REQUIRE(editor.document() != rigged);
    editor.redo();
    REQUIRE(editor.document() == rigged);
}

TEST_CASE("Inspector view sets and thumbnail chooser survive duplicate save and reopen") {
    EditorController editor;
    editor.newScene();
    REQUIRE(editor.importParts(paths({partFixture + "torso__base.png", partFixture + "head__front.png"})));
    const int head = editor.selectedLayer();
    const int body = int(editor.document().layers[1].id);
    editor.makeCharacter();
    const int root = editor.characterId();
    editor.setSelectedLayer(body);
    editor.setParent(root);
    const int original = editor.selectedSubstitution();
    REQUIRE(editor.substitutionThumbnail(original).startsWith("data:image/png;base64,"));
    editor.createSubstitution(true);
    const int alternative = editor.selectedSubstitution();
    editor.moveSubstitution(alternative, -1);
    REQUIRE(editor.substitutions().front().toMap().value("id").toInt() == alternative);
    editor.stepSubstitution(1);
    REQUIRE(editor.selectedSubstitution() == original);
    editor.captureCharacterView();
    const int view = editor.selectedView();
    REQUIRE(view > 0);
    REQUIRE(editor.characterViews().size() == 1);
    editor.renameCharacterView("Front");
    editor.duplicateCharacterView();
    REQUIRE(editor.characterViews().size() == 2);
    editor.removeCharacterView();
    REQUIRE(editor.characterViews().size() == 1);
    editor.selectView(view);
    editor.applyCharacterView();
    REQUIRE(editor.document().drawingAt(body, 0)->id == opentoon::Id(original));
    editor.setSelectedLayer(head);
    editor.duplicateCharacter();
    REQUIRE(editor.document().layer(editor.selectedLayer()).kind == opentoon::LayerKind::Character);
    REQUIRE(editor.document().layer(editor.selectedLayer()).views.size() == 1);
    const auto snapshot = editor.document();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto project = QUrl::fromLocalFile(directory.filePath("views.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == snapshot);
    editor.undo();
    REQUIRE(editor.document() != snapshot);
    editor.redo();
    REQUIRE(editor.document() == snapshot);
}

TEST_CASE("Inspector rig branch and view-range commands stay undoable and saveable") {
    EditorController editor;
    editor.newScene();
    REQUIRE(editor.importParts(paths({partFixture + "torso__base.png", partFixture + "head__front.png"})));
    editor.makeCharacter();
    const int root = editor.characterId();
    editor.attachUnparentedDrawings();
    REQUIRE(editor.document().layer(root).kind == opentoon::LayerKind::Character);
    editor.captureCharacterView();
    REQUIRE(editor.characterViews().size() == 1);
    const int part = editor.selectedLayer();
    editor.createSubstitution(true);
    editor.updateSelectedPartInView();
    editor.duplicateLayer(false);
    const int copy = editor.selectedLayer();
    REQUIRE(copy != part);
    REQUIRE(editor.document().layer(root).views.front().choices.size() == 3);
    editor.addPeg();
    const int peg = int(editor.document().layer(copy).parent);
    editor.setSelectedLayer(peg);
    editor.duplicateLayer(false);
    REQUIRE(editor.document().layer(editor.selectedLayer()).kind == opentoon::LayerKind::Peg);
    editor.deleteRigBranch();
    REQUIRE(editor.document().layer(root).views.front().choices.size() == 3);
    editor.setSelectedLayer(part);
    editor.captureCharacterView();
    REQUIRE(editor.characterViews().size() == 2);
    editor.moveCharacterView(-1);
    editor.stepCharacterView(1);
    editor.selectTimelineRange(4, 8, 0, 0);
    editor.applyCharacterViewToRange();
    const auto snapshot = editor.document();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto project = QUrl::fromLocalFile(directory.filePath("rig-branch.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == snapshot);
    editor.undo();
    REQUIRE(editor.document() != snapshot);
    editor.redo();
    REQUIRE(editor.document() == snapshot);
}

TEST_CASE("Nineteen imported parts complete the inspector view and substitution journey") {
    QFile specification(QStringLiteral(OPENTOON_SOURCE_DIR "/tests/fixtures/harmony-moment/shot.json"));
    REQUIRE(specification.open(QIODevice::ReadOnly));
    const auto shot = QJsonDocument::fromJson(specification.readAll()).object();
    const auto centers = shot.value("reference_centers_px").toObject();
    const auto roles = shot.value("part_roles").toArray();
    EditorController editor;
    editor.newScene();
    editor.setWorkspaceMode("Rig");
    editor.setScene("Clockwork Hello review", 1920, 1080, 480, 24, 1);
    QVariantList imports;
    std::map<std::string, int> partIds;
    for (const auto& value : roles) {
        const QString role = value.toString();
        const QString variant = role == "mouth" ? "front__rest" :
                                (role == "head" || role == "hair" || role == "eyes") ? "front" :
                                role.startsWith("hand_") ? "open" : "base";
        imports.push_back(QUrl::fromLocalFile(partFixture + role + "__" + variant + ".png"));
    }
    REQUIRE(imports.size() == 19);
    REQUIRE(editor.importParts(imports));
    for (const auto& layer : editor.document().layers)
        for (const auto& value : roles) {
            const auto role = value.toString().toStdString();
            if (layer.name.starts_with(role + "__"))
                partIds.emplace(role, int(layer.id));
        }
    REQUIRE(partIds.size() == 19);
    editor.setSelectedLayer(partIds.at("torso"));
    editor.makeCharacter();
    const int root = editor.characterId();
    REQUIRE(root > 0);
    editor.attachUnparentedDrawings();
    for (const auto& [role, id] : partIds) {
        editor.setSelectedLayer(id);
        editor.setPartRole(QString::fromStdString(role));
        const auto center = centers.value(QString::fromStdString(role)).toArray();
        REQUIRE(center.size() == 2);
        editor.setTransform("x", center[0].toDouble() - 128);
        editor.setTransform("y", center[1].toDouble() - 128);
        REQUIRE(editor.document().layer(id).kind == opentoon::LayerKind::Part);
        REQUIRE(opentoon::characterFor(editor.document(), id) == opentoon::Id(root));
        REQUIRE(editor.document().drawingAt(id, 0));
    }
    // Import order follows filenames. The shot's intended stacking is an artist
    // operation, so exercise the same adjacent layer moves exposed by the UI.
    const auto paintOrder = shot.value("reference_paint_order").toArray();
    REQUIRE(paintOrder.size() == 19);
    for (int target = 0; target < paintOrder.size(); ++target) {
        const int id = partIds.at(paintOrder[target].toString().toStdString());
        while (true) {
            const auto& layers = editor.document().layers;
            const auto current = std::find_if(layers.begin(), layers.end(),
                                              [id](const auto& layer) { return layer.id == opentoon::Id(id); });
            REQUIRE(current != layers.end());
            const int position = int(std::distance(layers.begin(), current));
            if (position == target + 1)
                break;
            editor.setSelectedLayer(id);
            editor.moveLayer(position > target + 1 ? -1 : 1);
        }
    }
    for (const auto& [role, id] : partIds)
        REQUIRE(editor.document().drawingAt(id, 0));
    editor.setSelectedLayer(partIds.at("upper_arm_right"));
    editor.addPeg();
    const int armPeg = int(editor.document().layer(partIds.at("upper_arm_right")).parent);
    editor.setSelectedLayer(partIds.at("hand_right"));
    editor.setParent(armPeg);
    REQUIRE(editor.document().layer(partIds.at("hand_right")).parent == opentoon::Id(armPeg));
    editor.setSelectedLayer(armPeg);
    editor.addKey();
    editor.setFrame(120);
    editor.addKey();
    editor.setFrame(0);
    const auto pegKeys = editor.document().layer(armPeg).keys;
    REQUIRE(pegKeys.size() == 2);
    editor.setSelectedLayer(root);
    editor.captureCharacterView();
    const int front = editor.selectedView();
    editor.renameCharacterView("Front");
    REQUIRE(editor.document().layer(root).views.front().choices.size() == 19);
    const auto reference = QImage(QStringLiteral(OPENTOON_SOURCE_DIR "/tests/fixtures/harmony-moment/reference_0000.png"))
                               .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    REQUIRE_FALSE(reference.isNull());
    const auto assembled = opentoon::SceneRenderer::render(editor.document(), 0);
    if (qEnvironmentVariableIsSet("OPENTOON_HM03_REVIEW_DIR")) {
        QDir output(qEnvironmentVariable("OPENTOON_HM03_REVIEW_DIR"));
        REQUIRE(assembled.save(output.filePath("hm03-assembled.png")));
    }
    REQUIRE(assembled.size() == reference.size());
    REQUIRE(assembled.format() == reference.format());
    int changed = 0;
    int largestChannelError = 0;
    for (int y = 0; y < assembled.height(); ++y)
        for (int x = 0; x < assembled.width(); ++x) {
            const QRgb rendered = assembled.pixel(x, y);
            const QRgb sampled = reference.pixel(x, y);
            if (rendered != sampled)
                ++changed;
            largestChannelError = std::max({largestChannelError,
                                            std::abs(qRed(rendered) - qRed(sampled)),
                                            std::abs(qGreen(rendered) - qGreen(sampled)),
                                            std::abs(qBlue(rendered) - qBlue(sampled))});
        }
    REQUIRE(largestChannelError <= 2);
    REQUIRE(changed < 20000);
    const auto frontImage = opentoon::SceneRenderer::render(editor.document(), 0, {480, 270});
    const auto localizedChange = [&](const QImage& before, const QImage& after,
                                     const QString& role) {
        REQUIRE(before.size() == after.size());
        const auto center = centers.value(role).toArray();
        REQUIRE(center.size() == 2);
        const QRect registered(int(center[0].toDouble()) - 128,
                               int(center[1].toDouble()) - 128, 256, 256);
        int changed = 0;
        for (int y = 0; y < before.height(); ++y)
            for (int x = 0; x < before.width(); ++x)
                if (before.pixel(x, y) != after.pixel(x, y)) {
                    REQUIRE(registered.contains(x, y));
                    ++changed;
                }
        REQUIRE(changed > 0);
    };

    editor.setFrame(120);
    for (const auto& [role, filename] : std::vector<std::pair<std::string, QString>>{
             {"head", "head__three_quarter.png"}, {"hair", "hair__three_quarter.png"},
             {"eyes", "eyes__three_quarter.png"}, {"mouth", "mouth__three_quarter__ah.png"}}) {
        editor.setSelectedLayer(partIds.at(role));
        editor.createSubstitution(false);
        editor.importImage(QUrl::fromLocalFile(partFixture + filename));
        REQUIRE(editor.substitutions().size() == 2);
        REQUIRE(editor.substitutionThumbnail(editor.selectedSubstitution()).startsWith("data:image/png;base64,"));
    }
    editor.setSelectedLayer(root);
    editor.captureCharacterView();
    const int turned = editor.selectedView();
    editor.renameCharacterView("Three-quarter");
    REQUIRE(editor.document().layer(root).views.size() == 2);
    const auto turnedImage = opentoon::SceneRenderer::render(editor.document(), 120, {480, 270});
    REQUIRE(turnedImage != frontImage);

    editor.setFrame(240);
    editor.setSelectedLayer(partIds.at("hand_right"));
    editor.createSubstitution(false);
    editor.importImage(QUrl::fromLocalFile(partFixture + "hand_right__point.png"));
    const int pointedHand = editor.selectedSubstitution();
    REQUIRE(editor.document().drawingAt(partIds.at("hand_right"), 240)->id == opentoon::Id(pointedHand));
    localizedChange(opentoon::SceneRenderer::render(editor.document(), 120),
                    opentoon::SceneRenderer::render(editor.document(), 240), "hand_right");
    editor.setSelectedLayer(root);
    editor.selectView(front);
    editor.setFrame(300);
    editor.applyCharacterView();
    REQUIRE(editor.document().drawingAt(partIds.at("mouth"), 300)->id ==
            editor.document().drawingAt(partIds.at("mouth"), 0)->id);
    REQUIRE(editor.document().drawingAt(partIds.at("hand_right"), 300)->id ==
            editor.document().drawingAt(partIds.at("hand_right"), 0)->id);
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 300, {480, 270}) == frontImage);
    editor.selectView(turned);
    editor.setFrame(360);
    editor.applyCharacterView();
    REQUIRE(editor.document().drawingAt(partIds.at("mouth"), 360)->id ==
            editor.document().drawingAt(partIds.at("mouth"), 120)->id);
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 360, {480, 270}) == turnedImage);
    REQUIRE(editor.document().layer(armPeg).keys == pegKeys);

    // Exercise every supplied mouth drawing, both coordinated views and all
    // hand choices through the same controller actions available to the artist.
    const auto mouth = partIds.at("mouth");
    for (const QString& viewName : {QStringLiteral("three_quarter"), QStringLiteral("front")}) {
        if (viewName == "front") {
            editor.setSelectedLayer(root);
            editor.selectView(front);
            editor.setFrame(400);
            editor.applyCharacterView();
            REQUIRE(opentoon::SceneRenderer::render(editor.document(), 400, {480, 270}) == frontImage);
        }
        int frame = viewName == "front" ? 401 : 380;
        for (const QString& choice : {QStringLiteral("rest"), QStringLiteral("mbp"),
                                      QStringLiteral("fv"), QStringLiteral("ee"),
                                      QStringLiteral("ah"), QStringLiteral("oh"),
                                      QStringLiteral("l"), QStringLiteral("wide")}) {
            if (viewName == "front" && choice == "rest")
                continue;
            if (viewName == "three_quarter" && choice == "ah")
                continue;
            editor.setFrame(frame++);
            editor.setSelectedLayer(mouth);
            const auto beforeChoice = opentoon::SceneRenderer::render(editor.document(), editor.frame());
            editor.createSubstitution(false);
            editor.importImage(QUrl::fromLocalFile(partFixture + "mouth__" + viewName + "__" + choice + ".png"));
            REQUIRE(editor.document().drawingAt(mouth, editor.frame())->id ==
                    opentoon::Id(editor.selectedSubstitution()));
            const auto afterChoice = opentoon::SceneRenderer::render(editor.document(), editor.frame());
            localizedChange(beforeChoice, afterChoice, "mouth");
            const auto changed = editor.document();
            editor.undo();
            editor.undo();
            REQUIRE(opentoon::SceneRenderer::render(editor.document(), editor.frame()) == beforeChoice);
            editor.redo();
            editor.redo();
            REQUIRE(editor.document() == changed);
            REQUIRE(opentoon::SceneRenderer::render(editor.document(), editor.frame()) == afterChoice);
        }
    }
    editor.setSelectedLayer(root);
    editor.selectView(turned);
    editor.setFrame(419);
    editor.applyCharacterView();
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 419, {480, 270}) == turnedImage);
    for (const QString& role : {QStringLiteral("hand_right"), QStringLiteral("hand_left")}) {
        const int hand = partIds.at(role.toStdString());
        for (const QString& choice : {QStringLiteral("fist"), QStringLiteral("point")}) {
            const int frame = role == "hand_right" ? (choice == "fist" ? 420 : 421)
                                                     : (choice == "fist" ? 423 : 424);
            editor.setFrame(frame);
            editor.setSelectedLayer(hand);
            const auto beforeChoice = opentoon::SceneRenderer::render(editor.document(), frame);
            if (role == "hand_right" && choice == "point")
                editor.selectSubstitution(pointedHand);
            else {
                editor.createSubstitution(false);
                editor.importImage(QUrl::fromLocalFile(partFixture + role + "__" + choice + ".png"));
            }
            localizedChange(beforeChoice, opentoon::SceneRenderer::render(editor.document(), frame), role);
        }
        editor.setFrame(role == "hand_right" ? 422 : 425);
        editor.setSelectedLayer(hand);
        const auto beforeOpen = opentoon::SceneRenderer::render(editor.document(), editor.frame());
        editor.selectSubstitution(editor.document().drawingAt(hand, 0)->id);
        localizedChange(beforeOpen, opentoon::SceneRenderer::render(editor.document(), editor.frame()), role);
    }
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 425, {480, 270}) == turnedImage);
    editor.setSelectedLayer(root);
    editor.selectView(turned);
    editor.setFrame(440);
    editor.applyCharacterView();
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 440, {480, 270}) == turnedImage);
    const auto rigged = editor.document();
    editor.undo();
    REQUIRE(editor.document() != rigged);
    editor.redo();
    REQUIRE(editor.document() == rigged);

    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto project = QUrl::fromLocalFile(directory.filePath("character-review.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == rigged);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 360, {480, 270}) == turnedImage);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 300, {480, 270}) == frontImage);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 440, {480, 270}) == turnedImage);
    reopened.setSelectedLayer(root);
    reopened.duplicateCharacter();
    const auto clone = opentoon::Id(reopened.selectedLayer());
    REQUIRE(clone != opentoon::Id(root));
    REQUIRE(reopened.document().layer(clone).views.size() == 2);
    const auto originalMouth = reopened.document().drawingAt(partIds.at("mouth"), 360)->id;
    const auto& cloneChoices = reopened.document().layer(clone).views.back().choices;
    const auto copiedMouth = std::find_if(cloneChoices.begin(), cloneChoices.end(),
                                          [&](const opentoon::ViewChoice& choice) {
                                              return reopened.document().layer(choice.part).role == "mouth";
                                          });
    REQUIRE(copiedMouth != cloneChoices.end());
    const auto copiedMouthId = copiedMouth->part;
    reopened.setSelectedLayer(int(copiedMouthId));
    reopened.setFrame(360);
    reopened.createSubstitution(true);
    REQUIRE(reopened.document().drawingAt(partIds.at("mouth"), 360)->id == originalMouth);
    REQUIRE(reopened.document().drawingAt(copiedMouthId, 360)->id != originalMouth);
    if (qEnvironmentVariableIsSet("OPENTOON_HM03_REVIEW_DIR")) {
        QDir output(qEnvironmentVariable("OPENTOON_HM03_REVIEW_DIR"));
        REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0)
                    .save(output.filePath("hm03-front.png")));
        REQUIRE(opentoon::SceneRenderer::render(editor.document(), 120)
                    .save(output.filePath("hm03-three-quarter.png")));
    }
}

TEST_CASE("Rejected registered part batch leaves the scene and selection intact") {
    EditorController editor;
    editor.newScene();
    const auto before = editor.document();
    const auto selected = editor.selectedLayer();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto corrupt = directory.filePath("broken.png");
    QFile invalid(corrupt);
    REQUIRE(invalid.open(QIODevice::WriteOnly));
    REQUIRE(invalid.write("not a PNG") == 9);
    invalid.close();
    REQUIRE_FALSE(editor.importParts(paths({partFixture + "torso__base.png", corrupt})));
    REQUIRE(editor.document() == before);
    REQUIRE(editor.selectedLayer() == selected);
    QImage mismatched(16, 16, QImage::Format_RGBA8888);
    mismatched.fill(Qt::transparent);
    const auto wrongSize = directory.filePath("wrong-size.png");
    REQUIRE(mismatched.save(wrongSize));
    REQUIRE_FALSE(editor.importParts(paths({partFixture + "torso__base.png", wrongSize})));
    REQUIRE(editor.document() == before);
    REQUIRE(editor.selectedLayer() == selected);
}

TEST_CASE("Numbered PNG sequence orders frames, reports gaps and round-trips") {
    EditorController editor;
    editor.newScene();
    editor.setFrame(47);
    const auto before = editor.document();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto first = directory.filePath("walk_0001.png");
    const auto third = directory.filePath("walk_0003.png");
    REQUIRE(QFile::copy(partFixture + "torso__base.png", first));
    REQUIRE(QFile::copy(partFixture + "head__front.png", third));
    REQUIRE(editor.importImageSequence(paths({third, first})));
    const auto imported = editor.document();
    REQUIRE(imported.duration == 50);
    REQUIRE(imported.layers.size() == before.layers.size() + 1);
    const auto& layer = imported.layers.back();
    REQUIRE(layer.name == "walk_");
    REQUIRE(layer.exposures.size() == 2);
    REQUIRE(layer.exposures[0].start == 47);
    REQUIRE(layer.exposures[0].end == 48);
    REQUIRE(layer.exposures[1].start == 49);
    REQUIRE(layer.exposures[1].end == 50);
    REQUIRE_FALSE(imported.drawingAt(layer.id, 48));
    REQUIRE(editor.status().contains("1 missing frame"));
    const auto project = QUrl::fromLocalFile(directory.filePath("sequence.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == imported);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 49) ==
            opentoon::SceneRenderer::render(imported, 49));
    editor.undo();
    REQUIRE(editor.document() == before);
    REQUIRE_FALSE(editor.importImageSequence(paths({first, first})));
    REQUIRE(editor.document() == before);
}
TEST_CASE("Asynchronous export uses one snapshot and publishes complete rational-time metadata") {
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    EditorController editor;
    editor.loadDemo();
    editor.setScene("Export fixture", 1920, 1080, 48, 24000, 1001);
    const auto snapshot = editor.document();
    editor.exportFrames(QUrl::fromLocalFile(temporary.path()));
    REQUIRE(editor.exporting());
    // A visible edit after dispatch must not leak into the running render.
    editor.setSwatchColor(editor.selectedSwatch(), Qt::red);
    waitForExport(editor);
    QDir root(temporary.path());
    auto folders = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    REQUIRE(folders.size() == 1);
    auto info = manifest(root, folders.front());
    REQUIRE(info["status"].toString() == "complete");
    REQUIRE(info["framesWritten"].toInt() == 48);
    REQUIRE(info["fpsNumerator"].toInt() == 24000);
    REQUIRE(info["fpsDenominator"].toInt() == 1001);
    QDir output(root.filePath(folders.front()));
    REQUIRE(output.entryList({"frame_*.png"}, QDir::Files).size() == 48);
    auto actual =
        QImage(output.filePath("frame_000001.png")).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    REQUIRE(actual == opentoon::SceneRenderer::render(snapshot, 0));
}
TEST_CASE("Animated output camera matches reopened preview and exported PNG frames") {
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    EditorController editor;
    editor.newScene();
    editor.setScene("Camera export", 64, 64, 48, 24, 1);
    editor.commitStroke({{12, 14, 1}, {26, 34, 1}});
    editor.addCamera();
    REQUIRE(editor.activeCamera() > 0);
    editor.setTransform("x", 40);
    editor.setFrame(24);
    editor.setTransform("x", 24);
    const auto snapshot = editor.document();
    REQUIRE(snapshot.layer(snapshot.activeCamera).keys.size() >= 2);
    const auto before = opentoon::SceneRenderer::render(snapshot, 0);
    const auto after = opentoon::SceneRenderer::render(snapshot, 24);
    REQUIRE(before != after);
    const auto project = QUrl::fromLocalFile(temporary.filePath("camera.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == snapshot);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 0) == before);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 24) == after);
    editor.exportFrames(QUrl::fromLocalFile(temporary.path()));
    waitForExport(editor);
    QDir root(temporary.path());
    const auto folders = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    REQUIRE(folders.size() == 1);
    QDir output(root.filePath(folders.front()));
    REQUIRE(manifest(root, folders.front())["status"].toString() == "complete");
    REQUIRE(QImage(output.filePath("frame_000001.png"))
                .convertToFormat(QImage::Format_ARGB32_Premultiplied) == before);
    REQUIRE(QImage(output.filePath("frame_000025.png"))
                .convertToFormat(QImage::Format_ARGB32_Premultiplied) == after);
}
TEST_CASE("Cancellation publishes an explicitly partial export and the next job can complete") {
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    EditorController editor;
    editor.setScene("Cancellation fixture", 64, 64, 1000, 24, 1);
    editor.exportFrames(QUrl::fromLocalFile(temporary.path()));
    editor.cancelExport();
    waitForExport(editor);
    QDir root(temporary.path());
    auto first = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    REQUIRE(first.size() == 1);
    auto cancelled = manifest(root, first.front());
    REQUIRE(cancelled["status"].toString() == "cancelled");
    REQUIRE(cancelled["framesWritten"].toInt() < 1000);
    editor.newScene();
    editor.setScene("Next job", 64, 64, 48, 24, 1);
    editor.exportFrames(QUrl::fromLocalFile(temporary.path()));
    waitForExport(editor);
    auto next = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    REQUIRE(next.size() == 2);
    next.removeAll(first.front());
    REQUIRE(manifest(root, next.front())["status"].toString() == "complete");
}
TEST_CASE("Recovery saves an immutable snapshot without clearing newer unsaved edits") {
    EditorController editor;
    editor.setScene("Before autosave", 128, 128, 48, 24, 1);
    const auto saved = editor.document();
    editor.autosave();
    REQUIRE(editor.savingRecovery());
    editor.setScene("After autosave", 128, 128, 48, 24, 1);
    QElapsedTimer timeout;
    timeout.start();
    while (editor.savingRecovery() && timeout.elapsed() < 30000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    REQUIRE_FALSE(editor.savingRecovery());
    REQUIRE(editor.modified());
    REQUIRE(editor.sceneName() == "After autosave");
    auto path = QSettings().value("recoveryPath").toString();
    INFO(editor.status().toStdString());
    REQUIRE_FALSE(path.isEmpty());
    REQUIRE(opentoon::ProjectStore::load(std::filesystem::path(path.toStdString())).document == saved);
    QFile::remove(path);
}

TEST_CASE("Visual curve commands preserve poses and undo atomically") {
    EditorController editor;
    editor.newScene();
    auto before = editor.document();
    REQUIRE(editor.addCurveKey(20, "x", 100));
    REQUIRE(editor.setCurveHandles(0, "x", .25, 0, .65, 1.8));
    auto eased = editor.document();
    editor.setFrame(0);
    editor.addKey();
    REQUIRE(editor.document() == eased);
    REQUIRE_FALSE(editor.setCurveHandles(0, "x", .8, 0, .2, 1));
    REQUIRE(editor.document() == eased);
    editor.undo();
    editor.undo();
    REQUIRE(editor.document() == before);
}

TEST_CASE("Clear removes range exposures and keys together and undo restores both") {
    EditorController editor;
    editor.newScene();
    editor.holdDrawing(40);
    REQUIRE(editor.addCurveKey(10, "x", 100));
    REQUIRE(editor.addCurveKey(20, "y", 60));
    REQUIRE(editor.setPoseCurveHandles(0, .3, 0, .7, 1));
    auto original = editor.document();
    editor.selectTimelineRange(0, 10, 0, 0);
    editor.clearTimelineRange();
    const auto& layer = editor.document().layer(editor.selectedLayer());
    REQUIRE(layer.keys.size() == 1);
    REQUIRE(layer.keys.front().frame == 20);
    REQUIRE_FALSE(editor.document().drawingAt(layer.id, 0));
    REQUIRE(editor.document().drawingAt(layer.id, 11));
    editor.undo();
    REQUIRE(editor.document() == original);
    editor.setFrame(10);
    editor.clearExposure();
    REQUIRE(editor.document().layer(editor.selectedLayer()).keys.size() == 2);
    REQUIRE_FALSE(editor.document().drawingAt(editor.selectedLayer(), 10));
    editor.undo();
    REQUIRE(editor.document() == original);
    editor.toggleLayer(editor.selectedLayer(), "locked");
    auto locked = editor.document();
    editor.clearTimelineRange();
    REQUIRE(editor.document() == locked);
}
int main(int argc, char** argv) {
    QGuiApplication application(argc, argv);
    QCoreApplication::setApplicationName("OPEN-TOON-export-tests");
    QCoreApplication::setOrganizationName("OPEN-TOON-tests");
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid())
        return 1;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    QStandardPaths::setTestModeEnabled(true);
    return Catch::Session().run(argc, argv);
}

TEST_CASE("Animation inspector edits, curve sampling, navigation and saved rendering agree") {
    EditorController editor;
    editor.loadDemo();
    const auto layer = editor.selectedLayer();
    editor.setAnimateMode(true);
    editor.setAutoKey(true);
    editor.setFrame(0);
    editor.setTransform("x", 0);
    editor.setFrame(20);
    editor.setTransform("x", 200);
    editor.setAutoKey(false);
    editor.setFrame(10);
    const auto before = editor.document();
    editor.setTransform("x", 999);
    REQUIRE(editor.document() == before);
    REQUIRE(editor.keyState() == "Interpolated pose");
    editor.nextKey(1);
    REQUIRE(editor.frame() == 20);
    editor.nextKey(-1);
    REQUIRE(editor.frame() == 0);
    REQUIRE(editor.updateKey(20, 24, "x", 240, 1));
    editor.setFrame(12);
    REQUIRE(editor.transform()["x"].toDouble() == 120);
    auto samples = editor.curveSamples("x", 48);
    REQUIRE_FALSE(samples.empty());
    for (auto sample : samples) {
        auto map = sample.toMap();
        REQUIRE(map["value"].toDouble() ==
                opentoon::evaluateTransform(editor.document().layer(layer), map["frame"].toInt()).x);
    }
    QTemporaryDir dir;
    auto path = QUrl::fromLocalFile(dir.filePath("animation.otoon"));
    REQUIRE(editor.saveProject(path));
    auto image = opentoon::SceneRenderer::render(editor.document(), 12);
    EditorController reopened;
    REQUIRE(reopened.openProject(path));
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 12) == image);
    auto snapshot = editor.document();
    REQUIRE_FALSE(editor.updateKey(24, 0, "x", 240, 0));
    REQUIRE(editor.document() == snapshot);
    editor.undo();
    REQUIRE(editor.document().layer(layer).keys.back().frame == 20);
}

TEST_CASE("Drawing selections respect locked layers and media filters without adding empty drawings") {
    EditorController editor;
    const auto empty = editor.document();
    REQUIRE_FALSE(editor.editDrawingRegion({0, 0, 1920, 1080}, opentoon::SelectionMedia::Both,
                                           opentoon::SelectionAction::Delete));
    REQUIRE(editor.document() == empty);
    editor.loadDemo();
    const auto before = editor.document();
    REQUIRE_FALSE(editor.editDrawingRegion({0, 0, 1920, 1080}, opentoon::SelectionMedia::Raster,
                                           opentoon::SelectionAction::Delete));
    REQUIRE(editor.document() == before);
    editor.toggleLayer(editor.selectedLayer(), "locked");
    const auto locked = editor.document();
    REQUIRE_FALSE(editor.editDrawingRegion({0, 0, 1920, 1080}, opentoon::SelectionMedia::Both,
                                           opentoon::SelectionAction::Delete));
    REQUIRE(editor.document() == locked);
    editor.setBrushOpacity(.25);
    REQUIRE(editor.brushOpacity() == .25);
}

TEST_CASE("Pose-key selection edits only animation and reconciles after history or layer changes") {
    EditorController editor;
    editor.newScene();
    editor.addCurveKey(4, "x", 20);
    editor.addCurveKey(8, "x", 40);
    editor.addCurveKey(12, "x", 80);
    const auto layer = editor.selectedLayer();
    editor.clearPoseSelection();
    editor.selectPoseKey(4);
    editor.selectPoseKey(12, true);
    REQUIRE(editor.selectedPoseFrames() == QVariantList{4, 8, 12});
    editor.selectPoseKey(8, false, true);
    REQUIRE(editor.selectedPoseFrames() == QVariantList{4, 12});
    auto before = editor.document();
    REQUIRE(editor.moveSelectedPoseKeys(2));
    REQUIRE(editor.selectedPoseFrames() == QVariantList{6, 14});
    REQUIRE(editor.document().drawings == before.drawings);
    REQUIRE(editor.document().layer(layer).exposures == before.layer(layer).exposures);
    REQUIRE_FALSE(editor.moveSelectedPoseKeys(2)); // frame 8 is occupied by the unselected key.
    REQUIRE(editor.selectedPoseFrames() == QVariantList{6, 14});
    editor.undo();
    REQUIRE(editor.document() == before);
    REQUIRE(editor.selectedPoseFrames().empty());
    editor.selectPoseRange(4, 12);
    editor.copyPoseKeys();
    editor.addLayer();
    const auto target = editor.selectedLayer();
    REQUIRE(editor.selectedPoseFrames().empty());
    REQUIRE(editor.hasPoseClipboard());
    editor.setFrame(20);
    before = editor.document();
    REQUIRE(editor.pastePoseKeys());
    REQUIRE(editor.selectedPoseFrames() == QVariantList{20, 24, 28});
    REQUIRE(editor.document().drawings == before.drawings);
    REQUIRE(editor.document().layer(layer) == before.layer(layer));
    REQUIRE(editor.document().layer(target).exposures == before.layer(target).exposures);
    auto pasted = editor.document();
    REQUIRE_FALSE(editor.pastePoseKeys());
    REQUIRE(editor.document() == pasted);
    REQUIRE(editor.deleteSelectedPoseKeys());
    REQUIRE(editor.document() == before);
    editor.undo();
    REQUIRE(editor.document() == pasted);
    editor.selectPoseRange(20, 28);
    editor.toggleLayer(target, "locked");
    before = editor.document();
    REQUIRE_FALSE(editor.moveSelectedPoseKeys(1));
    REQUIRE_FALSE(editor.deleteSelectedPoseKeys());
    REQUIRE(editor.document() == before);
    editor.newScene();
    REQUIRE(editor.selectedPoseFrames().empty());
    REQUIRE(editor.hasPoseClipboard());
}

TEST_CASE("Motion-path position commands preserve easing and pixels through undo and persistence") {
    EditorController editor;
    editor.loadDemo();
    const auto id = editor.selectedLayer();
    editor.addCurveKey(12, "x", 50);
    editor.addCurveKey(24, "x", 100);
    editor.setCurveHandles(12, "x", .2, 0, .8, 1.4);
    const auto before = editor.document();
    REQUIRE(editor.setPoseKeyPosition(12, 140, 30));
    REQUIRE(editor.document().drawings == before.drawings);
    auto key = std::find_if(editor.document().layer(id).keys.begin(), editor.document().layer(id).keys.end(),
                            [](const auto& key) { return key.frame == 12; });
    REQUIRE(key->easing.at("x").y2 == 1.4);
    QTemporaryDir tmp;
    auto file = QUrl::fromLocalFile(tmp.path() + "/path.otoon");
    auto pixels = opentoon::SceneRenderer::render(editor.document(), 18);
    REQUIRE(editor.saveProject(file));
    editor.undo();
    REQUIRE(editor.document() == before);
    REQUIRE(editor.openProject(file));
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 18) == pixels);
    editor.toggleLayer(id, "locked");
    const auto locked = editor.document();
    REQUIRE_FALSE(editor.setPoseKeyPosition(12, 0, 0));
    REQUIRE(editor.document() == locked);
    REQUIRE_FALSE(editor.setPoseKeyPosition(999, 0, 0));
    REQUIRE(editor.document() == locked);
}

TEST_CASE("Layer pose menu copies full and masked transforms without changing other channels") {
    EditorController editor;
    editor.newScene();
    REQUIRE_FALSE(editor.pasteTransformPose(0));
    editor.setTransform("x", 80);
    editor.setTransform("y", 40);
    editor.setTransform("rotation", 15);
    editor.setTransform("scaleX", 1.5);
    editor.setTransform("scaleY", .8);
    editor.setTransform("opacity", .7);
    editor.setTransform("pivotX", 5);
    editor.setTransform("pivotY", 7);
    const auto copied = editor.document().layer(editor.selectedLayer()).transform;
    editor.copyTransformPose();
    REQUIRE(editor.hasCopiedTransform());
    editor.addLayer();
    const auto target = editor.selectedLayer();
    const auto baseline = editor.document();
    for (int mode = 0; mode <= 7; ++mode) {
        REQUIRE(editor.pasteTransformPose(mode));
        const auto& pose = editor.document().layer(target).transform;
        REQUIRE(pose.x == (mode == 0 || mode == 1 || mode >= 6 ? copied.x : 0));
        REQUIRE(pose.y == (mode == 0 || mode == 1 || mode >= 6 ? copied.y : 0));
        REQUIRE(pose.rotation == (mode == 0 || mode == 2 || mode >= 6 ? copied.rotation : 0));
        REQUIRE(pose.scaleX == (mode == 6 ? -copied.scaleX :
                                mode == 0 || mode == 3 || mode == 7 ? copied.scaleX : 1));
        REQUIRE(pose.scaleY == (mode == 7 ? -copied.scaleY :
                                mode == 0 || mode == 3 || mode == 6 ? copied.scaleY : 1));
        REQUIRE(pose.opacity == (mode == 0 || mode == 4 || mode >= 6 ? copied.opacity : 1));
        REQUIRE(pose.pivotX == (mode == 0 || mode == 5 || mode >= 6 ? copied.pivotX : 0));
        REQUIRE(pose.pivotY == (mode == 0 || mode == 5 || mode >= 6 ? copied.pivotY : 0));
        editor.undo();
        REQUIRE(editor.document() == baseline);
    }
    REQUIRE_FALSE(editor.pasteTransformPose(8));
    REQUIRE(editor.document() == baseline);
    REQUIRE(editor.pasteTransformPose(0));
    REQUIRE(editor.resetTransformPose());
    REQUIRE(editor.document() == baseline);
    editor.toggleLayer(target, "locked");
    const auto locked = editor.document();
    REQUIRE_FALSE(editor.pasteTransformPose(0));
    REQUIRE_FALSE(editor.resetTransformPose());
    REQUIRE(editor.document() == locked);
}

TEST_CASE("Explicit pose paste creates a later key, preserves rest and reopens identically") {
    EditorController editor;
    editor.newScene();
    editor.setTransform("x", 100);
    editor.setTransform("scaleX", -1);
    editor.copyTransformPose();
    editor.addLayer();
    const auto target = editor.selectedLayer();
    editor.setFrame(12);
    editor.setAnimateMode(true);
    editor.setAutoKey(false);
    const auto before = editor.document();
    REQUIRE(editor.pasteTransformPose(0));
    const auto pasted = editor.document();
    REQUIRE(pasted.layer(target).transform.x == 0);
    REQUIRE(pasted.layer(target).keys.size() == 2);
    REQUIRE(pasted.layer(target).keys[0].frame == 0);
    REQUIRE(pasted.layer(target).keys[0].value.x == 0);
    REQUIRE(pasted.layer(target).keys[1].frame == 12);
    REQUIRE(pasted.layer(target).keys[1].value.x == 100);
    REQUIRE(pasted.layer(target).keys[1].value.scaleX == -1);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    auto project = QUrl::fromLocalFile(directory.filePath("copied-pose.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == pasted);
    editor.undo();
    REQUIRE(editor.document() == before);
    REQUIRE(editor.resetTransformPose());
    REQUIRE(editor.document() != before);
    REQUIRE(editor.document().layer(target).keys.back().value == before.layer(target).transform);
}

TEST_CASE("Single image import creates its layer atomically and converts tagged color to sRGB") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    QImage source(2, 2, QImage::Format_RGBA8888);
    source.fill(QColor(220, 45, 120, 180));
    source.setColorSpace(QColorSpace(QColorSpace::DisplayP3));
    const auto imagePath = directory.filePath("wide-gamut.png");
    REQUIRE(source.save(imagePath));
    auto expected = QImage(imagePath).convertedToColorSpace(QColorSpace(QColorSpace::SRgb))
                                    .convertToFormat(QImage::Format_RGBA8888);
    REQUIRE_FALSE(expected.isNull());
    EditorController editor;
    editor.newScene();
    editor.removeLayer();
    const auto empty = editor.document();
    REQUIRE(empty.layers.empty());
    editor.importImage(QUrl::fromLocalFile(directory.filePath("missing.png")));
    REQUIRE(editor.document() == empty);
    editor.importImage(QUrl::fromLocalFile(imagePath));
    const auto imported = editor.document();
    REQUIRE(imported.layers.size() == 1);
    REQUIRE(imported.drawings.size() == 1);
    REQUIRE(imported.palette == empty.palette);
    const auto& asset = *imported.drawings.begin()->second.image;
    REQUIRE(asset.width == 2);
    REQUIRE(asset.height == 2);
    for (int row = 0; row < 2; ++row)
        REQUIRE(std::memcmp(asset.rgba.data() + row * 8, expected.constScanLine(row), 8) == 0);
    const auto project = QUrl::fromLocalFile(directory.filePath("imported.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == imported);
    editor.undo();
    REQUIRE(editor.document() == empty);
    editor.redo();
    REQUIRE(editor.document() == imported);
}
