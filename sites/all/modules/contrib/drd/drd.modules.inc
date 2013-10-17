<?php
/**
 * Created by JetBrains PhpStorm.
 * User: jurgenhaas
 * Date: 07.04.12
 * Time: 11:56
 * To change this template use File | Settings | File Templates.
 */

function drd_modules($mode = 'all', $sid = 0, $did = 0) {
  drupal_set_title(t('Drupal Remote Dashboard - Modules'));
  include_once 'drd.server.pages.inc';
  $build['info'] = drupal_get_form('drd_server_filter_form');

  $queue = db_select('drd_project', 'p');
  $queue->innerJoin('drd_project_release', 'pr', 'p.pid = pr.pid');
  $queue->innerJoin('drd_project_release_domain', 'prd', 'pr.rid = prd.rid');
  $queue->innerJoin('drd_domain', 'd', 'prd.did = d.did');
  $queue->innerJoin('drd_server', 's', 'd.sid = s.sid');
  $queue->orderBy('p.type');
  $queue->orderBy('p.name');
  $queue->orderBy('pr.version');
  $queue->orderBy('s.sid');
  $queue->fields('p')
    ->fields('pr', array('rid', 'version'))
    ->fields('d', array('did'))
    ->fields('s', array('sid'));
  $queue->addField('pr', 'link', 'pr_link');
  $queue->addField('pr', 'timestamp', 'pr_timestamp');
  $queue->addField('pr', 'details', 'pr_details');
  $queue->addField('d', 'name', 'domain_name');
  $queue->addField('d', 'url', 'domain_url');
  $queue->addField('s', 'name', 'server_name');
  if ($sid) {
    $queue->condition('s.sid', $sid);
    $build['info'] = drd_server_info($sid);
  }
  elseif ($did) {
    $queue->condition('d.did', $did);
    $build['info'] = drd_domain_info($did);
  }
  else {
    drd_server_build_filter_query($queue);
  }
  $modules = $queue->execute();
  $servers = array();
  $data = array();
  while ($module = $modules->fetch()) {
    $details = unserialize($module->details);
    $info = _drd_modules_status($details[$module->version]['status']);
    if ($mode == 'updates' && $info['type'] == 'ok') {
      // Ignore this.
      continue;
    }
    if ($mode == 'security' && $info['type'] != 'security') {
      // Ignore this.
      continue;
    }

    if (empty($data[$module->type][$module->pid])) {
      $data[$module->type][$module->pid] = array(
        'title' => $module->title,
        'link' => $module->link,
        'details' => $details,
        'versions' => array(),
      );
    }
    if (empty($servers[$module->sid])) {
      $servers[$module->sid] = $module;
    }
    if (empty($data[$module->type][$module->pid]['versions'][$module->version])) {
      $data[$module->type][$module->pid]['versions'][$module->version] = array(
        'rid' => $module->rid,
        'version' => $module->version,
        'details' => unserialize($module->pr_details),
        'servers' => array(),
      );
    }
    $data[$module->type][$module->pid]['versions'][$module->version]['servers'][$module->sid][] = $module->did;
  }

  $header = array(
    'project' => array('data' => ''),
    'version' => array('data' => ''),
    'update' => array('data' => ''),
  );
  foreach ($servers as $server) {
    $header['s_' . $server->sid] = array('data' => '<span title="' . $server->server_name . '">' . substr($server->server_name, 0, 4) . '...</span>', 'class' => 'server');
  }

  $rows = array();
  $c = 3 + count($servers);
  $link = drd_icon('open', 'extlink');
  foreach ($data as $type => $installs) {
    $rows[$type] = array(
      'data' => array(
        'project' => array(
          'data' => '<h2>' . $type . '</h2>',
          'colspan' => $c,
        ),
      ),
      'class' => array('drd-type'),
    );

    foreach ($installs as $pid => $install) {
      $linktext = empty($install['link']) ? '' : l($link, $install['link'], array('html' => TRUE, 'attributes' => array('target' => '_blank')));
      $rows['p_' . $pid] = array(
        'data' => array(
          'project' => array(
            'data' => $install['title'] . $linktext,
            'colspan' => $c,
          ),
        ),
        'class' => array('drd-project'),
      );

      foreach ($install['versions'] as $v => $version) {
        $info = _drd_modules_status($install['details'][$version['version']]['status']);
        $extra = empty($info['title']) ? '' : '<span class="description">' . $info['title'] . '</span>';
        $update = '';
        if (empty($info['ok'])) {
          $r = $install['details'][$version['version']]['recommended'];
          if (empty($install['versions'][$r])) {
            $data[$type][$pid]['versions'][$r]['details'] = unserialize(
              db_select('drd_project_release', 'pr')
                ->fields('pr', array('details'))
                ->condition('pr.pid', $pid)
                ->condition('pr.version', $r)
                ->execute()
                ->fetchField()
            );
            $install['versions'][$r] = $data[$type][$pid]['versions'][$r];
          }
          if (empty($install['versions'][$r])) {
            $update = 'error';
          }
          else {
            $x = $install['versions'][$r]['details'];
            $label = empty($x['version']) ? 'unknown' : $x['version'];
            $update = l($label, $x['download_link']);
          }
        }

        $rows['r_' . $version['rid']] = array(
          'data' => array(
            'project' => array(
              'data' => '',
              'class' => array('empty'),
            ),
            'version' => array(
              'data' => $version['version'] . $extra,
              'class' => array('version'),
            ),
            'update' => array(
              'data' => $update,
              'class' => array('update'),
            ),
          ),
          'class' => array('drd-version', $info['type']),
        );

        foreach ($servers as $sid => $server) {
          if (empty($version['servers'][$sid])) {
            $count = '';
          }
          else {
            $count = count($version['servers'][$sid]);
            $count .= '<span class="drd-display">' . drd_icon('show', 'info');
            $count .= '<div class="detail">';
            foreach ($version['servers'][$sid] as $did) {
              $domain = drd_domain_load($did);
              $count .= '<p>' . l($domain->name . ' (' .  $domain->url . ')', 'admin/drd/domain/' . $did) . '</p>';
            }
            $count .= '</div>';
            $count .= '</span>';
          }
          $rows['r_' . $version['rid']]['data']['s_' . $sid] = $count;
        }
      }
    }
  }
  $build['table'] = array(
    '#markup' => theme('table', array(
      'header' => $header,
      'rows' => $rows,
      'attributes' => array(
        'id' => 'drd-modules',
      ),
      'sticky' => TRUE,
      'empty' => t('No data available.'),
    ))
  );
  return $build;
}

