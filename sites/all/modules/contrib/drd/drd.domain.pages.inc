<?php

/**
 * Builds the output for the domain list in the DRD dashboard including
 * filters, operations and the list of domains with their details.
 *
 * @param type $sid
 * @return type
 */
function drd_domain($sid = 0) {
  if ($sid) {
    $_SESSION['drd']['filter']['domain'] = array();
    if ($sid == 'reset') {
      $_SESSION['drd']['filter']['domain']['sids'] = array();
    }
    else {
      $_SESSION['drd']['filter']['domain']['sids'] = array($sid);
    }
    drupal_goto('admin/drd/domains');
  }
  elseif (!isset($_SESSION['drd']['filter']['domain']['sids'])) {
    $_SESSION['drd']['filter']['domain']['sids'] = array();
  }
  if (isset($_POST['op'])) {
    if ($_POST['op'] == t('Reset')) {
      $_SESSION['drd']['filter']['domain'] = array();
      drupal_goto('admin/drd/domains');
    }
    if ($_POST['op'] == t('Execute')) {
      if (empty($_POST['operation'])) {
        drupal_set_message('No operation selected.', 'warning');
      }
      else if (empty($_POST['domains'])) {
        drupal_set_message('No domain selected.', 'warning');
      }
      else {
        drd_operation('domain', $_POST['domains'], $_POST['operation']);
      }
    }
  }
  drupal_set_title(t('Drupal Remote Dashboard - Domains'));

  $build['serverinfo'] = drd_server_info($_SESSION['drd']['filter']['domain']['sids']);
  $build['domain_filter_form'] = drupal_get_form('drd_domain_filter_form');
  $build['domain_list'] = drupal_get_form('drd_domain_list');

  return $build;
}

/**
 * Builds the domain list according to the current filter settings.
 *
 * @param type $form
 * @param type $form_state
 * @return string
 */
function drd_domain_list($form, &$form_state) {
  $header = array(
    'status' => array('data' => t('Status'),    'field' => 'd.status'),
    'name'   => array('data' => t('Name'),      'field' => 'd.name', 'sort' => 'asc'),
    'aliases'=> array('data' => t('Aliases')),
    'usessl' => array('data' => t('SSL'),       'field' => 'd.usessl'),
    'inst'   => array('data' => t('Installed'), 'field' => 'd.installed'),
    'healthy'=> array('data' => t('Healthy')),
    'secure' => array('data' => t('Secure')),
    'tuning' => array('data' => t('Tuned')),
    'seo'    => array('data' => t('SEO')),
    'other'  => array('data' => t('Other')),
    'heartbeat'=> array(
      'data' => t('Heartbeat') . '<span class="drd-heartbeat-toggle pointer off">' . drd_icon('toggle', 'power', 'heartbeat', 'off') . '</span>'
    ),
  );

  $query = db_select('drd_domain', 'd');

  drd_domain_build_filter_query($query);

  $count_query = clone $query;
  $count_query->addExpression('COUNT(d.did)');

  $query = $query->extend('PagerDefault')->extend('TableSort');
  $query
    ->fields('d')
    ->limit(variable_get('drd_items_in_lists', 25))
    ->orderByHeader($header)
    ->setCountQuery($count_query);
  $result = $query->execute();

  if ($result->rowCount() > 0) {
    drd_operation_build_form($form, 'domain');
  }

  $domains = array();
  foreach ($result as $d) {
    $domains[$d->did] = drd_domain_build_row($d);
  }

  $form['domains'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $domains,
    '#empty' => t('No domains available.'),
  );
  $form['domain_list_pager'] = array('#theme' => 'pager');

  drupal_add_js(array('drd' => array('urlAjax' => url('drd/ajax/domain/'))), 'setting');

  return $form;
}

