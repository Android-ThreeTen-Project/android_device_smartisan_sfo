#!/usr/bin/env python3
# Copyright (C) 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
"""Pack SFO's legacy Qualcomm DT section using the standard Android payloads."""

import argparse
import hashlib
import os
import struct
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--dt", type=Path)
    legacy, remaining = parser.parse_known_args()
    root = Path(__file__).resolve().parents[4]
    sys.path.insert(0, str(root / "system/tools/mkbootimg"))
    import mkbootimg

    sys.argv[1:] = remaining
    args = mkbootimg.parse_cmdline()
    if args.header_version != 0 or args.vendor_boot is not None:
        parser.error("SFO requires a legacy boot image without vendor_boot")

    dt_path = legacy.dt or Path(args.kernel.name).parent / "dt.img"
    if not dt_path.is_file() and legacy.dt is None:
        # Releasetools can rebuild the recovery two-step image from target-files.
        dt_path = Path(os.environ.get("OUT_DIR", "out")) / "target/product/sfo/dt.img"
        if not dt_path.is_absolute():
            dt_path = root / dt_path
    dt = dt_path.read_bytes()
    if len(dt) < 16 or dt[:4] != b"QCDT":
        parser.error("missing Qualcomm device tree table")
    version, entries = struct.unpack_from("<2I", dt, 4)
    if version != 2 or not entries or 12 + entries * 24 + 4 > len(dt):
        parser.error("SFO requires a valid QCDT v2 table")
    for index in range(entries):
        _, _, _, _, offset, size = struct.unpack_from("<6I", dt, 12 + index * 24)
        if offset % args.pagesize or not size or offset + size > len(dt):
            parser.error("invalid QCDT entry bounds/alignment")
        if dt[offset:offset + 4] != b"\xd0\x0d\xfe\xed":
            parser.error("QCDT entry does not contain an FDT")

    # Header v0's later 'header_version' word occupies the original dt_size slot.
    # Keep all standard addresses, command line, OS version and payload encoding,
    # then populate the legacy dt_size and the four-payload image ID.
    mkbootimg.write_header(args)
    mkbootimg.write_data(args, args.pagesize)
    sha = hashlib.sha1()
    for payload in (args.kernel, args.ramdisk, args.second):
        if payload is not None:
            payload.seek(0)
        mkbootimg.update_sha(sha, payload)
    sha.update(dt)
    sha.update(struct.pack("<I", len(dt)))
    args.output.seek(40)
    args.output.write(struct.pack("<I", len(dt)))
    args.output.seek(576)
    args.output.write(sha.digest().ljust(32, b"\0"))
    args.output.seek(0, os.SEEK_END)
    args.output.write(dt)
    mkbootimg.pad_file(args.output, args.pagesize)
    args.output.close()


if __name__ == "__main__":
    main()
