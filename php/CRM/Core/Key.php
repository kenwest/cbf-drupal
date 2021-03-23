<?php
/*
 +--------------------------------------------------------------------+
 | Copyright CiviCRM LLC. All rights reserved.                        |
 |                                                                    |
 | This work is published under the GNU AGPLv3 license with some      |
 | permitted exceptions and without any warranty. For full license    |
 | and copyright information, see https://civicrm.org/licensing       |
 +--------------------------------------------------------------------+
 */

/**
 *
 * @package CRM
 * @copyright CiviCRM LLC https://civicrm.org/licensing
 * $Id$
 *
 */
class CRM_Core_Key {

  /**
   * The length of the randomly-generated, per-session signing key.
   *
   * Expressed as number of bytes. (Ex: 128 bits = 16 bytes)
   *
   * @var int
   */
  const PRIVATE_KEY_LENGTH = 16;

  /**
   * @var string
   * @see hash_hmac_algos()
   */
  const HASH_ALGO = 'sha256';

  /**
   * The length of a generated signature/digest (expressed in hex digits).
   * @var int
   */
  const HASH_LENGTH = 64;

  public static $_key = NULL;

  public static $_sessionID = NULL;

  /**
   * Generate a private key per session and store in session.
   *
   * @return string
   *   private key for this session
   */
  public static function privateKey() {
    if (!self::$_key) {
      $session = CRM_Core_Session::singleton();
      self::$_key = $session->get('qfPrivateKey');
      if (!self::$_key) {
        self::$_key = base64_encode(random_bytes(self::PRIVATE_KEY_LENGTH));
        $session->set('qfPrivateKey', self::$_key);
      }
    }
    return self::$_key;
  }

  /**
   * @return mixed|null|string
   */
  public static function sessionID() {
    if (!self::$_sessionID) {
      $session = CRM_Core_Session::singleton();
      self::$_sessionID = $session->get('qfSessionID');
      if (!self::$_sessionID) {
        self::$_sessionID = session_id();
        $session->set('qfSessionID', self::$_sessionID);
      }
    }
    return self::$_sessionID;
  }

  /**
   * Generate a form key based on form name, the current user session
   * and a private key. Modelled after drupal's form API
   *
   * @param string $name
   * @param bool $addSequence
   *   Should we add a unique sequence number to the end of the key.
   *
   * @return string
   *   valid formID
   */
  public static function get($name, $addSequence = FALSE) {
    $key = self::sign($name);

    if ($addSequence) {
      // now generate a random number between 1 and 100K and add it to the key
      // so that we can have forms in mutiple tabs etc
      $key = $key . '_' . mt_rand(1, 10000);
    }
    return $key;
  }

  /**
   * Validate a form key based on the form name.
   *
   * @param string $key
   * @param string $name
   * @param bool $addSequence
   *
   * @return string
   *   if valid, else null
   */
  public static function validate($key, $name, $addSequence = FALSE) {
    if (stripos($_SERVER['HTTP_REFERER'], 'paypal.com') !== false) {
      if (isset($_SERVER['HTTP_X_FORWARDED_FOR'])) {
        $ip  = $_SERVER['HTTP_X_FORWARDED_FOR'];
        $url = $_SERVER['HTTP_X_FORWARDED_PROTO'] . '://' . $_SERVER['HTTP_X_HOST'] . $_SERVER['REQUEST_URI'];
      }
      else {
        $ip  = $_SERVER['REMOTE_ADDR'];
        $url = $_SERVER['REQUEST_SCHEME'] . '://' . $_SERVER['SERVER_NAME'] . $_SERVER['REQUEST_URI'];
      }
      CRM_Core_Error::debug_var('Key validate', [
        'key' => $key,
        'name' => $name,
        'addSequence' => $addSequence,
        'privateKey' => self::privateKey(),
        'sessionID' => self::sessionID(),
        'signed' => self::sign($name),
        'ip' => $ip,
        'url' => $url,
      ]);
    }

    if (!is_string($key)) {
      return NULL;
    }

    if ($addSequence) {
      list($k, $t) = explode('_', $key);
      if ($t < 1 || $t > 10000) {
        return NULL;
      }
    }
    else {
      $k = $key;
    }

    if (!hash_equals($k, self::sign($name))) {
      return NULL;
    }
    return $key;
  }

  /**
   * @param $key
   *
   * @return bool
   *   TRUE if the signature ($key) is well-formed.
   */
  public static function valid($key) {
    // a valid key is a hex number
    // followed by an optional _ and a number between 1 and 10000
    if (strpos('_', $key) !== FALSE) {
      list($hash, $seq) = explode('_', $key);

      // ensure seq is between 1 and 10000
      if (!is_numeric($seq) ||
        $seq < 1 ||
        $seq > 10000
      ) {
        return FALSE;
      }
    }
    else {
      $hash = $key;
    }

    // ensure that hash is a hex number (of expected length)
    return preg_match('#[0-9a-f]{' . self::HASH_LENGTH . '}#i', $hash) ? TRUE : FALSE;
  }

  /**
   * @param string $name
   *   The name of the form
   * @return string
   *   A signed digest of $name, computed with the per-session private key
   */
  private static function sign($name) {
    $privateKey = self::privateKey();
    $sessionID = self::sessionID();
    $delim = chr(0);
    if (strpos($sessionID, $delim) !== FALSE || strpos($name, $delim) !== FALSE) {
      throw new \RuntimeException("Failed to generate signature. Malformed session-id or form-name.");
    }
    // Note: Unsure why $sessionID is included, but it's always been there, and it doesn't seem harmful.
    return hash_hmac(self::HASH_ALGO, $sessionID . $delim . $name, $privateKey);

  }

}
