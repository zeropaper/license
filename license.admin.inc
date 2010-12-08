<?php


/**
 * Page callback: license descriptions overview.
 */
function license_admin_descriptions() {
  
  // Load license descriptions and present them in table form.
  $licenses = license_load();
  $header = array(
    0 => t('Title'),
    1 => t('Version'),
    2 => t('Language'),
    3 => t('Explanation'),
    4 => t('Name'),
    5 => t('Source'),
    6 => '',
    7 => '',
    8 => ''
  );
  $rows = array();

  foreach ($licenses as $key => $license) {
    $row = array();
    $row[0] = l($license->title, $license->url) .'<br/>'. theme('license_button_by_code', $license->name);
    $row[1] = check_plain($license->version);
    $row[2] = check_plain($license->language);
    $row[3] = check_plain($license->explanation);
    $row[4] = check_plain($license->name);

    $row[6] = l(t('Edit'),
      'admin/settings/license/edit/' . $license->name,
    array('attributes' => array('class' => 'link-edit')));
    switch ($license->export_type) {
      case 1:
        // "Normal" type.
        $row[5] = '<em>' . t('Normal') . '</em>';
        $row[7] = l(t('Delete'),
          'admin/settings/license/delete/' . $license->name,
        array('attributes' => array('class' => 'link-delete')));
        break;
      case 3:
        // "Overridden" type.
        $row[5] = '<em>' . t('Overridden') . '</em>';
        $row[7] = l(t('Revert'),
          'admin/settings/license/delete/' . $license->name,
        array('attributes' => array('class' => 'link-delete')));
        break;
      default:
        $row[5] = '<em>' . t('Default') . '</em>';
        $row[7] = '';
    }
    $row[8] = l(t('Export'),
      'admin/settings/license/export/' . $license->name,
    array('attributes' => array('class' => 'link-export')));
    ksort($row);

    $rows[] = $row;
  }

  $table = theme('table', $header, $rows);

  // Add links beneath the table.
  $links = array(
    'link-add' => array(
      'title' => t('Add license description'),
      'href' => 'admin/settings/license/add',
    ),
      'link-import' => array(
        'title' => t('Import license description'),
        'href' => 'admin/settings/license/import',
    ),
  );
  $links = theme('links', $links, array('class' => 'license-presets-links'));

  return $table . $links;
}


/**
 * license description settings form.
 *
 * @param $name
 *   The license description's machine readable name.
 */
function license_admin_description($form_state, $license = NULL) {
  // Check if we're supposed to edit a preset or add a new one.
  if ($license) {
    drupal_set_title(t('#name settings', array('#name' => check_plain($license->description))));
  }
  else {
    $license = new stdClass;
    $license->name = '';
    $license->explanation = '';
    $license_options = license_default_options();
    $license->cc_by = $license_options;
  }

  $form = array();

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('license description name'),
    '#description' => t('The unique, machine readable name of this license description.'),
    '#default_value' => $license->name,
    '#size' => 20,
    '#required' => true,
  );
  $form['explanation'] = array(
    '#type' => 'textfield',
    '#title' => t('license description explanation'),
    '#description' => t('The human readable name or explanation of this license description.'),
    '#default_value' => $license->description,
    '#size' => 20,
    '#required' => true,
  );

  // Add FFmpeg Wrapper's configuration form.
  //$form += cc_by_configuration_form($license->cc_by, "edit-license-presets-$license->name-cc-by-");
  $form['cc_by']['#collapsible'] = true;
  $form['cc_by']['#tree'] = true;

  // Check this license description's export type/storage.
  if (isset($license->export_type)) {
    switch ($license->export_type) {
      case 2:
        // "Default" type.
        $form['name']['#disabled'] = true;
        $form['name']['#value'] = $form['name']['#default_value'];
        break;
      case 3:
        // "Overridden" type.
        $form['name']['#disabled'] = true;
        $form['name']['#value'] = $form['name']['#default_value'];
        break;
    }
  }

  // Save extra license description info for the submit handler.
  if (!empty($license->lid)) {
    $form['lid'] = array(
      '#type' => 'value',
      '#value' => $license->lid,
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save')
  );

  return $form;

}

/**
 * Validate a license description form.
 */
function license_admin_description_validate($form, &$form_state) {
  // Check for illegal characters in preset names.
  if (preg_match('/[^0-9a-zA-Z_-]/', $form_state['values']['name'])) {
    form_set_error('name', t('Please only use alphanumeric characters, underscores (_), and hyphens (-) for license description names.'));
  }
}

