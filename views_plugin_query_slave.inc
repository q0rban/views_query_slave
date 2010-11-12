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
   * Until we have wrapper methods for db_query() and db_query_range() in the
   * parent class, we are forced to override the execute method from
   * views_plugin_query_default. This is fine, except that we'll need to check the
   * base class occasionally to see if there have been any updates, and modify
   * this function accordingly.
   *
   * @see http://drupal.org/node/968830
   */
  function execute(&$view) {
    // Load up the cache settings. In this example, we'll assume that if this
    // display can be cached, it can also run it's queries against the slave
    // database. If caching is enabled, we use Pressflow's db_query_slave() and
    // db_query_range_slave() in lieu of db_query() & db_query_range(),
    // respectively. Feel free to use your own method for determining which views
    // are 'slave_safe'.
    $cache_settings = $view->display_handler->get_option('cache');
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

    // If you update Views and the patch from #968830 gets committed, this will
    // call the parent execute method. Yay!
    if (method_exists('views_plugin_query_default', 'db_query')) {
      return parent::execute($view);
    }

    // Everything past this point is a straight copy from the parent class, except
    // that calls to db_query(), execute_count_query() and db_query_slave() have
    // been replaced with the methods defined in this class.

    $external = FALSE; // Whether this query will run against an external database.
    $query = db_rewrite_sql($view->build_info['query'], $view->base_table, $view->base_field, array('view' => &$view));
    $count_query = db_rewrite_sql($view->build_info['count_query'], $view->base_table, $view->base_field, array('view' => &$view));
    $args = $view->build_info['query_args'];

    vpr($query);

    $items = array();
    if ($query) {
      $replacements = module_invoke_all('views_query_substitutions', $view);
      $query = str_replace(array_keys($replacements), $replacements, $query);
      $count_query = 'SELECT COUNT(*) FROM (' . str_replace(array_keys($replacements), $replacements, $count_query) . ') count_alias';

      if (is_array($args)) {
        foreach ($args as $id => $arg) {
          $args[$id] = str_replace(array_keys($replacements), $replacements, $arg);
        }
      }

      // Detect an external database.
      if (isset($view->base_database)) {
        db_set_active($view->base_database);
        $external = TRUE;
      }

      $start = views_microtime();

      if ($this->pager->use_count_query() || !empty($view->get_total_rows)) {
        $this->execute_count_query($count_query, $args);
      }

      // Let the pager modify the query to add limits.
      $this->pager->pre_execute($query, $args);

      if (!empty($this->limit) || !empty($this->offset)) {
        // We can't have an offset without a limit, so provide a very large limit instead.
        $limit  = intval(!empty($this->limit) ? $this->limit : 999999);
        $offset = intval(!empty($this->offset) ? $this->offset : 0);
        $result = $this->db_query_range($query, $args, $offset, $limit);
      }
      else {
        $result = $this->db_query($query, $args);
      }

      $view->result = array();
      while ($item = db_fetch_object($result)) {
        $view->result[] = $item;
      }

      $this->pager->post_execute($view->result);

      if ($this->pager->use_pager()) {
        $view->total_rows = $this->pager->get_total_items();
      }

      if ($external) {
        db_set_active();
      }
    }
    $view->execute_time = views_microtime() - $start;
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

  /**
   * Execute the count query, which will be done just prior to the query
   * itself being executed.
   *
   * @see views_plugin_pager::execute_count_query()
   */
  function execute_count_query(&$count_query, $args) {
    $this->pager->total_items = db_result($this->db_query($count_query, $args));
    if (!empty($this->pager->options['offset'])) {
      $this->pager->total_items -= $this->pager->options['offset'];
    }

    $this->pager->update_page_info();
    return $this->pager->total_items;
  }
}