#pragma once
#include "opentoon/document.h"
#include <QSize>
#include <QString>
#include <QVariantList>
#include <optional>
#include <vector>

struct ImportedImage {
    QString name;
    int number = 0;
    opentoon::ImageAsset image;
};

struct ImageBatch {
    std::vector<ImportedImage> images;
    QString sequencePrefix;
    QSize canvas;
    int span = 1;
    bool hasUntaggedColor = false;
};

// Decode and validate every source before any document command is attempted.
std::optional<ImageBatch> loadImageBatch(const QVariantList& urls, bool sequence,
                                         opentoon::Frame start, QString& error);
