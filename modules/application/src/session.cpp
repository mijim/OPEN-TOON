#include "opentoon/session.h"
namespace opentoon {
Session::Session() {
    replace(makeDocument());
}
bool Session::apply(const std::string& label, const std::function<void(Document&)>& operation) {
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
void Session::trimHistory() {
    std::size_t bytes = 0, keep = 0;
    for (auto it = undo_.rbegin(); it != undo_.rend(); ++it) {
        std::size_t size = sizeof(Document);
        for (const auto& [id, d] : it->document->drawings) {
            (void)id;
            for (const auto& s : d.strokes)
                size += s.points.size() * sizeof(Point);
            if (d.image)
                size += d.image->rgba.size();
        }
        bytes += size;
        if (++keep >= 100 || bytes > 128 * 1024 * 1024)
            break;
    }
    if (undo_.size() > keep)
        undo_.erase(undo_.begin(), undo_.end() - static_cast<std::ptrdiff_t>(keep));
}
void Session::replace(Document document, bool saved) {
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
