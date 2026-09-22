#include "revision_render_cache.h"
#include <algorithm>
#include <stdexcept>

namespace opentoon {
RevisionRenderCache::RevisionRenderCache(std::size_t budgetBytes) : budget_(budgetBytes) {}
bool RevisionRenderCache::olderThanCurrent(const RenderCacheKey& key) const {
    return key.scene < scene_ || (key.scene == scene_ && key.revision < revision_);
}
void RevisionRenderCache::advance(const RenderCacheKey& key) {
    if (scene_ == key.scene && revision_ == key.revision)
        return;
    scene_ = key.scene;
    revision_ = key.revision;
    ++serial_;
    entries_.clear();
    retained_ = 0;
}
std::optional<QImage> RevisionRenderCache::lookup(const RenderCacheKey& key) {
    std::lock_guard lock(mutex_);
    if (olderThanCurrent(key))
        return std::nullopt;
    advance(key);
    for (auto it = entries_.begin(); it != entries_.end(); ++it)
        if (it->key == key) {
            auto image = it->image;
            entries_.splice(entries_.begin(), entries_, it);
            return image;
        }
    return std::nullopt;
}
RenderTicket RevisionRenderCache::begin(const RenderCacheKey& key) {
    std::lock_guard lock(mutex_);
    if (olderThanCurrent(key))
        return {key, 0};
    advance(key);
    return {key, ++serial_};
}
bool RevisionRenderCache::current(const RenderTicket& ticket) const {
    std::lock_guard lock(mutex_);
    return ticket.serial != 0 && ticket.serial == serial_ && ticket.key.scene == scene_ &&
           ticket.key.revision == revision_;
}
bool RevisionRenderCache::publish(const RenderTicket& ticket, QImage image) {
    if (image.isNull() || image.width() != ticket.key.width ||
        image.height() != ticket.key.height)
        return false;
    std::lock_guard lock(mutex_);
    if (ticket.serial == 0 || ticket.serial != serial_ || ticket.key.scene != scene_ ||
        ticket.key.revision != revision_)
        return false;
    const auto bytes = std::size_t(image.sizeInBytes());
    for (auto it = entries_.begin(); it != entries_.end(); ++it)
        if (it->key == ticket.key) {
            retained_ -= it->bytes;
            entries_.erase(it);
            break;
        }
    if (bytes > budget_)
        return true; // Valid result, too large to retain.
    while (!entries_.empty() && retained_ + bytes > budget_) {
        retained_ -= entries_.back().bytes;
        entries_.pop_back();
    }
    retained_ += bytes;
    entries_.push_front({ticket.key, std::move(image), bytes});
    return true;
}
QImage RevisionRenderCache::resolve(const RenderCacheKey& key,
                                    const std::function<QImage()>& renderer) {
    if (auto image = lookup(key))
        return *image;
    auto ticket = begin(key);
    if (!ticket.serial)
        throw std::invalid_argument("Render request is older than the current scene.");
    auto image = renderer();
    return publish(ticket, image) ? image : QImage{};
}
void RevisionRenderCache::clear() {
    std::lock_guard lock(mutex_);
    ++serial_;
    scene_ = 0;
    revision_ = 0;
    entries_.clear();
    retained_ = 0;
}
std::size_t RevisionRenderCache::retainedBytes() const {
    std::lock_guard lock(mutex_);
    return retained_;
}
} // namespace opentoon
