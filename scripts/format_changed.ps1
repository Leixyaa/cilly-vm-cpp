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

  $t = $s.Trim()

  if ($t.Length -ge 2 -and $t.StartsWith('"') -and $t.EndsWith('"')) {
    $t = $t.Substring(1, $t.Length - 2)
  }

  $t = -join ($t.ToCharArray() | Where-Object { [int]$_ -ge 32 })
  $t = $t.Replace('/', '\')

  return $t.Trim()
}

function Get-ChangedFilesFlat() {
  $raw1 = (& git diff --name-only -z) 2>$null
  $raw2 = (& git diff --name-only --cached -z) 2>$null

  $paths = New-Object System.Collections.Generic.List[string]

  foreach ($raw in @($raw1, $raw2)) {
    if (-not $raw) { continue }
    foreach ($p in $raw.Split([char]0)) {
      $c = Clean-RelPath $p
      if ($c) { [void]$paths.Add($c) }
    }
  }

  return $paths.ToArray() | Sort-Object -Unique
}

# ---------------- main ----------------
Assert-Command git
Assert-Command clang-format

$repoRoot = Get-RepoRoot

$extOk = @(".h", ".hpp", ".hh", ".c", ".cc", ".cpp", ".cxx")
$excludeSubstrings = @("\bazel-", "\third_party\", "\external\", "\out\", "\build\", "\.git\")

$files = @(Get-ChangedFilesFlat)
if ($files.Length -eq 0) {
  Write-Host "No changed files."
  exit 0
}

$targets = New-Object System.Collections.Generic.List[string]

foreach ($rel in $files) {
  $relStr = [string]$rel
  if (-not $relStr) { continue }

  $full = Join-Path -Path $repoRoot -ChildPath $relStr
  $pLower = $full.ToLowerInvariant()

  $skip = $false
  foreach ($sub in $excludeSubstrings) {
    if ($pLower.Contains($sub)) { $skip = $true; break }
  }
  if ($skip) { continue }

  if (-not (Test-Path -LiteralPath $full)) { continue }

  $ext = [IO.Path]::GetExtension($full).ToLowerInvariant()
  if ($extOk -contains $ext) {
    [void]$targets.Add($full)
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
