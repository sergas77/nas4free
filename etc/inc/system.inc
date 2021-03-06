<?php
/*
	system.inc

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (c) 2012-2017 The NAS4Free Project <info@nas4free.org>.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	The views and conclusions contained in the software and documentation are those
	of the authors and should not be interpreted as representing official policies,
	either expressed or implied, of the NAS4Free Project.
*/
require_once 'functions.inc';
require_once 'util.inc';
require_once 'rc.inc';

function system_reboot() {
	// Initiate halt. Everything will be done automatically
	// in /etc/rc.shutdown by executing rc-init scripts in
	// reverse order (the 'KEYWORD: shutdown' must be defined).
	mwexec("/sbin/shutdown -r now");
}

function system_halt() {
	// Initiate halt. Everything will be done automatically
	// in /etc/rc.shutdown by executing rc-init scripts in
	// reverse order (the 'KEYWORD: shutdown' must be defined).
	mwexec("/sbin/shutdown -p now");
}

/* Init language environment */
function system_language_load()
{
	global $config, $g_languages;

	/* Get the language configured. */
	$language = $config['system']['language'];
	$domain = strtolower(get_product_name());
	$codeset = $g_languages[$language]['codeset'];

	putenv("LANG=$language");
	setlocale(LC_MESSAGES, $language);
	bindtextdomain($domain, "/usr/local/share/locale");
	bind_textdomain_codeset($domain, $codeset);
	textdomain($domain);
}

/* Get the codeset of the current configured language. */
/* Return: String containing codeset of current laguage. */
function system_get_language_codeset() {
	global $config, $g_languages;

	$language = $config['system']['language'];
	$codeset = $g_languages[$language]['codeset'];

	if (empty($codeset))
		$codeset = "UTF-8"; // Set default codeset.

	return $codeset;
}

/* Get the entity of LanguageCode of the current configured language. */
/* Return: String containing language code of current laguage. */
function system_get_language_code() {
	global $config, $g_languages;

	// a language code, as per [RFC3066]
	$language = $config['system']['language'];
	//$code = $g_languages[$language]['code'];
	$code = str_replace("_", "-", $language);

	if (empty($code))
		$code = "en-US"; // Set default code.

	return $code;
}

// Get list of available groups from /etc/group.
// Result:
//	Array (
//		[wheel] => 0
//		[sshd] => 22
//		[www] => 80
//		[nobody] => 65534
//		[admin] => 1000
//		...
//	)
function system_get_group_list() {
	// List of groups to filter from result list.
	$filterlist = ['_dhcp','_pflogd'];

	$grouplist = [];

	preg_match_all("/(\S+):\*:(\d+):.*\n/", @file_get_contents("/etc/group"), $matches, PREG_SET_ORDER);

	if (is_array($matches)) {
		foreach ($matches as $group) {
			if (false === in_array($group[1], $filterlist)) {
				$grouplist[$group[1]] = $group[2];
			}
		}
		ksort($grouplist);
	}
	return $grouplist;
}

// Get list of available users from /etc/master.passwd.
// Result:
//  Array (
//    [test] => Array ( [name] => test
//                      [password] => $1$yuQLaTPN$lkwYlZEB7B8n85flXPkHd0
//                      [uid] => 1001
//                      [gid] => 1001
//                      [class] =>
//                      [change] => 0
//                      [expire] => 0
//                      [gecos] => test
//                      [home_dir] => /mnt
//                      [shell] => /usr/local/bin/scponly
//                    )
//    [root] => ...
//    [toor] => ..
//    [daemon] => ...
//    [operator] => ...
//    ...
//  )
function system_get_user_list() {
	$userlist = [];

	foreach (explode("\n", @file_get_contents("/etc/master.passwd")) as $userinfov) {
		if (empty($userinfov))
			continue;

		// Extract user information
		$userinfo = explode(":", $userinfov);
		$user = [];
		$user['name'] = $userinfo[0];
		$user['password'] = $userinfo[1];
		$user['uid'] = $userinfo[2];
		$user['gid'] = $userinfo[3];
		$user['class'] = $userinfo[4];
		$user['change'] = $userinfo[5];
		$user['expire'] = $userinfo[6];
		$user['gecos'] = $userinfo[7];
		$user['home_dir'] = $userinfo[8];
		$user['shell'] = $userinfo[9];
		$userlist[$user['name']] = $user;
	}
	return $userlist;
}

