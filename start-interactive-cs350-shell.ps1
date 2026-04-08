$ErrorActionPreference = "Stop"

$hostDir = (Resolve-Path "$PSScriptRoot").Path

# Docker accepts forward slashes in Windows bind mount paths.
$dockerPath = $hostDir -replace "\\", "/"

Write-Host "Mounting: $hostDir -> /root/cs350-os161"

docker run --volume "${dockerPath}:/root/cs350-os161" --interactive --tty uberi/cs350:latest bash