function drd_domain_build_row($d) {
  static $destination;
  static $charts;
  if (!isset($destination)) {
    $destination = drupal_get_destination();
  }
  if (!isset($charts)) {
    $charts = drd_access('heartbeat');
  }

  $d->name = trim($d->name);
  $d->name = empty($d->name) ? $d->url : $d->name;
  $name = (strlen($d->name) > 35) ? substr($d->name, 0, 30) . '...' : $d->name;
  if ($d->status) {
    $heartbeat   = ($charts) ? l(drd_icon('heartbeat', 'heart'), 'admin/drd/domain/' . $d->did . '/heartbeat', array('html' => TRUE)) : '';
    $stat_health = drd_icon_status($d->status_w_health, $d->status_e_health);
    $stat_secure = drd_icon_status($d->status_w_secure, $d->status_e_secure);
    $stat_tuning = drd_icon_status($d->status_w_tuning, $d->status_e_tuning);
    $stat_seo    = drd_icon_status($d->status_w_seo, $d->status_e_seo);
    $stat_other  = drd_icon_status($d->status_w_other, $d->status_e_other);
  }
  else {
    $heartbeat = $stat_health = $stat_secure = $stat_tuning = $stat_seo = $stat_other = '';
  }
  $edit = drd_icon('edit', 'pencil');
  $link = drd_icon('open', 'extlink');
  $count = db_select('drd_domain_alias', 'a')->fields('a', array('aid'))->condition('a.did', $d->did)->countQuery()->execute()->fetchField() - 1;
  if ($count) {
    $count .= '<span class="drd-display">' . drd_icon('show', 'info');
    $count .= '<div class="detail">';

    $aliases = db_select('drd_domain_alias', 'a')->fields('a', array('url'))->condition('a.did', $d->did)->execute()->fetchAll();
    foreach ($aliases as $alias) {
      $count .= '<p>' . $alias->url . '</p>';
    }

    $count .= '</div>';
    $count .= '</span>';
  }
  $status = ($d->status) ? t('Active') : t('Disabled');

  return array(
    '#attributes' => array(
      'id' => 'drd-domain-' . $d->did,
      'class' => array('drd-data'),
    ),
    'status' => $status,
    'name'   => array(
      'data' => '<span title="' . $d->url . '">' . l($name, 'admin/drd/domain/' . $d->did) . '</span>' .
        l($edit, 'admin/drd/domain/' . $d->did . '/edit', array('html' => TRUE, 'query' => $destination, 'attributes' => array('title' => $d->name))) .
        $heartbeat .
        l($link, drd_url($d), array('html' => TRUE, 'attributes' => array('target' => '_blank'))),
      'class' => array('name'),
    ),
    'aliases'=> $count,
    'usessl' => ($d->usessl) ? t('Yes') : t('No'),
    'inst'   => ($d->installed) ? t('Yes') : t('No'),
    'healthy'=> $stat_health,
    'secure' => $stat_secure,
    'tuning' => $stat_tuning,
    'seo'    => $stat_seo,
    'other'  => $stat_other,
    'heartbeat' => array(
      'data' => '<div oid="' . $d->did . '" class="drd-heartbeat">&nbsp;</div>',
    ),
  );
}

/**
 * Builds the filter form for the domain list.
 *
 * @return type
 */
function drd_domain_filter_form($form, &$form_state) {
  if (!isset($_SESSION['drd']['filter']['domain']['sids'])) {
    $_SESSION['drd']['filter']['domain']['sids'] = array();
  }
  if (!isset($_SESSION['drd']['filter']['domain']['tags'])) {
    $_SESSION['drd']['filter']['domain']['tags'] = array();
  }
  if (!isset($_SESSION['drd']['filter']['domain']['drupalversions'])) {
    $_SESSION['drd']['filter']['domain']['drupalversions'] = array();
  }
  if (!isset($_SESSION['drd']['filter']['domain']['status'])) {
    $_SESSION['drd']['filter']['domain']['status'] = -1;
  }
  if (!isset($_SESSION['drd']['filter']['domain']['ssl'])) {
    $_SESSION['drd']['filter']['domain']['ssl'] = -1;
  }
  if (!isset($_SESSION['drd']['filter']['domain']['installed'])) {
    $_SESSION['drd']['filter']['domain']['installed'] = -1;
  }
  if (!isset($_SESSION['drd']['filter']['domain']['status_w'])) {
    $_SESSION['drd']['filter']['domain']['status_w'] = -1;
  }
  if (!isset($_SESSION['drd']['filter']['domain']['status_e'])) {
    $_SESSION['drd']['filter']['domain']['status_e'] = -1;
  }
  $form['filter_wrapper'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filter'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['filter_wrapper']['filter_sids'] = array(
    '#type' => 'select',
    '#title' => t('Drupal core'),
    '#multiple' => TRUE,
    '#options' => drd_server_load_all(),
    '#default_value' => $_SESSION['drd']['filter']['domain']['sids'],
    '#size' => 10,
  );
  $form['filter_wrapper']['filter_status'] = array(
    '#type' => 'select',
    '#title' => t('Status'),
    '#options' => array(
      '-1' => t('- any -'),
      '1' => t('Active'),
      '0' => t('Disabled'),
    ),
    '#default_value' => $_SESSION['drd']['filter']['domain']['status'],
  );
  $form['filter_wrapper']['filter_ssl'] = array(
    '#type' => 'select',
    '#title' => t('SSL'),
    '#options' => array(
      '-1' => t('- any -'),
      '1' => t('Yes'),
      '0' => t('No'),
    ),
    '#default_value' => $_SESSION['drd']['filter']['domain']['ssl'],
  );
  $form['filter_wrapper']['filter_installed'] = array(
    '#type' => 'select',
    '#title' => t('Installed'),
    '#options' => array(
      '-1' => t('- any -'),
      '1' => t('Yes'),
      '0' => t('No'),
    ),
    '#default_value' => $_SESSION['drd']['filter']['domain']['installed'],
  );
  $form['filter_wrapper']['filter_status_w'] = array(
    '#type' => 'select',
    '#title' => t('Warning Status'),
    '#options' => array(
      '-1' => t('- any -'),
      '1' => t('Yes'),
      '0' => t('No'),
    ),
    '#default_value' => $_SESSION['drd']['filter']['domain']['status_w'],
  );
  $form['filter_wrapper']['filter_status_e'] = array(
    '#type' => 'select',
    '#title' => t('Error Status'),
    '#options' => array(
      '-1' => t('- any -'),
      '1' => t('Yes'),
      '0' => t('No'),
    ),
    '#default_value' => $_SESSION['drd']['filter']['domain']['status_e'],
  );
  $form['filter_wrapper']['filter_drupalversions'] = array(
    '#type' => 'select',
    '#title' => t('Drupal Versions'),
    '#multiple' => TRUE,
    '#options' => drd_drupalversion_load_all(),
    '#default_value' => $_SESSION['drd']['filter']['domain']['drupalversions'],
  );

  _drd_tags_build_filter_form('domain', $form);

  $form['filter_wrapper']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Apply'),
  );
  $form['filter_wrapper']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset'),
  );
  return $form;
}

