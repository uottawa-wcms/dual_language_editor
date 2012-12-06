<?php

/**
 * Callback function for creating an entity.
 *
 * @param EntityCRUDUIBase $entity_type The CRUD handler for the entity type.
 * @param string $bundle_type The name of the bundle type we're  creating.
 * @return array Form render array for creating an entity.
 */
function dual_language_editor_create_entity($entity_type, $bundle_type) {
  // This initializes the object (via entity_create)
  $entity = $entity_type->newObject($bundle_type);
  return drupal_get_form(
      'dual_language_editor_entity_form',
      $entity_type,
      $entity
  );
}

/**
 * Form for creating / editing entities.
 *
 * @param EntityCRUDUIBase $entity_type The CRUD handler for the entity type.
 * @param stdClass $entity Entity that we're editing or a stub entity if we're
 *   creating.
 * @return array Form render array.
 */
function dual_language_editor_entity_form($form, &$form_state, $entity_type, $entity) {
  $entity_type->prepareObject($entity);
  // Find the currently selected left-hand language.
  $primary_lang = dual_language_editor_default_primary_language();
  if (isset($form_state['values']['language_switcher']['left'])) {
    $primary_lang = $form_state['values']['language_switcher']['left'];
  }
  // Find the currently selected right-hand language.
  $secondary_lang = dual_language_editor_default_secondary_language();
  if (isset($form_state['values']['language_switcher']['right'])) {
    $secondary_lang = $form_state['values']['language_switcher']['right'];
  }
  // Verification to make sure the left- and right-hand langauages are not the
  // same.
  if ($secondary_lang === $primary_lang) {
    // If they are, let's look for the first unused language
    foreach (array_keys(language_list()) as $langkey) {
      if ($langkey != $primary_lang) {
        $secondary_lang = $langkey;
        $form_state['values']['language_switcher']['right'] = $secondary_lang;
        $form_state['input']['language_switcher']['right'] = $secondary_lang;
        drupal_set_message(t('Could not set right-hand language, it conflicts
          with left-hand language. Setting to default.'), 'warning');
        break;
      }
    }
  }

  // We'll need these on callback
  $form['#plang'] = $primary_lang;
  $form['#slang'] = $secondary_lang;
  $form['entity_raw_type'] = array(
    '#type' => 'value',
    '#value' => $entity_type->getType(),
  );
  $form['entity_type'] = array(
    '#type' => 'value',
    '#value' => $entity_type,
  );
  $form['entity'] = array(
    '#type' => 'value',
    '#value' => $entity,
  );

  // This fieldset holds all fields which are translatable in pairs.
  $form['language_dep'] = array(
    '#type' => 'fieldset',
    '#title' => t('Translatable Fields'),
    '#collapsible' => TRUE,
    '#attached' => array(
      'css' => array(
        drupal_get_path('module', 'dual_language_editor') .
            '/dual_language_editor.css',
      ),
      'js' => array(
        drupal_get_path('module', 'dual_language_editor') .
            '/dual_language_editor.js',
      ),
    ),
    '#prefix' => '<div id="dual-language-refresh-target">',
    '#suffix' => '</div>',
    'header' => dual_language_editor_header_field(
        $primary_lang,
        $secondary_lang
    ),
    'language_switcher' => dual_language_editor_language_selector(
        $primary_lang,
        $secondary_lang
    ),
  );

  // Holds all non-translatable fields.
  $form['language_ind'] = array(
    '#type' => 'fieldset',
    '#title' => t('Entity Settings'),
    '#collapsible' => TRUE,
  );

  // Alter the form to include basic properties
  // @todo this needs to change, to support translatable properties
  $entity_type->alterForm($form['language_ind'], $form_state, $entity);

  // Attach fields to the form
  dual_language_editor_field_attach(
      $form,
      $form_state,
      $entity_type->getType(),
      $entity,
      $primary_lang,
      $secondary_lang
  );

  // Append action buttons to the form
  $form['actions'] = array(
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Save'),
      '#weight' => -100,
    ),
  ) + $entity_type->getFormActions();

  // Only add a delete button for existing items
  if (empty($entity->is_new)) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#submit' => array('entity_crud_ui_delete_redirect'),
      '#weight' => 100,
    );
  }
  return $form;
}

/*
 * Validation callback for the DLE form.
 *
 * @see dual_language_editor_entity_form().
 */
