#requires -version 5.1
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Command([string]$name) {
  if (-not (Get-Command $name -ErrorAction SilentlyContinue)) {
    throw "Command not found: $name. Please ensure it is installed and in PATH."
  }
}

function Get-RepoRoot() {
  $root = (& git rev-parse --show-toplevel) 2>$null
  if (-not $root) { throw "Not a git repository (git rev-parse failed)." }
  return $root.Trim()
}

function Clean-RelPath([string]$s) {
  if ($null -eq $s) { return "" }

  # 去首尾空白
  $t = $s.Trim()

  # 如果 git 输出出现了带引号的路径，去掉外层引号
  if ($t.Length -ge 2 -and $t.StartsWith('"') -and $t.EndsWith('"')) {
    $t = $t.Substring(1, $t.Length - 2)
  }

  # 去掉所有控制字符（包含 \r \n \t 等）
  $t = -join ($t.ToCharArray() | Where-Object { [int]$_ -ge 32 })

  # 统一分隔符（git 通常输出 /）
  $t = $t.Replace('/', '\')

  return $t.Trim()
}

function Get-ChangedFiles() {
  # 用 -z 输出（NUL 分隔），避免 CRLF/LF/特殊字符导致的行污染
  $raw1 = (& git diff --name-only -z) 2>$null
  $raw2 = (& git diff --name-only --cached -z) 2>$null

  $paths = @()
  if ($raw1) { $paths += $raw1.Split([char]0) }
  if ($raw2) { $paths += $raw2.Split([char]0) }

  $paths = $paths |
    ForEach-Object { Clean-RelPath $_ } |
    Where-Object { $_ -ne "" } |
    Sort-Object -Unique

  return ,$paths
}

# ---------------- main ----------------
Assert-Command git
Assert-Command clang-format

$repoRoot = Get-RepoRoot

# 只处理 C/C++ 文件
$extOk = @(".h", ".hpp", ".hh", ".c", ".cc", ".cpp", ".cxx")

# 排除目录（统一用小写匹配）
$excludeSubstrings = @("\bazel-", "\third_party\", "\external\", "\out\", "\build\", "\.git\")

$files = Get-ChangedFiles
if (-not $files -or $files.Count -eq 0) {
  Write-Host "No changed files."
  exit 0
}

$targets = @()
foreach ($rel in $files) {
  $full = Join-Path $repoRoot $rel
  $pLower = $full.ToLowerInvariant()

  $skip = $false
  foreach ($sub in $excludeSubstrings) {
    if ($pLower.Contains($sub)) { $skip = $true; break }
  }
  if ($skip) { continue }

  if (-not (Test-Path -LiteralPath $full)) { continue }

  $ext = [IO.Path]::GetExtension($full).ToLowerInvariant()
  if ($extOk -contains $ext) {
    $targets += $full
  }
}

if ($targets.Count -eq 0) {
  Write-Host "No changed C/C++ files."
  exit 0
}

Write-Host ("Formatting changed files: {0}" -f $targets.Count)
foreach ($f in $targets) {
  clang-format -i --style=file $f
}
Write-Host "Done."
