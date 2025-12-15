$libDir = "E:\book\Game_pro\Game\raylib\lib"
Write-Host "Listing *.lib under:$libDir"
Get-ChildItem -Path $libDir -Recurse -Filter *.lib | ForEach-Object { Write-Host $_.FullName }
Write-Host "Exists raylib.lib? " (Test-Path (Join-Path $libDir "raylib.lib"))