function drd_domain_filter_form_validate($form, &$form_state) {
}

function drd_domain_filter_form_submit($form, $form_state) {
  $values = $form_state['values'];
  if ($_POST['op'] == t('Apply')) {
    $_SESSION['drd']['filter']['domain']['sids'] = $values['filter_sids'];
    $_SESSION['drd']['filter']['domain']['status'] = $values['filter_status'];
    $_SESSION['drd']['filter']['domain']['ssl'] = $values['filter_ssl'];
    $_SESSION['drd']['filter']['domain']['installed'] = $values['filter_installed'];
    $_SESSION['drd']['filter']['domain']['status_w'] = $values['filter_status_w'];
    $_SESSION['drd']['filter']['domain']['status_e'] = $values['filter_status_e'];
    $_SESSION['drd']['filter']['domain']['tags'] = $values['filter_tags'];
    $_SESSION['drd']['filter']['domain']['drupalversions'] = $values['filter_drupalversions'];
  }
}

/**
 * Builds the filter query for the domain list.
 *
 * @param type $query
 */
function drd_domain_build_filter_query(&$query) {
  if (!empty($_SESSION['drd']['filter']['domain']['sids'])) {
    $query->condition('d.sid', $_SESSION['drd']['filter']['domain']['sids']);
  }
  if ($_SESSION['drd']['filter']['domain']['status'] >= 0) {
    $query->condition('d.status', $_SESSION['drd']['filter']['domain']['status']);
  }
  if ($_SESSION['drd']['filter']['domain']['ssl'] >= 0) {
    $query->condition('d.usessl', $_SESSION['drd']['filter']['domain']['ssl']);
  }
  if ($_SESSION['drd']['filter']['domain']['installed'] >= 0) {
    $query->condition('d.installed', $_SESSION['drd']['filter']['domain']['installed']);
  }
  if ($_SESSION['drd']['filter']['domain']['status_w'] >= 0) {
    $query->condition('d.status_w', $_SESSION['drd']['filter']['domain']['status_w']);
  }
  if ($_SESSION['drd']['filter']['domain']['status_e'] >= 0) {
    $query->condition('d.status_e', $_SESSION['drd']['filter']['domain']['status_e']);
  }
  if (!empty($_SESSION['drd']['filter']['domain']['drupalversions'])) {
    $query->join('drd_server', 's', 'd.sid = s.sid');
    $query->condition('s.drupalversion', $_SESSION['drd']['filter']['domain']['drupalversions']);
  }
  if (!empty($_SESSION['drd']['filter']['domain']['tags'])) {
    $query->join('drd_term', 't', 'd.did = t.oid');
    $query->condition('t.mode', 'domain')
        ->condition('t.tid', $_SESSION['drd']['filter']['domain']['tags']);
  }
}