// SMBIOS get various info
function get_smbios_info() {
	$smbios = [];
	$cmd = "/bin/kenv -q";
	unset($rawdata, $rc);
	mwexec2($cmd, $rawdata, $rc);
	if ($rc != 0) {
		return $smbios;
	}
	foreach ($rawdata as $line) {
		if (preg_match("/^smbios\.([^\.=]+)\.([^\.=]+)=\"(.*)\"/", $line, $match)) {
			$smbios[$match[1]][$match[2]] = $match[3];
		}
	}
	$platform = get_platform_type();
	if (preg_match('/^rpi-/', $platform) || preg_match('/^rpi2-/', $platform)) {
		// fake data on RPi
		$smbios['bios']['reldate'] = "12/01/2014";
		$smbios['bios']['vendor'] = "Raspberry Pi";
		$smbios['bios']['version'] = "1.0";
		$smbios['planar']['maker'] = "Raspberry Pi Foundation";
		$smbios['planar']['product'] = "Raspberry Pi";

		$revision = chop(`/sbin/sysctl -n hw.board.revision`);
		$scheme = ($revision & 0x800000) >> 23;
		$ram = ($revision & 0x700000) >> 20;
		$revision &= 0xffff;
		$model = "";
		if (!empty($revision) && $scheme == 0) {
			if ($revision <= 0x0006)
				$model = "Model B (256MB)";
			else if ($revision >= 0x0007 && $revision <= 0x0009)
				$model = "Model A";
			else if ($revision >= 0x000d && $revision <= 0x000f)
				$model = "Model B";
			else if ($revision == 0x0010 || $revision == 0x0013)
				$model = "Model B+";
			else if ($revision == 0x0011 || $revision == 0x0014)
				$model = "Compute Module";
			else if ($revision == 0x0012)
				$model = "Model A+";
			else
				$model = sprintf("rev. 0x%04x", $revision);
		} else if (!empty($revision) && $scheme != 0) {
			$processor = ($revision & 0xf000) >> 12;
			$type = ($revision & 0x0ff0) >> 4;
			if ($type == 0)
				$model = "Model A";
			else if ($type == 1)
				$model = "Model B";
			else if ($type == 2)
				$model = "Model A+";
			else if ($type == 3)
				$model = "Model B+";
			else if ($type == 4)
				$model = "2 Model B";
			else if ($type == 5)
				$model = "Alpha";
			else if ($type == 6)
				$model = "Compute Module";
			else
				$model = sprintf("rev. 0x%04x", $revision);
			if (!empty($model)) {
				if ($ram == 0)
					$model .= " (256MB)";
			}
		}
		if (!empty($model))
			$smbios['planar']['product'] .= " $model";
		$serial = chop(`/sbin/sysctl -n hw.board.serial`);
		$serial = preg_replace('/^0+/', '', $serial);
		if (!empty($serial))
			$smbios['planar']['product'] .= " (S/N: $serial)";
	} else if (preg_match('/^oc1-/', $platform)) {
		// fake data on ODROID-C1
		$smbios['bios']['reldate'] = "01/01/2015";
		$smbios['bios']['vendor'] = "ODROID-C1";
		$smbios['bios']['version'] = "1.0";
		$smbios['planar']['maker'] = "Hardkernel";
		$smbios['planar']['product'] = "ODROID-C1";
		$serial = chop(`/sbin/sysctl -n hw.board.serial`);
		if (!empty($serial))
			$smbios['planar']['product'] .= " (S/N: $serial)";
	}   else if (preg_match('/^bpi-/', $platform)) {
		// fake data on BANANA-PI
		$smbios['bios']['reldate'] = "01/01/2015";
		$smbios['bios']['vendor'] = "BANANA-PI";
		$smbios['bios']['version'] = "1.0";
		$smbios['planar']['maker'] = "Sinovoip Co";
		$smbios['planar']['product'] = chop(`sysctl -n dev.cpu.0.%pnpinfo | awk -F "," '{ print $2; }'`);
		$serial = chop(`/sbin/sysctl -n hw.board.revision`);
		if (!empty($serial))
			$smbios['planar']['product'] .= " (S/N: $serial)";
	} 	else if ($smbios['bios']['vendor'] == 'BHYVE') {
		$smbios['bios']['reldate'] = "01/01/2015";
		$smbios['bios']['vendor'] = "BHyVe";
		$smbios['bios']['version'] = "1.0";
		$smbios['planar']['maker'] = "";
		$smbios['planar']['product'] = "BHyVe";
	}
	return $smbios;
}

// Get number of CPUs
function system_get_cpus() {
	$cpus = @exec("/sbin/sysctl -q -n kern.smp.cpus");
	if ($cpus == '' || $cpus < 1) $cpus = 1;
	return $cpus;
}

