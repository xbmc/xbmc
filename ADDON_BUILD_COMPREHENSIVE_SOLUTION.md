# ADDON_BUILD_COMPREHENSIVE_SOLUTION

## 1. Executive Summary

This guide is the master reference for stabilizing Windows addon builds in this repository.

### Three Critical Issues
1. **Workflow path + step structure errors** in `build-windows.yaml` addon stages.
2. **MSVC environment not guaranteed** before addon build (`nmake`/toolchain discovery failures).
3. **Milkdrop intrinsic conflicts** (`__floor`/`__ceil`) in addon source compiled with newer MSVC.

### Quick Reference Table

| Area | Problem | Fix | Result |
|---|---|---|---|
| Workflow paths | `...\windows\win32\...` paths | Use `tools\buildsteps\windows\...` | Correct scripts execute |
| Addon build steps | Duplicate build phase | Keep single canonical build step | No redundant/conflicting execution |
| MSVC env | Toolchain not loaded in addon step | Load `vcvars32.bat` before addon commands | `nmake` and cl toolchain available |
| Milkdrop C code | Intrinsic conflicts on MSVC | `_MSC_VER` guards + fallback to `floor`/`ceil` | No C2169/C7552 blocking errors |

### Success Criteria Overview
- Addon bootstrap and build steps execute once, in correct order.
- Addon CMake/NMake phase succeeds with MSVC environment loaded.
- Milkdrop addons no longer fail on intrinsic symbol/address usage.
- Addon outputs are collected and included in release artifacts.

---

## 2. Workflow YAML Fixes (`.github/workflows/build-windows.yaml`)

### Required Before/After (Lines 247-265)

#### Before (problematic structure)
```yaml
# lines 247-265 (before)
- name: Bootstrap addons
  shell: cmd
  env:
    vcarch: amd64_x86
  run: call tools\buildsteps\windows\win32\bootstrap-addons.bat

- name: Build addons
  id: build-addons
  shell: cmd
  env:
    vcarch: amd64_x86
  run: call tools\buildsteps\windows\win32\make-addons.bat

- name: Build addons
  shell: cmd
  env:
    vcarch: amd64_x86
  run: |
    cd tools\buildsteps\windows
    call make-addons.bat
```

#### After (corrected structure)
```yaml
# lines 247-265 (after)
- name: Bootstrap addons
  shell: cmd
  run: |
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars32.bat"
    call tools\buildsteps\windows\bootstrap-addons.bat

- name: Build addons
  id: build-addons
  shell: cmd
  run: |
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars32.bat"
    call tools\buildsteps\windows\make-addons.bat
```

> **Note:** `%%i` is correct because this command runs inside a `cmd` batch context in GitHub Actions. Interactive `cmd` usage would use `%i`.

### Change-by-Change Explanation

| Line | Change | Why |
|---|---|---|
| 251 | `...\windows\bootstrap-addons.bat` (remove `win32`) | Matches actual script location |
| 258 | `...\windows\make-addons.bat` (remove `win32`) | Fixes incorrect path resolution |
| 264 | Load `vcvars32.bat` before addon build | Ensures MSVC/NMake tools are in PATH |
| 259-265 | Remove duplicate addon build step | Prevents double execution and inconsistent state |

### `vcvars32.bat` Loading Rationale
- Addon builds use MSVC/NMake generator paths and require full VS environment setup.
- GitHub Actions steps do not reliably carry Visual Studio shell setup into later `cmd` steps, so each addon step should initialize its own MSVC toolchain context.
- Calling `vcvars32.bat` in each addon-related `cmd` step makes toolchain availability deterministic.

### Path Correction Rationale (`win32` → `windows`)
- Addon helper scripts are in `tools/buildsteps/windows/`.
- Referencing `tools/buildsteps/windows/win32/` causes file-not-found failures.

---

## 3. C Code Patch Details

> These sources are downloaded during addon configure/build and patched in build context.
>
> Line references below are exact for the targeted Leia failing snapshot (issue context dated 2026-04-17). If upstream source shifts, locate patch points by matching the shown code patterns.

### Affected Addon Source Files
- `visualization.milkdrop/lib/vis_milkdrop/ns-eel2/nseel-compiler.c`
- `visualization.milkdrop2/lib/vis_milk2/ns-eel2/nseel-compiler.c`

### Patch Location 1 (Lines 480-481)

#### Before
```c
double __floor(double x);
double __ceil(double x);
```

#### After
```c
#if !defined(_MSC_VER)
double __floor(double x);
double __ceil(double x);
#endif
```

### Patch Location 2 (Lines 578-579)

#### Before
```c
/* registration block A */
func->func_ptr = (void*)__floor;

/* registration block B */
func->func_ptr = (void*)__ceil;
```

#### After
```c
/* floor registration location */
#if !defined(_MSC_VER)
func->func_ptr = (void*)__floor;
#else
func->func_ptr = (void*)floor;
#endif

/* ceil registration location */
#if !defined(_MSC_VER)
func->func_ptr = (void*)__ceil;
#else
func->func_ptr = (void*)ceil;
#endif
```

### Why `_MSC_VER` Guards Are Required
- Newer MSVC treats `__floor`/`__ceil` as intrinsic-only names that cannot be redefined or addressed like normal symbols.
- Guarding declarations and assignments avoids redefinition/address-taking errors on MSVC while preserving non-MSVC behavior.

