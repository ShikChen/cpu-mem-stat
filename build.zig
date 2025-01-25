const std = @import("std");

const targets: []const std.Target.Query = &.{
    .{ .cpu_arch = .aarch64, .os_tag = .macos },
    .{ .cpu_arch = .aarch64, .os_tag = .linux, .abi = .gnu },
    .{ .cpu_arch = .aarch64, .os_tag = .linux, .abi = .musl },
    .{ .cpu_arch = .x86_64, .os_tag = .macos },
    .{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .gnu },
    .{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .musl },
};

pub fn buildRelease(b: *std.Build) !void {
    const release_step = b.step("release", "Build release targets");
    for (targets) |target| {
        const target_exe = b.addExecutable(.{
            .name = "cpu-mem-stat",
            .target = b.resolveTargetQuery(target),
            .optimize = .ReleaseSmall,
        });
        target_exe.addCSourceFiles(.{
            .files = &.{"main.c"},
            .flags = &.{ "-std=gnu23", "-Wall", "-Werror" },
        });
        target_exe.linkLibC();
        const target_output = b.addInstallArtifact(target_exe, .{
            .dest_dir = .{
                .override = .{
                    .custom = try target.zigTriple(b.allocator),
                },
            },
        });
        release_step.dependOn(&target_output.step);
    }
}

pub fn build(b: *std.Build) !void {
    const exe = b.addExecutable(.{
        .name = "cpu-mem-stat",
        .target = b.standardTargetOptions(.{}),
        .optimize = b.standardOptimizeOption(.{}),
    });
    exe.addCSourceFiles(.{ .files = &.{"main.c"} });
    exe.linkLibC();
    b.installArtifact(exe);

    const run_exe = b.addRunArtifact(exe);
    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_exe.step);

    try buildRelease(b);
}
