{*
 +--------------------------------------------------------------------+
 | CiviCRM version 4.6                                                |
 +--------------------------------------------------------------------+
 | Copyright CiviCRM LLC (c) 2004-2015                                |
 +--------------------------------------------------------------------+
 | This file is a part of CiviCRM.                                    |
 |                                                                    |
 | CiviCRM is free software; you can copy, modify, and distribute it  |
 | under the terms of the GNU Affero General Public License           |
 | Version 3, 19 November 2007 and the CiviCRM Licensing Exception.   |
 |                                                                    |
 | CiviCRM is distributed in the hope that it will be useful, but     |
 | WITHOUT ANY WARRANTY; without even the implied warranty of         |
 | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               |
 | See the GNU Affero General Public License for more details.        |
 |                                                                    |
 | You should have received a copy of the GNU Affero General Public   |
 | License and the CiviCRM Licensing Exception along                  |
 | with this program; if not, contact CiviCRM LLC                     |
 | at info[AT]civicrm[DOT]org. If you have questions about the        |
 | GNU Affero General Public License or the licensing of CiviCRM,     |
 | see the CiviCRM license FAQ at http://civicrm.org/licensing        |
 +--------------------------------------------------------------------+
*}
<div>
  {if $groupExist}
    <div class="messages status no-popup">
      {ts}Are you sure you want to be removed from the mailing list(s) shown below:{/ts}<br/>
    </div>
    <table class="selector" style="width: auto; margin-top: 20px;">
      {counter start=0 skip=1 print=false}
      {foreach from=$groups item=group}
        <tr class="{cycle values="odd-row,even-row"}">
          <td><strong>{$group.title}</strong></td>
          <td>&nbsp;&nbsp;{$group.description}&nbsp;</td>
        </tr>
      {/foreach}
    </table>
    <div class="crm-block crm-form-block crm-miscellaneous-form-block">
      <p>{ts}You are requesting to unsubscribe this email address:{/ts}</p>
      <h3>{$email_masked}</h3>
      <p>{ts}If this is not your email address, there is no need to do anything. You have <i><b>not</b></i> been added to any mailing lists. If this is your email address and you <i><b>wish to unsubscribe</b></i> please enter your email address below for verification purposes:{/ts}</p>
      <table class="form-layout">
        <tbody>
          <tr>
            <td class="label">{$form.email_confirm.label}</td>
            <td class="content">{$form.email_confirm.html}</td>
          </tr>
        </tbody>
      </table>
      <div class="crm-submit-buttons">
        {include file="CRM/common/formButtons.tpl" location="bottom"}
      </div>
      <br/>
    </div>
  {else}
    <script>
      {literal}
        (function(t,e,n,o){var s,c,a;t.SMCX=t.SMCX||[],e.getElementById(o)||(s=e.getElementsByTagName(n),c=s[s.length-1],a=e.createElement(n),a.type="text/javascript",a.async=!0,a.id=o,a.src=["https:"===location.protocol?"https://":"http://","widget.surveymonkey.com/collect/website/js/IJ_2FXZPXyt_2FRG1IGVdRe3FotMk6Np7Z3xRm8Idm4Q7FHjaB7_2FPiPLC6InSuaHjEvP.js"].join(""),c.parentNode.insertBefore(a,c))})(window,document,"script","smcx-sdk");
      {/literal}
    </script>
    <a style="font-size: 12px; color: #999; text-decoration: none;" href="https://www.surveymonkey.com/s/3GGMJB7" target="_blank">
      Take a survey to help us understand why you are unsubscribing
    </a>
  {/if}
</div>

