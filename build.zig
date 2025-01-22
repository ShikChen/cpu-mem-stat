const std = @import("std");

pub fn build(b: *std.Build) void {
    const exe = b.addExecutable(.{
        .name = "cpu-mem-stat",
        .target = b.standardTargetOptions(.{}),
        .optimize = b.standardOptimizeOption(.{}),
    });
    exe.addCSourceFiles(.{ .files = &.{"main.c"} });
    exe.linkLibC();
    b.installArtifact(exe);
}
