"""A machine path that is looked up when a tool uses it, not when its module is imported, so `--help`
and tools that never read it work without tools/common/paths.local.json.

    DEFAULT_WOLVENKIT = Lazy(lambda: paths.WOLVENKIT)

It stands in for the path (os.fspath, str, subprocess arguments) or for a list of paths (iteration,
len, indexing, `*x`)."""

import os


class Lazy(os.PathLike):
    def __init__(self, resolve):
        self._resolve = resolve

    def __fspath__(self):
        return os.fspath(self._resolve())

    def __str__(self):
        return str(self._resolve())

    def __repr__(self):
        return f"Lazy({self._resolve!r})"

    def __iter__(self):
        return iter(self._resolve())

    def __len__(self):
        return len(self._resolve())

    def __getitem__(self, i):
        return self._resolve()[i]