// Get current CPU usage.
// Return current CPU usage in percent.
function cb_map_array_diff2($a, $b) {
	return $b - $a;
}
function system_get_smp_cpu_usage() {
	$cpus = system_get_cpus();
	$a_stat1 = explode(" ", @exec("/sbin/sysctl -q -n kern.cp_times"));
	sleep(1);
	$a_stat2 = explode(" ", @exec("/sbin/sysctl -q -n kern.cp_times"));
	$a_dstat = array_map("cb_map_array_diff2", $a_stat1, $a_stat2);
	$a_usage = array_fill_keys(range(0, $cpus - 1), 0);
	for ($i = 0; $i < $cpus; $i++) {
		// user, nice, system, interrupt, idle
		$a = array_splice($a_dstat, 0, 5);
		$idle = $a[4];
		$total = array_sum($a);
		$a_usage[$i] = floor(100 * (($total - $idle) / $total));
	}
	return $a_usage;
}

function system_get_cpu_usage() {
	$cpuTicks1 = explode(" ", `/sbin/sysctl -n kern.cp_time`);
	sleep(1);
	$cpuTicks2 = explode(" ", `/sbin/sysctl -n kern.cp_time`);

	$diff = [];
	$diff['user'] = ($cpuTicks2[0] - $cpuTicks1[0]);
	$diff['nice'] = ($cpuTicks2[1] - $cpuTicks1[1]);
	$diff['sys'] = ($cpuTicks2[2] - $cpuTicks1[2]);
	$diff['intr'] = ($cpuTicks2[3] - $cpuTicks1[3]);
	$diff['idle'] = ($cpuTicks2[4] - $cpuTicks1[4]);
	$totalDiff = $diff['user'] + $diff['nice'] + $diff['sys'] + $diff['intr'] + $diff['idle'];
	$totalused = $diff['user'] + $diff['nice'] + $diff['sys'] + $diff['intr'];

	if (isset($totalused) && $totalused <= 0) {
		$totalused = 0.001;
	}

	return floor(100 * ($totalused / $totalDiff));
}

// Get CPUID
function system_get_cpuid($cpuidx, $level) {
	$r = explode(" ", @exec("/usr/sbin/cpucontrol -i $level /dev/cpuctl${cpuidx} | awk '{ print $4, $5, $6, $7 }'"));
	$a = [];
	$a['eax'] = $r[0];
	$a['ebx'] = $r[1];
	$a['ecx'] = $r[2];
	$a['edx'] = $r[3];
	return $a;
}

// Get various CPU informations.
// Result:
//  Array (
//    [number] => 1
//    [model] => AMD Athlon(tm) 64 Processor 3200+
//    [clockrate] => 1994
//    [temperature] => 65C !!! May be empty if sensor is not available. !!!
//    [freq] => 995
//    [freqlevels] => 1990/67000 1791/64700 995/28600
//  )
function system_get_cpu_info() {
	$cpuinfo = [];
	$cpuinfo['number'] = chop(`/sbin/sysctl -n hw.ncpu`);
	$cpuinfo['model'] = chop(`/sbin/sysctl -n hw.model`);
	if (empty($cpuinfo['model'])) {
		$platform = chop(`/sbin/sysctl -n hw.platform`);
		if ($platform == "bcm2835")
			$cpuinfo['model'] = "Broadcom BCM2835";
		else if ($platform == "bcm2836")
			$cpuinfo['model'] = "Broadcom BCM2836";
		else if ($platform == "meson8b")
			$cpuinfo['model'] = "Amlogic S805";
		else if ($platform == "a20")
			$cpuinfo['model'] = "Allwinner a20";
		else
			$cpuinfo['model'] = $platform;
	}
	$cpuinfo['clockrate'] = chop(`/sbin/sysctl -n hw.clockrate`);
	if (chop(`/sbin/sysctl -n hw.platform`) == 'a20') $cpuinfo['temperature'] = chop(`/sbin/sysctl -n dev.axp209_pmu.0.temp`);
	else $cpuinfo['temperature'] = chop(`/sbin/sysctl -n hw.acpi.thermal.tz0.temperature`);
	$cpus = system_get_cpus();
	$cpuinfo['temperature2'] = [];
	$temp = @exec("/sbin/sysctl -q -n dev.cpu.0.temperature");
	if ($temp != '' && $temp != '-1') {
		$adj = 0;
		$cpuid = system_get_cpuid(0, "0x0");
		for ($i = 0; $i < $cpus; $i++) {
			$temp = @exec("/sbin/sysctl -q -n dev.cpu.{$i}.temperature");
			if ($temp != '') {
				$temp = sprintf("%.1f", ($temp + $adj));
			}
			$cpuinfo['temperature2'][$i] = $temp;
		}
	}
	$cpuinfo['freq'] = chop(`/sbin/sysctl -n dev.cpu.0.freq`);
	$cpuinfo['freqlevels'] = chop(`/sbin/sysctl -n dev.cpu.0.freq_levels`);

	return $cpuinfo;
}

