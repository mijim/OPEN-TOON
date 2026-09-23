#pragma once
#include "revision_render_cache.h"
#include "scene_renderer.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace opentoon {
// One bounded producer for speculative preview frames. The owner keeps the cache alive.
class PreviewRenderQueue {
  public:
    explicit PreviewRenderQueue(RevisionRenderCache&);
    ~PreviewRenderQueue();
    PreviewRenderQueue(const PreviewRenderQueue&) = delete;
    PreviewRenderQueue& operator=(const PreviewRenderQueue&) = delete;
    [[nodiscard]] bool request(std::shared_ptr<const Document>, RenderCacheKey);
    void cancel();

  private:
    struct Job {
        std::shared_ptr<const Document> document;
        RenderTicket ticket;
    };
    void run();
    RevisionRenderCache& cache_;
    std::mutex mutex_;
    std::condition_variable_any wake_;
    std::optional<Job> pending_;
    std::optional<RenderTicket> running_;
    std::uint64_t generation_ = 0;
    bool stopping_ = false;
    std::thread worker_;
};
} // namespace opentoon
