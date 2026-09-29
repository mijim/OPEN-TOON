#include "opentoon/session.h"
#include <set>
#include <stdexcept>
namespace opentoon {
Session::Session() {
    replace(makeDocument());
}
bool Session::apply(const std::string& label, const std::function<void(Document&)>& operation) {
    coalesced_.reset();
    auto next = std::make_shared<Document>(*current_);
    operation(*next);
    next->validate();
    if (*next == *current_)
        return false;
    undo_.push_back({label, current_});
    redo_.clear();
    current_ = std::move(next);
    ++revision_;
    trimHistory();
    return true;
}
bool Session::applyCoalesced(const std::string& label, std::uint64_t gesture,
                             const std::function<void(Document&)>& operation) {
    if (!gesture)
        throw std::invalid_argument("Coalesced edit needs a nonzero gesture ID.");
    if (coalesced_ && coalesced_->gesture != gesture)
        coalesced_.reset();
    const auto baseline = coalesced_ ? coalesced_->baseline : current_;
    auto next = std::make_shared<Document>(*baseline);
    operation(*next);
    next->validate();
    if (*next == *current_)
        return false;
    if (!coalesced_) {
        undo_.push_back({label, baseline});
        redo_.clear();
        coalesced_ = CoalescedEdit{gesture, baseline};
    }
    current_ = std::move(next);
    ++revision_;
    if (*current_ == *baseline) {
        undo_.pop_back();
        coalesced_.reset();
    }
    return true;
}
void Session::endCoalesced(std::uint64_t gesture) {
    if (coalesced_ && coalesced_->gesture == gesture) {
        coalesced_.reset();
        trimHistory();
    }
}
void Session::trimHistory() {
    std::size_t bytes = 0, keep = 0;
    std::set<const void*> media;
    for (const auto& [id, drawing] : current_->drawings) {
        (void)id;
        if (drawing.image)
            media.insert(drawing.image->rgba.data());
        if (drawing.raster)
            for (const auto& [position, tile] : drawing.raster->tiles) {
                (void)position;
                media.insert(tile.data());
            }
    }
    for (auto it = undo_.rbegin(); it != undo_.rend(); ++it) {
        std::size_t size = sizeof(Document);
        for (const auto& [id, d] : it->document->drawings) {
            (void)id;
            for (const auto& s : d.strokes)
                size += s.points.size() * sizeof(Point);
            if (d.image && media.insert(d.image->rgba.data()).second)
                size += d.image->rgba.size();
            if (d.raster)
                for (const auto& [position, tile] : d.raster->tiles) {
                    (void)position;
                    size += sizeof(void*) * 6;
                    if (media.insert(tile.data()).second)
                        size += tile.size() * sizeof(std::uint16_t);
                }
        }
        bytes += size;
        if (++keep >= 100 || bytes > 128 * 1024 * 1024)
            break;
    }
    if (undo_.size() > keep)
        undo_.erase(undo_.begin(), undo_.end() - static_cast<std::ptrdiff_t>(keep));
}
void Session::replace(Document document, bool saved) {
    coalesced_.reset();
    document.validate();
    current_ = std::make_shared<Document>(std::move(document));
    saved_ = saved ? current_ : nullptr;
    undo_.clear();
    redo_.clear();
    ++revision_;
}
void Session::markSaved(std::shared_ptr<const Document> snapshot) {
    saved_ = std::move(snapshot);
}
std::string Session::undoLabel() const {
    return undo_.empty() ? "Undo" : "Undo " + undo_.back().label;
}
bool Session::undo() {
    coalesced_.reset();
    if (undo_.empty())
        return false;
    auto e = undo_.back();
    undo_.pop_back();
    redo_.push_back({e.label, current_});
    current_ = e.document;
    ++revision_;
    return true;
}
bool Session::redo() {
    coalesced_.reset();
    if (redo_.empty())
        return false;
    auto e = redo_.back();
    redo_.pop_back();
    undo_.push_back({e.label, current_});
    current_ = e.document;
    ++revision_;
    return true;
}
} // namespace opentoon
