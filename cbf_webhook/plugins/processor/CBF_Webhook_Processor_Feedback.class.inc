<?php

/**
 * CBF Webhook Processor Feedback class.
 *
 * Processes Feedback and Contact forms.
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

    $webhookUrlMap = [
      'contact' => [
        'webhook' => 'Contact',
        'activity' => 'Contact form submission',
      ],
      'course-group' => [
        'webhook' => 'Group licence participation',
        'activity' => 'Course registration',
      ],
      'feedback' => [
        'webhook' => 'Feedback',
        'activity' => 'Feedback form submission',
      ],
    ];

    // The pattern for the webhook path is /webhook/NAME so look up the webhook
    // and the Activity name
    $requestPath = explode('/', request_path());
    $requestPath = end($requestPath);
    $webhook = $webhookUrlMap[$requestPath]['webhook'] ?? 'Unknown';
    $activityName = $webhookUrlMap[$requestPath]['activity'] ?? 'Unknown';

    civicrm_initialize();

    CRM_Core_Error::debug_var("$webhook webhook invoked", [
      'Agent' => $request->agent,
      'Raw data' => $request->raw,
      'Cooked data' => $request->cooked,
    ]);

    if ($webhook == 'Unknown') {
      return;
    }

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
            if (strtolower($firstName) != strtolower($contact['first_name'])
              && !stristr(' ' . $contact['nick_name'] . ',', ' ' . $firstName . ',')) {
              $progress[] = 'Contact ' . $contact['id'] . ' nickname doesn\'t match first name';
              continue;
            }
            if (strtolower($lastName) != strtolower($contact['last_name'])
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
                ->addValue('location_type_id:name', 'Main')
              );
            }
            if ($last9digits) {
              $results = $results->addChain('mobile', \Civi\Api4\Phone::create(false)
                ->addValue('contact_id', '$id')
                ->addValue('phone', $mobile)
                ->addValue('location_type_id:name', 'Main')
                ->addValue('phone_type_id:name', 'Mobile')
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
           * Process any contact details that have been supplied
           */
          if (
            $request->cooked['yearEnteredWorkforce'] ||
            $request->cooked['church']
          ) {
            $results = \Civi\Api4\Contact::update(false);
            if ($request->cooked['yearEnteredWorkforce']) {
              $results->addValue('Classification.Year_started_work', $request->cooked['yearEnteredWorkforce']);
              $progress[] = 'Target entered workforce in ' . $request->cooked['yearEnteredWorkforce'];
            }
            if ($request->cooked['church']) {
              $results->addValue('Classification.Church', $request->cooked['church']);
              $progress[] = 'Target church is \'' . $request->cooked['church'] . '\'';
            }
            $results->addWhere('id', '=', $target['id'])
              ->execute();
            foreach ($results as $result) {
              break;
            }
          }

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
          $isChristian = false;
          $isNonChristian = false;
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
            foreach ($request->cooked['christianIf'] as $v) {
              if (stristr($option, $v) !== false) {
                $isChristian = true;
              }
            }
            foreach ($request->cooked['nonChristianIf'] as $v) {
              if (stristr($option, $v) !== false) {
                $isNonChristian = true;
              }
            }
          }
          if ($isChristian && $isNonChristian) { // Ignore ambiguous instructions
            $isChristian = false;
            $isNonChristian = false;
          }

          /* ------------------------------------------------------------------
           * Process the lists
           */
          if ($isList && $network) {
            foreach ($network['lists'] as $list) {
              $groupContact = null;
              $groupContacts = \Civi\Api4\GroupContact::get(false)
                ->addSelect('id', 'status:label')
                ->addWhere('group_id', '=', $list['group_id'])
                ->addWhere('contact_id', '=', $target['id'])
                ->setLimit(1)
                ->execute();
              foreach ($groupContacts as $groupContact) {
                $progress[] =
                  'Discovered contact ' . $target['id'] .
                  ' has status \'' . $groupContact['status:label'] .
                  '\' in list \'' . $list['group_id:label'] . '\'';
              }
              if (!$groupContact) {
                $results = \Civi\Api4\GroupContact::create(false)
                  ->addValue('group_id', $list['group_id'])
                  ->addValue('contact_id', $target['id'])
                  ->addValue('status', 'Added')
                  ->execute();
                foreach ($results as $result) {
                  $progress[] = 'Added contact ' . $target['id'] . ' to list \'' . $list['group_id:label'] . '\'';
                }
              }
              else if ($groupContact['status:label'] != 'Added') {
                $results = \Civi\Api4\GroupContact::update(false)
                  ->addValue('status:name', 'Added')
                  ->addWhere('id', '=', $groupContact['id'])
                  ->execute();
                foreach ($results as $result) {
                  $progress[] =
                  'Changed contact ' . $target['id'] .
                  '\' status in list \'' . $list['group_id:label'] . '\'' .
                  ' from \'' . $groupContact['status:label'] . '\' to \'Added\'';
                }
              }
            }
          }

          /* ------------------------------------------------------------------
           * Process the campaign
           */
          $campaign = null;
          if ($request->cooked['campaign']) {
            $campaigns = \Civi\Api4\Campaign::get(false)
            ->addSelect('id')
            ->addWhere('title', '=', $request->cooked['campaign'])
            ->addOrderBy('id', 'DESC')
            ->setLimit(1)
            ->execute();
            foreach ($campaigns as $c) {
              $campaign = $c;
            }
          }

          /* --------------------------------------------------------------------
           * Validate the group licence token before processing the Activity
           */
          $addActivity = true;
          $groupLicenceToken = $request->cooked['groupLicenceToken'];
          if ($groupLicenceToken) {
            $groupLicence = null;
            $activities = \Civi\Api4\Activity::get(false)
              ->addSelect(
                'id',
                'activity_date_time',
                'Contact_form_fields.Course',
                'Contact_form_fields.Course:label',
                'Group_license.Group_license_expiry',
                'Group_license.Group_licenses_remaining')
              ->addWhere('Group_license.Group_license_token', '=', $groupLicenceToken)
              ->addWhere('activity_type_id:name', '=', 'Course registration')
              ->setLimit(1)
              ->execute();
            foreach ($activities as $activity) {
              $groupLicence = [
                'activity' => $activity['id'],
                'course' => $activity['Contact_form_fields.Course'],
                'name' => $activity['Contact_form_fields.Course:label'],
                'created' => strtotime($activity['activity_date_time']),
                'expiry' => strtotime($activity['Group_license.Group_license_expiry']),
                'remaining' => 'Group_license.Group_licenses_remaining',
              ];
            }
            $errors = [];
            if (!$groupLicence) {
              $errors[] = 'The supplied token doesn\'t match any Group Licences';
            }
            else {
              if ($groupLicence['expiry'] < strtotime('now')) {
                $errors[] = 'The Group Licence has expired';
              }
              if ($groupLicence['remaining'] == 0) {
                $errors[] = 'The Group Licence has no participations left - all have been used';
              }
            }
            if ($errors) {
              cbf_course_failed_group_license_participation(
                $firstName,
                $lastName,
                $email,
                $groupLicence['name'],
                $errors);
              $addActivity = false;
            }
            else {
              $expiry = new DateObject(strtotime('now') + ($groupLicence['expiry'] - $groupLicence['created']), 'UTC');
              $expiry = $expiry->format('Y-m-d H:i:s');

              cbf_course_group_license_participation(
                $firstName,
                $lastName,
                $email,
                $target['id'],
                $groupLicence['course'],
                $groupLicence['name'],
                $expiry);

              $results = \Civi\Api4\Activity::update(false)
                ->addValue('Group_license.Group_licenses_remaining', $groupLicence['remaining'] - 1)
                ->addWhere('id', '=', $groupLicence['activity'])
                ->execute();
              foreach ($results as $result) {
                // do something
              }
            }
          }

          /* ------------------------------------------------------------------
           * Process the activity
           */
          if ($addActivity) {
            $comment = '<p><strong>Comment</strong>: ' . $request->cooked['comment'] . '</p>';
            $options = implode('</li><li>', array_filter($request->cooked['options']));
            if ($options) {
              $comment .= '<p><strong>Options selected</strong>:</p>';
              $comment .= '<ul><li>' . $options . '</li></ul>';
            }
            if ($groupLicenceToken && ($groupLicence['course'] ?? false)) {
              $actions = "Registered contact in the '{$groupLicence['name']}' group licence $groupLicenceToken";
            }
            if ($actions) {
              $comment .= '<p><strong>System actions</strong>:</p>';
              $comment .= '<ul><li>' . $actions . '</li></ul>';
            }

            $results = \Civi\Api4\Activity::create(false)
            ->addValue('activity_type_id:name', $activityName)
            ->addValue('source_contact_id', $agent['id'])
            ->addValue('target_contact_id', [ $target['id'], ])
            ->addValue('subject', $request->cooked['subject'])
            ->addValue('details', $comment)
            ->addValue('status_id:name', 'Completed');
            if ($network) {
              $results = $results
              ->addValue('assignee_contact_id', [ $network['id'], ])
              ->addValue('Contact_form_fields.Network:name', $network['Network_details.Network:name'] ?? $network['name']);
            }
            if ($ministryCentre) {
              $results = $results
              ->addValue('Contact_form_fields.Ministry_Centre:name', $ministryCentre['label']);
            }
            if ($campaign) {
              $results = $results
              ->addValue('campaign_id', $campaign['id']);
            }
            if ($groupLicenceToken && ($groupLicence['course'] ?? false)) {
              $results = $results
              ->addValue('Contact_form_fields.Course', $groupLicence['course']);
            }
            if ($isLead) {
              $results = $results
              ->addValue('Contact_form_fields.Customer_value_journey_stage:name', 'Lead');
            }
            if ($isChristian) {
              $results = $results
              ->addValue('Contact_form_fields.Is_Christian:name', 'Christian');
            }
            if ($isNonChristian) {
              $results = $results
              ->addValue('Contact_form_fields.Is_Christian:name', 'Non_Christian');
            }
            $results = $results->execute();
            foreach ($results as $result) {
              $progress[] = 'Activity ' . $result['id'] . ' created';
            }
          }
        }
      }
    } catch (\API_Exception $e) {
      CRM_Core_Error::debug_var("$webhook webhook exception", $e);
    }
    
    CRM_Core_Error::debug_var("$webhook webhook progress", $progress);
  }
}
