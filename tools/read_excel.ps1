$excel = New-Object -ComObject Excel.Application
$excel.Visible = $false
$excel.DisplayAlerts = $false
$workbook = $excel.Workbooks.Open("C:\SSL96\ADB96_LED_IC_Mapping_Netlist.xlsx")
$worksheet = $workbook.Worksheets.Item(1)
$csvPath = "$env:TEMP\mapping123.csv"
if (Test-Path $csvPath) { Remove-Item $csvPath -Force }
$worksheet.SaveAs($csvPath, 6)
$workbook.Close($false)
$excel.Quit()
[System.Runtime.Interopservices.Marshal]::ReleaseComObject($excel) | Out-Null

Get-Content $csvPath
