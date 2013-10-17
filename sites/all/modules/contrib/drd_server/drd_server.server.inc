<?php

/**
 * @file
 * Provides core related functionality triggered by DRD.
 */

/**
 * DRD Operation to find out all domains on this Drupal installation.
 *
 * @return string
 *   Returnable string to DRD.
 */
function drd_server_server_domains() {
  $sites = drd_server_read_sites();
  $output = array();
  $error = array();
  foreach ($sites as $url => $id) {
    if (isset($output[$id])) {
      $output[$id]['aliases'][] = $url;
    }
    elseif (isset($error[$id])) {
      // Ignore this alias, it won't work.
    }
    else {
      $code = drd_server_server_check_host($url);
      if ($code === FALSE) {
        $error[$id] = TRUE;
      }
      else {
        $output[$id] = array(
          'url' => $url,
          'aliases' => array(),
          'drd installed' => ($code === TRUE),
        );
      }
    }
  }
  return drd_server_result('server.domains', $output);
}

/**
 * Function called by drd_server_server_domains() to check a specific URL
 * if it really exists and if the module drd_server is installed.
 *
 * @param string $url
 *   The url that should be checked.
 *
 * @return boolean
 *   TRUE if domain is reachable, FALSE otherwise.
 */
function drd_server_server_check_host($url) {
  $id = rand();
  try {
    $check = drupal_http_request('http://' . $url . '/admin/drd_server/' . $id);
    $ok = (($check->code == 200) || ($check->code == 302 && $check->redirect_code == 200));
    if ($ok && $check->data == ($id / 2)) {
      return TRUE;
    }
    return $check->code;
  }
  catch (Exception $e) {
    return FALSE;
  }
}

/**
 * DRD Operation to determine all available updates for the current core including all disabled modules.
 *
 * @return string
 *   Returnable string to DRD.
 */
function drd_server_server_list_updates() {
  if (!module_exists('update')) {
    module_enable(array('update'));
  }
  $org_setting = variable_get('update_check_disabled', FALSE);
  variable_set('update_check_disabled', TRUE);
  variable_set('update_last_check', 0);
  $result = drd_server_domain_list_updates('server');
  variable_set('update_check_disabled', $org_setting);

  // Just return result as is, it has been processed already.
  return $result;
}

/**
 * DRD Operation to retrieve the PHP error log from the OS.
 *
 * @param int $max_length
 *   Number of bytes that shold be retrieved from the end of the php error log.
 *
 * @return string
 *   Returnable string to DRD.
 */
function drd_server_server_php_error_log($max_length = 5000) {
  $log_file = ini_get('error_log');
  if (!file_exists($log_file)) {
    return drd_server_error(t('PHP Error Log does not exist.'), DRD_ERROR_NO_LOG_PHP);
  }
  $offset = max(-1, (filesize($log_file) - $max_length));
  $log = file_get_contents($log_file, FILE_BINARY, NULL, $offset);
  $result['cache']['php_error_log']['server'] = array(
    'title' => 'PHP Error Log',
    'callback' => 'drupal_render',
    'arguments' => array(
      'data' => array(
        '#markup' => $log,
        '#prefix' => '<pre>',
        '#suffix' => '</pre>',
      ),
    ),
  );
  return drd_server_result('server.php.error.log', $result);
}
