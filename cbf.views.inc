<?php

function cbf_views_query_alter(&$view, &$query) {
  if ($view->name == 'cbf2019_local_office') {
    $localcity = cbf_visitor_city_string();
    if (!empty($localcity)) {
      if (count($query->where[1]['conditions']) == 1) {
        $city = taxonomy_get_term_by_name($localcity, 'office');
        $join = new views_join;
        $join->construct('field_data_field_office', 'entityform', 'entityform_id', 'entity_id', "(field_data_field_office.entity_type = 'entityform' AND field_data_field_office.deleted = '0')", 'INNER');
        $query->add_relationship('field_data_field_office', $join, 'entityform');
        $query->add_where(1, 'field_data_field_office.field_office_tid', reset($city)->tid, '=');
      }
    }
  }
}
