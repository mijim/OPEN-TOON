#pragma once
#include "opentoon/document.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace opentoon {
class Session {
  public:
    Session();
    [[nodiscard]] const Document& document() const { return *current_; }
    [[nodiscard]] std::shared_ptr<const Document> snapshot() const { return current_; }
    [[nodiscard]] bool modified() const { return current_ != saved_; }
    [[nodiscard]] bool canUndo() const { return !undo_.empty(); }
    [[nodiscard]] bool canRedo() const { return !redo_.empty(); }
    [[nodiscard]] std::string undoLabel() const;
    [[nodiscard]] std::uint64_t revision() const { return revision_; }
    bool apply(const std::string& label, const std::function<void(Document&)>& operation);
    bool applyCoalesced(const std::string& label, std::uint64_t gesture,
                        const std::function<void(Document&)>& operation);
    void endCoalesced(std::uint64_t gesture);
    void replace(Document document, bool saved = true);
    void markSaved(std::shared_ptr<const Document> snapshot);
    bool undo();
    bool redo();

  private:
    struct Entry {
        std::string label;
        std::shared_ptr<const Document> document;
    };
    std::shared_ptr<const Document> current_, saved_;
    std::vector<Entry> undo_, redo_;
    std::uint64_t revision_ = 0;
    struct CoalescedEdit {
        std::uint64_t gesture = 0;
        std::shared_ptr<const Document> baseline;
    };
    std::optional<CoalescedEdit> coalesced_;
    void trimHistory();
};
} // namespace opentoon
