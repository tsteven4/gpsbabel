Param(
  [Parameter(Mandatory=$true,
    Position=0,
    ValueFromPipeline=$true)]
    $thumbprint,
  
  [Parameter(Position=1)]
  $certstore = 'Cert:\CurrentUser\My'
)

$ErrorActionPreference = 'Stop'

# remove our certificate from the certificate store.
Get-ChildItem -Path $certstore | Where-Object -Property Thumbprint -eq $thumbprint | Remove-Item
