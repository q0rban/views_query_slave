<?php
// $Id$

/**
 * @file views_plugin_query_slave.inc
 * Defines the default query object which builds SQL to execute using the slave
 * database API within Pressflow.
 */

/**
 * Extension of views_plugin_query_default to query a slave db if it exists. The
 * slave database is only queried against if there is caching in place on the
 * view.
 */
class views_plugin_query_slave extends views_plugin_query_default{
  /**
   * Whether or not this view is safe to be run against the Slave database.
   *
   * @var boolean
   */
  protected $slave_safe = FALSE;

  /**
   * Builds the necessary info to execute the query.
   *
   * The only reason this is here is to set up $slave_safe. In this example, we'll
   * assume that if this display can be cached, it can also run it's queries
   * against the slave database. If caching is enabled, we use Pressflow's
   * db_query_slave() and db_query_range_slave() in lieu of db_query() &
   * db_query_range(), respectively. Feel free to use your own method for
   * determining which views are 'slave_safe'.
   */
  function build(&$view) {
    // Load up the cache settings.
    $cache_settings = $view->display_handler->get_option('cache');
    // If caching is anything but 'none', we assume this display is 'slave_safe'.
    $this->slave_safe = $cache_settings['type'] != 'none';

    // Here's just a little informational message to show us it's working. You'll
    // probably want to delete this. ;)
    if ($this->slave_safe) {
      $t_args = array(
        '%display' => $view->current_display,
        '%view' => $view->name,
      );
      drupal_set_message(t('The %display display of the %view view is querying the slave database. Yay!', $t_args));
    }

    return parent::build($view);
  }

  /**
   * Wrapper method for db_query().
   *
   * @param $query
   *   A string containing an SQL query.
   * @param $args
   *   An array containing the query arguments.
   * @return
   *   A database query result resource, or FALSE if the query was not executed
   *   correctly.
   */
  function db_query($query, $args = array()) {
    $fnc = $this->slave_safe ? 'db_query_slave' : 'db_query';
    return $fnc($query, $args);
  }

  /**
   * Wrapper method for db_query_range().
   *
   * @param $query
   *   A string containing an SQL query.
   * @param $from
   *   The first result row to return.
   * @param $count
   *   The maximum number of result rows to return.
   * @param $args
   *   An array containing the query arguments.
   * @return
   *   A database query result resource, or FALSE if the query was not executed
   *   correctly.
   */
  function db_query_range($query, $from, $count, $args = array()) {
    $fnc = $this->slave_safe ? 'db_query_range_slave' : 'db_query_range';
    return $fnc($query, $from, $count, $args);
  }
}