param(
    [bool]$Debug = $false
)
$DebugFlag = $null
if ($Debug) {
    $DebugFlag = "-g"
}

$src = "./*.cpp"
$include = "./include"
$exe = "main.exe"


& clang++.exe $src -I $include $DebugFlag -D_UNICODE -DUNICODE -o ./bin/$exe -std=c++17

if ($? -and -not $Debug) {
   & .\bin\$exe
}