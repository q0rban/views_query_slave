<?php
// $Id$

/**
 * @file
 * Views API Implementations
 */

/**
 * We only want to modify the query plugin if db_query_slave() exists. This is
 * in effect saying, "Hey, are we on Pressflow 6?"
 */
if (function_exists('db_query_slave')) {
  /**
   * Implementation of hook_views_data_alter().
   */
  function views_query_slave_views_data_alter(&$data) {
    foreach ($data as $table => &$table_data) {
      if (isset($table_data['table']['base'])) {
        // If query class is set and it contains views_query, we can swap it out.
        $is_views_query = isset($table_data['table']['base']['query class']) && ($table_data['table']['base']['query class'] == 'views_query');
        // If query class isn't set, we can assume that it's using views_query.
        if ($is_views_query || empty($table_data['table']['base']['query class'])) {
          $table_data['table']['base']['query class'] = 'views_query_slave';
        }
      }
    }
  }

  /**
   * Implementation of hook_views_plugins
   */
  function views_query_slave_views_plugins() {
    $plugins = array( // This just tells our themes are elsewhere.
      'query' => array(
        'views_query_slave' => array(
          'title' => t('SQL Query (slave)'),
          'help' => t('Query will be generated and run using the Pressflow Slave database API.'),
          'handler' => 'views_plugin_query_slave',
          'parent' => 'views_query',
        ),
      ),
    );

    return $plugins;
  }
}