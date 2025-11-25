# Semantic Search - Pull Request Checklist

This checklist ensures that all pull requests for the Semantic Search feature meet quality standards before merging.

---

## Pre-Submission Checklist

Before creating a pull request, ensure you have completed all applicable items.

### Code Quality

- [ ] **Code follows Kodi coding standards**
  - Uses C prefix for classes (`CSemanticDatabase`)
  - Uses I prefix for interfaces (`IContentParser`)
  - Uses m_ prefix for member variables
  - 2-space indentation
  - 100-character line limit
  - No trailing whitespace

- [ ] **Code is formatted with clang-format**
  ```bash
  clang-format -i xbmc/semantic/**/*.cpp xbmc/semantic/**/*.h
  ```

- [ ] **No compiler warnings**
  ```bash
  cmake .. -DCMAKE_BUILD_TYPE=Debug
  make 2>&1 | grep -i "semantic.*warning"
  # Should return nothing
  ```

- [ ] **Code passes static analysis**
  ```bash
  cppcheck --enable=all --suppress=missingIncludeSystem xbmc/semantic/
  ```

### Documentation

- [ ] **Public APIs have Doxygen comments**
  - `\brief` description
  - `\param` for all parameters
  - `\return` for return values
  - Usage examples where applicable

- [ ] **Complex algorithms explained**
  - Inline comments for non-obvious logic
  - Algorithm references in comments where applicable

- [ ] **README files updated** (if applicable)
  - Component-specific READMEs
  - Examples updated
  - Known limitations documented

- [ ] **User-facing documentation updated**
  - UserGuide.md (if user-visible changes)
  - DeveloperGuide.md (if API changes)
  - Phase documentation (if architectural changes)

### Testing

- [ ] **Unit tests written for new code**
  - All new public methods have tests
  - Edge cases covered
  - Error conditions tested

- [ ] **Unit tests pass locally**
  ```bash
  cd build
  make test
  # Or run specific tests:
  ./kodi-test --gtest_filter=Semantic*
  ```

- [ ] **Test coverage meets minimum threshold**
  - New code: >80% coverage
  - Critical paths: 100% coverage
  ```bash
  cmake .. -DENABLE_COVERAGE=ON
  make
  make test
  lcov --capture --directory . --output-file coverage.info
  # Check coverage report
  ```

- [ ] **Integration tests pass** (if applicable)
  - End-to-end workflows tested
  - Database migrations tested
  - Settings integration tested

- [ ] **Manual testing completed**
  - Tested on development environment
  - Tested with real media files
  - Tested edge cases manually

### Performance

- [ ] **No performance regressions**
  - Benchmark critical paths
  - Compare before/after if modifying existing code
  - Document performance characteristics for new features

- [ ] **Resource usage is reasonable**
  - Memory usage profiled (valgrind or similar)
  - No memory leaks detected
  - CPU usage is acceptable for background operations

- [ ] **Database operations optimized**
  - Batch operations use transactions
  - Queries use appropriate indexes
  - Large operations are cancellable

### Compatibility

- [ ] **Backward compatible with existing databases**
  - Schema migrations implemented (if schema changes)
  - Old data can be read/upgraded
  - Migration tested with sample databases

- [ ] **Settings changes are backward compatible**
  - Old settings still work
  - New defaults are sensible
  - Settings migration implemented if needed

- [ ] **No breaking API changes** (or documented as breaking)
  - Existing callers continue to work
  - Deprecation warnings added for old APIs
  - Migration guide provided for breaking changes

### Dependencies

- [ ] **No new external dependencies** (or approved)
  - If adding dependency, justify in PR description
  - Verify license compatibility (GPL-2.0-or-later)
  - Check cross-platform availability

- [ ] **FFmpeg/SQLite version requirements documented**
  - Minimum versions specified
  - Tested with minimum versions

---

## Code Review Checklist

Reviewers should verify the following before approving.

### Functional Review

- [ ] **Feature works as described**
  - PR description accurately describes changes
  - Acceptance criteria met
  - Edge cases handled

- [ ] **No regressions introduced**
  - Existing tests still pass
  - Existing functionality not broken
  - Manual testing confirms no issues

