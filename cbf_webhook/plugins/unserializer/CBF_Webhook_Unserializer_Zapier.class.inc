<?php

/**
 * Webhook unserializer plugin for Zapier data.
 */
class CBF_Webhook_Unserializer_Zapier implements Webhook_Plugins_Unserializer_Interface {

  public function unserialize($data) {

    $json = webhook_load_unserializer('json');

    $result = new stdClass();
    $result->agent = $_SERVER['HTTP_WEBHOOK_AGENT'] ?? $_SERVER['HTTP_WEBHOOKAGENT'] ?? '';
    $result->raw = $data;
    $result->cooked = $json->unserialize($data);

    return $result;
  }
}
