<?php

function cbf_views_query_alter(&$view, &$query) {
  if ($view->name == 'cbf2019_local_office') {
    $localCity = cbf_visitor_city_string();
    $ajaxRequest = (strpos($_SERVER['REQUEST_URI'], '/views/ajax') !== false);
    if (!empty($localCity) && !$ajaxRequest) {
      if (count($query->where[1]['conditions']) == 1) {
        $city = taxonomy_get_term_by_name($localCity, 'office');
        $join = new views_join;
        $join->construct('field_data_field_office', 'entityform', 'entityform_id', 'entity_id', "(field_data_field_office.entity_type = 'entityform' AND field_data_field_office.deleted = '0')", 'INNER');
        $query->add_relationship('field_data_field_office', $join, 'entityform');
        $query->add_where(1, 'field_data_field_office.field_office_tid', reset($city)->tid, '=');
      }
    }
  }
  if ($view->name == 'cbf2019_local_events') {
    $localCity = cbf_visitor_city_string();
    $ajaxRequest = (strpos($_SERVER['REQUEST_URI'], '/views/ajax') !== false);
    if (!empty($localCity) && !$ajaxRequest) {
      if (count($query->where[1]['conditions']) == 1) {
        $city = taxonomy_get_term_by_name($localCity, 'vocabulary_1');
        $join = new views_join;
        $join->construct('field_data_taxonomy_vocabulary_1', 'civicrm_event', 'id', 'entity_id', "(field_data_taxonomy_vocabulary_1.entity_type = 'civicrm_event' AND field_data_taxonomy_vocabulary_1.deleted = '0')", 'INNER');
        $query->add_relationship('field_data_taxonomy_vocabulary_1', $join, 'civicrm_event');
        $query->add_where(1, 'field_data_taxonomy_vocabulary_1.taxonomy_vocabulary_1_tid', reset($city)->tid, '=');
      }
    }
  }
}
