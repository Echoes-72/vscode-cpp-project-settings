param(
    [bool]$Debug = $false
)
$DebugFlag = $null
if ($Debug) {
    $DebugFlag = "-g"
}

$srcs = @(
    ".\main.cpp"
)

$include = "./include"
$exe = "main.exe"

foreach ($src in $srcs) {
    $output = ($src -split "[\\.]")[-2]
    & clang++.exe $src -I $include $DebugFlag -D_UNICODE -DUNICODE -c -o ./bin/$output.o -std=c++17
}

& clang++.exe .\bin\*.o .\bin\resource.res $DebugFlag -o .\bin\$exe 

if ($? -and -not $Debug) {
    & .\bin\$exe
}