<?php

/**
 * @file
 * Example plugin class for unserializing Thinkific data.
 */

/**
 * Webhook unserializer plugin for Thinkific data.
 */
class CBF_Webhook_Unserializer_Thinkific implements Webhook_Plugins_Unserializer_Interface {

  public function unserialize($data) {

    $hmac = drupal_hmac_base64($data, variable_get('cbf_thinkific_api'));

    $json = webhook_load_unserializer('json');

    $result = new stdClass();
    $result->hmac = new stdClass();
    $result->hmac->them = $_SERVER['HTTP_X_THINKIFIC_HMAC_SHA256'];
    $result->hmac->us = $hmac;
    $result->subdomain = new stdClass();
    $result->subdomain->them = $_SERVER['HTTP_X_THINKIFIC_SUBDOMAIN'];
    $result->subdomain->us = variable_get('cbf_thinkific_subdomain');
    $result->raw = $data;
    $result->cooked = $json->unserialize($data);

    return $result;
  }
}