- [ ] **Error handling is robust**
  - Errors logged appropriately
  - User-facing errors are clear
  - No silent failures

### Code Review

- [ ] **Code is readable and maintainable**
  - Variable names are descriptive
  - Functions are appropriately sized
  - Complex logic is well-commented

- [ ] **No code duplication**
  - Common logic extracted to helper functions
  - Reuses existing utilities where possible

- [ ] **Thread safety considered**
  - Shared data protected by mutexes
  - Atomic operations used where appropriate
  - No data races (verified with ThreadSanitizer if possible)

- [ ] **Resource management is correct**
  - RAII used for resource lifetime
  - No resource leaks (file handles, memory, etc.)
  - Database connections properly managed

### Security Review

- [ ] **No SQL injection vulnerabilities**
  - All queries use parameterized statements
  - User input is sanitized
  - FTS5 queries properly escaped

- [ ] **API keys not hardcoded**
  - Credentials loaded from settings
  - No secrets in code or logs

- [ ] **Input validation present**
  - File paths validated
  - JSON parsing handles malformed input
  - Numeric inputs range-checked

### Database Review

- [ ] **Schema changes include migration**
  - `UpdateTables()` implements migration
  - Schema version incremented
  - Migration tested with old databases

- [ ] **Indexes are appropriate**
  - Queries use indexes efficiently
  - No missing indexes for common queries
  - No unnecessary indexes

- [ ] **Transactions used correctly**
  - Batch operations wrapped in transactions
  - No long-running transactions blocking others
  - Proper commit/rollback handling

### Testing Review

- [ ] **Tests are comprehensive**
  - Happy path tested
  - Error cases tested
  - Edge cases covered

- [ ] **Tests are reliable**
  - No flaky tests
  - No test interdependencies
  - Tests clean up after themselves

- [ ] **Test names are descriptive**
  - Test name indicates what is being tested
  - Test name indicates expected behavior

---

## CI/CD Integration

Automated checks that must pass before merge.

### Continuous Integration Checks

- [ ] **Linux build passes**
  - Ubuntu 20.04, 22.04, 24.04
  - GCC and Clang compilers

- [ ] **Windows build passes**
  - MSVC 2019, 2022

- [ ] **macOS build passes**
  - macOS 11, 12, 13

- [ ] **All unit tests pass on all platforms**
  - Linux, Windows, macOS
  - Debug and Release builds

- [ ] **Static analysis passes**
  - cppcheck
  - clang-tidy
  - No new warnings

- [ ] **Code coverage meets threshold**
  - >80% for new code
  - Coverage report generated

### Optional CI Checks

- [ ] **Performance benchmarks run** (for performance-critical changes)
  - Results compared to baseline
  - No significant regressions

- [ ] **Memory sanitizer clean** (for memory-sensitive changes)
  - AddressSanitizer
  - MemorySanitizer
  - ThreadSanitizer

---

## Merge Requirements

All of the following must be true before merging:

### Required Approvals

- [ ] **At least one code review approval**
  - From a maintainer or experienced contributor
  - All review comments addressed

- [ ] **No unresolved review comments**
  - All discussions resolved
  - Requested changes completed

### CI Status

- [ ] **All CI checks passing**
  - Build successful on all platforms
  - All tests passing
  - No new warnings

### Documentation

- [ ] **Changelog updated** (if user-visible changes)
  - Entry added to CHANGELOG.md
  - Includes version number
  - Clear description of change

- [ ] **PR description is complete**
  - Clear summary of changes
  - Motivation explained
  - Breaking changes highlighted
  - Testing performed described

### Version Control

- [ ] **Commits are clean**
  - Commit messages follow conventional format
  - No "WIP" or "fix tests" commits (squash if needed)
  - Each commit builds successfully

- [ ] **Branch is up to date with target**
  - No merge conflicts
  - Rebased on latest main/master

---

## Post-Merge Tasks

After merging, ensure:

- [ ] **Branch deleted** (if feature branch)

- [ ] **Related issues closed/updated**
  - Link PR in issue comments
  - Update issue status

- [ ] **Documentation deployed** (if docs-only change)
  - Wiki updated
  - User guides published

