# PowerShell script to watch for file changes and auto-rebuild the plugin DLL

$watchPath = "hr_src", "hr_include"
$buildCommand = "cmake --build build --config Debug --target plug"

Write-Host "Watching for changes in: $($watchPath -join ', ')" -ForegroundColor Green
Write-Host "Press Ctrl+C to stop watching..." -ForegroundColor Yellow

# Create file system watcher
$watcher = New-Object System.IO.FileSystemWatcher
$watcher.Path = $PWD
$watcher.IncludeSubdirectories = $true
$watcher.Filter = "*.*"
$watcher.EnableRaisingEvents = $true

# Debounce timer to avoid multiple builds
$lastBuild = [DateTime]::MinValue
$debounceSeconds = 1

# Define the action on file change
$action = {
    $path = $Event.SourceEventArgs.FullPath
    $changeType = $Event.SourceEventArgs.ChangeType
    
    # Only watch hr_src and hr_include directories
    if ($path -match "\\hr_src\\|\\hr_include\\") {
        $now = [DateTime]::Now
        $global:lastBuild ??= [DateTime]::MinValue
        
        # Debounce: only build if at least 1 second has passed
        if (($now - $global:lastBuild).TotalSeconds -gt 1) {
            $global:lastBuild = $now
            Write-Host "`n[$(Get-Date -Format 'HH:mm:ss')] Change detected: $changeType - $path" -ForegroundColor Cyan
            Write-Host "Building plugin..." -ForegroundColor Yellow
            
            & cmake --build build --config Debug --target plug 2>&1 | ForEach-Object {
                if ($_ -match "error") {
                    Write-Host $_ -ForegroundColor Red
                } elseif ($_ -match "warning") {
                    Write-Host $_ -ForegroundColor Yellow
                } else {
                    Write-Host $_
                }
            }
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host "Build successful! Press 'R' in the app to reload.`n" -ForegroundColor Green
            } else {
                Write-Host "Build failed!`n" -ForegroundColor Red
            }
        }
    }
}

# Register the events
Register-ObjectEvent -InputObject $watcher -EventName Changed -Action $action | Out-Null
Register-ObjectEvent -InputObject $watcher -EventName Created -Action $action | Out-Null
Register-ObjectEvent -InputObject $watcher -EventName Renamed -Action $action | Out-Null

Write-Host "`nWatcher started. Edit files in hr_src/ or hr_include/ to trigger auto-build.`n" -ForegroundColor Green

# Keep script running
try {
    while ($true) {
        Start-Sleep -Seconds 1
    }
} finally {
    # Cleanup on exit
    $watcher.Dispose()
    Get-EventSubscriber | Unregister-Event
}
