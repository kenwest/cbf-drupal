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

    CRM_Core_Error::debug_var('Feedback webhook invoked', [
      '$_SERVER' => $request->_server,
      'Raw data' => $request->raw,
      'Unserialized data' => $request->cooked,
    ]);
    return;
  }
}
