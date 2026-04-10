# cpu-mem-stat

Extracted from my dotfiles to use GitHub Actions for prebuilt binaries.

## Development

Requires [Zig](https://ziglang.org/) 0.15+.

```sh
zig build --summary all                           # build
zig build run                                     # run
zig build run -- --help                           # run with arguments
zig build test --summary all                      # test
zig build release -Dversion=1.2.3 --summary all   # cross-compile for all targets
zig build archive -Dversion=1.2.3 --summary all   # create release tarballs
```

### Release

Push a version tag to trigger CI to build, test, and create a GitHub Release.

```sh
git tag v1.2.3
git push origin v1.2.3
```
