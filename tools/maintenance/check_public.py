"""Fail if a tracked file points outside this repo: at private tooling, a session, a person's say-so
or a machine path. This repo is the mod's source and must stand alone (a fresh clone builds, tests
and runs its tools without anything beside it). Run by .github/workflows/build.yaml.

  python tools/maintenance/check_public.py          # exit 1 and list the lines that match
"""
import re
import subprocess
import sys

PATTERN = re.compile(
    r"cp2077_tools|LtbfBridge|LtbfDraw|claude_bridge|\bbridge\.py\b|\bgame\.py\b|[Cc]laude|CLAUDE\.local|agent_git"
    r"|\bJack\b(?! Humbert)|(?:^|[^A-Za-z0-9])[A-Z]:[\\/]|\bkevin\b|cp2077_work|cp2077_depot|scratchpad"
    r"|docs/history\.md|docs/research/|docs/archive/|docs/testing\.md|feedback-queue"
)
# paths (prefixes) not checked: the Nexus page texts are written for players, experiments/ is scratch that is not
# built (vehicle names in it), and this script holds the pattern
SKIP = ("docs/nexus/", "experiments/", "tools/maintenance/check_public.py")
# whole lines that may match: .gitignore keeps the local agent config out of git
ALLOW = {(".gitignore", ".claude/")}
TEXT_SUFFIXES = (".cpp", ".hpp", ".h", ".c", ".reds", ".py", ".ps1", ".md", ".txt", ".tsv", ".json", ".yaml", ".yml",
                 ".tweak", ".xl", ".xml", ".cmake", ".toml", ".ini", ".gitignore", ".gitmodules", ".gitattributes",
                 "CMakeLists.txt", ".redscript-ide", ".clang-tidy")


def main():
    files = subprocess.run(["git", "ls-files", "-z"], capture_output=True, check=True).stdout.decode("utf-8").split("\0")
    hits = []
    for path in files:
        if not path or path.startswith(SKIP) or not path.endswith(TEXT_SUFFIXES):
            continue
        # the archive source is CR2W data (game paths, not ours); its raw localization is text players see
        if path.startswith("src/wolvenkit/"):
            continue
        try:
            text = open(path, encoding="utf-8").read()
        except (OSError, UnicodeDecodeError):
            continue
        for n, line in enumerate(text.splitlines(), 1):
            m = PATTERN.search(line)
            if m and (path, line.strip()) not in ALLOW:
                hits.append(f"{path}:{n}: {m.group(0).strip()!r}: {line.strip()[:160]}")
    if hits:
        print("\n".join(hits))
        print(f"\n{len(hits)} line(s) point outside the repo (private tools, sessions, people or machine paths). "
              "Say what the code does instead; keep test and machine notes outside this repo.")
        return 1
    print("check_public: clean")
    return 0


if __name__ == "__main__":
    sys.exit(main())
