# cpu-mem-stat

A compact, fixed-width cpu and memory usage monitor for tmux status bars.

```
$ cpu-mem-stat -d 900 # sample for 900ms
 42% 12.3/32G
```

## Supported platforms

- macOS (aarch64, x86_64)
- Linux (aarch64, x86_64, glibc and musl)

## Install

Download prebuilt binaries from [GitHub Releases](https://github.com/ShikChen/cpu-mem-stat/releases),
or install with [mise](https://mise.jdx.dev/):

```sh
mise use --global github:ShikChen/cpu-mem-stat
```

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

## Related projects

- [tmux-mem-cpu-load](https://github.com/thewtex/tmux-mem-cpu-load)
