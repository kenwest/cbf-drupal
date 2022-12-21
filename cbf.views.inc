<?php

function cbf_views_query_alter(&$view, &$query) {
  /*
   * Can this view be localised? (ie, if the visitor's location is available
   * then can we limit the results to those in the visitor's city?)
   */
  $filter_vocabulary = null;
  switch ($view->name) {
    case 'cbf2019_local_office':
      $filter_vocabulary = 'office';
      $filter_table = 'field_data_field_office';
      $filter_tid = 'field_office_tid';
      break;

    case 'cbf2019_local_events':
    case 'cbf2019_local_activities':
      $filter_vocabulary = 'vocabulary_1';
      $filter_table = 'field_data_taxonomy_vocabulary_1';
      $filter_tid = 'taxonomy_vocabulary_1_tid';
      break;

    case 'cbf2019_mappable':
      if ($view->current_display == 'block_4') {
        $filter_vocabulary = 'vocabulary_1';
        $filter_table = 'field_data_taxonomy_vocabulary_1';
        $filter_tid = 'taxonomy_vocabulary_1_tid';
      }
      break;
  }

  /*
   * The view is localised if the following conditions hold ...
   *
   * The view can be localised, and this invocation is the loading of the page
   * (refreshes use AJAX), and the $filter_table is not already in the query,
   * and the visitor's $localCity is known.
   */
  if (
    isset($filter_vocabulary)
      && strpos($_SERVER['REQUEST_URI'], '/views/ajax') === false
      && empty($query->get_table_info($filter_table))
      && !empty($localCity = cbf_visitor_city_string())
  ) {
    $city = taxonomy_get_term_by_name($localCity, $filter_vocabulary);
    $join = new views_join;
    $join->construct(
      $filter_table,
      $query->base_table,
      $query->base_field,
      'entity_id',
      "({$filter_table}.entity_type = '{$query->base_table}'"
      . " AND {$filter_table}.deleted = '0')",
      'INNER'
    );
    $query->add_relationship($filter_table, $join, $query->base_table);
    $query->add_where(1, "{$filter_table}.{$filter_tid}", reset($city)->tid, '=');
  }

  /*
   * The cbf2019_rated_content view supplies a 'More by' block ('block_1') and
   * a 'More on' block ('block_2'). We need to exclude the article on whose
   * page this block is being displayed.
   */
  if ($view->name == 'cbf2019_rated_content'
      && ($view->current_display == 'block_1'
          || $view->current_display == 'block_2')
      && is_numeric($view->args[0])
      && $view->args[0] > 0
  ) {
    $view->query->where[] = [
      'conditions' => [
        [
          'field' => 'node.nid',
          'value' => $view->args[0],
          'operator' => '!=',
        ],
      ],
      'args' => [],
      'type' => 'AND',
    ];
  }
}
