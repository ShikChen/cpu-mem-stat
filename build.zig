const std = @import("std");
const AddCSourceFilesOptions = std.Build.Module.AddCSourceFilesOptions;

const targets: []const std.Target.Query = &.{
    .{ .cpu_arch = .aarch64, .os_tag = .macos },
    .{ .cpu_arch = .aarch64, .os_tag = .linux, .abi = .gnu },
    .{ .cpu_arch = .aarch64, .os_tag = .linux, .abi = .musl },
    .{ .cpu_arch = .x86_64, .os_tag = .macos },
    .{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .gnu },
    .{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .musl },
};

pub fn buildRelease(
    b: *std.Build,
    version: []const u8,
    c_opts: AddCSourceFilesOptions,
) !void {
    const release_step = b.step("release", "Build release targets");
    const archive_step = b.step("archive", "Build archive for release");
    archive_step.dependOn(release_step);
    for (targets) |target| {
        const target_exe = b.addExecutable(.{
            .name = "cpu-mem-stat",
            .target = b.resolveTargetQuery(target),
            .optimize = .ReleaseSmall,
        });
        target_exe.addCSourceFiles(c_opts);
        target_exe.linkLibC();
        const triple = try target.zigTriple(b.allocator);
        const artifact = b.addInstallArtifact(target_exe, .{
            .dest_dir = .{
                .override = .{
                    .custom = triple,
                },
            },
        });
        release_step.dependOn(&artifact.step);

        const wf = b.addWriteFiles();
        const dist_exe = b.fmt("dist/{s}", .{target_exe.out_filename});
        _ = wf.addCopyFile(target_exe.getEmittedBin(), dist_exe);

        const tar = b.addSystemCommand(&.{ "tar", "zcvf" });
        tar.setCwd(wf.getDirectory());
        const out_name = b.fmt("cpu-mem-stat-{s}-{s}.tar.gz", .{ version, triple });
        const tar_file = tar.addOutputFileArg(out_name);
        tar.addArgs(&.{ "-C", "dist", target_exe.out_filename });
        const install_tar = b.addInstallFileWithDir(
            tar_file,
            .{ .custom = "archive" },
            out_name,
        );
        archive_step.dependOn(&install_tar.step);
    }
}

pub fn build(b: *std.Build) !void {
    const version = b.option([]const u8, "version", "program version") orelse "0.0.0";
    const c_opts: AddCSourceFilesOptions = .{
        .files = &.{ "main.c", "lib.c" },
        // Sync with compile_flags.txt.
        // TODO: Can we generate this from compile_flags.txt automatically?
        .flags = &.{
            "-std=gnu23",
            "-Wall",
            "-Werror",
            b.fmt("-DVERSION=\"{s}\"", .{version}),
        },
    };

    const exe = b.addExecutable(.{
        .name = "cpu-mem-stat",
        .target = b.standardTargetOptions(.{}),
        .optimize = b.standardOptimizeOption(.{}),
    });
    exe.addCSourceFiles(c_opts);
    exe.linkLibC();
    b.installArtifact(exe);

    const run_exe = b.addRunArtifact(exe);
    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_exe.step);

    const test_exe = b.addTest(.{ .root_source_file = b.path("test.zig") });
    test_exe.addCSourceFiles(.{
        .files = &.{"lib.c"},
        .flags = &.{"-std=gnu23"},
    });
    test_exe.linkLibC();
    test_exe.addIncludePath(b.path("."));
    const run_test_exe = b.addRunArtifact(test_exe);
    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_test_exe.step);

    try buildRelease(b, version, c_opts);
}
