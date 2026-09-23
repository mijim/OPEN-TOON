#include "image_batch_importer.h"
#include <QColorSpace>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <cstdint>

std::optional<ImageBatch> loadImageBatch(const QVariantList& urls, bool sequence,
                                         opentoon::Frame start, QString& error) {
    struct Source {
        QString path;
        QString name;
        int number = 0;
    };
    const int limit = sequence ? 512 : 64;
    if (urls.isEmpty() || urls.size() > limit) {
        error = QString("Select between 1 and %1 PNG files.").arg(limit);
        return std::nullopt;
    }
    std::vector<Source> sources;
    sources.reserve(urls.size());
    QSet<QString> names;
    QSet<int> numbers;
    QString sequencePrefix;
    int sequenceDigits = -1;
    const QRegularExpression numberedName("^(.*?)(\\d+)$");
    for (const auto& item : urls) {
        const auto url = item.toUrl();
        if (!url.isLocalFile()) {
            error = "Import requires local PNG files.";
            return std::nullopt;
        }
        QFileInfo file(url.toLocalFile());
        if (file.suffix().compare("png", Qt::CaseInsensitive) != 0) {
            error = "This import accepts PNG files only.";
            return std::nullopt;
        }
        const QString name = file.completeBaseName();
        if (name.isEmpty() || name.toUtf8().size() > 4096) {
            error = "An imported image has an invalid or oversized name.";
            return std::nullopt;
        }
        Source source{file.absoluteFilePath(), name};
        if (sequence) {
            const auto match = numberedName.match(name);
            if (!match.hasMatch()) {
                error = "Sequence filenames need a common prefix and trailing frame number.";
                return std::nullopt;
            }
            const auto prefix = match.captured(1);
            const auto digits = match.captured(2);
            bool valid = false;
            const auto number = digits.toInt(&valid);
            if (!valid || number < 0 || (sequenceDigits >= 0 &&
                                        (prefix != sequencePrefix || digits.size() != sequenceDigits))) {
                error = "Sequence filenames need one prefix and one number width.";
                return std::nullopt;
            }
            if (numbers.contains(number)) {
                error = "Sequence contains a duplicate frame number.";
                return std::nullopt;
            }
            numbers.insert(number);
            sequencePrefix = prefix;
            sequenceDigits = digits.size();
            source.number = number;
        } else {
            const auto folded = name.toCaseFolded();
            if (names.contains(folded)) {
                error = "Registered parts need distinct filenames.";
                return std::nullopt;
            }
            names.insert(folded);
        }
        sources.push_back(std::move(source));
    }
    if (sequence)
        std::sort(sources.begin(), sources.end(), [](const Source& a, const Source& b) {
            return a.number < b.number;
        });
    else
        std::sort(sources.begin(), sources.end(), [](const Source& a, const Source& b) {
            const auto left = a.name.toCaseFolded(), right = b.name.toCaseFolded();
            return left == right ? a.path < b.path : left < right;
        });
    const std::int64_t requestedSpan = sequence
                                           ? std::int64_t(sources.back().number) - sources.front().number + 1
                                           : 1;
    if (sequence && (requestedSpan > 10000 || std::int64_t(start) + requestedSpan > 1000000)) {
        error = "Sequence span exceeds the supported timeline range.";
        return std::nullopt;
    }
    ImageBatch batch;
    batch.images.reserve(sources.size());
    batch.sequencePrefix = sequencePrefix;
    batch.span = static_cast<int>(requestedSpan);
    std::int64_t decodedBytes = 0;
    for (const auto& source : sources) {
        QImageReader reader(source.path);
        reader.setAutoTransform(false); // Preserve the supplied pixel origin.
        const auto size = reader.size();
        if (reader.format().toLower() != "png" || !size.isValid() || size.width() > 4096 ||
            size.height() > 4096 || size.width() <= 0 || size.height() <= 0) {
            error = "Invalid PNG or image exceeds 4096 × 4096 pixels: " + source.name;
            return std::nullopt;
        }
        if (batch.canvas.isValid() && size != batch.canvas) {
            error = "Images must have the same canvas size to preserve registration: " + source.name;
            return std::nullopt;
        }
        batch.canvas = size;
        decodedBytes += std::int64_t(size.width()) * size.height() * 4;
        if (decodedBytes > 256LL * 1024 * 1024) {
            error = "Image batch exceeds the 256 MiB decoded-media limit.";
            return std::nullopt;
        }
        auto image = reader.read();
        if (image.isNull() || image.size() != batch.canvas || !image.hasAlphaChannel()) {
            error = "Could not read a transparent PNG image: " + source.name;
            return std::nullopt;
        }
        if (image.colorSpace().isValid() && image.colorSpace() != QColorSpace(QColorSpace::SRgb)) {
            error = "Only sRGB PNG artwork is supported in this import: " + source.name;
            return std::nullopt;
        }
        batch.hasUntaggedColor |= !image.colorSpace().isValid();
        image = image.convertToFormat(QImage::Format_RGBA8888);
        std::vector<std::uint8_t> rgba;
        rgba.reserve(static_cast<std::size_t>(size.width()) * size.height() * 4);
        for (int y = 0; y < size.height(); ++y) {
            const auto* row = image.constScanLine(y);
            rgba.insert(rgba.end(), row, row + size.width() * 4);
        }
        batch.images.push_back({source.name, source.number,
                                {size.width(), size.height(), std::move(rgba)}});
    }
    return batch;
}
