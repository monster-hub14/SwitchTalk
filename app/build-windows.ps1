param(
    [Parameter(Mandatory=$true)][string]$DevkitPro,
    [string]$Python = 'python',
    [string]$OutputDirectory = "$PSScriptRoot/build-windows"
)
$ErrorActionPreference = 'Stop'
$env:DEVKITPRO = (Resolve-Path $DevkitPro).Path.Replace('\','/')
$cc = Join-Path $DevkitPro 'devkitA64/bin/aarch64-none-elf-gcc.exe'
$cxx = Join-Path $DevkitPro 'devkitA64/bin/aarch64-none-elf-g++.exe'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$OutputDirectory = (Resolve-Path $OutputDirectory).Path
$arch = @('-march=armv8-a+crc+crypto','-mtune=cortex-a57','-mtp=soft','-fPIE')
$flags = @('-g','-O2','-std=gnu11','-Wall','-Wextra','-Werror','-ffunction-sections','-ftls-model=local-exec','-D__SWITCH__') + $arch
$flags += @("-I$PSScriptRoot/include", "-I$DevkitPro/libnx/include", "-I$DevkitPro/portlibs/switch/include")
$RtcRoot = (Resolve-Path "$PSScriptRoot/vendor/rtc").Path
$PeerDir = (Get-ChildItem $RtcRoot -Directory -Filter 'libpeer-*' | Select-Object -First 1).FullName
$flags += @("-I$PeerDir/src", "-I$RtcRoot/mbedtls-2.28.10/include", "-Wno-misleading-indentation")
$objects = @()
foreach ($name in @('visibility','json','rich_text','main','talk','tls','messages','network','pcm','mic_input','audio_test','call_state','call_view','gui','server_url','saved_login','avatars','avatar_http','avatar_image','cJSON','call_http','call_ws','call_protocol','call')) {
    $obj = Join-Path $OutputDirectory "$name.o"
    & $cc @flags -c "$PSScriptRoot/source/$name.c" -o $obj
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $name" }
    $objects += $obj
    Write-Output "Compiled $name.c (warnings treated as errors)"
}
$cppflags=@($flags | Where-Object {$_ -notlike '-std=*'})+@('-std=gnu++26','-fno-exceptions','-fno-rtti','-Wno-missing-field-initializers')
& $cxx @cppflags -c "$PSScriptRoot/source/updater.cpp" -o "$OutputDirectory/updater.o"
if($LASTEXITCODE -ne 0){throw 'Updater compile failed'}
$objects += "$OutputDirectory/updater.o"
$elf = Join-Path $OutputDirectory 'SwitchTalk.elf'
& $cxx "-specs=$DevkitPro/libnx/switch.specs" -g @arch "-Wl,-Map,$OutputDirectory/SwitchTalk.map" "-Wl,--wrap=sslConnectionDoHandshake" @objects "$RtcRoot/build/librtc-switch.a" "-L$DevkitPro/portlibs/switch/lib" "-L$DevkitPro/libnx/lib" -lopus -lcurl -lz -lnx -lm -o $elf
if ($LASTEXITCODE -ne 0) { throw 'Link failed' }
Write-Output 'Linked SwitchTalk.elf with devkitA64/libnx/libcurl/zlib'
& "$PSScriptRoot/tools/build_romfs.exe" "$PSScriptRoot/romfs" "$OutputDirectory/SwitchTalk.romfs"
if($LASTEXITCODE -ne 0){throw 'RomFS packaging failed'}
& $Python "$PSScriptRoot/package_nro.py" $elf "$PSScriptRoot/icon.jpg" "$OutputDirectory/SwitchTalk.nro" "$OutputDirectory/SwitchTalk.romfs"
if ($LASTEXITCODE -ne 0) { throw 'NRO packaging failed' }

& $cxx @cppflags -c "$PSScriptRoot/source/launcher.cpp" -o "$OutputDirectory/launcher.o"
if($LASTEXITCODE -ne 0){throw 'Launcher compile failed'}
& $cxx "-specs=$DevkitPro/libnx/switch.specs" -g @arch "-Wl,-Map,$OutputDirectory/Launcher.map" "$OutputDirectory/launcher.o" "-L$DevkitPro/libnx/lib" -lnx -lm -o "$OutputDirectory/Launcher.elf"
if($LASTEXITCODE -ne 0){throw 'Launcher link failed'}
& $Python "$PSScriptRoot/package_nro.py" "$OutputDirectory/Launcher.elf" "$PSScriptRoot/icon.jpg" "$OutputDirectory/Launcher.nro"
if($LASTEXITCODE -ne 0){throw 'Launcher packaging failed'}
