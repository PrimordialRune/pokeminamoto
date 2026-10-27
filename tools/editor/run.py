#!/usr/bin/env python3
"""Start the Minamoto data editor: python3 tools/editor/run.py [--port 8766] [--out DIR] [--in-place] [--no-browser]"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from medit import server  # noqa: E402

if __name__ == "__main__":
    sys.exit(server.main())