/**
 * Get the system uptime (how long the system is already running).
 * @return The current uptime as string.
 */
function system_get_uptime() {
	exec("/sbin/sysctl -n kern.boottime", $boottime);
	preg_match("/sec = (\d+)/", $boottime[0], $matches);
	$boottime = $matches[1];
	$uptime = time() - $boottime;

	if ($uptime > 60) $uptime += 30;
	$updays = (int)($uptime / 86400);
	$uptime %= 86400;
	$uphours = (int)($uptime / 3600);
	$uptime %= 3600;
	$upmins = (int)($uptime / 60);
	$upsecs = $uptime % 60;

	$gt_seconds = (1 === $upsecs) ? gtext('Second') : gtext('Seconds');
	$gt_minutes = (1 === $upmins) ? gtext('Minute') : gtext('Minutes');
	$gt_hours = (1 === $uphours) ? gtext('Hour') : gtext('Hours');
	$gt_days = (1 === $updays) ? gtext('Day') : gtext('Days');

	if(($updays < 1) && ($uphours < 1)):
		$uptime = sprintf('%d %s %d %s',$upmins,$gt_minutes,$upsecs,$gt_seconds);
	elseif($updays < 1):
		$uptime = sprintf('%d %s %d %s %d %s',$uphours,$gt_hours,$upmins,$gt_minutes,$upsecs,$gt_seconds);
	else:
		$uptime = sprintf('%d %s %d %s %d %s %d %s',$updays,$gt_days,$uphours,$gt_hours,$upmins,$gt_minutes,$upsecs,$gt_seconds);
	endif;
	return $uptime;
}

/**
 * Get the system date/time for conversion strings.
 * @return The current uptime as string.
 */
function get_datetime_locale_samples() {
	$retval = [];
	$sphere_array = [
		'D M d H:i:s T Y',    // Thu Feb 25 22:52:29 CET 2016
		'D d M Y H:i:s',      // Thu 25 Feb 2016 22:52:29
		'D d M Y H:i:s T',    // Thu 25 Feb 2016 22:52:29 CET
		'D d M Y H:i:s T O',  // Thu 25 Feb 2016 22:52:29 CET +0100
		'l F d H:i:s T Y',    // Thursday February 25 22:52:29 BST 2016
		'l d F Y H:i:s',      // Thursday 25 February 2016 22:52:29
		'l d F Y H:i:s T',    // Thursday 25 February 2016 22:52:29 BST
		'l d F Y H:i:s T O',  // Thursday 25 February 2016 22:52:29 BST +0100
		'D M d h:i:s A T Y',  // Thu Feb 25 10:52:29 PM PT 2016
		'l F d h:i:s A T Y',  // Thursday February 25 10:52:29 PM BST 2016
		'd.m.Y H:i:s',        // 25.02.2016 22:31:42
		'd/m/Y H:i:s',        // 25/02/2016 22:31:42
		'd/m/Y h:i:s A',      // 25/02/2016 10:31:42 PM
		'm/d/Y H:i:s',        // 02/25/2016 22:31:42
		'm/d/Y h:i:s A',      // 02/25/2016 10:31:42 PM
		'Y-m-d H:i:s'         // 2016-02-02 22:31:42
	];
	$now = shell_exec('date "+%s"');
	foreach ($sphere_array as $sphere_record) {
		$retval[$sphere_record] = get_datetime_locale($now, $sphere_record);
	}
	return $retval;
}

function get_datetime_locale($timestamp = null, $format = null) {
	global $config;
	$retval = '';
	$timestamp = (int)(($timestamp === null) ? shell_exec('date "+%s"') : $timestamp);
	$format = ($format === null) ? (isset($config['system']['datetimeformat']) ? $config['system']['datetimeformat'] : 'D M d H:i:s T Y') : $format;
	$datetime_object = new DateTime();
	$datetime_object->setTimestamp($timestamp);
	$format_symbols = ['D','l','M','F'];
	$replacement_symbols = ['%1','%2','%3','%4'];
	$localised_names = [
		gtext($datetime_object->format($format_symbols[0])),
		gtext($datetime_object->format($format_symbols[1])),
		gtext($datetime_object->format($format_symbols[2])),
		gtext($datetime_object->format($format_symbols[3]))
	];
	$retval = str_replace($replacement_symbols, $localised_names, $datetime_object->format(str_replace($format_symbols, $replacement_symbols, $format))); // Do the whole thing in one go
	return $retval;
}

