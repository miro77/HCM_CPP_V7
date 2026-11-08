# Ruft HCMMS_V7_DLL der Referenz-DLL fuer jede Zeile der Fall-Datei (SPEC-003)
# auf und schreibt je Fall eine Ergebniszeile (SPEC-004). Alle Faelle laufen in
# einem Prozess nacheinander; der Zustand der DLL wirkt von Fall zu Fall nach.
# ${FX} in Feldwerten wird durch -FixtureDir ersetzt.
# Run with a PowerShell matching the DLL bitness:
#   32 bit: C:\Windows\SysWOW64\WindowsPowerShell\v1.0\powershell.exe
#   64 bit: pwsh / C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe
param(
    [Parameter(Mandatory)] [string] $DllPath,
    [Parameter(Mandatory)] [string] $CasesFile,
    [Parameter(Mandatory)] [string] $FixtureDir
)
$ErrorActionPreference = 'Stop'
$DllPath = (Resolve-Path $DllPath).Path
$cls = 'Hcmms_' + [Guid]::NewGuid().ToString('N')
$dllLiteral = $DllPath.Replace('\', '\\')

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class $cls {
    // 32 bit: __stdcall (CVF / gfortran STDCALL). 64 bit: the only x64 convention.
    // The hidden length of I_str follows I_str (CVF: int32, gfortran: size_t) -> IntPtr.
    [DllImport("$dllLiteral", EntryPoint = "HCMMS_V7_DLL", CallingConvention = CallingConvention.StdCall)]
    public static extern void Call(
        ref int C_mode, ref int bor_dis, ref double PD, ref double Distance,
        ref short H_Datab_Tx, ref short H_Datab_Rx, ref int HCM_error,
        ref float Heff, ref float Dh, ref float Dh_corr, ref float Power_to_Rx,
        ref float Free_space_FS, ref float Land_FS, ref float Sea_FS,
        ref float Tx_ant_corr, ref float Tx_ant_type_corr,
        ref double Dir_Tx_Rx, ref double V_angle_Tx_Rx,
        ref float Tx_TCA, ref float Rx_TCA, ref float Tx_TCA_corr, ref float Rx_TCA_corr,
        ref double D_sea_calculated, ref float Rx_ant_corr, ref float Rx_ant_type_corr,
        ref double Delta_frequency, ref float Corr_delta_f,
        ref float Calculated_FS, ref float Perm_FS, ref float CBR_D,
        ref float ERP_ref_Tx, ref float Prot_margin,
        byte[] I_str, IntPtr I_str_len);
}
"@

$inv = [Globalization.CultureInfo]::InvariantCulture
function Pad([string]$s, [int]$n) { if ($s.Length -gt $n) { $s.Substring(0, $n) } else { $s.PadRight($n) } }

foreach ($line in Get-Content $CasesFile) {
    if ($line -match '^\s*(#|$)') { continue }
    # name | C_mode | bor_dis | PD | field=value;field=value...
    $p = $line.Split('|')
    $name = $p[0].Trim(); $cmode = [int]$p[1]; $bord = [int]$p[2]; $pd = [double]::Parse($p[3], $inv)

    # I_str layout (see HCMMS_V7_DLL.F90): start position (1-based) and length
    $layout = [ordered]@{
        Coo_Tx=1,15; Coo_Rx=16,15; H_Tx=31,4; H_Rx=35,4; AntH_Tx=39,7; AntV_Tx=46,7
        Azi_Tx=53,5; Ele_Tx=58,5; Hant_Tx=63,4; Hant_Rx=67,4; AntType_Tx=71,1; Power=72,6
        Tx_f=78,12; ChanOcc=90,1; SeaT=91,1; Rad_Tx=92,5; Rad_Rx=97,5; Dsea=102,5
        Rx_f=107,12; Emis_Rx=119,9; Emis_Tx=128,9; AntH_Rx=137,7; AntV_Rx=144,7
        Azi_Rx=151,5; Ele_Rx=156,5; AntType_Rx=161,1; Gain_Rx=162,4; Depol=166,4
        PermFS=170,5; CorDf=175,4; Land_to=179,3; Land_from=182,3; MaxCBR=185,3
        Topo=188,63; Border=251,63; Morpho=314,63
    }
    $buf = (' ' * 600).ToCharArray()
    foreach ($kv in $p[4].Split(';')) {
        if (-not $kv.Contains('=')) { continue }
        $k, $v = $kv.Split('=', 2); $v = $v.Replace('${FX}', $FixtureDir)
        if (-not $layout.Contains($k.Trim())) { throw "Unbekanntes Feld '$k' in Fall $name" }
        $pos, $len = $layout[$k.Trim()]
        (Pad $v $len).ToCharArray().CopyTo($buf, $pos - 1)
    }
    $bytes = [Text.Encoding]::ASCII.GetBytes([string]::new($buf))

    $err = 0; $hTx = [int16]0; $hRx = [int16]0; $dist = 0.0
    $dir = 0.0; $vang = 0.0; $dsea = 0.0; $df = 0.0     # DOUBLE PRECISION outputs
    [float]$heff=0;[float]$dh=0;[float]$dhc=0;[float]$prx=0;[float]$fs0=0;[float]$lfs=0;[float]$sfs=0
    [float]$tac=0;[float]$tatc=0;[float]$ttca=0;[float]$rtca=0;[float]$ttcac=0;[float]$rtcac=0
    [float]$rac=0;[float]$ratc=0;[float]$cdf=0;[float]$cfs=0;[float]$pfs=0;[float]$cbr=0;[float]$erp=0;[float]$pm=0

    $type = $cls -as [type]
    $type::Call([ref]$cmode, [ref]$bord, [ref]$pd, [ref]$dist, [ref]$hTx, [ref]$hRx, [ref]$err,
        [ref]$heff, [ref]$dh, [ref]$dhc, [ref]$prx, [ref]$fs0, [ref]$lfs, [ref]$sfs,
        [ref]$tac, [ref]$tatc, [ref]$dir, [ref]$vang, [ref]$ttca, [ref]$rtca, [ref]$ttcac, [ref]$rtcac,
        [ref]$dsea, [ref]$rac, [ref]$ratc, [ref]$df, [ref]$cdf, [ref]$cfs, [ref]$pfs, [ref]$cbr,
        [ref]$erp, [ref]$pm, $bytes, [IntPtr]$bytes.Length)

    $s = [Text.Encoding]::ASCII.GetString($bytes)
    $vals = @($err, $dist, $hTx, $hRx, $heff, $dh, $dhc, $prx, $fs0, $lfs, $sfs, $tac, $tatc, $dir, $vang,
              $ttca, $rtca, $ttcac, $rtcac, $dsea, $rac, $ratc, $df, $cdf, $cfs, $pfs, $cbr, $erp, $pm) |
            ForEach-Object { if ($_ -is [double] -or $_ -is [float]) { $_.ToString('R', $inv) } else { "$_" } }
    "{0}|err={1}|{2}|ver='{3}'|txnew='{4}'|rxnew='{5}'|info='{6}'|depol='{7}'|seat='{8}'" -f $name, $err, ($vals -join ' '),
        $s.Substring(376, 6), $s.Substring(402, 15), $s.Substring(417, 15), $s.Substring(382, 20),
        $s.Substring(165, 4), $s.Substring(90, 1)
}
