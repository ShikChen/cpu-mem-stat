const std = @import("std");

const targets: []const std.Target.Query = &.{
    .{ .cpu_arch = .aarch64, .os_tag = .macos },
    .{ .cpu_arch = .aarch64, .os_tag = .linux, .abi = .gnu },
    .{ .cpu_arch = .aarch64, .os_tag = .linux, .abi = .musl },
    .{ .cpu_arch = .x86_64, .os_tag = .macos },
    .{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .gnu },
    .{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .musl },
};

const cSourceFilesOptions: std.Build.Module.AddCSourceFilesOptions = .{
    .files = &.{ "main.c", "lib.c" },
    // Sync with compile_flags.txt.
    // TODO: Can we generate this from compile_flags.txt automatically?
    .flags = &.{ "-std=gnu23", "-Wall", "-Werror" },
};

pub fn buildRelease(b: *std.Build) !void {
    const release_step = b.step("release", "Build release targets");
    for (targets) |target| {
        const target_exe = b.addExecutable(.{
            .name = "cpu-mem-stat",
            .target = b.resolveTargetQuery(target),
            .optimize = .ReleaseSmall,
        });
        target_exe.addCSourceFiles(cSourceFilesOptions);
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
    exe.addCSourceFiles(cSourceFilesOptions);
    exe.linkLibC();
    b.installArtifact(exe);

    const run_exe = b.addRunArtifact(exe);
    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_exe.step);

    const test_exe = b.addTest(.{ .root_source_file = b.path("test.zig") });
    test_exe.addCSourceFiles(
        .{ .files = &.{"lib.c"}, .flags = &.{"-std=gnu23"} },
    );
    test_exe.linkLibC();
    test_exe.addIncludePath(b.path("."));
    const run_test_exe = b.addRunArtifact(test_exe);
    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_test_exe.step);

    try buildRelease(b);
}
