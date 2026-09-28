// SPDX-License-Identifier: Apache-2.0
#define LOG_TAG "SfoPower"

#include <aidl/android/hardware/power/BnPower.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <perfmgr/HintManager.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <mutex>

using namespace aidl::android::hardware::power;
using android::perfmgr::HintManager;
using ndk::ScopedAStatus;

namespace {

ScopedAStatus unsupported() {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

// The original perfd client has no perf_hint API or matching daemon. Use the
// kernel's cpu-boost policy request; libperfmgr expires and combines requests.
class Power final : public BnPower {
  public:
    explicit Power(std::unique_ptr<HintManager> hints) : mHints(std::move(hints)) {}

    ScopedAStatus setMode(Mode type, bool enabled) override {
        std::lock_guard lock(mMutex);
        if (type == Mode::INTERACTIVE) {
            mInteractive = enabled;
            if (!enabled) {
                mHints->EndHint("INTERACTION");
                mHints->EndHint("LAUNCH");
            }
        } else if (type == Mode::LAUNCH) {
            if (enabled && mInteractive) {
                mHints->DoHint("LAUNCH");
            } else {
                mHints->EndHint("LAUNCH");
            }
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus isModeSupported(Mode type, bool* supported) override {
        *supported = type == Mode::INTERACTIVE || type == Mode::LAUNCH;
        return ScopedAStatus::ok();
    }

    ScopedAStatus setBoost(Boost type, int32_t durationMs) override {
        if (type != Boost::INTERACTION) return ScopedAStatus::ok();
        std::lock_guard lock(mMutex);
        if (durationMs < 0 || !mInteractive) {
            mHints->EndHint("INTERACTION");
        } else {
            // Zero requests the default boost. Bound caller-provided durations
            // so a legacy device never stays boosted without further activity.
            const int32_t timeout = durationMs == 0 ? 250 : std::clamp(durationMs, 1, 5000);
            mHints->DoHint("INTERACTION", std::chrono::milliseconds(timeout));
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus isBoostSupported(Boost type, bool* supported) override {
        *supported = type == Boost::INTERACTION;
        return ScopedAStatus::ok();
    }

    ScopedAStatus createHintSession(int32_t, int32_t, const std::vector<int32_t>&, int64_t,
                                   std::shared_ptr<IPowerHintSession>*) override {
        return unsupported();
    }

    ScopedAStatus createHintSessionWithConfig(int32_t, int32_t, const std::vector<int32_t>&,
                                            int64_t, SessionTag, SessionConfig*,
                                            std::shared_ptr<IPowerHintSession>*) override {
        return unsupported();
    }

    ScopedAStatus getHintSessionPreferredRate(int64_t*) override { return unsupported(); }
    ScopedAStatus getSessionChannel(int32_t, int32_t, ChannelConfig*) override {
        return unsupported();
    }
    ScopedAStatus closeSessionChannel(int32_t, int32_t) override { return unsupported(); }

    ScopedAStatus getSupportInfo(SupportInfo* info) override {
        *info = SupportInfo{};
        info->modes = (int64_t{1} << static_cast<int>(Mode::INTERACTIVE)) |
                      (int64_t{1} << static_cast<int>(Mode::LAUNCH));
        info->boosts = int64_t{1} << static_cast<int>(Boost::INTERACTION);
        info->compositionData.maxBatchSize = 1;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getCpuHeadroom(const CpuHeadroomParams&, CpuHeadroomResult*) override {
        return unsupported();
    }
    ScopedAStatus getGpuHeadroom(const GpuHeadroomParams&, GpuHeadroomResult*) override {
        return unsupported();
    }
    ScopedAStatus sendCompositionData(const std::vector<CompositionData>&) override {
        return unsupported();
    }
    ScopedAStatus sendCompositionUpdate(const CompositionUpdate&) override {
        return unsupported();
    }

    binder_status_t dump(int fd, const char**, uint32_t) override {
        mHints->DumpToFd(fd);
        return STATUS_OK;
    }

  private:
    std::unique_ptr<HintManager> mHints;
    std::mutex mMutex;
    bool mInteractive = true;
};

}  // namespace

int main() {
    std::unique_ptr<HintManager> hints(
            HintManager::GetFromJSON("/vendor/etc/powerhint-sfo.json", false));
    CHECK(hints != nullptr) << "Failed to load SFO power hints";
    CHECK(hints->IsHintSupported("INTERACTION"));
    CHECK(hints->IsHintSupported("LAUNCH"));
    CHECK(hints->Start()) << "Failed to start SFO power hints";
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    auto power = ndk::SharedRefBase::make<Power>(std::move(hints));
    const std::string instance = std::string(Power::descriptor) + "/default";
    CHECK_EQ(AServiceManager_addService(power->asBinder().get(), instance.c_str()), STATUS_OK);
    LOG(INFO) << "SFO CPU interaction and launch boosts ready";
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
