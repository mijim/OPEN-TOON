#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#include <miniaudio.h>
#include "audio_device.h"
#include "opentoon/audio.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <stdexcept>

namespace opentoon {
struct AudioDevice::Impl {
    std::shared_ptr<const Document> snapshot;
    AudioMixPlan mix;
    ma_context context{};
    ma_device device{};
    bool contextReady = false, deviceReady = false;
    std::atomic<std::int64_t> cursor{0};
    std::atomic<std::int64_t> scrubEnd{-1};
    std::atomic_bool interrupted{false};
    std::atomic_bool active{false};
    std::atomic_bool looping{true};
    std::atomic<std::uint64_t> callbacks{0}, processingOverruns{0}, maximumCallbackNanoseconds{0};

    explicit Impl(std::shared_ptr<const Document> source)
        : snapshot(std::move(source)), mix(*snapshot, 48000) {}

    static void data(ma_device* device, void* destination, const void*, ma_uint32 frameCount) noexcept {
        const auto started = std::chrono::steady_clock::now();
        auto* self = static_cast<Impl*>(device->pUserData);
        auto* output = static_cast<std::int16_t*>(destination);
        std::array<double, 4096 * 2> scratch;
        const auto sceneEnd = self->mix.sceneSamples();
        const auto scrubEnd = self->scrubEnd.load(std::memory_order_acquire);
        const auto boundary = scrubEnd >= 0 ? std::min(sceneEnd, scrubEnd) : sceneEnd;
        const auto origin = self->cursor.load(std::memory_order_relaxed);
        std::int64_t cursor = origin;
        ma_uint32 done = 0;
        while (done < frameCount) {
            if (scrubEnd < 0 && cursor >= sceneEnd) {
                if (!self->looping.load(std::memory_order_relaxed))
                    break;
                cursor = 0;
            }
            if (cursor >= boundary)
                break;
            const auto count = std::size_t(std::min<std::int64_t>(
                {std::int64_t(frameCount - done), 4096, boundary - cursor}));
            if (count == 0)
                break;
            try {
                self->mix.renderInto(cursor, {output + done * 2, count * 2},
                                     {scratch.data(), count * 2});
            } catch (...) {
                std::fill_n(output + done * 2, count * 2, std::int16_t(0));
                self->interrupted.store(true, std::memory_order_relaxed);
            }
            done += ma_uint32(count);
            cursor += std::int64_t(count);
        }
        if (done < frameCount)
            std::fill_n(output + done * 2, (frameCount - done) * 2, std::int16_t(0));
        auto previous = origin;
        self->cursor.compare_exchange_strong(previous, cursor, std::memory_order_release,
                                             std::memory_order_relaxed);
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - started).count();
        self->callbacks.fetch_add(1, std::memory_order_relaxed);
        if (elapsed > std::int64_t(frameCount) * 1000000000 / 48000)
            self->processingOverruns.fetch_add(1, std::memory_order_relaxed);
        auto maximum = self->maximumCallbackNanoseconds.load(std::memory_order_relaxed);
        while (elapsed > std::int64_t(maximum) &&
               !self->maximumCallbackNanoseconds.compare_exchange_weak(
                   maximum, std::uint64_t(elapsed), std::memory_order_relaxed)) {}
    }
    static void notification(const ma_device_notification* event) noexcept {
        auto* self = static_cast<Impl*>(event->pDevice->pUserData);
        if (event->type == ma_device_notification_type_interruption_began ||
            event->type == ma_device_notification_type_rerouted) {
            self->interrupted.store(true, std::memory_order_relaxed);
        }
    }
};
AudioDevice::AudioDevice(std::shared_ptr<const Document> snapshot, bool nullBackend) {
    if (!snapshot)
        throw std::invalid_argument("Audio playback needs a document snapshot.");
    impl_ = std::make_unique<Impl>(std::move(snapshot));
    ma_backend backend = ma_backend_null;
    const auto contextResult = ma_context_init(nullBackend ? &backend : nullptr,
                                               nullBackend ? 1 : 0, nullptr,
                                               &impl_->context);
    if (contextResult != MA_SUCCESS)
        throw std::runtime_error("Could not initialize the audio backend.");
    impl_->contextReady = true;
    auto config = ma_device_config_init(ma_device_type_playback);
    config.sampleRate = 48000;
    config.playback.format = ma_format_s16;
    config.playback.channels = 2;
    config.dataCallback = Impl::data;
    config.notificationCallback = Impl::notification;
    config.pUserData = impl_.get();
    if (ma_device_init(&impl_->context, &config, &impl_->device) != MA_SUCCESS) {
        ma_context_uninit(&impl_->context);
        impl_->contextReady = false;
        throw std::runtime_error("Could not open an audio output device.");
    }
    impl_->deviceReady = true;
}
AudioDevice::~AudioDevice() {
    if (impl_ && impl_->deviceReady)
        ma_device_uninit(&impl_->device);
    if (impl_ && impl_->contextReady)
        ma_context_uninit(&impl_->context);
}
void AudioDevice::start(Frame frame) {
    impl_->scrubEnd.store(-1, std::memory_order_release);
    seek(frame);
    impl_->interrupted.store(false, std::memory_order_relaxed);
    if (ma_device_start(&impl_->device) != MA_SUCCESS)
        throw std::runtime_error("Could not start the audio output device.");
    impl_->active.store(true, std::memory_order_release);
}
void AudioDevice::scrub(Frame frame) {
    if (frame < 0 || frame >= impl_->snapshot->duration)
        throw std::invalid_argument("Audio scrub frame is outside the scene.");
    const auto first = impl_->snapshot->rate.sampleAt(frame, 48000);
    impl_->scrubEnd.store(std::min(impl_->mix.sceneSamples(), first + 3840),
                          std::memory_order_release);
    impl_->cursor.store(first, std::memory_order_release);
    impl_->interrupted.store(false, std::memory_order_relaxed);
    if (!impl_->active.load(std::memory_order_acquire)) {
        if (ma_device_start(&impl_->device) != MA_SUCCESS)
            throw std::runtime_error("Could not start audio scrubbing.");
        impl_->active.store(true, std::memory_order_release);
    }
}
void AudioDevice::stop() {
    if (impl_->active.exchange(false, std::memory_order_acq_rel))
        ma_device_stop(&impl_->device);
}
void AudioDevice::seek(Frame frame) {
    if (frame < 0 || frame >= impl_->snapshot->duration)
        throw std::invalid_argument("Audio seek frame is outside the scene.");
    impl_->cursor.store(impl_->snapshot->rate.sampleAt(frame, 48000),
                        std::memory_order_release);
}
void AudioDevice::setLooping(bool looping) {
    impl_->looping.store(looping, std::memory_order_release);
}
bool AudioDevice::finished() const {
    return !impl_->looping.load(std::memory_order_acquire) &&
           impl_->scrubEnd.load(std::memory_order_acquire) < 0 &&
           impl_->cursor.load(std::memory_order_acquire) >= impl_->mix.sceneSamples();
}
std::int64_t AudioDevice::currentSample() const {
    const auto value = impl_->cursor.load(std::memory_order_acquire);
    return value == impl_->mix.sceneSamples() &&
                   impl_->looping.load(std::memory_order_acquire) ? 0 : value;
}
Frame AudioDevice::currentFrame() const {
    const auto sample = currentSample();
    Frame low = 0, high = impl_->snapshot->duration;
    while (low + 1 < high) {
        const Frame mid = low + (high - low) / 2;
        if (impl_->snapshot->rate.sampleAt(mid, 48000) <= sample)
            low = mid;
        else
            high = mid;
    }
    return low;
}
bool AudioDevice::running() const {
    return impl_->active.load(std::memory_order_acquire) &&
           ma_device_is_started(&impl_->device);
}
bool AudioDevice::interrupted() const {
    return impl_->interrupted.load(std::memory_order_acquire);
}
AudioDeviceStats AudioDevice::stats() const {
    return {impl_->callbacks.load(std::memory_order_relaxed),
            impl_->processingOverruns.load(std::memory_order_relaxed),
            impl_->maximumCallbackNanoseconds.load(std::memory_order_relaxed)};
}
} // namespace opentoon
