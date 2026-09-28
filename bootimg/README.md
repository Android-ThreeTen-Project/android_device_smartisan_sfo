# SFO boot image format

SFO aboot selects the current two-cell `qcom,msm-id` and separate `board-id`
through a **QCDT v2 table**. Its older appended-DTB path instead reads three
MSM-ID cells and cannot select these device trees.

`mkbootimg.py` reuses the platform writer for the kernel, ramdisk, addresses,
command line and OS version. It then writes the legacy DT size at header offset
40, updates the image ID to include `dt.img`, and appends the aligned DT section.
`Android.mk` adds `dt.img` as a prerequisite for boot and recovery images.

`BOARD_CUSTOM_BOOTIMG` keeps the resulting images in `BOOTABLE_IMAGES` in the
target-files package, so OTA tools use the generated legacy images.

Recovery alone uses `--recovery-xz-armthumb` to recompress its CPIO
with the ARM-Thumb BCJ filter and the extreme XZ preset. The kernel enables
`CONFIG_XZ_DEC_ARMTHUMB`; CRC32 and the existing 32 MiB dictionary are preserved.
This keeps the recovery image within its physical 16,384,000-byte
partition. The normal boot ramdisk keeps the platform compression settings.

Compression uses `/usr/bin/xz` from the usual `xz-utils` build dependency, with
ARM-Thumb encoding support. `--recovery-xz-tool` can select another host path.
Android's prebuilt Python lacks `_lzma`, and its PATH XZ omits that encoder.

`--recovery-no-editor` removes nano, its configuration, `libncurses_recovery.so`
and terminfo from the packed Recovery CPIO. Nano is the only Recovery ELF using
that ncurses library; SELinux's `libpcre2.so` is retained. All other records keep
their original contents and metadata. This provides the additional margin that
non-A/B OTA packaging requires (image size strictly below 99% of the partition).
The normal Android system still includes nano.
