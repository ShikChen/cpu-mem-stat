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

test "format_stats" {
    var buf = [_:0]u8{0} ** 128;
    const mem = c.mem_stat{ .used = 1 << 30, .total = 2 << 30 };
    c.format_stats(50, mem, &buf[0], buf.len);
    const str = std.mem.span(@as([*:0]u8, buf[0..]));
    try expectEqualStrings(" 50% 1024/2048M", str);
}