function dual_language_editor_entity_form_validate($form, &$form_state) {
  $entity_type = $form_state['values']['entity_type'];
  $entity = $form_state['values']['entity'];

  // trigger entity property validation
  $entity_type->formValidate(
      $form_state['values']['entity'],
      $form,
      $form_state
  );

  // field validation
  field_attach_form_validate(
      $entity_type->getType(),
      $entity,
      $form,
      $form_state
  );
}

/**
 * Submit handler for the DLE form.
 *
 * @see dual_language_editor_entity_form().
 */
function dual_language_editor_entity_form_submit($form, &$form_state) {
  $entity_type = $form_state['values']['entity_type'];
  $entity = $form_state['values']['entity'];
  entity_form_submit_build_entity($entity_type->getType(), $entity, $form, $form_state);
  $entity_type->propertyAttach($entity, $form, $form_state);
  foreach (module_implements('dual_language_editor_postattach') as $module) {
    $function = $module . '_dual_language_editor_postattach';
    if (function_exists($function)) {
      $function($entity_type->getType(), $entity, $form, $form_state);
    }
  }
  $entity_type->presave($entity);
  $entity_type->save($entity);
  $entity_type->postsave($entity, $form_state);
}

/**
 * Helper function to setup non-field fields in a side-by-side fashion.
 *
 * @param array $left_field Form render array for the left field.
 * @param array $right_field Form render array for the right field.
 * @param array $extras Parameters to pass to the top-level row.
 * @return array Form render array for the combined container.
 */
function dual_language_editor_row_setup($left_field, $right_field, $extras) {
  $row = array(
    '#type' => 'container',
    'left' => $left_field,
    'right' => $right_field,
    '#tree' => TRUE,
  ) + $extras;
  dual_language_editor_row_helper($row, 'left', 'right');
  return $row;
}

/**
 * Sets up prefixes on side-by-side containers.
 *
 * @param array $container Form render array containing left and right fields.
 * @param string $left_key The key within $container of the left-hand field.
 * @param string $right_key The key within $container of the right-hand field.
 */
function dual_language_editor_row_helper(&$container, $left_key, $right_key) {
  if (empty($container['#prefix'])) {
    $container['#prefix'] = '<div class="dual-language-editor-row">';
  } else {
    $container['#prefix'] = '<div class="dual-language-editor-row">' . $container['#prefix'];
  }
  if (empty($container['#suffix'])) {
    $container['#suffix'] = '<br style="clear: both;" /></div>';
  } else {
    $container['#suffix'] = $container['#suffix'] . '<br style="clear: both;" /></div>';
  }
  // Note: we no longer use -left and -right classes, as it causes problems
  // on form reload via AJAX. Instead, we rely on :nth-of-type to arrange
  // the form properly.
}

/**
 * Attaches form fields to a form, for a dual-language approach.
 *
 * @param array $form The form we'll be updating.
 * @param array $form_state Form state information.
 * @param string $entity_type The name of the entity type. Note that this is
 *   different from $entity_type in other places.
 * @param stdClass $entity Entity object we're attaching form fields for
 * @param string $primary_lang Two-letter ISO code for the left-hand language.
 * @param string $secondary_lang Two-letter ISO code for the right-hand
 *   language.
 */
