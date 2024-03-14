{*
 +--------------------------------------------------------------------+
 | Copyright CiviCRM LLC. All rights reserved.                        |
 |                                                                    |
 | This work is published under the GNU AGPLv3 license with some      |
 | permitted exceptions and without any warranty. For full license    |
 | and copyright information, see https://civicrm.org/licensing       |
 +--------------------------------------------------------------------+
*}
{if $action & 1024}
    {include file="CRM/Event/Form/Registration/PreviewHeader.tpl"}
{/if}

<div class="crm-event-id-{$event.id} crm-block crm-event-thankyou-form-block">
  {if $paidEvent}
    <div class="progress-outer">
      <div class="progress payment">
        <div class="progress-bar past">
          1. Entered <br/> details &#x2713;
        </div>
        <div class="progress-bar past">
          2. Checked <br/> details &#x2713;
        </div>
        <div class="progress-bar present">
          3. Registered <br/> &#x2713;
        </div>
      </div>
    </div>
  {/if}
    {* Don't use "normal" thank-you message for Waitlist and Approval Required registrations - since it will probably not make sense for those situations. dgg *}
    {if array_key_exists('thankyou_text', $event) AND (not $isOnWaitlist AND not $isRequireApproval)}
        <div id="intro_text" class="crm-section event_thankyou_text-section">
            <p>
            {$event.thankyou_text|purify}
            </p>
        </div>
    {/if}

    {* Show link to Tell a Friend (CRM-2153) *}
    {if $friendText}
        <div id="tell-a-friend" class="crm-section tell_friend_link-section">
            <a href="{$friendURL}" title="{$friendText|escape:'html'}" class="button"><span><i class="crm-i fa-chevron-right" aria-hidden="true"></i> {$friendText}</span></a>
       </div><br /><br />
    {/if}

    {* Add button for donor to create their own Personal Campaign page *}
    {if $pcpLink}
      <div class="crm-section create_pcp_link-section">
            <a href="{$pcpLink}" title="{$pcpLinkText|escape:'html'}" class="button"><span><i class="crm-i fa-chevron-right" aria-hidden="true"></i> {$pcpLinkText}</span></a>
        </div><br /><br />
    {/if}

    <div class="help">
        {if $isOnWaitlist}
            <p>
                <span class="bold">{ts}You have been added to the WAIT LIST for this event.{/ts}</span>
                {ts}If space becomes available you will receive an email with a link to a web page where you can complete your registration.{/ts}
             </p>
        {elseif $isRequireApproval}
            <p>
                <span class="bold">{ts}Your registration has been submitted.{/ts}
                {ts}Once your registration has been reviewed, you will receive an email with a link to a web page where you can complete the registration process.{/ts}</span>
            </p>
        {elseif $is_pay_later and $paidEvent and !$isAmountzero}
            <div class="bold">{$pay_later_receipt}</div>
            {if $is_email_confirm}
                <p>{ts 1=$email}An email with event details has been sent to %1.{/ts}</p>
            {/if}
        {* This text is determined by the payment processor *}
        {elseif $eventConfirmText}
            <p>{$eventConfirmText}</p>
            {if $is_email_confirm}
                <p>{$eventEmailConfirmText}</p>
            {/if}
        {else}
            <p>{ts}Your registration has been processed successfully.{/ts}</p>
            {if $is_email_confirm}
                <p>{ts 1=$email}A registration confirmation email has also been sent to %1{/ts}</p>
            {/if}
        {/if}
    </div>
    <div class="spacer"></div>

    <div class="crm-group event_info-group">
        <div class="header-dark">
            {ts}Event Information{/ts}
        </div>
        <div class="display-block">
            {include file="CRM/Event/Form/Registration/EventInfoBlock.tpl" context="ThankYou"}
        </div>
    </div>

    {if $paidEvent && !$isRequireApproval && !$isOnWaitlist}
        <div class="crm-group event_fees-group">
            <div class="header-dark">
                {$event.fee_label}
            </div>
            {if $lineItem}
                {include file="CRM/Price/Page/LineItem.tpl" context="Event"}
            {elseif $amount || $amount == 0}
              <div class="crm-section no-label amount-item-section">
                    {foreach from=$finalAmount item=amount key=level}
                  <div class="content">
                      {$amount.amount|crmMoney:$currency}&nbsp;&nbsp;{$amount.label}
                  </div>
                  <div class="clear"></div>
                    {/foreach}
                </div>
                {if $totalTaxAmount}
                  <div class="content bold">{ts}Tax Total{/ts}:&nbsp;&nbsp;{$totalTaxAmount|crmMoney:$currency}</div>
                  <div class="clear"></div>
                {/if}
                {if $totalAmount}
                 <div class="crm-section no-label total-amount-section">
                    <div class="content bold">{ts}Total Amount{/ts}:&nbsp;&nbsp;{$totalAmount|crmMoney:$currency}</div>
                    <div class="clear"></div>
                  </div>

                    {if $hookDiscount}
                        <div class="crm-section hookDiscount-section">
                            <em>({$hookDiscount.message})</em>
                        </div>
                    {/if}
                {/if}
            {/if}

            {if $receive_date}
                <div class="crm-section no-label receive_date-section">
                    <div class="content bold">{ts}Transaction Date{/ts}: {$receive_date|crmDate}</div>
                  <div class="clear"></div>
                </div>
            {/if}
            {if $trxn_id}
                <div class="crm-section no-label trxn_id-section">
                    <div class="content bold">{ts}Transaction #{/ts}: {$trxn_id}</div>
                <div class="clear"></div>
              </div>
            {/if}
        </div>

    {elseif $participantInfo}
        <div class="crm-group participantInfo-group">
            <div class="header-dark">
                {ts}Additional Participant Email(s){/ts}
            </div>
            <div class="crm-section no-label participant_info-section">
                <div class="content">
                    {foreach from=$participantInfo  item=mail key=no}
                        <strong>{$mail}</strong><br />
                    {/foreach}
                </div>
            <div class="clear"></div>
          </div>
        </div>
    {/if}

    {if $event.participant_role neq 'Attendee' and $defaultRole}
        <div class="crm-group participant_role-group">
            <div class="header-dark">
                {ts}Participant Role{/ts}
            </div>
            <div class="crm-section no-label participant_role-section">
                <div class="content">
                    {$event.participant_role}
                </div>
            <div class="clear"></div>
          </div>
        </div>
    {/if}

    {include file="CRM/Event/Form/Registration/DisplayProfile.tpl"}

    {* <KW: CRM-3224/> *}

    {if $credit_card_type}
      {crmRegion name="event-thankyou-billing-block"}
        <div class="crm-group credit_card-group">
          <div class="header-dark">
            {ts}Credit Card Information{/ts}
          </div>
          <div class="crm-section no-label credit_card_details-section">
            <div class="content">{$credit_card_type}</div>
            <div class="clear"></div>
            <div class="content">{$credit_card_number}</div>
            <div class="clear"></div>
            <div class="content">{$credit_card_holder}</div>    {* <KW: CRM-3224/> *}
            <div class="clear"></div>
            <div class="content">{if $credit_card_exp_date}{ts}Expires{/ts}: {$credit_card_exp_date|truncate:7:''|crmDate}{/if}</div>
            <div class="clear"></div>
          </div>
        </div>
      {/crmRegion}
    {/if}

    {if array_key_exists('thankyou_footer_text', $event) && $event.thankyou_footer_text}
        <div id="footer_text" class="crm-section event_thankyou_footer-section">
            <p>{$event.thankyou_footer_text|purify}</p>
        </div>
    {/if}
</div>
