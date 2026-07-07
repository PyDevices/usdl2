# Publishing and releases

How changes in this repo become a versioned **`usdl2`** package on [TestPyPI](https://test.pypi.org/project/usdl2/), and how to install it.

The CPython package is pure Python (`python/usdl2/` ctypes FFI). GitHub Actions builds a universal wheel (`py3-none-any`) plus sdist and uploads with twine.

## Pipeline overview

```text
usdl2 (your machine)
  commit → push main
           │
           ▼
  ./scripts/publish_release_tag.sh --push   (or manual git tag vX.Y.Z)
           │
           ▼
usdl2: Publish TestPyPI                    (on tag push v*.*.* or workflow_dispatch)
  python -m build → twine upload
```

## Version numbers

Format: **`X.Y.Z`** (semver)

| Part | Source |
|------|--------|
| **First release** | `setup.py` version when no `v*.*.*` tags exist yet (**`0.0.1`**) |
| **Later releases** | highest existing tag + 1 patch (`v0.0.1` → `v0.0.2`, …) |

Preview the next version:

```bash
./scripts/next_release_version.sh --verbose
```

TestPyPI rejects re-uploading the same version — each release needs a new tag (handled automatically by `publish_release_tag.sh`).

## One-time setup

### TestPyPI token (organization secret)

`TESTPYPI_API_TOKEN` can live at the **PyDevices organization** level instead of per repository:

1. **PyDevices** org → **Settings** → **Secrets and variables** → **Actions**
2. Organization secret name: `TESTPYPI_API_TOKEN`
3. Under **Repository access**, grant access to **`usdl2`** (and any other publish repos such as `pydisplay`, `lv_cpython_mod`)

Workflows reference the secret by name — no YAML changes required:

```yaml
TWINE_PASSWORD: ${{ secrets.TESTPYPI_API_TOKEN }}
```

Notes:

- Org secrets are not visible to every repo unless you grant access (all repos or selected repos).
- If a repository also defines `TESTPYPI_API_TOKEN` at the **repository** level, the **repo secret overrides** the org secret.
- Use the same TestPyPI account token across PyDevices publish workflows.

### Optional repository secret

| Secret | Required | Purpose |
|--------|----------|---------|
| `RELEASE_WORKFLOW_TOKEN` | only for future CI that tags from `GITHUB_TOKEN` | PAT with **`actions:write`** on this repo — tag pushes from `GITHUB_TOKEN` do not trigger other workflows; this PAT can dispatch **Publish TestPyPI** after tagging |

For manual or local tag pushes (normal `./scripts/publish_release_tag.sh --push` from your machine), `TESTPYPI_API_TOKEN` alone is enough.

## Release (local clone)

```bash
# 1. Commit and push your changes
git push origin main

# 2. Tag and publish
./scripts/publish_release_tag.sh --push
```

Preview without tagging:

```bash
./scripts/next_release_version.sh --verbose
./scripts/publish_release_tag.sh --dry-run
```

Manual tag (equivalent):

```bash
git tag -a v0.0.1 -m "Release 0.0.1"
git push origin v0.0.1
```

## Manual release (GitHub CLI, no clone)

```bash
# Tag must already exist on the remote, or pass version explicitly:
gh workflow run publish-testpypi.yml --repo PyDevices/usdl2 -f version=0.0.1

gh run list --repo PyDevices/usdl2
gh run watch --repo PyDevices/usdl2
```

Or use **Actions → Publish TestPyPI → Run workflow** in the GitHub UI.

## GitHub Actions workflows

| Workflow | Trigger | What it does |
|----------|---------|--------------|
| [publish-testpypi.yml](.github/workflows/publish-testpypi.yml) | Tag push `v*.*.*`; workflow_dispatch | Build sdist + `py3-none-any` wheel → TestPyPI upload |

## Scripts

| Script | Purpose |
|--------|---------|
| `scripts/next_release_version.sh` | Print next semver (`setup.py` or highest tag + 1 patch) |
| `scripts/publish_release_tag.sh` | Create annotated tag `vX.Y.Z` and optionally push (triggers publish) |

## Local build (dev)

Reproduce the CI packaging path without uploading:

```bash
echo "0.0.0.dev" > VERSION
python3 -m venv .venv
.venv/bin/pip install build
.venv/bin/python -m build
ls dist/
```

Install locally for smoke tests:

```bash
pip install -e .
xvfb-run -a python3 test_usdl2.py   # headless Linux
```

## Install from TestPyPI

```bash
pip install -i https://test.pypi.org/simple/ usdl2
```

Or add TestPyPI as an extra index (e.g. in `buildozer.spec` / p4a) when pulling pre-release wheels alongside PyPI packages.

The package is platform-independent Python; one wheel works on Linux, macOS, Windows, and Android (CPython + SDL2 at runtime).

## Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| Publish fails: 403 on TestPyPI | Bad or missing `TESTPYPI_API_TOKEN`; confirm org secret grants access to this repo |
| Publish fails: 400 duplicate version | Tag already uploaded; run `./scripts/publish_release_tag.sh --push` for the next patch |
| Tag pushed but no workflow run | Check Actions tab; ensure tag matches `v*.*.*` |
| `publish_release_tag.sh`: uncommitted changes | Commit or stash before tagging |
| `publish_release_tag.sh`: tag already exists | Version already released; preview next with `./scripts/next_release_version.sh --verbose` |
