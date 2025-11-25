# sqlite-vec - Vector Search Extension for SQLite

This directory contains the sqlite-vec extension for vector similarity search.

## Source Files

The sqlite-vec source files should be obtained from the official repository:
- Repository: https://github.com/asg017/sqlite-vec
- License: Apache 2.0 / MIT (dual licensed)
- Latest Release: v0.1.6

### Required Files

Download the following files from the GitHub repository and place them in this directory:

1. `sqlite-vec.h` - Header file with API declarations
2. `sqlite-vec.c` - Implementation (amalgamation build)

These files can be obtained from:
- GitHub Releases: https://github.com/asg017/sqlite-vec/releases/latest
- Or build from source following the repository instructions

### Integration

The extension provides:
- `vec0` virtual table for storing and querying vectors
- Support for float32 vectors (384 dimensions for our use case)
- Distance metrics: cosine, L2, inner product
- Efficient nearest neighbor search

### API Functions

Key functions used in integration:
- `sqlite3_vec_init()` - Initialize the extension on a database connection
- Virtual table creation via SQL: `CREATE VIRTUAL TABLE ... USING vec0()`

### License Compliance

sqlite-vec is dual-licensed under Apache 2.0 and MIT licenses.
Ensure proper attribution is included in LICENSES/ directory.