// Get the current RAM information.
// Returns an array listing the amount of memory installed in the hardware.
function system_get_ram_info() {
	exec("/sbin/sysctl -n vm.stats.vm.v_active_count vm.stats.vm.v_inactive_count vm.stats.vm.v_wire_count vm.stats.vm.v_cache_count vm.stats.vm.v_free_count hw.physmem", $memory);
	exec("/sbin/sysctl -n hw.realmem", $hwmemory);

	$raminfo = [];
	$raminfo['real'] = $hwmemory[0];
	$raminfo['physical'] = $memory[5];
	$raminfo['total'] = $memory[0] + $memory[1] + $memory[2] + $memory[3] + $memory[4];
	$raminfo['free'] = $memory[4] + $memory[1];
	$raminfo['used'] = $raminfo['total'] - $raminfo['free'];
	return $raminfo;
}

// Get the current swap information.
// Result:
//  Array (
//    [device] => "/dev/ad0s2b"
//    [total] => "20M"
//    [used] => "0B"
//    [avail] => "20M"
//    [capacity] => "0%"
//  )
function system_get_swap_info() {
	exec("/usr/sbin/swapinfo -k", $swap);

	// blocks
	$val = 512;
	if (preg_match("/(?:Device)(?:\s+)(\d+)(?:K-blocks|-blocks)(?:\s+)(?:Used)(?:\s+)(?:Avail)(?:\s)(?:Capacity)/", $swap[0], $matches)) {
		if ($matches[1] == 1) {
			$val = $matches[1] * 1024;
		} else {
			$val = $matches[1];
		}
	}
	array_shift($swap);
	$swapinfo = [];

	foreach ($swap as $swapv) {
		if (preg_match("/(\/dev\/.*)(?:\s+)(\d+)(?:\s+)(\d+)(?:\s+)(\d+)(?:\s)(.+)/", $swapv, $matches)) {
			$devswap = [];
			$devswap['device'] = trim($matches[1]);
			$devswap['total'] = decode_size(trim($matches[2])*$val);
			$devswap['used'] = decode_size(trim($matches[3])*$val);
			$devswap['avail'] = decode_size(trim($matches[4])*$val);
			$devswap['capacity'] = trim($matches[5]);

			$swapinfo[] = $devswap;
		}
	}
	return $swapinfo;
}

/**
 * Get the system hostname.
 * @return The hostname
 */
function system_get_hostname() {
	return @exec("hostname");
}

/**
 * Get device I/O statistics.
 * @param[in] device	The device name, e.g. ad1
 * @return	An array containing the I/O statistics kpt (kilobytes per transfer),
 *					tps (transfers per second) and mps (megabytes per second). On
 *					failure, FALSE will be returned.
 * @code
 * array(
 *   [kpt] => 11.93
 *   [tps] => 61
 *   [mps] => 0.71)
 * @endcode
 */
function system_get_device_iostat($device) {
	$result = FALSE;

	mwexec2("iostat -d {$device}", $rawdata);
	if (preg_match("/^\s*(\S+)\s*(\S+)\s*(\S+)/", $rawdata[2], $matches)) {
		$result = [];
		$result['kpt'] = $matches[1];
		$result['tps'] = $matches[2];
		$result['mps'] = $matches[3];
	}
	return $result;
}

/**
 * Get device temperature.
 * @param[in] device	The device name, e.g. ad1
 * @return	The temperature in C if available, otherwise FALSE.
 */
