# CI handoff (orchestrator pattern)

Use this folder for **committed** templates. For **private** scratch notes, use `/_ci-handoff/` at the repo root (gitignored).

## GitHub Actions prerequisite

Add repository secret **`ORGANIZERRO_PAT`** (Settings → Secrets and variables → Actions) with the **same value** as `C:\src\xbmc\.git\organizerro_pat.txt`. The workflow [`build-windows.yaml`](../../.github/workflows/build-windows.yaml) requires it for `actions/checkout` and `gh release create`.

## Local log script

From the repo root:

```powershell
.\tools\ci\run-ghworkflowfails.ps1 -ScriptPath Z:\ghworkflowfails.js
```

Paste failed-job summaries into `LAST_RUN.md` (or your private `_ci-handoff/LAST_RUN.md`).

## Branch

The workflow triggers on **`Leia`**. If your local branch is `leia`, rename and push:

```powershell
git branch -m leia Leia
git push -u origin Leia
```

Use credentials from `organizerro_pat.txt` (see plan / `run-ghworkflowfails.ps1` pattern).
