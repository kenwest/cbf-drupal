<?php

/**
 * Webhook unserializer plugin for Zapier data.
 */
class CBF_Webhook_Unserializer_Zapier implements Webhook_Plugins_Unserializer_Interface {

  public function unserialize($data) {

    $json = webhook_load_unserializer('json');

    $result = new stdClass();
    $result->agent = $_SERVER['HTTP_WEBHOOK_AGENT'] ?? $_SERVER['HTTP_WEBHOOKAGENT'] ?? '';
    $result->raw = $data ?? '';

    $cooking = $json->unserialize($result->raw);
    $cooked = [];
    $map = [ // Fields have various external names, and need filtering
      'id' => [
        'name' => ['contact', 'id'], // the first listed has priority
        'validate' => FILTER_VALIDATE_INT,
        'options' => ['options' => ['default' => 0]],
      ],
      'first' => [
        'name' => ['firstName', 'first_name', 'first'],
      ],
      'last' => [
        'name' => ['lastName', 'last_name', 'last'],
      ],
      'email' => [
        'name' => ['email'],
        'sanitise' => FILTER_SANITIZE_EMAIL,
        'validate' => FILTER_VALIDATE_EMAIL,
        'options' => ['options' => ['default' => '']],
      ],
      'mobile' => [
        'name' => ['mobile', 'phone'],
      ],
      'ministryCentre' => [
        'name' => ['ministryCentre'],
      ],
      'network' => [
        'name' => ['network'],
      ],
      'options' => [
        'name' => ['options', 'actions'],
        'separator' => ', ',
      ],
      'subject' => [
        'name' => ['subject'],
      ],
      'comment' => [
        'name' => ['comment'],
      ],
      'campaign' => [
        'name' => ['campaign'],
      ],
      'listIf' => [
        'name' => ['listIf', 'list_if', 'list'],
        'separator' => ', ',
      ],
      'leadIf' => [
        'name' => ['leadIf', 'lead_if', 'lead'],
        'separator' => ', ',
      ],
      'christianIf' => [
        'name' => ['christianIf', 'christian_if', 'christian'],
        'separator' => ', ',
      ],
      'nonChristianIf' => [
        'name' => ['nonChristianIf', 'non_christian_if', 'non_christian'],
        'separator' => ', ',
      ],
      'timestamp' => [
        'name' => ['timestamp'],
      ],
    ];

    // Get internal fields from external
    foreach ($map as $field => $source) {
      $t = null;
      foreach ($source['name'] as $name) {
        $t = $t ?? $cooking->$name ?? null;
      }
      $t = htmlspecialchars(trim($t));
      $t = filter_var($t, $source['sanitise'] ?? FILTER_UNSAFE_RAW);
      $t = filter_var($t, $source['validate'] ?? FILTER_UNSAFE_RAW, $source['options'] ?? []);
      if ($source['separator'] ?? false) {
        $t = explode($source['separator'], $t);
      }
      $cooked[$field] = $t;
    }

    $result->cooked = $cooked;

    return $result;
  }
}
