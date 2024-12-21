param(
    [bool]$Debug = $false
)
$DebugFlag = $null
if ($Debug) {
    $DebugFlag = "-g"
}

$src = "./main.cpp ./src/*.cpp"
$Iinclude = "./include"
$exe = "main.exe"


& clang++.exe $src -I $Iinclude $DebugFlag -D_UNICODE -DUNICODE -o ./bin/$exe -std=c++17

if ($? -and -not $Debug) {
   & .\bin\$exe
}