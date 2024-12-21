param(
    [bool]$Debug = $false
)
$DebugFlag = $null
if ($Debug) {
    $DebugFlag = "-g"
}

$src = "./main.cpp ./src/*.cpp"
$Iinclude = "-I ./include ./lib/include"
$exe = "main.exe"


& clang++.exe $src $Iinclude $DebugFlag -D_UNICODE -DUNICODE -o ./bin/$exe -std=c++17

if ($? -and -not $Debug) {
   & .\bin\$exe
}