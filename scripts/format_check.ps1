#requires -version 5.1
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$exts = @("*.h","*.hpp","*.hh","*.c","*.cc","*.cpp","*.cxx")

$excludeSubstrings = @(
  "\bazel-",
  "\third_party\",
  "\external\",
  "\out\",
  "\build\",
  "\.git\"
)

$files = Get-ChildItem -Path $repoRoot -Recurse -File -Include $exts | Where-Object {
  $p = $_.FullName.ToLowerInvariant()
  foreach ($sub in $excludeSubstrings) {
    if ($p.Contains($sub)) { return $false }
  }
  return $true
}

if ($files.Count -eq 0) { exit 0 }

Write-Host ("Checking {0} files..." -f $files.Count)

$failed = $false
foreach ($f in $files) {
$xml = clang-format --style=file -output-replacements-xml $f.FullName
if ($xml -match "<replacement ") {
  Write-Host ("NOT FORMATTED: {0}" -f $f.FullName)
  $failed = $true
}

}

if ($failed) {
  Write-Host ""
  Write-Host "Fix by running:"
  Write-Host "  powershell -ExecutionPolicy Bypass -File scripts\format.ps1"
  exit 1
}

Write-Host "All formatted."
