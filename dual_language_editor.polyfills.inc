<?php

if (module_exists('field_group')) {
  if (!function_exists('field_group_dual_language_editor_field_attach_alter')) {
    /**
     * Implements hook_dual_language_editor_field_attach_alter().
     *
     * Fixes bugs with how field_group interacts with the DLE.
     *
     * Without this fix, field_group does not load the groups at all, because
     * it is looking for them in the root form element, instead of in our
     * language_dep and language_ind subgroups.
     *
     * Solution to this is to split the root set of #fieldgroups and
     * #group_children between language_dep and language_ind as needed, then
     * pre-render those elements with the same function that prerenders the
     * form root.
     *
     * Assumption: If there are nested groups, then #group_children is in a
     * sensible order (from the leaf form elements to the roots). If this
     * assumption is incorrect, the leaf children may not render properly, but
     * it appears to be valid through all tests to date.
     *
     * @param array $form The form render array.
     */
    function field_group_dual_language_editor_field_attach_alter(&$form) {
      // Set up some blank arrays to take the new items
      $form['language_dep']['#fieldgroups'] = array();
      $form['language_dep']['#group_children'] = array();
      $form['language_ind']['#fieldgroups'] = array();
      $form['language_ind']['#group_children'] = array();

      // Add render hooks
      $form['language_dep']['#pre_render'][] = 'field_group_build_pre_render';
      $form['language_ind']['#pre_render'][] = 'field_group_build_pre_render';

      // The pre_render hook is expecting these variables, so duplicate them.
      $form['language_dep']['#entity_type'] = $form['#entity_type'];
      $form['language_dep']['#bundle'] = $form['#bundle'];
      $form['language_ind']['#entity_type'] = $form['#entity_type'];
      $form['language_ind']['#bundle'] = $form['#bundle'];

      $group_lang_check = array(); // keeps tracks of which groups went where

      if (is_array($form['#group_children'])) {
        // loop through each child
        foreach ($form['#group_children'] as $field_name => $group_name) {

          // if the child is language dependent, then add it to the language-
          // dependent groups.
          if (!empty($form['language_dep'][$field_name])) {
            if (empty($form['language_dep']['#fieldgroups'][$group_name])) {
              $form['language_dep']['#fieldgroups'][$group_name] = $form['#fieldgroups'][$group_name];
              $group_lang_check[$group_name]['dep'] = TRUE;
            }
            $form['language_dep']['#group_children'][$field_name] = $group_name;
          }
          // Otherwise, if it is language independent, the add it to the
          // language-independent groups.
          elseif (!empty($form['language_ind'][$field_name])) {
            if (empty($form['language_ind']['#fieldgroups'][$group_name])) {
              $form['language_ind']['#fieldgroups'][$group_name] = $form['#fieldgroups'][$group_name];
              $group_lang_check[$group_name]['ind'] = TRUE;
            }
            $form['language_ind']['#group_children'][$field_name] = $group_name;
          }
          // Otherwise, see if it is a group name. If it is, we assume it's
          // children have already been processed, and that we can figure out
          // if it's language-dependent or language-independent.
          elseif (!empty($form['#fieldgroups'][$field_name])) {
            // the group is language dependent, load it appropriately.
            if (!empty($group_lang_check[$field_name]['dep'])) {
              if (empty($form['language_dep']['#fieldgroups'][$group_name])) {
                $form['language_dep']['#fieldgroups'][$group_name] = $form['#fieldgroups'][$group_name];
                $group_lang_check[$group_name]['dep'] = TRUE;
              }
              $form['language_dep']['#group_children'][$field_name] = $group_name;
            }
            // the group is language independent, load it appropriately.
            // this is done in addition to the language_dep placement, in case
            // part of the group is language dependent and part is not.
            if (!empty($group_lang_check[$field_name]['ind'])) {
              if (empty($form['language_ind']['#fieldgroups'][$group_name])) {
                $form['language_ind']['#fieldgroups'][$group_name] = $form['#fieldgroups'][$group_name];
                $group_lang_check[$group_name]['ind'] = TRUE;
              }
              $form['language_ind']['#group_children'][$field_name] = $group_name;
            }
          }
        }
      }
    }
  }
}

