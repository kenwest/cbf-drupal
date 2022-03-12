{*
 +--------------------------------------------------------------------+
 | Copyright CiviCRM LLC. All rights reserved.                        |
 |                                                                    |
 | This work is published under the GNU AGPLv3 license with some      |
 | permitted exceptions and without any warranty. For full license    |
 | and copyright information, see https://civicrm.org/licensing       |
 +--------------------------------------------------------------------+
*}
{* Block displays key event info for Registration Confirm and Thankyou pages *}
<table class="form-layout">
  <tr>
    <td colspan="2">
    {if $context EQ 'ThankYou'} {* Provide link back to event info page from Thank-you page *}
        <a href="{crmURL p='civicrm/event/info' q="reset=1&id=`$event.id`"}"title="{ts}View complete event information.{/ts}"><strong>{$event.event_title}</strong></a>
    {else}
        <strong>{$event.event_title}</strong>
    {/if}
    </td>
  </tr>
  <tr class="crm-section event_date_time-section">
      <td class="label">{ts}Event Start{/ts}</td>
      <td class="content">
          <time datetime="{$event.event_start_date_utc|date_format:"%Y-%m-%dT%H:%M:%S"}Z">{$event.event_start_date|crmDate}<span class="timezone"> {$event.event_tz}</span></time>
      </td>
  </tr>
  {if $event.event_end_date}
  <tr class="crm-section event_date_time-section">
      <td class="label">{ts}Event End{/ts}</td>
      <td class="content">
          <time datetime="{$event.event_end_date_utc|date_format:"%Y-%m-%dT%H:%M:%S"}Z">{strip}
          {* Only show end time if end date = start date *}
          {if $event.event_end_date|date_format:"%Y%m%d" == $event.event_start_date|date_format:"%Y%m%d"}
              {$event.event_end_date|crmDate:0:1}
          {else}
              {$event.event_end_date|crmDate}
          {/if}
          {/strip}<span class="timezone"> {$event.event_tz}</span></time>
      </td>
  </tr>
  {/if}

  {if $isShowLocation}
    {if $location.address.1}
      <tr class="crm-section">
        <td class="label">{ts}Location{/ts}</td>
        <td class="content">
          {$location.address.1.display|nl2br}
          {if ( $event.is_map &&
            $config->mapProvider &&
      ( ( !empty($location.address.1.geo_code_1) && is_numeric($location.address.1.geo_code_1) )  ||
        ( !empty($location.address.1.city) AND !empty($location.address.1.state_province) ) ) ) }
              <br/><a href="{crmURL p='civicrm/contact/map/event' q="reset=1&eid=`$event.id`"}" title="{ts}Map this Address{/ts}" target="_blank">{ts}Map this Location{/ts}</a>
          {/if}
        </td>
      </tr>
    {/if}
  {/if}{*End of isShowLocation condition*}

</table>
