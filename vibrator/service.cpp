/*
 * Copyright (C) 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Vibrator.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::vibrator::Vibrator;

// The legacy timed-output backend has no completion callback. Android's AIDL
// wrapper schedules completion itself when ON_CALLBACK is not advertised.
class SfoVibrator final : public Vibrator {
public:
    ndk::ScopedAStatus getCapabilities(int32_t* capabilities) override {
        *capabilities = 0;
        return ndk::ScopedAStatus::ok();
    }
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    auto vibrator = ndk::SharedRefBase::make<SfoVibrator>();
    const std::string instance = std::string(Vibrator::descriptor) + "/default";
    CHECK(AServiceManager_addService(vibrator->asBinder().get(), instance.c_str()) == STATUS_OK);
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
