<?php

/**
 * DRD Server Operation to retrieve the SVN status (not yet implemented.
 *
 * @return type
 */
function drd_vcs_svn_status($op, $mode, $sid, $options) {
  _drd_vcs_execute($sid, $options, 'status', '-u');
}

/**
 * DRD Server Operation to perform a SVN update (not yet implemented.
 *
 * @return type
 */
function drd_vcs_svn_update($op, $mode, $sid, $options) {
  _drd_vcs_execute($sid, $options, 'update', '--accept theirs-full --force --non-interactive');
}

function _drd_vcs_execute($sid, $options, $cmd, $param) {
  if (!function_exists('ssh2_connect')) {
    drupal_set_message('SSH2 library for PHP is not available.', 'warning');
    return;
  }
  $server = drd_server_load($sid);
  if (!$server->settings['ssh2']['enable']) {
    drd_set_message($server, 'Skipping domain, SSH2 is disabled.', 'warning');
    return;
  }
  $def = drd_operation_remote('drd.config.svn.read', 'server', $sid, $options);
  if (empty($def) || !is_array($def)) {
    drd_set_message($server, t('Can not read SVN settings from remote domain.'), 'error');
    return;
  }
  if (!$def['credentials']['enabled']) {
    drd_set_message($server, t('Skipping domain, SVN is disabled.'), 'error');
    return;
  }
  $connection = _drd_ssh2_connect($server);
  if ($connection) {
    foreach ($def['directories'] as $directory) {
      foreach ($directory['paths'] as $path) {
        $command = 'svn ' . $cmd . ' ' . $param . ' ' . $path . ' --username ' . $def['credentials']['username'] . ' --password ' . $def['credentials']['password'];
        $output = _drd_ssh2_command($server, $connection, $command);
        $key = 'svn:' . $cmd . ':' . $directory['label'];
        $cid = _drd_cache_id('server', $server->sid, $key);
        cache_set($cid, array(
          'title' => 'SVN ' . $cmd . ' for ' . $directory['label'] . ' (' . $path . ')',
          'callback' => 'drupal_render',
          'arguments' => array(
            '#markup' => '<pre>' . $output . '</pre>'
          )
        ), 'cache_drd');
      }
    }
  }
}
