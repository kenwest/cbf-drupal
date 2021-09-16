<?php

/**
 * @file
 * Example plugin class for logging github pushes to watchdog
 */

/**
 * CBF Webhook Processor Thinkific class.
 *
 * Records github actions in watchdog log.
 */
class CBF_Webhook_Processor_Thinkific implements Webhook_Plugins_Processor_Interface {

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
  public function process(stdClass $data) {
    civicrm_initialize();
    CRM_Core_Error::debug_var('Thinkific headers', [
      'X-Thinkific-Topic' => $_SERVER['HTTP_X_THINKIFIC_TOPIC'],
      'X-Thinkific-Hmac-Sha256' => $_SERVER['HTTP_X_THINKIFIC_HMAC_SHA256'],
      'X-Thinkific-Subdomain' => $_SERVER['HTTP_X_THINKIFIC_SUBDOMAIN'],
    ]);
    CRM_Core_Error::debug_var('Request data', $data);
    $resource = $data->resource;
    $action = $data->action;

    switch ($resource) {
      case 'enrollment':
        try {
          $enrolment = $data->payload->id;
          $course = $data->payload->course->id;
          $user = $data->payload->user->id;
          $percent = (int) 100 * $data->payload->percentage_completed;

          CRM_Core_Error::debug_var('Thinkific notification', 'Thinkific ' . $action . ' ' . $resource . ' ' . $enrolment . ' for user ' . $user . ' in course ' . $course);

          // Find the Enrolment if it exists
          $enrolments = \Civi\Api4\CustomValue::get('Courses')
            ->addWhere('Enrolment_Id', '=', $enrolment)
            ->setLimit(2) // we expect 0 or 1 but we warn of duplicates
            ->setCheckPermissions(false)
            ->execute();
          if ($enrolments->rowCount == 0) {
            CRM_Core_Error::debug_var('Thinkific processing', 'No ' . $resource . ' ' . $enrolment . ' for user ' . $user . ' in course ' . $course);

            // No enrolment found, so find other enrolments by this Thinkific user 
            $users = \Civi\Api4\CustomValue::get('Courses')
              ->addSelect('entity_id')
              ->addWhere('User_Id', '=', $user)
              ->setLimit(2)
              ->setCheckPermissions(false)
              ->execute();
            if ($users->rowCount == 0) {
              CRM_Core_Error::debug_var('Thinkific processing', 'No ' . $resource . ' records for user ' . $user);

              // No previous enrolments by this Thinkific user, so try to find a Contact that matches
              $contacts = \Civi\Api4\Contact::get()
                ->setJoin([['Email AS email', false],])
                ->addWhere('first_name', '=', $data->payload->user->first_name)
                ->addWhere('last_name', '=', $data->payload->user->last_name)
                ->addWhere('email.email', '=', $data->payload->user->email)
                ->setLimit(2)
                ->setCheckPermissions(false)
                ->execute();
              if ($contacts->rowCount == 0) {
                CRM_Core_Error::debug_var('Thinkific processing', 'No contact record for user ' . $user);

                // No matching Contact found, so create one. May need deduping later.
                $contacts = \Civi\Api4\Contact::create()
                ->addValue('first_name', $data->payload->user->first_name)
                ->addValue('last_name', $data->payload->user->last_name)
                  ->addValue('contact_type', 'Individual')
                  ->addChain('email', \Civi\Api4\Email::create()
                    ->addValue('contact_id', '$id')
                    ->addValue('location_type_id', 6 /* Main */)
                    ->addValue('email', $data->payload->user->email)
                    ->addValue('is_primary', TRUE)
                  )
                  ->setCheckPermissions(FALSE)
                  ->execute();
              }
              $contactId = $contacts->first()['id'] ?? null;
              $contactAction = 'Created';
            }
            else {
              $contactId = $users->first()['entity_id'];
              $contactAction = 'Found';
            }

            if ($contactId) {
              CRM_Core_Error::debug_var('Thinkific processing', $contactAction . ' contact ' . $contactId . ' for user ' . $user);
            }
            else {
              CRM_Core_Error::debug_var('Thinkific processing', 'Aborting: could not find or create a contact record corresponding to user ' . $user);
              break;
            }

            CRM_Core_Error::debug_var('Thinkific processing', 'Create ' . $resource . ' ' . $enrolment . ' for user ' . $user . ' in course ' . $course);
            $results = \Civi\Api4\CustomValue::create('Courses')
              ->addValue('entity_id', $contactId)
              ->addValue('Enrolment_Id', $enrolment)
              ->addValue('Course', $course)
              ->addValue('User_Id', $user)
              ->addValue('Percent_complete', $percent)
              ->setCheckPermissions(false)
              ->execute();
            if ($results->first()['is_error']) {
              CRM_Core_Error::debug_var('Thinkific processing', 'Aborting: could not create a record for ' . $resource . ' ' . $enrolment);
              break;
            }
          }
          else {
            CRM_Core_Error::debug_var('Thinkific processing', 'Update ' . $resource . ' ' . $enrolment . ' for user ' . $user . ' in course ' . $course);
            $results = \Civi\Api4\CustomValue::update('Courses')
              ->addWhere('Enrolment_Id', '=', $enrolment)
              ->addValue('Course', $course)
              ->addValue('User_Id', $user)
              ->addValue('Percent_complete', $percent)
              ->setLimit(25)
              ->setCheckPermissions(FALSE)
              ->execute();
            if ($results->first()['is_error']) {
              CRM_Core_Error::debug_var('Thinkific processing', 'Aborting: could not create a record for ' . $resource . ' ' . $enrolment);
              break;
            }
          }

          /*
           * ------------------------------------------------------------------------
           * This is a hack to work around API4 not being able to process date values
           *
           * When we add ->addValue('Completed', $data->payload->completed_at) to the
           * Civi\Api4\CustomValue::create() call, API4 interprets this payload value
           * correctly but then dies in the SQL because the date is not quoted as a
           * string.
           */
          $dateFields = [
            'Created' => $data->payload->created_at,
            'Activated' => $data->payload->activated_at,
            'Started' => $data->payload->started_at,
            'Updated' => $data->payload->updated_at,
            'Completed' => $data->payload->completed_at,
            'Expired' => $data->payload->expiry_date,
          ];
          $dateFieldKeys = array_keys($dateFields);
          $customGroups = \Civi\Api4\CustomGroup::get()
            ->addWhere('name', '=', 'Courses')
            ->setLimit(25)
            ->setCheckPermissions(FALSE)
            ->execute();
          $customFields = \Civi\Api4\CustomField::get()
            ->addWhere('custom_group_id:name', '=', 'Courses')
            ->setLimit(25)
            ->setCheckPermissions(FALSE)
            ->execute();
          $table = $customGroups->first()['table_name'];
          $assignments = [];
          $count = $customFields->count();
          for ($i = 0; $i < $count; $i++) {
            $column = $customFields->itemAt($i)['column_name'];
            $k = $customFields->itemAt($i)['name'];
            if (in_array($k, $dateFieldKeys)) {
              $value = $dateFields[$k];
              if ($value) {
                $value = '"' . date('Y-m-d H:i:s', strtotime($value)) . '"';
              }
              else {
                $value = 'null';
              }
              $assignments[] = "$column = $value";
            }
            else if ($k == 'Enrolment_Id') {
              $where = "$column = $enrolment";
            }
          }
          $assignments = implode(', ', $assignments);
          $update = "update $table set $assignments where $where";
          $outcome = CRM_Core_DAO::executeQuery($update);

          /*
           * End of hack
           * ------------------------------------------------------------------------
           */

          if ($enrolments->rowCount > 1) {
            CRM_Core_Error::debug_var('Thinkific processing', 'Warning: there are multiple records for ' . $resource . ' ' . $enrolment);
          }
        } catch (\API_Exception $e) {
          CRM_Core_Error::debug_var('Thinkific processing', 'Caught exception: ' . $e);
        }
        break;

      default:
        CRM_Core_Error::debug_var('Thinkific notification', 'Thinkific ' . $action . ' ' . $resource . ' but we are ignoring this');
        break;
    }
  }
}
