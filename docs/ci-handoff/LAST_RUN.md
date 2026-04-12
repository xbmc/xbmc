# Last workflow run

Fetched with `tools\ci\run-ghworkflowfails.ps1` (see `README.md`).

## Latest snapshot (local)

- **Workflow:** `build-windows.yaml` on `OrganizerRo/xbmc`
- **Most recent failed run (at time of capture):** run id `24310284108`, branch `Leia`
- **Failure:** checkout step reported `Input required and not supplied: token` when `ORGANIZERRO_PAT` was not set in repo secrets.
- **Mitigation pushed:** checkout and `gh release` now use `ORGANIZERRO_PAT` when non-empty, otherwise `github.token`. Add **`ORGANIZERRO_PAT`** in repo **Settings → Secrets → Actions** (same value as `.git/organizerro_pat.txt`) for full PAT parity with local tooling.

## Failed job logs

Downloaded under `/logs/` (gitignored). Re-run the PowerShell helper after new failures to refresh.
