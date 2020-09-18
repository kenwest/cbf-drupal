{*
 +--------------------------------------------------------------------+
 | Copyright CiviCRM LLC. All rights reserved.                        |
 |                                                                    |
 | This work is published under the GNU AGPLv3 license with some      |
 | permitted exceptions and without any warranty. For full license    |
 | and copyright information, see https://civicrm.org/licensing       |
 +--------------------------------------------------------------------+
*}
<div class="crm-block crm-form-block crm-contribution-preview-form-block">
    <fieldset><legend>{ts}Contribution Page{/ts}</legend>
    <div class="crm-submit-buttons">{include file="CRM/common/formButtons.tpl" location="top"}</div>
    <table class="form-layout-compressed">
    <tr class="crm-contribution-form-block-intro_text">
       <td class="label">{$form.intro_text.label}</td>
       <td class="html-adjust">{$form.intro_text.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-amount">
       <td class="label">{$form.amount.label}</td>
       <td class="html-adjust">{$form.amount.html}</td>
    </tr>
{if $is_allow_other_amount}
    <tr class="crm-contribution-form-block-amount_other">
       <td class="label">{$form.amount_other.label}</td>
       <td class="html-adjust">{$form.amount_other.html}</td>
    </tr>
{/if}
    <tr class="crm-contribution-form-block-email">
       <td class="label">{$form.email.label}</td>
       <td class="html-adjust">{$form.email.html}</td>
    </tr>
{include file="CRM/UF/Form/Block.tpl" fields=$customPre}
    <tr class="crm-contribution-form-block-_qf_Preview_next_express"><td></td><td>{$form._qf_Preview_next_express.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-first_name">
       <td class="label">{$form.first_name.label}</td>
       <td class="html-adjust">{$form.first_name.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-middle_name">
       <td class="label">{$form.middle_name.label}</td>
       <td class="html-adjust">{$form.middle_name.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-last_name">
       <td class="label">{$form.last_name.label}</td>
       <td class="html-adjust">{$form.last_name.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-street1">
       <td class="label">{$form.street1.label}</td>
       <td class="html-adjust">{$form.street1.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-city">
       <td class="label">{$form.city.label}</td>
       <td class="html-adjust">{$form.city.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-state_province">
       <td class="label">{$form.state_province.label}</td>
       <td class="html-adjust">{$form.state_province.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-postal_code">
       <td class="label">{$form.postal_code.label}</td>
       <td class="html-adjust">{$form.postal_code.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-country_id">
       <td class="label">{$form.country_id.label}</td>
       <td class="html-adjust">{$form.country_id.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-credit_card_number">
       <td class="label">{$form.credit_card_number.label}</td>
       <td class="html-adjust">{$form.credit_card_number.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-credit_card_holder">
       <td class="label">{$form.credit_card_holder.label}</td>
       <td class="html-adjust">{$form.credit_card_holder.html}</td>
    </tr>	{* <KW: CRM-3224/> *}
    <tr class="crm-contribution-form-block-cvv2">
       <td class="label">{$form.cvv2.label}</td>
       <td class="html-adjust">{$form.cvv2.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-credit_card_type">
       <td class="label">{$form.credit_card_type.label}</td>
       <td class="html-adjust">{$form.credit_card_type.html}</td>
    </tr>
    <tr class="crm-contribution-form-block-credit_card_exp_date">
       <td class="label">{$form.credit_card_exp_date.label}</td>
       <td class="html-adjust">{$form.credit_card_exp_date.html}</td>
    </tr>
{include file="CRM/UF/Form/Block.tpl" fields=$customPost}
    </table>
    <div class="crm-submit-buttons">{include file="CRM/common/formButtons.tpl" location="bottom"}</div>
    </fieldset>
</div>
