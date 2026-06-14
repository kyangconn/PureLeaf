package service

import (
	"fmt"
	"strings"
)

// ComputeDiff 生成两段文本的 unified diff（行级）。
//
// 实现基于 LCS（最长公共子序列），不依赖外部库。
// 输出格式与 GNU diff 一致。headerOld/headerNew 用作 diff 头部标识。
func ComputeDiff(headerOld, headerNew, oldText, newText string) string {
	oldLines := splitLines(oldText)
	newLines := splitLines(newText)

	ops := diffOps(oldLines, newLines)
	hunks := buildHunks(oldLines, newLines, ops, diffContext)
	if len(hunks) == 0 {
		return ""
	}

	var b strings.Builder
	fmt.Fprintf(&b, "--- %s\n", headerOld)
	fmt.Fprintf(&b, "+++ %s\n", headerNew)
	for _, h := range hunks {
		fmt.Fprintf(&b, "@@ -%s +%s @@\n", h.oldRange, h.newRange)
		for _, line := range h.lines {
			b.WriteString(line)
			b.WriteByte('\n')
		}
	}
	return b.String()
}

const diffContext = 3

// diffOp 描述一段对齐区间: op ∈ {equal, replace, delete, insert}。
// i1:i2 是 old 的索引区间（半开），j1:j2 是 new 的索引区间（半开）。
type diffOp struct {
	op             string
	i1, i2, j1, j2 int
}

// diffOps 基于 LCS 动态规划计算对齐操作序列。
func diffOps(a, b []string) []diffOp {
	la, lb := len(a), len(b)
	// dp[i][j] = a[i:] 与 b[j:] 的 LCS 长度
	dp := make([][]int, la+1)
	for i := range dp {
		dp[i] = make([]int, lb+1)
	}
	for i := la - 1; i >= 0; i-- {
		for j := lb - 1; j >= 0; j-- {
			if a[i] == b[j] {
				dp[i][j] = dp[i+1][j+1] + 1
			} else if dp[i+1][j] >= dp[i][j+1] {
				dp[i][j] = dp[i+1][j]
			} else {
				dp[i][j] = dp[i][j+1]
			}
		}
	}

	var ops []diffOp
	i, j := 0, 0
	for i < la && j < lb {
		if a[i] == b[j] {
			si, sj := i, j
			for i < la && j < lb && a[i] == b[j] {
				i++
				j++
			}
			ops = append(ops, diffOp{"equal", si, i, sj, j})
		} else if dp[i+1][j] >= dp[i][j+1] {
			si := i
			for i < la && dp[i+1][j] >= dp[i][j+1] {
				i++
			}
			ops = append(ops, diffOp{"delete", si, i, j, j})
		} else {
			sj := j
			for j < lb && dp[i+1][j] < dp[i][j+1] {
				j++
			}
			ops = append(ops, diffOp{"insert", i, i, sj, j})
		}
	}
	if i < la {
		ops = append(ops, diffOp{"delete", i, la, j, j})
	}
	if j < lb {
		ops = append(ops, diffOp{"insert", i, i, j, lb})
	}
	return ops
}

type diffHunk struct {
	oldStart int
	oldCount int
	newStart int
	newCount int
	oldRange string
	newRange string
	lines    []string
}