function dual_language_editor_field_attach(&$form, &$form_state, $entity_type, $entity, $primary_lang, $secondary_lang) {
  $forms = array(); // this will hold copies of the form
  $form_keys = array(); // this will hold all the form keys we care about

  // First, let's build the non-language form, so that we can load
  // language-independent properties correctly.
  $forms['und'] = array();
  field_attach_form($entity_type, $entity, $forms['und'], $form_state, 'und');

  // Next, let's build each language individually. We build all the languages
  // here because one day we'd like for the hidden languages to still remember
  // their state.
  $languages = array_keys(language_list());
  foreach ($languages as $langcode) {
    $forms[$langcode] = array();
    field_attach_form(
        $entity_type,
        $entity,
        $forms[$langcode],
        $form_state,
        $langcode
    );
    // save all the form keys into one array for looping
    $form_keys = array_merge(
        $form_keys,
        array_keys($forms[$langcode])
    );
  }

  // Unique keys only
  $form_keys = array_unique($form_keys);

  // Next, let's attach fields to the real form.
  foreach($form_keys as $key) {
    // these conditions indicate that it is a translatable field
    if (
        (is_array($forms[$primary_lang][$key])) &&
        !empty($forms[$primary_lang][$key]['#language']) &&
        ($forms[$primary_lang][$key]['#language'] != 'und')
    ) {
      // let's add it to the language-dependent fields.
      // this brings the primary language along with it.
      $form['language_dep'][$key] = $forms[$primary_lang][$key];
      // tell field attach that both languages need processing.
      $form['language_dep'][$key]['#language'] = array(
        $primary_lang,
        $secondary_lang
      );
      // Attach the secondary language.
      $form['language_dep'][$key][$secondary_lang] =
          $forms[$secondary_lang][$key][$secondary_lang];

      // Apply the necessary prefixes and suffixes.
      dual_language_editor_row_helper(
        $form['language_dep'][$key],
        $primary_lang,
        $secondary_lang
      );
    }

    // Otherwise, if it's not a field, let's just add it to the base array.
    elseif (!is_array($forms[$primary_lang][$key]) || (substr($key, 0, 1) == '#')) {
      $form[$key] = $forms['und'][$key];
    }

    // Finally, we can assume it's a language-independent field.
    else {
      // Add it to the list of language independent fields.
      $form['language_ind'][$key] = $forms['und'][$key];
    }
  }

  // After we've set up the form, fire a hook so we can operation on the
  // results. This is mostly to let us add polyfills for backwards compatibility
  // with key contrib and core features, but you can also use it for your own
  // code.
  foreach (module_implements('dual_language_editor_field_attach_alter') as $module) {
    $function = $module . '_dual_language_editor_field_attach_alter';
    if (function_exists($function)) {
      $function($form, $form_state, $entity_type, $entity, $primary_lang, $secondary_lang);
    }
  }
}

/**
 * Provides the container with language switchers if necessary.
 *
 * @param string $primary_lang ISO code for left-hand language.
 * @param string $secondary_lang ISO code for right-hand language.
 * @return array Form render array.
 */
function dual_language_editor_language_selector($primary_lang, $secondary_lang) {
  $languages = language_list();
  // We only need a language selector if there are more than two languages.
  if (count($languages) < 3) {
    return array();
  }
  // Assemble selections for languages.
  $opts = array();
  foreach ($languages as $key => $language) {
    $opts[$key] = t($language->name);
  }
  // Compile the form container.
  return dual_language_editor_row_setup(
    array(
      '#type' => 'select',
      '#options' => $opts,
      '#default_value' => $primary_lang,
      '#required' => TRUE,
      '#title' => t('Change Left-Hand Language'),
      '#ajax' => array(
        'callback' => 'dual_language_editor_reload_language',
        'wrapper' => 'dual-language-refresh-target',
        'effect' => 'fade',
      ),
    ),
    array(
      '#type' => 'select',
      '#options' => $opts,
      '#default_value' => $secondary_lang,
      '#required' => TRUE,
      '#title' => t('Change Right-Hand Language'),
      '#ajax' => array(
        'callback' => 'dual_language_editor_reload_language',
        'wrapper' => 'dual-language-refresh-target',
        'effect' => 'fade',
      ),
    ), array(
      '#weight' => -99999,
    )
  );
}

/**
 * Provides the container with the language headers.
 *
 * @param string $primary_lang ISO code for left-hand language.
 * @param string $secondary_lang ISO code for right-hand language.
 * @return array Form render array.
 */
function dual_language_editor_header_field($primary_language, $secondary_language) {
  $languages = language_list();
  // @todo should this be a theme function
  return dual_language_editor_row_setup(
    array(
      '#markup' => t($languages[$primary_language]->name . ' Content'),
      '#prefix' => '<div class="dual-language-header"><h2>',
      '#suffix' => '</h2>' . dual_language_editor_header_expand_buttons('left') . '</div>',
    ),
    array(
      '#markup' => t($languages[$secondary_language]->name . ' Content'),
      '#prefix' => '<div class="dual-language-header"><h2>',
      '#suffix' => '</h2>' . dual_language_editor_header_expand_buttons('right') . '</div>',
    ), array(
      '#weight' => -100000,
    )
  );
}

/**
 * Adds Expand and Collapse buttons to the form.
 *
 * @param string $side The side that the buttons are controlling: can be "left"
 *   or "right"
 * @return string HTML code for the expand / collapse buttons.
 */
function dual_language_editor_header_expand_buttons($side) {
  return '<span id="collapse-'.$side.'" class="collapse">' . t('Collapse') . '</span>' .
      '<span id="expand-' . $side . '" class="expand">'.t('Expand').'</span>';
}