function _drd_modules_status($status) {
  $info = array();
  switch ($status) {
    case UPDATE_NOT_SECURE:
    case UPDATE_REVOKED:
      $info['title'] = ' ' . t('(Security update)');
      $info['type'] = 'security';
      break;

    case UPDATE_NOT_SUPPORTED:
      $info['title'] = ' ' . t('(Unsupported)');
      $info['type'] = 'unsupported';
      break;

    case UPDATE_UNKNOWN:
    case UPDATE_NOT_FETCHED:
    case UPDATE_NOT_CHECKED:
    case UPDATE_NOT_CURRENT:
      $info['type'] = 'recommended';
      break;

    default:
      $info['type'] = 'ok';
      $info['ok'] = TRUE;
  }
  return $info;
}

function _drd_update_projects($projects, $domain) {
  db_delete('drd_project_release_domain')
    ->condition('did', $domain->did)
    ->execute();
  foreach ($projects as $name => $project) {
    $existing = array_shift(explode('+', $project['existing_version']));
    $stored_project = db_select('drd_project', 'p')
      ->fields('p')
      ->condition('p.name', $name)
      ->execute()
      ->fetchAssoc();
    if (empty($stored_project)) {
      $stored_project = array(
        'name' => $name,
        'type' => array_shift(explode('-', $project['project_type'])),
        'title' => isset($project['title']) ? $project['title'] : $name,
        'link' => isset($project['link']) ? $project['link'] : '',
        'details' => array(),
      );
      drupal_write_record('drd_project', $stored_project);
    }
    else {
      $stored_project['details'] = unserialize($stored_project['details']);
    }
    $stored_project['details'][$existing] = array(
      'existing_major' => isset($project['existing_major']) ? $project['existing_major'] : 1,
      'install_type' => isset($project['install_type']) ? $project['install_type'] : '',
      'latest_version' => isset($project['latest_version']) ? $project['latest_version'] : '',
      'recommended' => isset($project['recommended']) ? $project['recommended'] : '',
      'status' => isset($project['status']) ? $project['status'] : '',
    );
    drupal_write_record('drd_project', $stored_project, 'pid');
    $pid = $stored_project['pid'];
    if (empty($project['releases'])) {
      $project['releases'] = array(
        $existing => array(
          'download_link' => '',
          'date' => 0,
        ),
      );
    }
    if (empty($project['releases'][$existing])) {
      $project['releases'][$existing] = array(
        'download_link' => '',
        'date' => 0,
      );
    }
    foreach ($project['releases'] as $version => $release) {
      $stored_release = db_select('drd_project_release', 'r')
        ->fields('r')
        ->condition('r.pid', $pid)
        ->condition('r.version', $version)
        ->execute()
        ->fetchAssoc();
      if (empty($stored_release)) {
        $stored_release = array(
          'pid' => $pid,
          'version' => $version,
          'link' => isset($release['download_link']) ? $release['download_link'] : '',
          'timestamp' => isset($release['date']) ? $release['date'] : 0,
          'details' => $release,
        );
        drupal_write_record('drd_project_release', $stored_release);
      }
    }
    if ($project['project_status']) {
      $rid = db_select('drd_project_release', 'r')
        ->fields('r', array('rid'))
        ->condition('r.pid', $pid)
        ->condition('r.version', $existing)
        ->execute()
        ->fetchField();
      $values = array('rid' => $rid, 'did' => $domain->did);
      drupal_write_record('drd_project_release_domain', $values);
    }
  }
}
