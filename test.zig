const std = @import("std");
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;

const c = @cImport({
    @cInclude("lib.h");
});

test "get_cpu_stat" {
    const stat = c.get_cpu_stat();
    // Ticks may wrap around, so we cannot compare idle and total directly.
    try expect(stat.idle >= 0);
    try expect(stat.total > 0);
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

fn expectFormat(
    expected: []const u8,
    cpu: c_int,
    mem_used: i64,
    mem_total: i64,
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
    const P: i64 = 1 << 50;

    try expectFormat("  0% 1024/2048M", 0, 1 * G, 2 * G);
    try expectFormat(" 50% 1024/2048M", 50, 1 * G, 2 * G);
    try expectFormat("100% 1024/2048M", 100, 1 * G, 2 * G);

    try expectFormat(" 50% 5566/9999M", 50, 5566 * M, 9999 * M);
    try expectFormat(" 50% 5.44/9.77G", 50, 5566 * M, 9999 * M + M / 2);
    try expectFormat(" 50% 5.44/9.8G", 50, 5566 * M, 98 * G / 10);
    try expectFormat(" 50% 8.00/31.5G", 50, 8 * G, 315 * G / 10);
    try expectFormat(" 50% 8.00/32G", 50, 8 * G, 32 * G);
    try expectFormat(" 50% 50.0/99.9G", 50, 50 * G, 9994 * G / 100);
    try expectFormat(" 50% 50.0/100G", 50, 50 * G, 9996 * G / 100);
    try expectFormat(" 50% 100/128GB", 50, 100 * G, 128 * G);
    try expectFormat(" 50% 8.00/16T", 50, 8 * T, 16 * T);
    try expectFormat(" 50% 8.00/16P", 50, 8 * P, 16 * P);
}