- [ ] **Release notes prepared** (for significant features)
  - User-facing changes documented
  - API changes noted

---

## Special Considerations for Semantic Search

### Phase-Specific Checks

#### Phase 1 (Content Text Index)

- [ ] **FTS5 queries tested**
  - BM25 ranking verified
  - Snippet generation works
  - Query normalization correct

- [ ] **Chunk processing validated**
  - Word counts accurate
  - Timestamps preserved
  - Overlap logic correct

- [ ] **Parser compatibility verified**
  - SRT, ASS, VTT formats
  - Character encoding handling
  - Malformed file handling

#### Phase 2 (Vector Embeddings)

- [ ] **Embedding generation tested**
  - Model inference works
  - Vector dimensions correct (384)
  - Normalization applied

- [ ] **Vector search validated**
  - k-NN returns correct results
  - Distance calculations accurate
  - Performance acceptable

#### Phase 3 (Hybrid Search)

- [ ] **Result fusion tested**
  - BM25 + vector scores combined correctly
  - Weighting configurable
  - Ranking makes sense

- [ ] **Cross-lingual search works** (if applicable)
  - Multilingual embeddings used
  - Language detection correct

### Transcription Provider Checks

When adding/modifying transcription providers:

- [ ] **API integration tested**
  - Authentication works
  - Rate limiting handled
  - Error responses parsed

- [ ] **Cost tracking accurate**
  - Usage recorded correctly
  - Budget enforcement works
  - Cost estimates reasonable

- [ ] **Audio extraction tested**
  - FFmpeg commands correct
  - Chunking works for large files
  - Cleanup happens properly

---

## Common Issues to Watch For

### Database Issues

- ⚠️ **Long-running transactions** - Can cause "database locked" errors
  - Keep transactions short
  - Batch inserts in reasonable sizes (1000-5000 chunks)

- ⚠️ **Missing indexes** - Slow queries
  - Profile queries with EXPLAIN QUERY PLAN
  - Add indexes for columns used in WHERE/ORDER BY

- ⚠️ **Schema version not incremented** - Migration fails
  - Always increment `GetSchemaVersion()` when changing schema
  - Implement migration in `UpdateTables()`

### Threading Issues

- ⚠️ **Race conditions** - Intermittent failures
  - Use ThreadSanitizer during testing
  - Protect shared state with mutexes

- ⚠️ **Deadlocks** - Hangs
  - Acquire locks in consistent order
  - Avoid nested locks when possible

### Memory Issues

- ⚠️ **Memory leaks** - Growing memory usage
  - Use RAII (unique_ptr, shared_ptr)
  - Run valgrind during testing

- ⚠️ **Use-after-free** - Crashes
  - AddressSanitizer catches these
  - Careful with pointer lifetimes

### API Issues

- ⚠️ **Breaking changes** - Breaks existing code
  - Maintain backward compatibility
  - Deprecate old APIs before removing

- ⚠️ **Missing error handling** - Silent failures
  - Check all function return values
  - Log errors appropriately

---

## Resources

### Tools

- **clang-format:** Code formatting
  ```bash
  clang-format -i file.cpp
  ```

- **cppcheck:** Static analysis
  ```bash
  cppcheck --enable=all --suppress=missingIncludeSystem file.cpp
  ```

- **valgrind:** Memory leak detection
  ```bash
  valgrind --leak-check=full ./kodi-test
  ```

- **ThreadSanitizer:** Race condition detection
  ```bash
  cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=thread"
  make
  ./kodi-test
  ```

### Documentation

- [Kodi Development Guide](https://kodi.wiki/view/Development)
- [Semantic Search Phase 1 Docs](Phase1-ContentTextIndex.md)
- [Developer Guide](DeveloperGuide.md)
- [User Guide](UserGuide.md)

### Getting Help

- **Forum:** [forum.kodi.tv](https://forum.kodi.tv)
- **Discord:** #semantic-search channel
- **GitHub:** Open an issue for questions

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-11-25 | Initial checklist for Phase 1 |

---

**Document Owner:** Kodi Semantic Search Team
**Review Frequency:** Quarterly or with major changes
