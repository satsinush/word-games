#!/usr/bin/env python3
from pathlib import Path
import sys
from collections import defaultdict
import shutil

def main():
  repo_root = Path(__file__).resolve().parent.parent  # scripts/.. => project root
  src_dir = repo_root / "src"
  if not src_dir.exists():
    print(f"src directory not found: {src_dir}", file=sys.stderr)
    sys.exit(1)

  patterns = ("*.cpp", "*.hpp", "*.ui")
  files = sorted({p for pat in patterns for p in src_dir.rglob(pat)})

  if not files:
    print("no matching files found under", src_dir)
    return

  ext_counts = defaultdict(int)
  per_file = []
  total_lines = 0

  for p in files:
    try:
      with p.open("r", encoding="utf-8", errors="replace") as fh:
        n = sum(1 for _ in fh)
    except Exception as e:
      print(f"skipping {p}: {e}", file=sys.stderr)
      continue
    # try to show paths relative to repo root when possible
    try:
      rel = p.relative_to(repo_root)
    except Exception:
      rel = p
    per_file.append((str(rel), n))
    ext_counts[p.suffix] += n
    total_lines += n

  if not per_file:
    print("no files read", file=sys.stderr)
    return

  # sort by number of lines (descending)
  per_file.sort(key=lambda x: x[1], reverse=True)

  # determine column widths and terminal width for truncation
  term_cols = shutil.get_terminal_size((120, 20)).columns
  num_width = max(len(str(n)) for _, n in per_file + [("", total_lines)])  # width for numbers
  # reserve space: num_width + 2 spaces + at least 20 for path
  min_path_width = 20
  path_col_width = max(min_path_width, term_cols - num_width - 2)
  # but don't make path column larger than the longest path
  max_path_len = max(len(p) for p, _ in per_file)
  path_col_width = min(path_col_width, max_path_len)

  def truncate_left(s, width):
    if len(s) <= width:
      return s
    if width <= 3:
      return s[-width:]
    return "..." + s[-(width - 3):]

  # print table header
  header_file = "File"
  header_lines = "Lines"
  print(f"{header_file:>{path_col_width}}  {header_lines:>{num_width}}")
  print(f"{'-' * path_col_width}  {'-' * num_width}")

  for path_str, n in per_file:
    disp = truncate_left(path_str, path_col_width)
    print(f"{disp:<{path_col_width}}  {n:>{num_width}}")

  # summary
  print()
  print(f"files scanned: {len(per_file)}")
  print(f"total lines: {total_lines}")
  for ext, lines in sorted(ext_counts.items()):
    print(f"  {ext}: {lines}")

if __name__ == "__main__":
  main()