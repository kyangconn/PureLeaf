#!/usr/bin/env bash
# gen_changelog.sh — Generate CHANGELOG.md from conventional commits.
#
# Usage:
#   ./scripts/gen_changelog.sh [--since v0.1.0] [--root .]
#
# Grouping rules (based on conventional commit prefixes):
#   feat:     → Added
#   fix:      → Fixed
#   docs:     → Documentation
#   refactor: → Changed
#   perf:     → Changed
#   test:     → Internal
#   chore:    → Internal
#   style:    → Internal
#   build:    → Internal
#   ci:       → Internal

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SINCE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --since) SINCE="$2"; shift 2 ;;
        --root)  ROOT="$2";  shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

cd "$ROOT"

# Determine previous tag if --since not set.
if [[ -z "$SINCE" ]]; then
    SINCE="$(git describe --tags --abbrev=0 2>/dev/null || echo "")"
    if [[ -z "$SINCE" ]]; then
        # No tags yet — include all commits.
        SINCE="$(git rev-list --max-parents=0 HEAD)"
    fi
fi

# Get current tag or HEAD.
CURRENT="$(git describe --tags --exact-match HEAD 2>/dev/null || echo "Unreleased")"
CURRENT_DATE="$(date -u +%Y-%m-%d)"

group_commits() {
    local prefix="$1"
    local label="$2"
    local commits
    commits="$(git log "${SINCE}..HEAD" --no-merges --format="- %s (%h)" --grep="^${prefix}" 2>/dev/null || true)"
    if [[ -n "$commits" ]]; then
        echo ""
        echo "### ${label}"
        echo "$commits"
    fi
}

exec > CHANGELOG.md

echo "# Changelog"
echo ""
echo "## ${CURRENT} (${CURRENT_DATE})"
echo ""

group_commits "feat"     "Added"
group_commits "fix"      "Fixed"
group_commits "docs"     "Documentation"
group_commits "refactor" "Changed"
group_commits "perf"     "Changed"
group_commits "test"     "Internal"
group_commits "chore"    "Internal"
group_commits "style"    "Internal"
group_commits "build"    "Internal"
group_commits "ci"       "Internal"

# Catch commits that don't match any known prefix.
OTHER="$(git log "${SINCE}..HEAD" --no-merges --format="- %s (%h)" --invert-grep --grep="^(feat|fix|docs|refactor|perf|test|chore|style|build|ci)" 2>/dev/null || true)"
if [[ -n "$OTHER" ]]; then
    echo ""
    echo "### Other"
    echo "$OTHER"
fi
