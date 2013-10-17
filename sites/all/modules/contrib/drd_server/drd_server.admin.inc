<?php

/**
 * @file
 * Provides functionality for configuring drd_server and to return blocks for the admin module.
 */

/**
 * Settings form for the current module.
 *
 * @param array $form
 *   Form array.
 * @param array $form_state
 *   Form_state array.
 *
 * @return array
 *   Array containing the system settings form.
 */
function drd_server_settings($form, $form_state) {
  $form = array();
  $form['drd_allowed_referer'] = array(
    '#type' => 'textfield',
    '#title' => t('Allowed referer IP addresses'),
    '#default_value' => variable_get('drd_allowed_referer', ''),
    '#description' => t('Separate multiple values with a space character.')
  );
  $form['info'] = array(
    '#markup' => '<pre>' . print_r(variable_get('drd_aes_keys', array()), TRUE) . '</pre>',
    '#title' => t('Currently available AES keys'),
  );
  $form['#submit'][] = 'drd_server_settings_submit';
  return system_settings_form($form);
}

/**
 * Submit handler for the settings form.
 *
 * @param array $form
 *   Form definition.
 * @param array $form_state
 *   Form_state array including form values and more.
 */
function drd_server_settings_submit($form, $form_state) {
  $ips = explode(' ', $form_state['values']['drd_allowed_referer']);
  $aes_keys = variable_get('drd_aes_keys', array());
  foreach ($aes_keys as $key => $def) {
    $aes_keys[$key]['keep'] = FALSE;
  }
  $changed = FALSE;
  if (!empty($aes_keys)) {
    $keys = array_keys($aes_keys);
    $master = $aes_keys[$keys[0]];
    foreach ($ips as $ip) {
      if (!array_key_exists($ip, $aes_keys)) {
        $aes_keys[$ip] = $master;
        $aes_keys[$ip]['keep'] = TRUE;
        $changed = TRUE;
      }
      else {
        $aes_keys[$ip]['keep'] = TRUE;
      }
    }
  }
  foreach ($aes_keys as $key => $def) {
    if (!$aes_keys[$key]['keep']) {
      unset($aes_keys[$key]);
      $changed = TRUE;
    }
  }
  if ($changed) {
    variable_set('drd_aes_keys', $aes_keys);
    $sites = drd_server_read_sites();
    drd_server_key_remote($sites, $aes_keys);
  }
}

/**
 * DRD Server Admin block.
 *
 * @return array
 *   Renderable array containing admin details.
 */
function drd_server_admin_block() {
  $block = array(
    'subject' => t('Extra Admin'),
    'content' => '',
  );
  $menu = array();
  if (user_access('administer site configuration')) {
    $menu[] = array(
      'data' => l(t('Run Cron'), 'admin/reports/status/run-cron'),
      'class' => array('leaf'),
    );
    $menu[] = array(
      'data' => l(t('Run Update'), 'update.php'),
      'class' => array('leaf'),
    );
  }
  if (user_access('flush cache')) {
    $menu[] = array(
      'data' => l(t('Flush Cache'), 'admin/drd_server/flush/cache'),
      'class' => array('leaf'),
    );
  }
  if (user_access('translate interface')) {
    $menu[] = array(
      'data' => l(t('Update Translation'), 'admin/drd_server/update/translation'),
      'class' => array('leaf'),
    );
  }
  drupal_alter('drd_server_admin_block', $menu);
  if (empty($menu)) {
    return FALSE;
  }
  $block['content'] = theme('item_list', array('items' => $menu, 'attributes' => array('class' => array('menu'))));
  return $block;
}

/**
 * Function to update translations of the current domain.
 *
 * @param boolean $redirect
 *   If TRUE, this function will perform a redirect using drupal_goto() instead of just returning after completion.
 */
function drd_server_admin_update_translation($redirect = TRUE) {
  if (module_exists('l10n_update')) {
    set_time_limit(0);

    module_load_include('admin.inc', 'l10n_update');
    $steps = array(
      t('Refresh information'),
      t('Update translations'),
    );

    foreach ($steps as $step) {
      $projects = l10n_update_get_projects();
      $languages = l10n_update_language_list('name');
      if ($languages) {
        $history = l10n_update_get_history();
        $available = l10n_update_available_releases();
        $updates = l10n_update_build_updates($history, $available);
        $form_state = array(
          'values' => array(
            'op' => $step,
            'mode' => variable_get('l10n_update_import_mode', LOCALE_IMPORT_KEEP),
            // Send the empty array so that all languages get updated.
            'languages' => array(),
            'updates' => $updates,
          ),
        );
        $form = array();
        l10n_update_admin_import_form_submit($form, $form_state);
      }
    }
    $batch = &batch_get();
    if ($batch && !isset($batch['current_set'])) {
      // Set progressive to FALSE if called from xmlrpc.php.
      $batch['progressive'] = $redirect;
      $batch['form_state'] = array(
        'rebuild' => FALSE,
        'programmed' => FALSE,
      );
      batch_process();
    }
  }

  // Allow other modules as well to jump in with translation update routines.
  module_invoke_all('drd_server_update_translation');

  if ($redirect) {
    drupal_goto($_SERVER['HTTP_REFERER']);
  }
}
