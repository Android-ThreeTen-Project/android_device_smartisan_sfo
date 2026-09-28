/*
 * Copyright (C) 2020 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "SfoAudioShim"

#include <android/dlext.h>
#include <dlfcn.h>
#include <stdint.h>
#include <log/log.h>
#include <utils/Errors.h>
#include <utils/String8.h>

namespace {

void* getAudioClientHandle() {
    static void* handle = []() -> void* {
        using GetNamespace = android_namespace_t* (*)(const char*);
        using CreateNamespace = android_namespace_t* (*)(
                const char*, const char*, const char*, uint64_t, const char*,
                android_namespace_t*, const void*);

        auto getNamespace = reinterpret_cast<GetNamespace>(
                dlsym(RTLD_DEFAULT, "__loader_android_get_exported_namespace"));
        auto createNamespace = reinterpret_cast<CreateNamespace>(
                dlsym(RTLD_DEFAULT, "__loader_android_create_namespace"));
        android_namespace_t* ns = getNamespace ? getNamespace("system") : nullptr;
        if (!ns && createNamespace) {
            // Lineage's vendor section keeps the system namespace private.
            // Clone runtime's links: libc is actually loaded in its private
            // system dependency namespace. Linking runtime alone would load
            // another libc and fail its initial-exec TLS relocations.
            auto runtime = getNamespace ? getNamespace("com_android_runtime") : nullptr;
            if (!runtime) {
                ALOGE("The runtime namespace is unavailable");
                return nullptr;
            }
            // Bionic's ANDROID_NAMESPACE_TYPE_SHARED ABI. The runtime parent
            // shares bionic but does not import the vendor libbinder; framework
            // audio can therefore use its own /dev/binder connection.
            constexpr uint64_t kSharedNamespace = 2;
            ns = createNamespace("sfo_audio_system", nullptr,
                    "/system/lib:/system_ext/lib:/system/system_ext/lib",
                    kSharedNamespace, nullptr, runtime,
                    reinterpret_cast<const void*>(&getAudioClientHandle));
        }
        if (!ns) {
            ALOGE("Cannot create the framework audio namespace: %s", dlerror());
            return nullptr;
        }

        android_dlextinfo info = {};
        info.flags = ANDROID_DLEXT_USE_NAMESPACE;
        info.library_namespace = ns;
        void* result = android_dlopen_ext("libaudioclient.so", RTLD_NOW, &info);
        if (!result) {
            ALOGE("Cannot load framework libaudioclient: %s", dlerror());
        } else {
            ALOGI("Loaded framework libaudioclient for the legacy RIL");
        }
        // Keep the library loaded for AudioSystem's registered callback.
        return result;
    }();
    return handle;
}

template <typename Function>
Function audioSymbol(const char* name) {
    void* handle = getAudioClientHandle();
    auto function = handle ? reinterpret_cast<Function>(dlsym(handle, name)) : nullptr;
    if (!function) ALOGE("Framework AudioSystem symbol unavailable: %s", name);
    return function;
}

}  // namespace

extern "C" void _ZN7android11AudioSystem16setErrorCallbackEPFviE(void (*callback)(int)) {
    using AddCallback = uintptr_t (*)(void (*)(int));
    static auto add = audioSymbol<AddCallback>(
            "_ZN7android11AudioSystem16addErrorCallbackEPFviE");
    if (add) add(callback);
}

extern "C" android::status_t _ZN7android11AudioSystem13setParametersEiRKNS_7String8E(
        int io, const android::String8& parameters) {
    using SetParameters = android::status_t (*)(int, const android::String8&);
    static auto set = audioSymbol<SetParameters>(
            "_ZN7android11AudioSystem13setParametersEiRKNS_7String8E");
    return set ? set(io, parameters) : android::NO_INIT;
}

android::String8 legacyGetParameters(int io, const android::String8& keys)
        __asm__("_ZN7android11AudioSystem13getParametersEiRKNS_7String8E");

android::String8 legacyGetParameters(int io, const android::String8& keys) {
    using GetParameters = android::String8 (*)(int, const android::String8&);
    static auto get = audioSymbol<GetParameters>(
            "_ZN7android11AudioSystem13getParametersEiRKNS_7String8E");
    return get ? get(io, keys) : android::String8();
}