if (module_exists('metatag')) {

  if (!function_exists('metatag_dual_language_editor_alter_node_form')) {
    function metatag_dual_language_editor_alter_node_form(&$form, &$form_state) {
      if (isset($form['#dual-metatags'])) {
        dual_language_editor_metatag_form(
          $form['language_dep'],
          $form['#dual-metatags'],
          $form['#plang'],
          $form['#slang']
        );
        unset($form['#dual-metatags']);
        if (!isset($form['#submit'])) {
          $form['#submit'] = array();
        }
      }
    }
  }

  if (!function_exists('metatag_dual_language_editor_postattach')) {
    function metatag_dual_language_editor_postattach($entity_type, &$entity, $form, $form_state) {
      unset($entity->metatags);
      $entity->mtags = array(
        $form['#plang'] => array(),
        $form['#slang'] => array(),
      );
      foreach ($form_state['values']['language_dep']['metatags'] as $tag => $values) {
        foreach ($values as $langcode => $valueset) {
          if (!empty($langcode)) {
            $entity->mtags[$langcode][$tag] = $valueset;
          }
        }
      }
      $defaults = $form['language_dep']['metatags']['#metatag_defaults'];
      foreach (array_keys($entity->mtags) as $lcode) {
        metatag_filter_values_from_defaults($entity->mtags[$lcode], $defaults);
      }
    }
  }

  if (!function_exists('metatag_dual_language_editor_postsave')) {
    function metatag_dual_language_editor_postsave($entity_type, &$entity) {
      if (isset($entity->mtags)) {
        foreach ($entity->mtags as $key => $metadata) {
          $info = entity_get_info($entity_type);
          $id = $entity->{$info['entity keys']['id']};
          metatag_metatags_save($entity_type, $id, $metadata, $key);
        }
      }
    }
  }


  if (!function_exists('metatag_dual_language_editor_field_attach_alter')) {
    function metatag_dual_language_editor_field_attach_alter(&$form, &$state) {
      if (isset($form['#metatags'])) {
        // Replace the #metatags key with our own, to prevent metatags from
        // building it.
        $plang_meta = array();
        $slang_meta = array();
        metatag_field_attach_form(
            $form['entity_raw_type']['#value'],
            $form['entity']['#value'],
            $plang_meta,
            $state,
            $form['#plang']
        );
        metatag_field_attach_form(
            $form['entity_raw_type']['#value'],
            $form['entity']['#value'],
            $slang_meta,
            $state,
            $form['#slang']
        );
        $form['#dual-metatags'] = array(
          $form['#plang'] => $plang_meta['#metatags'],
          $form['#slang'] => $slang_meta['#metatags'],
        );
        unset($form['#metatags']);
      }
    }
  }
}

