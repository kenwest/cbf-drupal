<?php

/**
 * CBF Webhook Processor Feedback class.
 *
 * Processes Feedback forms.
 */
class CBF_Webhook_Processor_Feedback implements Webhook_Plugins_Processor_Interface {

  /**
   * Configuration form.
   */
  public function config_form() {

    // No configuration needed.
    return [];
  }

  /**
   * Processes data.
   */
  public function process(stdClass $request) {

    civicrm_initialize();

    CRM_Core_Error::debug_var('Feedback webhook invoked', [
      'Agent' => $request->agent,
      'Raw data' => $request->raw,
      'Cooked data' => $request->cooked,
    ]);

    $progress = [];

    try {

      /* ----------------------------------------------------------------------
       * Find the agent that will perform the actions
       */
      if ($request->agent) {

        $agent = null;

        $contacts = \Civi\Api4\Contact::get(false)
          ->addSelect('id')
          ->addWhere('contact_sub_type', '=', 'Agent')
          ->addWhere('api_key', '=', $request->agent)
          ->setLimit(1)
          ->execute();
        foreach ($contacts as $contact) {
          $agent = $contact;
        }
      }

      if (!$request->agent) {
        $progress[] = 'Agent API key not supplied... stopping';
      }
      else if (!$agent) {
        $progress[] = 'Agent not found... stopping';
      }
      else if ($agent) {
        $progress[] = 'Agent is contact ' . $agent['id'];

        /* --------------------------------------------------------------------
         * Find the target contact 
         */
        $firstName = $request->cooked['first'];
        $lastName = $request->cooked['last'];
        $email = $request->cooked['email'];
        $mobile = $request->cooked['mobile'];
        $last9digits = substr(preg_replace('/[^0-9]/', '', $mobile), -9);
        if (!is_numeric($last9digits) || $last9digits < 100000000) {
          $last9digits = '';
        }

        $target = null;

        if ($request->cooked['id']) {
          $contacts = \Civi\Api4\Contact::get(false)
            ->addSelect('id')
            ->addWhere('id', '=', $request->cooked['id'])
            ->addChain('ministryCentres', \Civi\Api4\EntityTag::get(false)
              ->addSelect('tag.name')
              ->addJoin('Tag AS tag', 'LEFT')
              ->addWhere('entity_table', '=', 'civicrm_contact')
              ->addWhere('entity_id', '=', '$id')
              ->addWhere('tag.parent_id', 'IS NULL')
              ->addWhere('tag.name', 'CONTAINS', 'Ministry centre'))
            ->setLimit(1)
            ->execute();
          foreach ($contacts as $contact) {
            $target = $contact;
          }
        }
        else if ($firstName && $lastName && ($email || $last9digits)) {
          CRM_Core_Error::debug_var('Contact get parameters', [
            'first' => $firstName,
            'last' => $lastName,
            'email' => $email,
            'last9digits' => $last9digits,
          ]);

          $contacts = \Civi\Api4\Contact::get(false)
            ->addSelect('id', 'first_name', 'last_name', 'nick_name', 'email.email', 'phone.phone_numeric')
            ->addJoin('Email AS email', 'LEFT')
            ->addJoin('Phone AS phone', 'LEFT')
            ->addClause('OR', ['first_name', '=', $firstName], ['nick_name', 'CONTAINS', $firstName])
            ->addClause('OR', ['last_name', '=', $lastName], ['nick_name', 'CONTAINS', $lastName])
            ->addChain('ministryCentres', \Civi\Api4\EntityTag::get(false)
              ->addSelect('tag.name')
              ->addJoin('Tag AS tag', 'LEFT')
              ->addWhere('entity_table', '=', 'civicrm_contact')
              ->addWhere('entity_id', '=', '$id')
              ->addWhere('tag.parent_id', 'IS NULL')
              ->addWhere('tag.name', 'CONTAINS', 'Ministry centre'));
          if ($last9digits) {
            if ($email) {
              $contacts = $contacts->addClause('OR', ['email.email', '=', $email], ['phone.phone_numeric', 'CONTAINS', $last9digits]);
            }
            else {
              $contacts = $contacts->addWhere('phone.phone_numeric', 'CONTAINS', $last9digits);
            }
          }
          else {
            $contacts = $contacts->addWhere('email.email', '=', $email);
          }
          $contacts = $contacts->execute();
          $potentials = [];
          foreach ($contacts as $contact) {
            // Tighten the search conditions to avoid false positives due to use of CONTAINS
            if ($firstName != $contact['first_name']
              && !stristr(' ' . $contact['nick_name'] . ',', ' ' . $firstName . ',')) {
              $progress[] = 'Contact ' . $contact['id'] . ' nickname doesn\'t match first name';
              continue;
            }
            if ($lastName != $contact['last_name']
              && !stristr(' ' . $contact['nick_name'] . ',', ' ' . $lastName . ',')) {
              $progress[] = 'Contact ' . $contact['id'] . ' nickname doesn\'t match last name';
              continue;
            }
            $found9digits = substr(preg_replace('/[^0-9]/', '', $contact['phone.phone_numeric']), -9);
            if (!is_numeric($found9digits) || $found9digits < 100000000) {
              $found9digits = '';
            }
            if ($last9digits && $last9digits != $found9digits) {
              if (!$email || $email != $contact['email.email']) {
                $progress[] = 'Contact ' . $contact['id'] . ' phone doesn\'t match mobile';
                continue;
              }
            }
            $potentials[$contact['id']] = $contact;
          }
          switch (count($potentials)) {
            case 0:
              break;
            case 1:
              $target = reset($potentials);
              break;
            default:
              $progress[] = 'Found ' . count($potentials) . ' matching contacts';
              break;
          }
        }
        
        /* --------------------------------------------------------------------
         * Create the target contact if it couldn't be found
         */
        if (!$target) {
          if ($firstName && $lastName && ($email || $last9digits)) {

            $progress[] = 'Unambiguous target not found';

            $results = \Civi\Api4\Contact::create(false)
              ->addValue('first_name', $firstName)
              ->addValue('last_name', $lastName);
            if ($email) {
              $results = $results->addChain('email', \Civi\Api4\Email::create(false)
                ->addValue('contact_id', '$id')
                ->addValue('email', $email)
              );
            }
            if ($last9digits) {
              $results = $results->addChain('mobile', \Civi\Api4\Phone::create(false)
                ->addValue('contact_id', '$id')
                ->addValue('phone', $mobile)
              );
            }
            $results = $results->execute();
            foreach ($results as $result) {
              $progress[] = 'Target created';
              $target = $result;
            }
            if (!$target) {
              $progress[] = 'Failed to create target contact';
              $progress[] = $results;
            }
          }
          else {
            $progress[] = 'Target not found, but not enough information has been supplied to create one';
          }
        }

        /* --------------------------------------------------------------------
         * Process the target contact
         */
        if ($target) {

          $progress[] = 'Target is contact ' . $target['id'];
          
          /* ------------------------------------------------------------------
           * Process the ministry centre
           */
          $ministryCentre = null;
          $optionValues = \Civi\Api4\OptionValue::get(false)
            ->addSelect('id', 'label', 'name')
            ->addWhere('option_group_id:name', '=', 'ministry_centre_20110828113048')
            ->addClause(
              'OR',
              ['label', '=', $request->cooked['ministryCentre']],
              ['description', 'CONTAINS', ' ' . $request->cooked['ministryCentre'] . ','])
            ->setLimit(1)
            ->execute();
          foreach ($optionValues as $optionValue) {
            $progress[] = $optionValue;
            $ministryCentre = $optionValue;
          }

          if ($ministryCentre) {
            $addTag = 'Ministry centre is ' . $ministryCentre['label'];
            $results = \Civi\Api4\EntityTag::create(false)
              ->addValue('entity_table', 'civicrm_contact')
              ->addValue('entity_id', $target['id'])
              ->addValue('tag_id.name', $addTag)
              ->execute();
            foreach ($results as $result) {
              $progress[] = 'Target tagged with \'' . $addTag . '\'';
            }
          }
          
          /* ------------------------------------------------------------------
           * Process the network
           */
          $network = null;
          $optionValues = \Civi\Api4\OptionValue::get(false)
            ->addSelect('id', 'label', 'name')
            ->addWhere('option_group_id.title', '=', 'Network')
            ->addWhere('label', '=', $request->cooked['network'])
            ->setLimit(1)
            ->execute();
          foreach ($optionValues as $optionValue) {
            $network = $optionValue;
          }
          
          if ($network) {
            $addTag = $network['label'];
            $results = \Civi\Api4\EntityTag::create(false)
              ->addValue('entity_table', 'civicrm_contact')
              ->addValue('entity_id', $target['id'])
              ->addValue('tag_id.name', $addTag)
              ->execute();
            foreach ($results as $result) {
              $progress[] = 'Target tagged with \'' . $addTag . '\'';
            }

            $contacts = \Civi\Api4\Contact::get(false)
              ->addSelect('id', 'Network_details.Network:label', 'Network_details.Network:name')
              ->addWhere('contact_type', '=', 'Organization')
              ->addWhere('contact_sub_type', '=', 'Network')
              ->addWhere('Network_details.Network:name', '=', $network['name'])
              ->setLimit(1)
              ->addChain('lists', \Civi\Api4\GroupContact::get(false)
                ->addSelect('group_id', 'group_id:label')
                ->addWhere('contact_id', '=', '$id')
              )
              ->execute();
            foreach ($contacts as $contact) {
              $progress[] = $contact;
              $network = $contact;
            }
          }

          /* ------------------------------------------------------------------
           * Process the if-conditions on the options selected
           */
          $isLead = false;
          $isList = false;
          $matchAll = '*';
          foreach (array_merge($request->cooked['options'], [$matchAll]) as $option) {
            foreach ($request->cooked['leadIf'] as $v) {
              if (stristr($option, $v) !== false) {
                $isLead = true;
              }
            }
            foreach ($request->cooked['listIf'] as $v) {
              if (stristr($option, $v) !== false) {
                $isList = true;
              }
            }
          }

          /* ------------------------------------------------------------------
           * Process the lists
           */
          if ($isList && $network) {
            foreach ($network['lists'] as $list) {
              $results = \Civi\Api4\GroupContact::create(false)
                ->addValue('group_id', $list['group_id'])
                ->addValue('contact_id', $target['id'])
                ->addValue('status', 'Added')
                ->execute();
              foreach ($results as $result) {
                $progress[] = 'Added contact ' . $target['id'] . ' to list \'' . $list['group_id:label' . '\''];
              }
            }
          }
          /* ------------------------------------------------------------------
           * Process the activity
           */
          $comment = '<p><strong>Comment</strong>: ' . $request->cooked['comment'] . '</p>';
          $options = implode('</li><li>', $request->cooked['options']);
          if ($options) {
            $comment .= '<p><strong>Options selected</strong>:</p>';
            $comment .= '<ul><li>' . $options . '</li></ul>';
          }

          $results = \Civi\Api4\Activity::create(false)
            ->addValue('activity_type_id:name', 'Feedback form submission')
            ->addValue('source_contact_id', $agent['id'])
            ->addValue('target_contact_id', [ $target['id'], ])
            ->addValue('subject', $request->cooked['subject'])
            ->addValue('details', $comment)
            ->addValue('status_id:name', 'Completed');
// @todo: handle Is Christian / Is non-Christian
// ->addValue('Contact_form.Is_Christian:name', 'Christian')
          if ($network) {
            $results = $results
              ->addValue('assignee_contact_id', [ $network['id'], ])
              ->addValue('Contact_form_fields.Network:name', $network['Network_details.Network:name'] ?? $network['name']);
          }
          if ($ministryCentre) {
            $results = $results
              ->addValue('Contact_form_fields.Ministry_Centre:name', $ministryCentre['label']);
          }
          if ($isLead) {
            $results = $results
              ->addValue('Contact_form_fields.Customer_value_journey_stage:name', 'Lead');
          }
          $results = $results->execute();
          foreach ($results as $result) {
            $progress[] = 'Activity ' . $result['id'] . ' created';
          }
        }
      }
    } catch (\API_Exception $e) {
      CRM_Core_Error::debug_var('Feedback webhook exception', $e);
    }
    
    CRM_Core_Error::debug_var('Feedback webhook progress', $progress);
  }
}
