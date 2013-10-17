<?php

/**
 * Settings form for the DRD module.
 *
 * @param type $form
 * @param type $form_state
 * @return type
 */
function drd_admin_settings($form, $form_state) {
  drupal_set_title(t('Drupal Remote Dashboard - Settings'));
  $form = array();
  $form['drd_debug'] = array(
    '#type' => 'checkbox',
    '#title' => t('Debug'),
    '#default_value' => variable_get('drd_debug', FALSE),
  );
  $form['drd_items_in_lists'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of items in lists'),
    '#default_value' => variable_get('drd_items_in_lists', 25),
  );

  // Display regular tasks
  $tasks = variable_get('drd_tasks', array());
  if (!empty($tasks)) {
    $operations = module_invoke_all('drd_operations');
    $form['tasks'] = array(
      '#type' => 'fieldset',
      '#title' => t('Tasks'),
    );
    foreach ($tasks as $key => $task) {
      $kkey = 'def_' . $key;
      $form['tasks'][$kkey] = array(
        '#type' => 'container',
      );
      $form['tasks'][$kkey]['button_' . $kkey] = array(
        '#type' => 'submit',
        '#value' => t('Delete task #!n', array('!n' => $key)),
      );
      $form['tasks'][$kkey]['info'] = array(
        '#markup' => t('@label: execute every !period, next execution scheduled for !time', array(
          '@label' => $operations[$task['op']]['label'],
          '!period' => format_interval($task['period']),
          '!time' => format_date($task['next']),
        )),
      );
    }
  }

  // Globally ignore requirements
  drd_requirements_build_form($form);

  $form['#submit'][] = 'drd_admin_settings_submit';

  return system_settings_form($form);
}

function drd_admin_settings_submit($form, $form_state) {
  $parts = explode('#', $_POST['op']);
  if (count($parts) > 1) {
    $key = (int)$parts[1];
    $tasks = variable_get('drd_tasks', array());
    unset($tasks[$key]);
    variable_set('drd_tasks', $tasks);
  }

  // Handle the requirements ignore settings
  $ignore = $form_state['values']['drd_requirements_ignore'];
  $changed = (serialize(variable_get('drd_requirements_ignore', array())) != serialize($ignore));
  if ($changed) {
    variable_set('drd_requirements_ignore', $ignore);
    $ids = drd_domain_load_all('settings');
    foreach ($ids as $id => $settings) {
      if (!empty($settings)) {
        $settings = unserialize($settings);
        if (!isset($settings['ignore'])) {
          $settings['ignore'] = array();
        }
      }
      else {
        $settings = array('ignore' => array());
      }
      $requirements = drd_domain_load_requirements($id);
      $update = _drd_update_domain_requirements($requirements, $settings['ignore']);
      db_update('drd_domain')
        ->fields($update)
        ->condition('did', $id)
        ->execute();
    }
  }
}

function drd_admin_add_task($form, $form_state, $mode) {
  drupal_set_title(t('Add new regular task'));
  $form['period'] = array(
    '#type' => 'textfield',
    '#title' => t('Period in seconds between tasks'),
  );
  drd_operation_build_form($form, $mode, TRUE);
  $form['options']['submit']['#value'] = t('Save');
  return $form;
}

function drd_admin_add_task_submit($form, $form_state) {
  $values = $form_state['values'];
  $tasks = variable_get('drd_tasks', array());
  $tasks[] = array(
    'mode' => $form['#drd-mode'],
    'next' => REQUEST_TIME + $values['period'],
    'period' => $values['period'],
    'op' => $values['operation'],
    'param' => $values,
  );
  variable_set('drd_tasks', $tasks);
  drupal_goto('admin/drd/settings');
}

function drd_admin_reset_projects($form, $form_state) {
  $form['info'] = array(
    '#markup' => t('Are you sure? This will delete all usage data about modules and themes and you will have to re-collect them from your remote domains.'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Yes, purge all data!'),
  );
  return $form;
}

function drd_admin_reset_projects_submit($form, $form_state) {
  db_truncate('drd_project_release_domain')->execute();
  db_truncate('drd_project_release')->execute();
  db_truncate('drd_project')->execute();
  drupal_set_message('Usage data has been purged.');
  drupal_goto('admin/drd/settings');
}