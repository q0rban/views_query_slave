<?php
// $Id$

/**
 * A simple extension to the base pager plugin, to route count queries to a
 * slave database, if applicable.
 *
 * @ingroup views_pager_plugins
 */
class views_plugin_pager_slave extends views_plugin_pager {
  /**
   * Execute the count query, which will be done just prior to the query
   * itself being executed.
   */
  function execute_count_query(&$count_query, $args) {
    dsm('hi');
    $query_plugin = $this->view_query;
    $this->total_items = db_result($query_plugin->db_query($count_query, $args));
    if (!empty($this->options['offset'])) {
      $this->total_items -= $this->options['offset'];
    }

    $this->update_page_info();
    return $this->total_items;
  }
}
