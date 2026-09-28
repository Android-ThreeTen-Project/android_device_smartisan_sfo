#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -e

DEVICE=sfo
VENDOR=smartisan

# Load extractutils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$MY_DIR" ]]; then MY_DIR="$PWD"; fi

LINEAGE_ROOT="$MY_DIR"/../../..

HELPER="$LINEAGE_ROOT"/tools/extract-utils/extract_utils.sh
if [ ! -f "$HELPER" ]; then
    echo "Unable to find helper script at $HELPER"
    exit 1
fi
. "$HELPER"

if [ $# -eq 0 ]; then
  SRC=adb
else
  if [ $# -eq 1 ]; then
    SRC=$1
  else
    echo "$0: bad number of arguments"
    echo ""
    echo "usage: $0 [PATH_TO_EXPANDED_ROM]"
    echo ""
    echo "If PATH_TO_EXPANDED_ROM is not specified, blobs will be extracted from"
    echo "the device using adb pull."
    exit 1
  fi
fi

function blob_fixup() {
    case "${1}" in
        vendor/lib/libril-qc-qmi-1.so)
            # The vendor shim provides all three legacy AudioSystem imports.
            "${PATCHELF}" --remove-needed libmedia.so "${2}"
            if ! "${PATCHELF}" --print-needed "${2}" | grep -qx libaudioclient_sfo_shim.so; then
                "${PATCHELF}" --add-needed libaudioclient_sfo_shim.so "${2}"
            fi
            ;;
        vendor/bin/mm-pp-daemon|vendor/lib/libarcsoft_panorama_burstcapture.so)
            # Only NDK sensor/looper imports are used from libandroid.
            "${PATCHELF}" --replace-needed libandroid.so libsensorndkbridge.so "${2}"
            ;;
        vendor/lib/libarcsoft_asd.so|vendor/lib/libarcsoft_beauty_shot.so|vendor/lib/libarcsoft_night_shot.so)
            # These blobs do not import any libandroid symbols.
            "${PATCHELF}" --remove-needed libandroid.so "${2}"
            ;;
        vendor/lib/libmmqjpeg_codec.so)
            "${PATCHELF}" --add-needed "libkkcomp.so" "${2}"
            ;;
        vendor/lib/mediadrm/libwvdrmengine.so)
            "${PATCHELF}" --replace-needed \
                "libprotobuf-cpp-lite.so" \
                "libprotobuf-cpp-lite-v27.so" \
                "${2}"
            ;;
    esac
}

# Initialize the helper
setup_vendor "$DEVICE" "$VENDOR" "$LINEAGE_ROOT"

extract "$MY_DIR"/proprietary-files-qc.txt "$SRC"
extract "$MY_DIR"/proprietary-files.txt "$SRC"

"$MY_DIR"/setup-makefiles.sh
