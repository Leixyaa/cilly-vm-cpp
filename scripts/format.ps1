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
if ($files.Count -eq 0) {
  Write-Host "No C/C++ files found."
  exit 0
}

Write-Host ("Formatting {0} files..." -f $files.Count)
foreach ($f in $files) {
  clang-format -i --style=file $f.FullName
}
Write-Host "Done."