function dual_language_editor_metatag_form(&$form, $tags, $plang, $slang) {
  $instance = $tags[$plang]['instance'];
  $options = $tags[$plang]['options'];

  $info = metatag_get_info();
  if (empty($info['tags'])) {
    return;
  }

  // Merge in the default options.
  $options += array(
    'token types' => array(),
    'defaults' => metatag_config_load_with_defaults($instance),
  );


  $form['metatags'] = array(
    '#type' => 'fieldset',
    '#tree' => TRUE,
    '#title' => t('Meta tags'),
    '#multilingual' => TRUE,
    '#parents' => array('language_dep', 'metatags'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
    '#access' => user_access('edit meta tags') || user_access('administer meta tags'),
    '#weight' => 40,
    '#attributes' => array(
      'class' => array('metatags-form'),
    ),
    '#metatag_defaults' => $options['defaults'],
  );

  foreach (array($plang, $slang) as $lang) {
    // Merge in the default meta tag configurations.
    $metatags = $tags[$lang]['metatags'];
    $metatags += $options['defaults'];

    // Build the form for each metatag.
    foreach ($info['tags'] as $metatag => $metatag_info) {
      // @todo Replace context matching with hook_metatag_access().
      if (isset($options['context']) && isset($metatag_info['context'])) {
        if (!in_array($options['context'], $metatag_info['context'])) {
          continue;
        }
      }

      $metatag_instance = metatag_get_instance($metatag, isset($metatags[$metatag]) ? $metatags[$metatag] : array());
      if (empty($metatag_instance)) {
        continue;
      }

      // Get the form element from the meta tag class.
      $metatag_form = $metatag_instance->getForm($options);

      // Add a default value form element.
      if (isset($options['defaults'][$metatag]['value'])) {
        $metatag_form['default'] = array(
          '#type' => 'hidden',
          '#value' => $options['defaults'][$metatag]['value'],
        );
      }

      if (!empty($metatag_info['group'])) {
        $group_key = $metatag_info['group'];
        if (isset($info['groups'][$group_key]['label']) && !isset($form['metatags'][$group_key])) {
          $group = $info['groups'][$group_key] + array('form' => array(), 'description' => NULL);
          $form['metatags'][$group_key] = $group['form'] + array(
            '#type' => 'fieldset',
            '#title' => check_plain($group['label']),
            '#description' => filter_xss($group['description']),
            '#collapsible' => TRUE,
            '#collapsed' => TRUE,
            '#tree' => TRUE,
          );
        }
        if (!isset($form['metatags'][$group_key][$metatag])) {
          $form['metatags'][$group_key][$metatag] = array(
            '#type' => 'container',
            '#parents' => array('language_dep', 'metatags', $metatag),
          );
          dual_language_editor_row_helper($form['metatags'][$group_key][$metatag], $plang, $slang);
        }
        $form['metatags'][$group_key][$metatag][$lang] = $metatag_form;
      }
      else {
        if (!isset($form['metatags'][$metatag])) {
          $form['metatags'][$metatag] = array(
            '#type' => 'container',
            '#parents' => array('language_dep', 'metatags', $metatag),
          );
          dual_language_editor_row_helper($form['metatags'][$metatag], $plang, $slang);
        }
        $form['metatags'][$metatag][$lang] = $metatag_form;
      }
    }

  }

  // Show the list of available tokens.
  $form['metatags']['tokens'] = array(
    '#theme' => 'token_tree',
    '#token_types' => $options['token types'],
    '#weight' => 100,
    '#dialog' => TRUE,
  );

}

if (module_exists('field')) {
  if (!function_exists('field_dual_language_editor_field_attach_alter')) {
    /**
     * Implements hook_dual_language_editor_field_attach_alter().
     *
     * Fixes a bug with add_more where in the left-hand Add More button added
     * fields to the right-hand list.
     *
     * Solution: We added the language code to the element name of each add
     * more button, then we replace the callback so we can use the language code
     * from the parent field to update the correct field.
     *
     * @todo There should be a better way to get the language than looking at
     *   the parent field.
     */
    function field_dual_language_editor_field_attach_alter(&$form, &$form_state, $entity_type, $entity, $plang, $slang, $parent_key = NULL) {
      foreach ($form as $key => $value) {
        if (is_array($value)) {
          if ($key == 'add_more') {
            if (!empty($form[$key]['#name'])) {
              // if we don't update the name, then the callback gets confused
              // between the two fields. Unique names are needed.
              $form[$key]['#name'] .= '_' . $parent_key;
              $form[$key]['#ajax']['callback'] = 'dual_language_editor_add_more_js';
            }
          } elseif (substr($key, 0, 1) != '#') {
            field_dual_language_editor_field_attach_alter($form[$key], $form_state, $entity_type, $entity, $plang, $slang, $key);
          }
        }
      }
    }
  }
}

/**
 * AJAX callback in support of the updates to the Add More functionality.
 *
 * Most of this code is copied from the original.
 *
 * @see field_dual_language_editor_field_attach_alter().
 * @see field_add_more_js().
 */
function dual_language_editor_add_more_js($form, $form_state) {
  $button = $form_state['triggering_element'];
  // Go one level up in the form, to the widgets container.
  $element = drupal_array_get_nested_value($form, array_slice($button['#array_parents'], 0, -1));
  $field_name = $element['#field_name'];
  // This is the difference from the original, in that we use the parent
  // element name (which is the language code) to get the language code.
  // Nothing else has been changed in this function.
  $langcode = reset(array_slice($button['#array_parents'], -2, 1));
  $parents = $element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);

  $field = $field_state['field'];
  if ($field['cardinality'] != FIELD_CARDINALITY_UNLIMITED) {
    return;
  }

  // Add a DIV around the delta receiving the Ajax effect.
  $delta = $element['#max_delta'];
  $element[$delta]['#prefix'] = '<div class="ajax-new-content">' . (isset($element[$delta]['#prefix']) ? $element[$delta]['#prefix'] : '');
  $element[$delta]['#suffix'] = (isset($element[$delta]['#suffix']) ? $element[$delta]['#suffix'] : '') . '</div>';

  return $element;
}