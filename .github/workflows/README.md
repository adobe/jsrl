# GitHub Actions CI/CD

This directory contains the continuous integration workflows for JSRL.

## Manual Approval Workflow

These workflows are configured to **require manual approval** before running:

### For Maintainers

**Option 1: Manual Trigger**
1. Go to the "Actions" tab in GitHub
2. Select the workflow you want to run (CI or Quick Check)
3. Click "Run workflow"
4. Select the branch
5. Click the green "Run workflow" button

**Option 2: Label Pull Requests**
For pull requests, add the label `safe to test` to trigger the CI:
1. Review the pull request code carefully
2. If safe, add the label "safe to test" to the PR
3. The CI workflow will automatically start

### For Contributors

When you submit a pull request:
1. The workflows will NOT run automatically
2. A maintainer must review your code first
3. They will either:
   - Add the "safe to test" label to run CI, or
   - Manually trigger the workflow from the Actions tab

This is a security measure to prevent malicious code from running in CI.

## Workflows

### `ci.yml` - Main CI Pipeline
**Triggers:** Manual dispatch, or pull requests with "safe to test" label

**Jobs:**
- **build-and-test**: Comprehensive testing across multiple platforms and compilers
  - **Platforms**: Ubuntu (Linux), macOS, Windows
  - **Compilers**: GCC 11, Clang 14, MSVC
  - **C++ Standards**: C++17, C++20
  - Runs all tests with CTest
  - Verifies installation succeeds

- **build-ubuntu-sanitizers**: Memory and undefined behavior detection
  - Tests with Address Sanitizer (ASan) - detects memory errors
  - Tests with Undefined Behavior Sanitizer (UBSan) - detects UB
  - Tests with Thread Sanitizer (TSan) - detects data races

- **build-coverage**: Code coverage reporting
  - Generates coverage report with lcov
  - Uploads to Codecov (requires `CODECOV_TOKEN` secret)

### `quick-check.yml` - Fast Feedback
**Triggers:** Manual dispatch only

A lightweight check for quick validation:
- Single platform (Ubuntu)
- Debug build
- All tests

## Setting Up

### Required Secrets
To enable code coverage reporting:
1. Go to [Codecov](https://codecov.io) and set up your repository
2. Get the upload token
3. Add it as a GitHub secret named `CODECOV_TOKEN`

### Creating the "safe to test" Label
1. Go to your repository's Issues tab
2. Click "Labels"
3. Click "New label"
4. Name: `safe to test`
5. Color: Choose any color (green recommended)
6. Description: "CI approved to run on this PR"
7. Click "Create label"

### Badge
Add this to your README to show workflow status (only shows when manually triggered):
```markdown
[![CI](https://github.com/YOUR-ORG/jsrl/actions/workflows/ci.yml/badge.svg)](https://github.com/YOUR-ORG/jsrl/actions/workflows/ci.yml)
```

## Security Notes

**Why manual approval?**
- Prevents malicious code in PRs from accessing secrets
- Prevents abuse of CI resources
- Common practice for open source projects
- GitHub's built-in protection for `pull_request_target`

**What to check before approving:**
- Review all changed files in the PR
- Check for suspicious code (crypto mining, data exfiltration, etc.)
- Verify the PR makes sense and appears legitimate
- Check the contributor's reputation if unsure

## Testing Locally

To run the same checks locally:

```bash
# Standard build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DJSRL_BUILD_TESTS=ON
cmake --build build -j
cd build && ctest --output-on-failure

# With Address Sanitizer
cmake -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
  -DJSRL_BUILD_TESTS=ON
cmake --build build-asan -j
cd build-asan && ASAN_OPTIONS=detect_leaks=1 ctest --output-on-failure

# With coverage
cmake -B build-coverage \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
  -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
  -DJSRL_BUILD_TESTS=ON
cmake --build build-coverage -j
cd build-coverage && ctest --output-on-failure
lcov --directory . --capture --output-file coverage.info
lcov --list coverage.info
```

## Maintenance

The workflows use GitHub Actions marketplace actions:
- `actions/checkout@v4` - Checkout code
- `egor-tensin/setup-gcc@v1` - Set up GCC
- `egor-tensin/setup-clang@v1` - Set up Clang
- `codecov/codecov-action@v4` - Upload coverage

These should be kept up to date periodically.
