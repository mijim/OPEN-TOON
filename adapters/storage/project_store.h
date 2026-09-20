#pragma once
#include "opentoon/document.h"
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
namespace opentoon {
struct StoredRevision {
    std::int64_t id;
    std::string created;
    std::string label;
};
struct LoadedProject {
    Document document;
    std::int64_t revision;
};
class ProjectStore {
  public:
    enum class SavePoint { BeforeTransaction, AfterInsert, BeforeCommit, AfterCommit };
    using FailureHook = std::function<void(SavePoint)>;
    [[nodiscard]] static std::int64_t save(const std::filesystem::path&, const Document&,
                                           const std::string& label = "Manual save",
                                           std::int64_t expectedRevision = -1, FailureHook hook = {});
    [[nodiscard]] static LoadedProject load(const std::filesystem::path&, std::int64_t revision = 0);
    struct CompactionResult {
        std::uintmax_t bytesBefore, bytesAfter;
        std::size_t retainedRevisions;
        std::filesystem::path backup;
    };
    [[nodiscard]] static CompactionResult compact(const std::filesystem::path&, int retainRevisions,
                                                  std::int64_t expectedRevision);
    [[nodiscard]] static std::vector<StoredRevision> revisions(const std::filesystem::path&);
};
} // namespace opentoon