### Standard Library Fallback Rationale
- `floor` and `ceil` are stable, addressable standard functions.
- On MSVC, mapping to standard names preserves functionality without intrinsic conflicts.

---

## 4. Implementation Checklist

### Phase 1: Workflow Updates
- [ ] Edit `.github/workflows/build-windows.yaml` addon section (247-265).
- [ ] Replace `windows\win32\bootstrap-addons.bat` with `windows\bootstrap-addons.bat`.
- [ ] Replace `windows\win32\make-addons.bat` with `windows\make-addons.bat`.
- [ ] Ensure `vcvars32.bat` is loaded before addon build command.
- [ ] Remove duplicate addon build step (old lines 259-265).

### Phase 2: C Code Patches (Per Addon)
- [ ] Patch `visualization.milkdrop/.../nseel-compiler.c` lines 480-481.
- [ ] Patch `visualization.milkdrop/.../nseel-compiler.c` lines 578-579.
- [ ] Patch `visualization.milkdrop2/.../nseel-compiler.c` lines 480-481.
- [ ] Patch `visualization.milkdrop2/.../nseel-compiler.c` lines 578-579.
- [ ] Verify `_MSC_VER` guards + MSVC fallback are present in both files.

### Phase 3: Testing and Validation
- [ ] Trigger workflow and confirm addon bootstrap passes.
- [ ] Confirm addon configure/build can invoke NMake successfully.
- [ ] Confirm no milkdrop intrinsic compile errors appear.
- [ ] Confirm addon outputs are collected into expected artifact locations.
- [ ] Confirm release packaging includes addon binaries.

---

## 5. Success Criteria

### Build Checkpoints

| Checkpoint | Expected Output | Validation Indicator |
|---|---|---|
| Bootstrap | `bootstrap-addons.bat` completes | Step exits 0 |
| NMake environment | CMake build can use NMake | No `nmake not found` errors |
| Compilation | Addons compile (including milkdrop targets) | No C2169/C7552 for `__floor/__ceil` |
| Collection | Built addons copied to output tree | Addon directories/files present |
| Release | Artifacts include addon bundles | Addon ZIPs/binaries in release assets |

### Performance Targets
- Bootstrap phase: **~3-8 min**
- Addon configure/build: **~15-30 min**
- Full workflow completion: **~45-75 min**

---

## 6. Troubleshooting Guide

### Common Issues and Solutions

| Symptom | Likely Cause | Resolution |
|---|---|---|
| `The system cannot find the path specified` | `win32` path used for scripts | Switch to `tools\buildsteps\windows\...` |
| `Running 'nmake' '-?' failed` | MSVC env not loaded for current step | Call `vcvars32.bat` inside that step |
| `C2169` / `C7552` on `__floor` / `__ceil` | Missing or incomplete guards in nseel patch | Re-apply guarded declarations + fallback assignments |
| Addons absent from artifacts | Build failed earlier or collection path mismatch | Inspect build logs, verify output and collection paths |

### Diagnostic Procedure
1. Inspect workflow logs for bootstrap/build step order.
2. Confirm `vcvars32.bat` call executes in addon step logs.
3. Inspect patched `nseel-compiler.c` content in addon build workspace.
4. Verify build logs for each addon target.

### Recovery for Failed Patches
1. Clean addon build directory.
2. Re-run configure/download stage.
3. Re-apply patch logic to both milkdrop sources.
4. Re-run addon build only; then full workflow.

---

## 7. Rollback Plan

### Step-by-Step Rollback
1. Identify commit(s) containing workflow and patch changes.
2. Revert commits in reverse dependency order.
3. Push rollback branch update.
4. Trigger workflow to confirm prior baseline behavior.

### Git Commands (Example)
```bash
# inspect recent history
git --no-pager log --oneline -n 20

# revert specific commit(s)
git revert <workflow_fix_commit_sha>
git revert <patch_commit_sha>

# or hard reset local branch to known good state (local only)
git reset --hard <known_good_sha>
```

### Fallback Testing Strategy
- Run workflow after each revert to isolate the problematic change.
- Validate bootstrap, addon build invocation, and packaging independently.
- If needed, keep only path and env fixes first, then reintroduce C patches.

---

## 8. Summary Table

| File | Line Range(s) | Change Type | Impact |
|---|---|---|---|
| `.github/workflows/build-windows.yaml` | 247-265 (primary), addon build section | Path/env/step structure fix | Restores deterministic addon build execution |
| `visualization.milkdrop/lib/vis_milkdrop/ns-eel2/nseel-compiler.c` | 480-481, 578-579 | `_MSC_VER` guards + fallback | Removes MSVC intrinsic conflicts |
| `visualization.milkdrop2/lib/vis_milk2/ns-eel2/nseel-compiler.c` | 480-481, 578-579 | `_MSC_VER` guards + fallback | Removes MSVC intrinsic conflicts |

### Overall Impact Assessment
- **Risk:** Low-to-medium (targeted workflow + localized C compatibility guards)
- **Benefit:** High (unblocks addon builds and release packaging reliability)
- **Maintainability:** Improved via explicit workflow structure and compiler-conditional handling