/**
 * Submit handler to save a license description.
 */
function license_admin_description_submit($form, &$form_state) {
  // Serialize cc_by data.
  $form_state['values']['cc_by'] = serialize($form_state['values']['cc_by']);

  if (empty($form_state['values']['lid'])) {
    // Save new license description.
    $res = drupal_write_record('license', $form_state['values']);
  }
  else {
    // Save existing license description.
    $res = drupal_write_record('license', $form_state['values'], 'lid');
  }
  if ($res) {
    drupal_set_message(t('Saved license description %license.', array('%license' => $form_state['values']['description'])));
  }
  else {
    drupal_set_message(t('Failed to save license description %license.', array('%license' => $form_state['values'])));
    drupal_goto('admin/settings/license');
  }
}

/**
 * Delete license description form.
 *
 * @param $name
 *   The license description's machine readable name.
 */
function license_admin_description_delete($form_state, $license) {
  $form = array();
  $form['lid'] = array(
    '#type' => 'value',
    '#value' => $license->lid,
  );
  // Save a couple of values for the submit handler.
  $form['explanation'] = array( // 'description' is used by confirm_form().
    '#type' => 'value',
    '#value' => $license->description,
  );
  $form['export_type'] = array(
    '#type' => 'value',
    '#value' => $license->export_type,
  );

  // Adjust the UI texts depending on export type.
  switch ($license->export_type) {
    case 1:
      // "Normal" type.
      $message = t('Are you sure you want to delete %title?', array('%title' => $license->description));
      $button = t('Delete');
      break;
    case 3:
      // "Overridden" type.
      $message = t('Are you sure you want to revert %title?', array('%title' => $license->description));
      $button = t('Revert');
      break;
    default:
      // There are no other export types that can be deleted.
      drupal_goto('admin/settings/license');
      return array();
  }

  return confirm_form($form,
    $message,
    isset($_GET['destination']) ? $_GET['destination'] : 'admin/settings/license',
    t('This action cannot be undone. If you are using this license description in other parts of your system, you should change those settings first.'),
    $button,
    t('Cancel')
  );

}

/**
 * Submit handler to delete a license description.
 */
function license_admin_description_delete_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    license_delete($form_state['values']['lid']);
    // Adjust message depending on export type.
    $message = $form_state['values']['export_type'] == 1 ?
      'Deleted license description %license.' :
      'Reverted license description %license.';
    drupal_set_message(t($message, array('%license' => $form_state['values']['explanation'])));
  }
  $form_state['redirect'] = 'admin/settings/license';
}


/**
 * Page callback: export a license description.
 *
 * @param $license
 *   A license description object.
 */
function license_admin_description_export($form_state, $license) {
  drupal_set_title(t('Export of license description %description', array('%description' => $license->description)));
  $form = array();

  // Get export code with Ctools.
  $export = ctools_export_object('license', $license);
  $export = "<?php\n{$export}return serialize(\$license);\n?>";

  // Create the export code textarea.
  ctools_include('export');
  $form['export'] = array(
    '#type' => 'textarea',
    '#title' => t('license description code'),
    '#rows' => 20,
    '#default_value' => $export,
  );

  return $form;
}

/**
 * Page callback: import license descriptions.
 */
function license_admin_description_import() {
  $form = array();
  $form['import'] = array(
    '#type' => 'textarea',
    '#rows' => 10,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );
  return $form;
}

/**
 * Validate a license description import.
 */
function license_admin_description_import_validate($form, &$form_state) {
  // Run the import code, which should return a serialized $license object.
  $license = unserialize(drupal_eval($form_state['values']['import']));
  if (empty($license) || !is_object($license) || empty($license->name)) {
    form_set_error('import', t('The submitted license description code could not be interperated.'));
  }
  elseif (license_load($license->name)) {
    form_set_error('import', t('A license description by that name already exists.'));
  }
  else {
    // Pass the parsed object on to the submit handler.
    $form_state['values']['import_parsed'] = $license;
  }
}

/**
 * Submit handler to import a license description.
 */
function license_admin_description_import_submit($form, &$form_state) {
  $license = (array) $form_state['values']['import_parsed'];

  if (drupal_write_record('license', $license)) {
    drupal_set_message(t('Imported license description %license.', array('%license' => $license['explanation'])));
  }
  else {
    drupal_set_message(t('Failed to import the license description.'), 'warning');
  }

  $form_state['redirect'] = 'admin/settings/license';
}