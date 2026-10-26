$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
    $qtRoot = Join-Path $projectRoot '.build-deps/Qt/6.8.3/mingw_64'
    $compilerBin = Join-Path $projectRoot '.build-deps/Qt/Tools/mingw1310_64/bin'
    $env:PATH = "$compilerBin;$qtRoot/bin;$env:PATH"
    if (!(Test-Path "$compilerBin/g++.exe")) { throw 'Missing local Qt/MinGW dependencies in .build-deps' }
    cmake --build build_interface --target irisemulator test_jaguar test_jaguar_input test_frame_pacer -j 6
    if ($LASTEXITCODE) { throw 'Compilation failed' }
    ctest --test-dir build_interface --output-on-failure
    if ($LASTEXITCODE) { throw 'Jaguar regression tests failed' }
    & "$qtRoot/bin/windeployqt.exe" --release --no-translations --compiler-runtime build_interface/irisemulator.exe
    if ($LASTEXITCODE) { throw 'Qt deployment failed' }
    Copy-Item -LiteralPath 'C:/tools/SDL2-2.30.9/x86_64-w64-mingw32/bin/SDL2.dll' -Destination build_interface/SDL2.dll
    Copy-Item -LiteralPath 'build_release6/Release/discord-rpc.dll' -Destination build_interface/discord-rpc.dll
    Write-Output "Ready: $projectRoot/build_interface/irisemulator.exe"
} finally {
    Pop-Location
}