function system_get_device_temp($devicespecialfile) {
	$result = FALSE;


	/* without smart */
	mwexec2("ataidle {$devicespecialfile} 2>/dev/null | grep 'SMART Enabled'", $rawdata);
	if (!empty($rawdata) && 1 === preg_match("/^SMART Enabled:\s+(\S+)/", $rawdata[0], $matches)) {
		if (strcasecmp($matches[1], "no") == 0) {
			/* SMART disabled */
			return $result;
		}
	}
	global $config;
	// get S.M.A.R.T. arguments from config
	global $config_disks;

	if(empty($_SESSION[$devicespecialfile]['temp']['value']) or
		time() - $_SESSION[$devicespecialfile]['temp']['timestamp'] > 120
		){

		$type_arg = '';

		if( !empty($config_disks[$devicespecialfile]['smart']['devicetypearg']) ){
			$type_arg = "-d ".$config_disks[$devicespecialfile]['smart']['devicetypearg'];
		}

		/* get temperature from S.M.A.R.T. */
		mwexec2("smartctl -A ".$type_arg." ".$config_disks[$devicespecialfile]['smart']['devicefilepath']."| grep -i 'Temperature'" , $rawdata);

		foreach ($rawdata as $rawdatav) {
			$arawdatav = preg_split("/\s+/", $rawdatav);

			if (!empty($arawdatav[1]) && ((0 == strncmp(trim($arawdatav[1]), "Temperature_", 12)) || (0 == strncmp(trim($arawdatav[1]), "Airflow_Temperature_", 20))) &&
				((0 != strcmp(trim($arawdatav[0]), "190")) || (0 != strcmp(trim($arawdatav[0]), "194")) )) {
				$result = chop($arawdatav[9]);
				break;
			} else if (preg_match("/Current Drive Temperature:\s+(\d+) C.*/", $rawdatav, $matches)) {
				$result = $matches[1];
				break;
			}
		}
		$_SESSION[$devicespecialfile]['temp']['value']=$result;
		$_SESSION[$devicespecialfile]['temp']['timestamp']=time();
		return $result;
	} else {
		return $_SESSION[$devicespecialfile]['temp']['value'];
	}
	return $result;
}

/**
 * Get controller
 * @param[in] device	The device name, e.g. /dev/ad1
 * @return The disk device serial number.
 */
function system_get_controller($devicespecialfile) {
	$devlist = [];
	$device = '';
	$controller = '';
	$controller_id = ''; 
	$target_id = '';
	$lun_id = '';

	mwexec2("/sbin/camcontrol devlist -v 2> /dev/null", $rawdata);

	foreach($rawdata as $line) {
		// Get controller
		if( preg_match("/(?<= on )[-A-z0-9]*(?= bus )+/", $line, $matches)){
			$controller = preg_split( '/([-A-z]+)/', $matches[0], -1, PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_NO_EMPTY);
			$device = '';
		}
		else if ( !strncmp($line, '<>' ,2) == 0){

			// Get device name
			// Parse substring between parentheses: (pass0,da0) or (da0,pass0)
			preg_match("/.*\(([^>]*)\).*/",$line,$match);
			$temp = preg_split("/,/", $match[1]);

			// Check whether device name is at the first (da0,pass0) or the second (pass0,da0) arguement
			$device = preg_split( '/([-A-z]+)/', $temp[1], -1, PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_NO_EMPTY);

			if (strncmp($device[0],'pass',4) == 0){
				$device = preg_split( '/([-A-z]+)/', $temp[0], -1, PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_NO_EMPTY);
			}

			if (strcmp($devicespecialfile,'/dev/'.$device[0].$device[1]) == 0){
				return $controller[0].$controller[1];
			}
		}
	}
	return gtext("n/a");
}

/**
 * Get volume serial number.
 * @param[in] device	The device name, e.g. /dev/ad1
 * @return The disk device serial number.
 */
function system_get_volume_serial($devicespecialfile) {

	global $config;

	// get serial from config
	global $config_disks;
	/* get serial from S.M.A.R.T. */
	$smart_arg = get_smartctl_info($config_disks[$devicespecialfile]);

	if( !empty($smart_arg['serial']) ){
		return $smart_arg['serial'];
	}

	/* without smart */
	mwexec2("ataidle {$devicespecialfile} 2>/dev/null | grep 'Serial'", $rawdata);
	if (!empty($rawdata) && 1 === preg_match("/^Serial:\s+(.+)$/", $rawdata[0], $matches)) {
		$serial = preg_replace("/[\x01-\x1f\x7f]/", "", $matches[1]);
		if (!empty($serial))
			return $serial;
	}

	/* get serial from CAM */
	mwexec2("camcontrol inquiry {$devicespecialfile} -S 2>/dev/null", $rawdata2);
	if (!empty($rawdata2) && !empty($rawdata2[0])) {
		$serial = preg_replace("/[\x01-\x1f\x7f]/", "", $rawdata2[0]);
		if (!empty($serial))
			return $serial;
	}
	return gtext("n/a");
}

/**
 * Get volume device model.
 * @param[in] device	The device name, e.g. /dev/ad1
 * @return The disk device model.
 */
