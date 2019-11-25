$ErrorActionPreference = 'Continue'

if ((Test-Path 'Env:\WINDOWS_CERT_PW') -and (Test-Path 'Env:\WINDOWS_CERT')) {
    $certstore= "Cert:\CurrentUser\My"

    $certpfx = New-TemporaryFile

    # recreate certificate pfx file from the correcponding base64 encoded appveyor secure variable
    [IO.File]::WriteAllBytes($certpfx, [Convert]::FromBase64String($env:WINDOWS_CERT))

    # create a securestring holding the certificate password from the corresponding appveyor secure variable
    $certpw = ConvertTo-SecureString -String $env:WINDOWS_CERT_PW -AsPlainText -Force

    # import our certificate
    $certificate = Import-PfxCertificate -Password $certpw -CertStoreLocation $certstore -FilePath $certpfx

    # delete the certificate pfx file
    Remove-Item -LiteralPath $certpfx

    Write-Host ''
    Write-Host 'Using code signing certificate:'
    Write-Host ($certificate | Format-List | Out-String)
    Write-Host ''

    return $certificate
} else {
    Write-Host ''
    Write-Host 'No code signing certificate available.'
    Write-Host 'Code will not be signed.'
    Write-Host ''
    return $null
}
