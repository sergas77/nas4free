#!/usr/local/bin/php
<?php
/*
	changes.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.
	
	Modified for XHTML by Daisuke Aoyama <aoyama@peach.ne.jp>
	Copyright (C) 2010 Daisuke Aoyama <aoyama@peach.ne.jp>.	
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	Copyright (C) 2008-2011 Michael Zoon <zoon01@nas4free.org>.
	Copyright (C) 2009 Volker Theile <votdev@gmx.de>.
	All rights reserved.
	
	Portions of m0n0wall (http://m0n0.ch/wall).
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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
	either expressed or implied, of the FreeBSD Project.
*/
// Configure page permission
$pgperm['allowuser'] = TRUE;

require("auth.inc");
require("guiconfig.inc");

$pgtitle = array(gettext("Help"), gettext("Release Notes"));
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellspacing="0" cellpadding="0">
				<?php html_titleline(gettext("Release Notes"));?>
				<tr>
					<td class="listt">
						<div>
							<textarea style="width: 98%;" id="content" name="content" class="listcontent" cols="10" rows="33" readonly="readonly"><?php unset($lines); exec("/bin/cat {$g['www_path']}/CHANGES", $lines); foreach ($lines as $line) { echo htmlspecialchars($line)."\n"; }?></textarea>
						</div>
					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>