function system_get_volume_model($devicespecialfile) {

	global $config;

	// get device model from config
	global $config_disks;
	if(!empty($config_disks[$devicespecialfile]['model'])){
		return $config_disks[$devicespecialfile]['model'];
	}
	else {
		/* get device model from S.M.A.R.T. */
		$smart_arg = get_smartctl_info($config_disks[$devicespecialfile]);

		if( !empty($smart_arg['model']) ){
			return $smart_arg['model'];
		}
	}

	/* without smart */
	mwexec2("ataidle {$devicespecialfile} 2>/dev/null | grep 'Model'", $rawdata);
	if (!empty($rawdata) && 1 === preg_match("/^Model:\s+(.+)$/", $rawdata[0], $matches)) {
		$model = preg_replace("/[\x01-\x1f\x7f]/", "", $matches[1]);
		if (!empty($model))
			return $model;
	}

	/* get device model from CAM */
	mwexec2("camcontrol inquiry {$devicespecialfile} -D 2>/dev/null", $rawdata2);
	if (!empty($rawdata2) && 1 === preg_match("/<([^>]+)>/", $rawdata2[0], $matches)) {
		$model = preg_replace("/[\x01-\x1f\x7f]/", "", $matches[1]);
		if (!empty($model))
			return $model;
	}
	return gtext("n/a");
}

/**
 * Get mount point usage information.
 * @return An array containg the requested informations:
 * @code
 * [mountpoint] => [
 *   [mountpoint] => /mnt/xyz
 *   [name] => xyz
 *   [filesystem] => /dev/ad0a
 *   [capacity] => 48%
 *   [used] => 2.4G
 *   [avail] => 2.6G
 *   [size] => 5.4G]
 * @endcode
 */
function system_get_mount_usage() {
	global $config, $g;

	$result = [];

	exec("/bin/df -h", $rawdata);
	foreach ($rawdata as $line) {
		if (0 == preg_match("/^(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\d+%)\s+(.+)/", $line, $aline))
			continue;

		$filesystem = chop($aline[1]);
		$size = chop($aline[2]);
		$used = chop($aline[3]);
		$avail = chop($aline[4]);
		$capacity = chop($aline[5]);
		$mountpoint = chop($aline[6]);

		if (is_array($config['mounts']) && is_array($config['mounts']['mount'])) {
			foreach ($config['mounts']['mount'] as $mountcfg) {
				if (0 == strcmp($mountpoint, "{$g['media_path']}/{$mountcfg['sharename']}")) {
					$result[$mountpoint] = [];
					$result[$mountpoint]['mountpoint'] = $mountpoint;
					$result[$mountpoint]['name'] = $mountcfg['sharename'];
					$result[$mountpoint]['filesystem'] = $filesystem;
					$result[$mountpoint]['capacity'] = $capacity;
					$result[$mountpoint]['avail'] = $avail;
					$result[$mountpoint]['used'] = $used;
					$result[$mountpoint]['size'] = $size;
				}
			}
		}
	}
	return $result;
}

function get_disk_usage() {
	$retval = [];
	$a_diskusage = [];
	$a_diskinfo = system_get_mount_usage();
	if (!empty($a_diskinfo)) {
		foreach ($a_diskinfo as $diskk => $diskv) {
			$id = get_mount_fsid($diskv['filesystem'], $diskk);
			$r_diskusage = [];
			$r_diskusage['id'] = $id;
			$r_diskusage['mountpoint'] = $diskv['mountpoint'];
			$r_diskusage['name'] = $diskv['name'];
			$r_diskusage['capacity'] = $diskv['capacity'];
			$r_diskusage['percentage'] = rtrim($diskv['capacity'], "%");
			$r_diskusage['size'] = $diskv['size'];
			$r_diskusage['used'] = $diskv['used'];
			$r_diskusage['avail'] = $diskv['avail'];
			$r_diskusage['capofsize'] = sprintf(gtext("%s of %sB"), $r_diskusage['capacity'], $r_diskusage['size']);
			$r_diskusage['tooltip']['used'] = sprintf(gtext("%sB used of %sB"), $r_diskusage['used'], $r_diskusage['size']);
			$r_diskusage['tooltip']['avail'] = sprintf(gtext("%sB available of %sB"), $r_diskusage['avail'], $r_diskusage['size']);
			$a_diskusage[] = $r_diskusage;
		}
		array_sort_key($a_diskusage, 'name');
		$retval = $a_diskusage;
	}
	return $retval;
}

