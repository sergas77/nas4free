<?php
/*
	services_rsyncd.php

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
require 'auth.inc';
require 'guiconfig.inc';

array_make_branch($config,'access','user');
array_sort_key($config['access']['user'],'login');
$a_user = &$config['access']['user'];
array_make_branch($config,'rsyncd');

$pconfig['enable'] = isset($config['rsyncd']['enable']);
$pconfig['port'] = $config['rsyncd']['port'];
$pconfig['motd'] = base64_decode($config['rsyncd']['motd']);
$pconfig['rsyncd_user'] = $config['rsyncd']['rsyncd_user'];
if(isset($config['rsyncd']['auxparam']) && is_array($config['rsyncd']['auxparam'])):
	$pconfig['auxparam'] = implode("\n",$config['rsyncd']['auxparam']);
endif;

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if (isset($_POST['enable']) && $_POST['enable']) {
		// Input validation.
		$reqdfields = ['rsyncd_user','port'];
		$reqdfieldsn = [gtext('Map to User'),gtext('TCP Port')];
		$reqdfieldst = ['string','port'];

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
		}	

	if (empty($input_errors)) {
		$config['rsyncd']['enable'] = isset($_POST['enable']) ? true : false;
		$config['rsyncd']['port'] = $_POST['port'];
		$config['rsyncd']['motd'] = base64_encode($_POST['motd']); // Encode string, otherwise line breaks will get lost
		$config['rsyncd']['rsyncd_user'] = $_POST['rsyncd_user'];

		# Write additional parameters.
		unset($config['rsyncd']['auxparam']);
		foreach (explode("\n", $_POST['auxparam']) as $auxparam) {
			$auxparam = trim($auxparam, "\t\n\r");
			if (!empty($auxparam))
				$config['rsyncd']['auxparam'][] = $auxparam;
		}

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("rsyncd");
			$retval |= rc_update_service("mdnsresponder");
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
	}
}
$pgtitle = [gtext('Services'),gtext('Rsync'),gtext('Server'),gtext('Settings')];
?>
<?php include 'fbegin.inc';?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis = !(document.iform.enable.checked || enable_change);
	document.iform.port.disabled = endis;
	document.iform.auxparam.disabled = endis;
	document.iform.motd.disabled = endis;
	document.iform.rsyncd_user.disabled = endis;
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabact"><a href="services_rsyncd.php" title="<?=gtext('Reload page');?>"><span><?=gtext("Server");?></span></a></li>
				<li class="tabinact"><a href="services_rsyncd_client.php"><span><?=gtext("Client");?></span></a></li>
				<li class="tabinact"><a href="services_rsyncd_local.php"><span><?=gtext("Local");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav2">
				<li class="tabact"><a href="services_rsyncd.php" title="<?=gtext('Reload page');?>"><span><?=gtext("Settings");?></span></a></li>
				<li class="tabinact"><a href="services_rsyncd_module.php"><span><?=gtext("Modules");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="services_rsyncd.php" method="post" name="iform" id="iform" onsubmit="spinner()">
				<?php if (!empty($input_errors)) print_input_errors($input_errors);?>
				<?php if (!empty($savemsg)) print_info_box($savemsg);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gtext("Rsync"), !empty($pconfig['enable']) ? true : false, gtext("Enable"), "enable_change(false)");?>
					<tr>
						<td valign="top" class="vncellreq"><?=gtext("Map to User");?></td>
						<td class="vtable">
							<select name="rsyncd_user" class="formfld" id="rsyncd_user">
								<option value="ftp" <?php if ("ftp" === $pconfig['rsyncd_user']) echo "selected=\"selected\"";?>><?=gtext("Guest");?></option>
								<?php foreach ($a_user as $user):?>
								<option value="<?=$user['login'];?>" <?php if ($user['login'] === $pconfig['rsyncd_user']) echo "selected=\"selected\"";?>><?php echo htmlspecialchars($user['login']);?></option>
								<?php endforeach;?>
							</select>
						</td>
					</tr>
					<tr>
						<td width="22%" valign="top" class="vncellreq"><?=gtext("TCP Port");?></td>
						<td width="78%" class="vtable">
							<input name="port" type="text" class="formfld" id="port" size="20" value="<?=htmlspecialchars($pconfig['port']);?>" />
							<br /><?=gtext("Alternate TCP port. (Default is 873).");?>
						</td>
					</tr>
					<?php
					html_textarea("motd", gtext("MOTD"), $pconfig['motd'], gtext("Message of the day."), false, 65, 7, false, false);
					$helpinghand = '<a href="'
						. 'http://rsync.samba.org/ftp/rsync/rsyncd.conf.html'
						. '" target="_blank">'
						. gtext('Please check the documentation')
						. '</a>.';
					html_textarea("auxparam", gtext("Additional Parameters"), !empty($pconfig['auxparam']) ? $pconfig['auxparam'] : "", sprintf(gtext("These parameters will be added to [global] settings in %s."), "rsyncd.conf") . " " . $helpinghand, false, 65, 5, false, false);
					?>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gtext("Save & Restart");?>" onclick="enable_change(true)" />
				</div>
				<?php include 'formend.inc';?>
			</form>
		</td>
	</tr>
</table>
<script type="text/javascript">
<!--
enable_change(false);
//-->
</script>
<?php include 'fend.inc';?>
