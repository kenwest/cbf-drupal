{*
 +--------------------------------------------------------------------+
 | Copyright CiviCRM LLC. All rights reserved.                        |
 |                                                                    |
 | This work is published under the GNU AGPLv3 license with some      |
 | permitted exceptions and without any warranty. For full license    |
 | and copyright information, see https://civicrm.org/licensing       |
 +--------------------------------------------------------------------+
*}

{if $status_id eq 2} {* Signer needs to confirm signature. *}
  <h2>{ts}STEP 2: Please Check Your Email{/ts}</h2>
  <p>{ts}To complete and confirm this action, please follow the instructions sent to the email address you provided.{/ts}</p>
  <p>

    <strong>{ts}IMPORTANT{/ts}</strong>: {ts}Validate your email address by clicking on the link in the confirmation e-mail. Sometimes our confirmation emails get flagged as spam.{/ts}
    <br/>
    {ts}If you haven't received an email within a few minutes, please check your spam folder.{/ts}
  </p>
{/if}

{if $status_id eq 4}
  <p>{ts}You have already signed up but we<strong>need to confirm your email address</strong>.{/ts}</p>
  <strong>{ts}IMPORTANT{/ts}</strong>
  : {ts}Validate your email address by clicking on the link in the confirmation e-mail. Sometimes our confirmation emails get flagged as spam.{/ts}
  <br/>
  {ts}If you haven't received an email within a few minutes, please check your spam folder.{/ts}
  <br/>
{/if}
{if $status_id eq 5}
  <p>{ts}You have already signed up.{/ts}</p>
{/if}

{if $status_id neq 2}{* if asked to confirm the email, focus on that and don't put additional messages *}
  {if $thankyou_text}
    <div id="thankyou_text" class="crm-section thankyou_text-section">
      {$thankyou_text}
    </div>
  {/if}
  {if $is_share}
    {include file="CRM/Campaign/Page/Petition/SocialNetwork.tpl" petition_id=$survey_id petitionTitle=$petitionTitle}
  {/if}
{/if}

