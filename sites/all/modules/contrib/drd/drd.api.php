<?php

/**
 * @defgroup drd_hooks DRD hook functions
 *
 * Core hooks for the DRD module suite.
 */

/**
 * @file
 * API documentation file.
 *
 * @ingroup drd_hooks
 */

/**
 * TODO: Add to return value documentation.
 * - you can user drupal_set_message during the execution of an operation. that message(s) will be displayed in your central DRD
 * - The return array can contain things like:
 *    - 'cache': array with 'info', etc
 *    - 'drd_db': array with instructions to execute sql queries
 *    -
 */

/**
 * Defines operations for your DRD dashboard.
 *
 * All the operations will be visible in the operations drop down above
 * the core list or the domain list in your DRD dashboard or on the page
 * with details of a domain or core.
 *
 * @return array
 *   Keyed array with operation definitions as arrays with the following possible keys:
 *   - category: (optional) All the operations are grouped in the drop down in your DRD by categories
 *        and here you can define your own category. It defaults to "- Main operations -".
 *   - label: (required) A label for this operation that's been used in the drop down list.
 *   - mode: (optional) Either "server" or "domain" (default). Determines whether this operation
 *        is displayed on the core list or the domain list in your DRD.
 *   - callback: (required) The name of a valid function that will be executed when this operation
 *        gets selected and executed.
 *   - file: (optional) If the function given in callback sits in a different file which won't be loaded
 *        by Drupal's bootstrap then you should provide the filename here so that the file gets loaded
 *        before the callback will be executed.
 *   - remote: (optional) TRUE or FALSE (default) to determine if the operation should be executed
 *        locally at your DRD or remotely at the selected core(s) or domain(s).
 *   - fields: (optional) An array to define any number of form fields (@see FAPI) that will be displayed
 *        next to the operations drop down list in your DRD and the field values will be forwarded to the
 *        function being given as the callback.
 *   - follower: (optional) An array of other DRD operations that will be executed subsequently after this
 *        operation, if execution finished successfully.
 *   - queue: (optional) TRUE (default) if this operation should be executed directly and the calling
 *        browser instance should be waiting for the result or FALSE to put the operation into the queue
 *        to be executed one after the other without holding up the interface.
 *   - refresh: (optional) TRUE if the selected core or domain should be updated in their list after
 *        successful execution of this operation or FALSE (default) otherwise.
 *
 * @ingroup drd_hooks
 */
function hook_drd_operations() {
  $operations = array(
    'mykey' => array(
      'category' => t('My Category'),
      'label' => t('My label for the operation'),
      'mode' => 'server',
      'remote' => TRUE,
      'fields' => array(
        'reset' => array(
          '#type' => 'checkbox',
          '#title' => t('Reset'),
          '#default_value' => FALSE,
        ),
      ),
    ),
  );
  return $operations;
}

function hook_drd_config_server() {
  $form = array();
  return $form;
}

function hook_drd_config_domain() {
  $form = array();
  return $form;
}

function hook_drd_svn_module() {

}

function hook_drd_server($mode, $drd_result, $args) {
  return $drd_result;
}

/**
 * TODO
 *
 * @return array
 *   Keyed array with operation definitions like in @see hook_drd_operations()
 *
 * @ingroup drd_hooks
 */
function hook_drd_server_operations() {

}

function hook_drd_server_update_translation() {

}
