#
# Copyright (C) 2026 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

MIN_SYSTEM_PARTITION_SECTORS = 6553600
SYSTEM_BLOCK_DEVICE = "/dev/block/platform/msm_sdcc.1/by-name/system"


def _AddSystemPartitionSizeAssertion(info):
  script = info.script
  script.AppendExtra('ui_print("Checking system partition size...");')
  script.AppendExtra(
      'assert(run_program("/sbin/sh", "-c", '
      '"system_sectors=$(blockdev --getsz {block} 2>/dev/null); '
      '[ \\"$system_sectors\\" -ge {sectors} ]") == 0 || '
      'abort("E3001: System partition is too small. '
      'Please manually expand it to at least 3200 MiB, reboot recovery, '
      'then install LineageOS again."));'.format(
          block=SYSTEM_BLOCK_DEVICE,
          sectors=MIN_SYSTEM_PARTITION_SECTORS))


def FullOTA_Assertions(info):
  _AddSystemPartitionSizeAssertion(info)


def IncrementalOTA_Assertions(info):
  _AddSystemPartitionSizeAssertion(info)
