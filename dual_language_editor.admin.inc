<?php

/**
 * Admin config form for the dual-language editor.
 */
function dual_language_editor_config_form($form, &$form_state) {
  $languages = language_list();
  $codes = array();
  foreach ($languages as $langcode => $language) {
    $codes[$langcode] = t($language->name);
  }
  // primary language
  $form['dual_language_editor_plang'] = array(
    '#title' => t('Primary Language'),
    '#type' => 'select',
    '#options' => $codes,
    '#default_value' => dual_language_editor_default_primary_language(),
    '#required' => TRUE,
  );
  // secondary language
  $form['dual_language_editor_slang'] = array(
    '#title' => t('Secondary Language'),
    '#type' => 'select',
    '#options' => $codes,
    '#default_value' => dual_language_editor_default_secondary_language(),
    '#required' => TRUE,
  );
  return system_settings_form($form);
}

function dual_language_editor_config_form_validate($form, &$form_state) {
  // we can't have the languages being the same.
  if ($form_state['values']['dual_language_editor_plang'] ==
    $form_state['values']['dual_language_editor_slang']) {
    form_set_error('dual_language_editor_slang',
      t('Primary and secondary language cannot be the same'));
  }
}