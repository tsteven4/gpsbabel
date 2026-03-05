# Script to build and create windows installer.
#
# Run this from a Qt Desktop command window that has the Qt and mingw compiler paths set up,
# such as the one Qt Creator will put on the start menu, or
# from a power shell prompt after settings up the paths with tools\ci_setup_windows.ps1.
# For example
# powershell.exe -ExecutionPolicy Unrestricted -File tools\make_windows_release.ps1
#
# The defaults should be compatible with github action builds.
param(
    [string] $build_dir_name = "bld",
    [string] $generator = "Ninja",
    [string] $qtdir = "C:/Qt/6.8.3/msvc2022_64",
    [string] $toolset = "",
    [ValidateSet('x86','amd64','arm','arm64')] [string] $host_arch = "amd64",
    [ValidateSet('x86','amd64','arm','arm64')] [string] $arch = "amd64"
)

$ErrorActionPreference = "Stop"

# setup visual studio development envirnonment
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installationPath = & "$vswhere" -latest -property installationPath
& "$installationPath\Common7\Tools\Launch-VsDevShell.ps1" -Arch $arch -HostArch $host_arch -SkipAutomaticLocation

# verify we are in the top of the gpsbabel clone
Get-Item tools/ci_script_windows.ps1 -ErrorAction Stop | Out-Null

$src_dir = $Pwd
$build_dir = Join-Path $src_dir $build_dir_name
$CMAKE_PREFIX_PATH = $qtdir
# make sure we are staring with a clean build directory
Remove-Item $build_dir -Recurse -ErrorAction Ignore
# Generate
$hashargs = "-G", $generator
if ( $generator -like "Visual Studio*") {
    if ( $toolset ) {
        $hashargs += "-T", $toolset
    }
    switch ($arch) {
        "x86" { $platform = "Win32" }
        "amd64" { $platform = "x64" }
        "arm" { $platform = "ARM" }
        "arm64" { $platform = "ARM64" }
    }
    $hashargs += "-A", $platform
}
else {
    $hashargs += "-DCMAKE_BUILD_TYPE:STRING=Release"
}
$hashargs += "-DCMAKE_PREFIX_PATH:PATH=$CMAKE_PREFIX_PATH"
Write-Output "cmake $hashargs $src_dir"
cmake $hashargs -B $build_dir -S $src_dir
if ($LastExitCode -ne 0) { $host.SetShouldExit($LastExitCode) }
# Build
switch -wildcard ($generator) {
    "Visual Studio*" { cmake --build $build_dir --config Release }
    default { cmake --build $build_dir }
}
if ($LastExitCode -ne 0) { $host.SetShouldExit($LastExitCode) }
# Package
switch -wildcard ($generator) {
    "Visual Studio*" { cmake --build $build_dir --config Release --target package_app }
    default { cmake --build $build_dir --target package_app }
}
if ($LastExitCode -ne 0) { $host.SetShouldExit($LastExitCode) }
