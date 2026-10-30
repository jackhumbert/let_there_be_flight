"""Puts the tool folders whose modules other folders import on sys.path: the bake's sector, collision
and mesh readers (navmap_bake), the pad entity builder (pads), the city collision tools (world).
Import it after common when a script uses a module from another folder."""

import os
import sys

_TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
for _folder in ("navmap_bake", "pads", "world", "kitbash"):
    _path = os.path.join(_TOOLS, _folder)
    if _path not in sys.path:
        sys.path.append(_path)