function get_pool_usage() {
	global $config;
 	$retval = [];
	$a_poolusage = [];
	$a_poolinfo = zfs_get_pool_list();
	if (!empty($a_poolinfo)) {
		foreach ($a_poolinfo as $poolk => $poolv) {
			$id = $poolv['name'];
			$id = preg_replace('/[-\.: ]/', '_', $id);
			$r_poolusage = [];
			$r_poolusage['id'] = $id;
			$r_poolusage['name'] = $poolv['name'];
			$r_poolusage['capacity'] = $poolv['cap'];
			$r_poolusage['percentage'] = rtrim($poolv['cap'], "%");
			$r_poolusage['size'] = $poolv['size'];
			if (isset($config['zfs']['settings']['showusedavail'])) {
				$r_poolusage['used'] = $poolv['used'];
				$r_poolusage['avail'] = $poolv['avail'];
			} else { // this is the default setting
				$r_poolusage['used'] = $poolv['alloc'];
				$r_poolusage['avail'] = $poolv['free'];
			}
			$r_poolusage['health'] = $poolv['health'];
			$r_poolusage['capofsize'] = sprintf(gtext("%s of %sB"), $r_poolusage['capacity'], $r_poolusage['size']);
			$r_poolusage['tooltip']['used'] = sprintf(gtext("%sB used of %sB"), $r_poolusage['used'], $r_poolusage['size']);
			$r_poolusage['tooltip']['avail'] = sprintf(gtext("%sB available of %sB"), $r_poolusage['avail'], $r_poolusage['size']);
			$a_poolusage[] = $r_poolusage;
		}
		array_sort_key($a_poolusage, 'name');
		$retval = $a_poolusage;
	}
	return $retval;
}

function get_swap_usage() {
	$retval = [];
	$a_swapusage = [];
	$a_swapinfo= system_get_swap_info();
	if (!empty($a_swapinfo)) {
		$id = 0;
		foreach ($a_swapinfo as $swapk => $swapv) {
			$r_swapusage = [];
			$r_swapusage['id'] = $id++;
			$r_swapusage['name'] = $swapv['device'];
			$r_swapusage['capacity'] = $swapv['capacity'];
			$r_swapusage['percentage'] = rtrim($swapv['capacity'], "%");
			$r_swapusage['size'] = $swapv['total'];
			$r_swapusage['used'] = $swapv['used'];
			$r_swapusage['avail'] = $swapv['avail'];
			$r_swapusage['capofsize'] = sprintf(gtext("%s of %sB"), $r_swapusage['capacity'], $r_swapusage['size']);
			$r_swapusage['tooltip']['used'] = sprintf(gtext("%sB used of %sB"), $r_swapusage['used'], $r_swapusage['size']);
			$r_swapusage['tooltip']['avail']  = sprintf(gtext("%sB available of %sB"), $r_swapusage['avail'], $r_swapusage['size']);
			$a_swapusage[]= $r_swapusage;
		}
		array_sort_key($a_swapusage, 'name');
		$retval = $a_swapusage;
	}
	return $retval;
}

/**
 * Get various system informations.
 * @return An array containing the system informations.
 */
function system_get_sysinfo() {
	$value['hostname'] = system_get_hostname();

	// Get CPU counts.
	$cpus = system_get_cpus();
	$value['cpus'] = $cpus;

	// Get RAM usage.
	$raminfo = system_get_ram_info();
	$percentage = round(($raminfo['used'] * 100) / $raminfo['total'], 0);
	$value['memusage']['percentage'] = $percentage;
	$value['memusage']['caption'] = sprintf(gtext("%d%% of %dMiB"), $percentage, round($raminfo['physical'] / 1024 / 1024));

	// Get load average.
	exec("uptime", $result);
	$value['loadaverage'] = substr(strrchr($result[0], "load averages:"), 15);
	// Get up-to-date CPU informations.
	$cpuinfo = system_get_cpu_info();
	$value['cputemp'] = $cpuinfo['temperature'];
	$value['cpufreq'] = $cpuinfo['freq'];
	if (!empty($cpuinfo['temperature2'])) {
		$value['cputemp2'] = $cpuinfo['temperature2'];
	}

	// Get CPU usage.
	if ($cpus > 1) {
		$value['cpuusage2'] = system_get_smp_cpu_usage();
		$value['cpuusage'] = floor(array_sum($value['cpuusage2']) / $cpus);
	} else {
		$value['cpuusage'] = system_get_cpu_usage();
		$value['cpuusage2'] = [$value['cpuusage']];
	}
	$value['diskusage'] = get_disk_usage(); // Get disk usage.
	$value['poolusage'] = get_pool_usage(); // Get pool info.
	$value['swapusage'] = get_swap_usage(); // Get swap info.

	// Get uptime and date.
	$value['uptime'] = system_get_uptime();
	$value['date'] = get_datetime_locale();
	return $value;
}
?>