# Absolute, merciless CMake/Ninja/VS build artifact purge for this tree.
# Usage:
#   powershell -ExecutionPolicy Bypass -File .\tools\ua_purge_cmake.ps1
#   powershell -ExecutionPolicy Bypass -File .\tools\ua_purge_cmake.ps1 -DryRun

param(
  [switch]$DryRun
)

function Remove-ItemSafe {
  param([string]$Path, [switch]$IsDir)
  if ($DryRun) {
    if ($IsDir) { Write-Host "[purge] RMDIR(dry) $Path" }
    else        { Write-Host "[purge] DEL  (dry) $Path" }
    return
  }
  try {
    if ($IsDir) {
      Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction SilentlyContinue
      Write-Host "[purge] RMDIR $Path"
    } else {
      Remove-Item -LiteralPath $Path -Force -ErrorAction SilentlyContinue
      Write-Host "[purge] DEL  $Path"
    }
  } catch { }
}

Write-Host "[purge] root: $PWD"
Write-Host ("[purge] mode: {0}" -f ($(if ($DryRun) { "DRY-RUN" } else { "DESTRUCTIVE" })))

# Directories to remove
$DirPatterns = @(
  "build","build-*","cmake-build-*","out\build","out\build-*",
  ".vs","_deps","_cmake_test_*","CMakeFiles","CMakeScripts",
  ".cache",".cmake",".ninja",
  "Debug","Release","RelWithDebInfo","MinSizeRel"
)

# Files to remove
$FilePatterns = @(
  "CMakeCache.txt","CMakeUserPresets.json","CMakePresets.json",
  "Makefile","build.ninja","rules.ninja",".ninja_deps",".ninja_log",
  "compile_commands.json","CTestTestfile.cmake","DartConfiguration.tcl",
  "*.sln","*.vcxproj","*.vcxproj.*","*.vcproj","*.vcproj.*","*.code-workspace",
  "*.obj","*.o","*.a","*.lib","*.exp","*.ilk","*.pdb","*.dll","*.exe","*.out","*.manifest",
  "*.cache","*.dir"
)

# Remove directories matching patterns (glob recursively)
foreach ($pat in $DirPatterns) {
  Get-ChildItem -Recurse -Force -Directory -Filter $pat -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-ItemSafe -Path $_.FullName -IsDir }
}

# Remove common named directories regardless of path depth
Get-ChildItem -Recurse -Force -Directory -ErrorAction SilentlyContinue |
  Where-Object { $_.Name -in @("CMakeFiles",".ninja",".cmake","_deps",".vs","Debug","Release","RelWithDebInfo","MinSizeRel") } |
  ForEach-Object { Remove-ItemSafe -Path $_.FullName -IsDir }

# Remove files matching patterns
foreach ($pat in $FilePatterns) {
  Get-ChildItem -Recurse -Force -File -Filter $pat -ErrorAction SilentlyContinue |
    ForEach-Object { Remove-ItemSafe -Path $_.FullName }
}

Write-Host "[purge] purge complete."
