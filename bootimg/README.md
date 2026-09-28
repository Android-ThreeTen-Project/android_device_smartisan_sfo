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
