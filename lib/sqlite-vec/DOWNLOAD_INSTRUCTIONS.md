# sqlite-vec Source Files Download Instructions

## Required Files

To complete the sqlite-vec integration, you need to download the following files from the official repository:

### Files Needed

1. **sqlite-vec.h** - Header file with API declarations
2. **sqlite-vec.c** - Implementation file (amalgamation build)

### Download Methods

#### Option 1: GitHub Releases (Recommended)

1. Visit the releases page: https://github.com/asg017/sqlite-vec/releases/latest
2. Download the release assets (v0.1.6 or later)
3. Look for files named:
   - `sqlite-vec.h`
   - `sqlite-vec.c`
   - Or a combined amalgamation file

4. Place downloaded files in this directory: `/home/user/xbmc/lib/sqlite-vec/`

#### Option 2: Build from Source

If the releases don't include pre-built amalgamation files:

```bash
# Clone the repository
git clone https://github.com/asg017/sqlite-vec.git
cd sqlite-vec

# Follow build instructions in the repository to generate amalgamation
# Copy resulting sqlite-vec.h and sqlite-vec.c to /home/user/xbmc/lib/sqlite-vec/
```

#### Option 3: Direct Download (if available)

```bash
cd /home/user/xbmc/lib/sqlite-vec/

# Download header
curl -L -o sqlite-vec.h https://raw.githubusercontent.com/asg017/sqlite-vec/main/dist/sqlite-vec.h

# Download implementation
curl -L -o sqlite-vec.c https://raw.githubusercontent.com/asg017/sqlite-vec/main/dist/sqlite-vec.c
```

**Note**: The exact URLs may vary depending on the repository structure. Check the repository for the correct paths.

### Verification

After downloading, verify the files exist:

```bash
ls -lh /home/user/xbmc/lib/sqlite-vec/
```

You should see:
```
CMakeLists.txt
LICENSE
README.md
sqlite-vec.h      # <-- Required
sqlite-vec.c      # <-- Required
```

### Build Test

Try building to verify the files are correct:

```bash
cd /home/user/xbmc
mkdir -p build && cd build
cmake ..
make sqlite-vec
```

If successful, the static library will be created and linked to Kodi.

## File Information

- **Version**: v0.1.6 or later
- **License**: Apache 2.0 / MIT (dual licensed)
- **Size**:
  - sqlite-vec.h: ~10-20 KB
  - sqlite-vec.c: ~200-500 KB (amalgamation)

## Troubleshooting

### Files Not Found Error

If CMake reports that sqlite-vec.c is not found:
- Verify files are in `/home/user/xbmc/lib/sqlite-vec/`
- Check file permissions (should be readable)
- Ensure files are named exactly `sqlite-vec.h` and `sqlite-vec.c`

### Compilation Errors

If compilation fails:
- Verify you have the correct version (v0.1.6+)
- Check that SQLite3 development headers are installed
- Review any compiler error messages

### Extension Load Failures

If the extension fails to load at runtime:
- Verify SQLITE_CORE=1 is defined during compilation
- Check CLog output for specific error messages
- Ensure SQLite version is compatible (3.20+)

## Resources

- **Repository**: https://github.com/asg017/sqlite-vec
- **Documentation**: Check repository README for latest docs
- **Issues**: Report problems to the sqlite-vec GitHub issues page

## License Compliance

After downloading, ensure Kodi's LICENSES/README.md includes attribution:
```
sqlite-vec v0.1.6
Copyright (c) 2024 Alex Garcia
Licensed under MIT License
Source: https://github.com/asg017/sqlite-vec
```
