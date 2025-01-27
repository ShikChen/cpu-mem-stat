const std = @import("std");
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;

const c = @cImport({
    @cInclude("lib.h");
});

test "get_cpu_stat" {
    const stat = c.get_cpu_stat();
    try expect(stat.idle >= 0);
    try expect(stat.total > 0);
    // We cannot check idle <= total because the ticks might have wrapped
    // around.
}

test "get_mem_stat" {
    const mem = c.get_mem_stat();
    try expect(mem.total > 0);
    try expect(mem.used >= 0);
    try expect(mem.used <= mem.total);
}

test "sample_cpu_usage" {
    const usage = c.sample_cpu_usage(10);
    try expect(usage >= 0 and usage <= 100);
}

fn test_format(
    cpu: c_int,
    mem_used: i64,
    mem_total: i64,
    expected: []const u8,
) !void {
    var buf = [_:0]u8{0} ** 128;
    const mem = c.mem_stat{ .used = mem_used, .total = mem_total };
    c.format_stats(cpu, mem, &buf[0], buf.len);
    const str = std.mem.span(@as([*:0]u8, buf[0..]));
    try expectEqualStrings(expected, str);
}

test "format_stats" {
    const M: i64 = 1 << 20;
    const G: i64 = 1 << 30;
    const T: i64 = 1 << 40;

    try test_format(0, 1 * G, 2 * G, "  0% 1024/2048M");
    try test_format(50, 1 * G, 2 * G, " 50% 1024/2048M");
    try test_format(100, 1 * G, 2 * G, "100% 1024/2048M");

    try test_format(50, 5566 * M, 8 * G, " 50% 5566/8192M");
    try test_format(50, 5566 * M, 8193 * M, " 50% 5.4/8G");
    try test_format(50, 8 * G, 315 * G / 10, " 50% 8.0/31.5G");
    try test_format(50, 8 * G, 32 * G, " 50% 8.0/32G");
    try test_format(50, 50 * G, 9994 * G / 100, " 50% 50.0/99.9G");
    try test_format(50, 50 * G, 9996 * G / 100, " 50% 50/100G");

    try test_format(50, 1 * T, 256 * T, " 50% 1/256T");
}