// buildHunks 将 opCodes 转为带上下文的 unified diff hunks。
// 相邻变更间隔 ≤ 2*context 的合并为同一 hunk。
func buildHunks(a, b []string, ops []diffOp, context int) []diffHunk {
	// 标记哪些 old 索引「附近」发生了变化。
	// delete：被删的 old 行 [i1,i2)。
	// insert：old 没有行变化，但插入发生在错点 i1，标记 i1 让它附近的 context 被纳入 hunk。
	changed := make([]bool, len(a)+1)
	for _, op := range ops {
		switch op.op {
		case "delete":
			for k := op.i1; k < op.i2; k++ {
				changed[k] = true
			}
		case "insert":
			// 错点可能等于 len(a)（末尾插入），changed 数组预留了一位。
			if op.i1 >= 0 && op.i1 <= len(a) {
				changed[op.i1] = true
			}
		}
	}

	// 找出所有需要展示的行范围（变化行 ± context）
	type span struct{ lo, hi int } // old 索引半开区间
	var spans []span
	lo, hi := -1, -1
	for k := 0; k <= len(a); k++ {
		// k 是否在某个变化点的 context 范围内（变化点可能 = len(a)，如末尾插入）
		near := false
		for d := -context; d <= context; d++ {
			idx := k + d
			if idx >= 0 && idx <= len(a) && changed[idx] {
				near = true
				break
			}
		}
		if near {
			if lo == -1 {
				lo = k
			}
			hi = k + 1
		} else if lo != -1 {
			spans = append(spans, span{lo, hi})
			lo, hi = -1, -1
		}
	}
	if lo != -1 {
		spans = append(spans, span{lo, hi})
	}

	// 为每个 span 构造 hunk：遍历 ops，把落入 span 的 old 区间对应的行挑出来
	var hunks []diffHunk
	for _, sp := range spans {
		h := diffHunk{oldStart: sp.lo + 1, newStart: 0}
		newStartSet := false

		for _, op := range ops {
			// equal 段：输出落在 span 内的部分
			if op.op == "equal" {
				for k := op.i1; k < op.i2; k++ {
					if k >= sp.lo && k < sp.hi {
						h.lines = append(h.lines, " "+a[k])
						if !newStartSet {
							// 对应的 new 行索引
							h.newStart = op.j1 + (k - op.i1) + 1
							newStartSet = true
						}
					}
				}
				continue
			}
			// delete：old 段。落在 span 内的部分输出 '-'。
			if op.i2 > sp.lo && op.i1 < sp.hi {
				for k := op.i1; k < op.i2; k++ {
					if k >= sp.lo && k < sp.hi {
						h.lines = append(h.lines, "-"+a[k])
						if !newStartSet {
							h.newStart = op.j1 + 1
							newStartSet = true
						}
					}
				}
			}
			// insert：new 段，紧跟在对应 old 位置。只要其锚点 old 索引落在 span 内就输出 '+'。
			if op.j1 < op.j2 {
				// insert 锚点 = op.i1（old 中插入点）
				if op.i1 >= sp.lo && op.i1 <= sp.hi {
					for k := op.j1; k < op.j2; k++ {
						h.lines = append(h.lines, "+"+b[k])
						if !newStartSet {
							h.newStart = op.j1 + 1
							newStartSet = true
						}
					}
				}
			}
		}
		if !newStartSet {
			h.newStart = h.oldStart
		}
		// 统计 old/new 行数
		for _, l := range h.lines {
			switch {
			case strings.HasPrefix(l, " "):
				h.oldCount++
				h.newCount++
			case strings.HasPrefix(l, "-"):
				h.oldCount++
			case strings.HasPrefix(l, "+"):
				h.newCount++
			}
		}
		h.oldRange = formatRange(h.oldStart, h.oldCount)
		h.newRange = formatRange(h.newStart, h.newCount)
		hunks = append(hunks, h)
	}
	return hunks
}

func formatRange(start, count int) string {
	if count == 0 {
		if start == 0 {
			return "0,0"
		}
		return fmt.Sprintf("%d,0", start-1)
	}
	if count == 1 {
		return fmt.Sprintf("%d", start)
	}
	return fmt.Sprintf("%d,%d", start, count)
}

func splitLines(s string) []string {
	if s == "" {
		return nil
	}
	s = strings.ReplaceAll(s, "\r\n", "\n")
	parts := strings.Split(s, "\n")
	if len(parts) > 0 && parts[len(parts)-1] == "" {
		parts = parts[:len(parts)-1]
	}
	return parts
}
