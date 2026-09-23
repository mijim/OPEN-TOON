#pragma once
#include "opentoon/document.h"
#include "opentoon/composition_graph.h"
#include <QImage>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <optional>

namespace opentoon {
struct RenderCacheKey {
    std::uint64_t scene = 0;
    std::uint64_t revision = 0;
    Frame frame = 0;
    int width = 0;
    int height = 0;
    CompositionProfile profile = CompositionProfile::LegacyQt;
    GraphTarget target = GraphTarget::Display;
    bool background = true;
    bool onionSkin = false;
    int onionRange = 1;
    bool ignoreCamera = false;
    bool operator==(const RenderCacheKey&) const = default;
};
struct RenderTicket {
    RenderCacheKey key;
    std::uint64_t serial = 0;
};
class RevisionRenderCache {
  public:
    explicit RevisionRenderCache(std::size_t budgetBytes = 96 * 1024 * 1024);
    [[nodiscard]] std::optional<QImage> lookup(const RenderCacheKey&);
    [[nodiscard]] RenderTicket begin(const RenderCacheKey&);
    [[nodiscard]] bool current(const RenderTicket&) const;
    [[nodiscard]] bool publish(const RenderTicket&, QImage);
    [[nodiscard]] QImage resolve(const RenderCacheKey&, const std::function<QImage()>&);
    [[nodiscard]] bool canRetain(const RenderCacheKey&) const;
    void cancelPending();
    void clear();
    [[nodiscard]] std::size_t retainedBytes() const;

  private:
    struct Entry { RenderCacheKey key; QImage image; std::size_t bytes = 0; };
    void advance(const RenderCacheKey&);
    [[nodiscard]] bool olderThanCurrent(const RenderCacheKey&) const;
    mutable std::mutex mutex_;
    std::list<Entry> entries_;
    std::uint64_t scene_ = 0, revision_ = 0, serial_ = 0;
    std::size_t budget_, retained_ = 0;
};
} // namespace opentoon
