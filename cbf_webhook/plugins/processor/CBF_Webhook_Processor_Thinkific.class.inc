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
  public function process(stdClass $request) {

    civicrm_initialize();

    if ($request->hmac->us != $request->hmac->them || $request->subdomain->us != $request->subdomain->them) {
      CRM_Core_Error::debug_var('Thinkific header mismatch', [
        'Supplied HMAC' => $request->hmac->them,
        'Generated HMAC' => $request->hmac->us,
        'Supplied subdomain' => $request->subdomain->them,
        'Local subdomain' => $request->subdomain->us,
        'Raw data' => $request->raw,
        'Unserialized data' => $request->cooked,
      ]);
    }

    $data = $request->cooked;
    $resource = $data->resource;
    $action = $data->action;
    $payload = $data->payload;

    switch ($resource) {
      case 'enrollment':
        try {
          $enrolment = $payload->id;
          $course = $payload->course->id;
          $user = $payload->user->id;
          $percent = (int) 100 * $payload->percentage_completed;

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
                ->addWhere('first_name', '=', $payload->user->first_name)
                ->addWhere('last_name', '=', $payload->user->last_name)
                ->addWhere('email.email', '=', $payload->user->email)
                ->setLimit(2)
                ->setCheckPermissions(false)
                ->execute();
              if ($contacts->rowCount == 0) {
                CRM_Core_Error::debug_var('Thinkific processing', 'No contact record for user ' . $user);

                // No matching Contact found, so create one. May need deduping later.
                $contacts = \Civi\Api4\Contact::create()
                ->addValue('first_name', $payload->user->first_name)
                ->addValue('last_name', $payload->user->last_name)
                  ->addValue('contact_type', 'Individual')
                  ->addChain('email', \Civi\Api4\Email::create()
                    ->addValue('contact_id', '$id')
                    ->addValue('location_type_id', 6 /* Main */)
                    ->addValue('email', $payload->user->email)
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
            else {
              CRM_Core_Error::debug_var('Thinkific processing', 'Created ' . $resource . ' ' . $enrolment . ' for user ' . $user . ' in course ' . $course);
            }
          }
          else {
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
            else {
              CRM_Core_Error::debug_var('Thinkific processing', 'Updated ' . $resource . ' ' . $enrolment . ' for user ' . $user . ' in course ' . $course);
            }
          }

          /*
           * ------------------------------------------------------------------------
           * This is a hack to work around API4 not being able to process date values
           *
           * When we add ->addValue('Completed', $payload->completed_at) to the
           * Civi\Api4\CustomValue::create() call, API4 interprets this payload value
           * correctly but then dies in the SQL because the date is not quoted as a
           * string.
           */
          $dateFields = [
            'Created' => $payload->created_at,
            'Activated' => $payload->activated_at,
            'Started' => $payload->started_at,
            'Updated' => $payload->updated_at,
            'Completed' => $payload->completed_at,
            'Expired' => $payload->expiry_date,
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
