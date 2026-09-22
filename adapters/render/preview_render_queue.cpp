#include "preview_render_queue.h"
#include "graph_renderer.h"

namespace opentoon {
PreviewRenderQueue::PreviewRenderQueue(RevisionRenderCache& cache)
    : cache_(cache), worker_([this] { run(); }) {}
PreviewRenderQueue::~PreviewRenderQueue() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
        ++generation_;
        pending_.reset();
        cache_.cancelPending();
    }
    wake_.notify_all();
    worker_.join();
}
bool PreviewRenderQueue::request(std::shared_ptr<const Document> document, RenderCacheKey key) {
    if (!document || key.width != document->width || key.height != document->height ||
        key.profile != document->composition || key.frame < 0 || key.frame >= document->duration ||
        key.target != GraphTarget::Display || key.profile != CompositionProfile::LinearSrgb ||
        !cache_.canRetain(key) || cache_.lookup(key))
        return false;
    std::lock_guard lock(mutex_);
    if (stopping_)
        return false;
    if ((pending_ && pending_->ticket.key == key && cache_.current(pending_->ticket)) ||
        (running_ && running_->key == key && cache_.current(*running_)))
        return false;
    ++generation_;
    const auto ticket = cache_.begin(key);
    if (!ticket.serial)
        return false;
    pending_ = Job{std::move(document), ticket};
    wake_.notify_one();
    return true;
}
void PreviewRenderQueue::cancel() {
    std::lock_guard lock(mutex_);
    ++generation_;
    pending_.reset();
    cache_.cancelPending();
}
void PreviewRenderQueue::run() {
    while (true) {
        Job job;
        std::uint64_t generation;
        {
            std::unique_lock lock(mutex_);
            wake_.wait(lock, [this] { return stopping_ || pending_.has_value(); });
            if (stopping_)
                break;
            job = std::move(*pending_);
            pending_.reset();
            running_ = job.ticket;
            generation = generation_;
        }
        auto cancelled = [this, ticket = job.ticket, generation] {
            if (!cache_.current(ticket))
                return true;
            std::lock_guard lock(mutex_);
            return stopping_ || generation != generation_;
        };
        try {
            RenderOptions options;
            options.background = job.ticket.key.background;
            options.onionSkin = job.ticket.key.onionSkin;
            options.onionRange = job.ticket.key.onionRange;
            options.cancelled = cancelled;
            auto pixels = GraphRenderer::render(CompositionGraph::orderedLayers(*job.document),
                                                *job.document, job.ticket.key.frame,
                                                {job.ticket.key.width, job.ticket.key.height},
                                                options, GraphTarget::Display);
            if (!cancelled())
                (void)cache_.publish(job.ticket, std::move(pixels));
        } catch (const RenderCancelled&) {
            // Superseded work is expected during playback and scrubbing.
        } catch (const std::exception&) {
            // Foreground rendering remains authoritative; never publish a failed preview.
        }
        std::lock_guard lock(mutex_);
        if (running_ && running_->serial == job.ticket.serial)
            running_.reset();
    }
}
} // namespace opentoon
