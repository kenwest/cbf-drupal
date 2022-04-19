<?php

/**
 * CBF Webhook Processor Thinkific class.
 *
 * Processes Thinkific enrolments.
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
        'Raw Thinkific data' => $request->raw,
        'Unserialized data' => $request->cooked,
      ]);
      return;
    }
    else {
      CRM_Core_Error::debug_var('Thinkific data', [
        'Raw Thinkific data' => $request->raw,
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
          $enrolmentId = $payload->id;
          $courseId = $payload->course->id;
          $userId = $payload->user->id;
          $percent = round(100 * $payload->percentage_completed);

          CRM_Core_Error::debug_var('Thinkific notification', 'Thinkific ' . $action . ' ' . $resource . ' ' . $enrolmentId . ' for user ' . $userId . ' in course ' . $courseId);

          // Find the Enrolment if it exists
          $enrolments = \Civi\Api4\CustomValue::get('Courses', false)
            ->addWhere('Enrolment_Id', '=', $enrolmentId)
            ->setLimit(1)
            ->execute();
          $enrolmentCount = 0;
          foreach ($enrolments as $_) {
            $enrolmentCount++;
          }

          if ($enrolmentCount == 0) {
            CRM_Core_Error::debug_var('Thinkific processing', 'No ' . $resource . ' ' . $enrolmentId . ' for user ' . $userId . ' in course ' . $courseId);

            // No enrolment found, so find other enrolments by this Thinkific user 
            $contactId = null;
            $contactAction = 'Found';
            $users = \Civi\Api4\CustomValue::get('Courses', false)
              ->addSelect('entity_id')
              ->addWhere('User_Id', '=', $userId)
              ->setLimit(1)
              ->execute();
            foreach ($users as $user) {
              $contactId = $user['entity_id'];
            }

            if (!$contactId) {
              CRM_Core_Error::debug_var('Thinkific processing', 'No ' . $resource . ' records for user ' . $userId);

              // No previous enrolments by this Thinkific user, so try to find a Contact that matches

              $first = $payload->user->first_name;
              $last = $payload->user->last_name;
              $email = $payload->user->email;

              $progress = [];
              $contact = _cbf_webhook_find_existing_contact('', $first, $last, $email, '', $progress);

              if ($progress) {
                CRM_Core_Error::debug_var('Thinkific processing', implode(', ', $progress));
              }

              if ($contact) {
                $contactId = $contact['id'];
              }
              else if ($first && $last && $email) {
                CRM_Core_Error::debug_var('Thinkific processing', 'No contact record for user ' . $userId);

                // No matching Contact found, so create one. May need deduping later.
                $contactAction = 'Created';
                $progress = [];
                $contact = _cbf_webhook_populate_contact('', $first, $last, $email, '', $progress);

                if ($progress) {
                  CRM_Core_Error::debug_var('Thinkific processing', implode(', ', $progress));
                }

                if ($contact) {
                  $contactId = $contact['id'];
                }
              }
            }

            if ($contactId) {
              CRM_Core_Error::debug_var('Thinkific processing', $contactAction . ' contact ' . $contactId . ' for user ' . $userId);
            }
            else {
              CRM_Core_Error::debug_var('Thinkific processing', 'Aborting: could not find or create a contact record corresponding to user ' . $userId);
              break;
            }

            $enrolmentAction = 'create';
            $results = \Civi\Api4\CustomValue::create('Courses', false)
              ->addValue('entity_id', $contactId)
              ->addValue('Enrolment_Id', $enrolmentId);
          }
          else {
            $enrolmentAction = 'update';
            $results = \Civi\Api4\CustomValue::update('Courses', false)
              ->addWhere('Enrolment_Id', '=', $enrolmentId);
          }

          if ($results) {
            $results
              ->addValue('Course', $courseId)
              ->addValue('User_Id', $userId)
              ->addValue('Percent_complete', $percent)
              ->execute();
            $resultCount = 0;
            foreach ($results as $_) {
              $enrolmentAction = ucfirst($enrolmentAction) . 'd';
              CRM_Core_Error::debug_var('Thinkific processing', "$enrolmentAction $resource $enrolmentId for user $userId in course $courseId");
              $resultCount++;
            }
            if ($resultCount == 0) {
              CRM_Core_Error::debug_var('Thinkific processing', "Aborting: could not $enrolmentAction a record for $resource $enrolmentId");
              break;
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
            ->setCheckPermissions(false)
            ->execute();
          $customFields = \Civi\Api4\CustomField::get()
            ->addWhere('custom_group_id:name', '=', 'Courses')
            ->setLimit(25)
            ->setCheckPermissions(false)
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
              $where = "$column = $enrolmentId";
            }
          }
          $assignments = implode(', ', $assignments);
          $update = "update $table set $assignments where $where";
          $_ = CRM_Core_DAO::executeQuery($update);

          /*
           * End of hack
           * ------------------------------------------------------------------------
           */
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
