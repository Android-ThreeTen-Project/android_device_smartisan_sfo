#!/usr/bin/env python3
"""Add explicit receiver flags to a generated copy of the legacy ANT service."""
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
for receiver, intent_filter in [('mReceiver', 'filter'),
                                ('mStateChangedReceiver', 'stateChangedFilter')]:
    old = f'registerReceiver({receiver}, {intent_filter});'
    new = f'registerReceiver({receiver}, {intent_filter}, Context.RECEIVER_EXPORTED);'
    if source.count(old) != 1:
        raise SystemExit(f'ANT receiver source changed; expected one occurrence of {old}')
    source = source.replace(old, new)
Path(sys.argv[2]).write_text(source)
