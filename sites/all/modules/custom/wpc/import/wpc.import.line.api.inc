<?php

/**
 * @file
 * Process import of a csv line, i.e. of a term or a list of terms.
 */
$module_dir = drupal_get_path('module', 'wpc');

//require_once($module_dir . '/import/wpc.import.line.api.inc');
/**
 * Process the import of items.
 *
 * @param $line
 *   Array which contains items of a cleaned and checked csv line.
 * @param $options
 *   An associative array of import options:
 *   - import_format  : format of the csv line (see taxonomy.api.inc)
 *   - keep_order     : boolean. keep order of imported terms or not (default)
 *   - vocabulary     : vocabulary object where to import terms into
 *   - update_or_ignore: indicates what will become existing terms, if any.
 * @param $previous_items
 *   (Optional) Cleaned and checked previous imported line names and tids array.
 *   Needed with some contents as one term array structure.
 * @param $terms_count
 *   (Optional integer) Total of imported terms (duplicate included) is needed
 *   to set weight of terms and to keep order of items, if wished.
 *
 * @return
 *   Result array:
 *   - 'name' => array of imported terms names,
 *   - 'tid'  => array of imported terms tids,
 *   - 'msg'  => array of message codes,
 *   - 'terms_count' => number of imported terms.
 */
//$module_dir = drupal_get_path('module', 'commerce');
//require_once($module_dir . '\product\commerce_product.api');
function wpc_line_import($line, $options, $previous_items = array(), $terms_count = 0) {
    // Define default values.
    $result = array(
      'name' => array(),
      'tid' => array(),
      'msg' => array(),
      'terms_count' => $terms_count,
    );

    // Only count check because function variables are already checked.
    if (count($line)) {
        switch ($options['import_format']) {
            case TAXONOMY_CSV_FORMAT_FLAT:
                $result = wpc_line_import_flat($line, $options, $terms_count);
                break;

            case TAXONOMY_CSV_FORMAT_STRUCTURE:
            case TAXONOMY_CSV_FORMAT_TREE:        // Internally, tree import format use ignore_previous and not update.
                if ($options['update_or_ignore'] == TAXONOMY_CSV_EXISTING_UPDATE) {
                    $options['update_or_ignore'] = TAXONOMY_CSV_EXISTING_IGNORE_PREVIOUS;
                }
                $result = wpc_line_import_structure($line, $options, $previous_items, $terms_count);
                break;

            case TAXONOMY_CSV_FORMAT_POLYHIERARCHY:
                // Internally, polyhierarchy import format doesn't support Ignore.
                $options['update_or_ignore'] = TAXONOMY_CSV_EXISTING_UPDATE;

                $result = wpc_line_import_structure($line, $options, $previous_items, $terms_count);
                break;

            case TAXONOMY_CSV_FORMAT_FIELDS:
                $result = wpc_line_import_fields($line, $options, $terms_count);
                break;

            case TAXONOMY_CSV_FORMAT_TRANSLATE:
                if (!module_exists('i18n_taxonomy')) {
                    $result['msg'][] = 360; // Translation error.
                    break;
                }

                switch ($options['vocabulary']->i18n_mode) {
                    case I18N_MODE_LOCALIZE:
                        $result = wpc_line_import_localize($line, $options, $terms_count);
                        break;
                    case I18N_MODE_TRANSLATE:
                    case I18N_MODE_MULTIPLE:
                        $result = wpc_line_import_translate($line, $options, $terms_count);
                        break;
                    default:
                        $result['msg'][] = 361; // Translation of vocabulary error.
                }
                break;

            default:
                $result['msg'][] = 306; // Error unknown import format.
        }
    }
    else {
        $result['msg'][] = 685; // No term to process.
    }

    return $result;
}

/**
 * Import a flat line.
 *
 * @see wpc_line_import()
 */
function wpc_line_import_flat($line, $options, $terms_count = 0) {
    // Define default values.
    $result = array(
      'name' => array(),
      'tid' => array(),
      'msg' => array(),
      'terms_count' => $terms_count,
    );

    foreach ($line as $term_name) {
        $term = new stdClass;
        $term->name = $term_name;
        $term->vid = $options['vocabulary']->vid;
        $term->vocabulary_machine_name = $options['vocabulary']->machine_name;
        $term->format = $options['filter_format'];
        $term->language = $options['language'];
        // Weight is not set above in order to keep existing one if it exists.
        ++$terms_count;
        if ($options['keep_order']) {
            $term->weight = $terms_count;
        }

        // Import term then store and check result.
        $current_result = wpc_term_import($term, $options['update_or_ignore']);
        if (_wpc_line_result($result, $current_result, $terms_count)) {
            return $result;
        }
    }

    return $result;
}

/**
 * Import a structure line.
 *
 * @see wpc_line_import()
 */
function wpc_line_import_structure($line, $options, $previous_items = array(), $terms_count = 0) {
    $result = array(
      'name' => array(),
      'tid' => array(),
      'msg' => array(),
      'terms_count' => $terms_count,
    );
    /* Edited By shylaja */
    // 3. Import each new term then store and check result.
    $linecount = count($line);
    //$linecount = 18;
    // 1. Check if one term or partial line format is used in order to virtually
    // complete line with previous line.
    // Find first non empty item as line can be full, partial or one term.
    for ($first = 0; ($first < $linecount) && empty($line[$first]); $first++) {
        
    }
    // Else line is full. As with one term import, imported items on previous line
    // can be bypassed if lines are ordered. So, look for first different item.
    if (!$first) {
        for ($first = 0; ($first < count($line) - 1) && ($first < count($previous_items['name'])) && ($line[$first] == $previous_items['name'][$first]); $first++) {
            
        }
    }

    // 2. Remove and remember superabondant previous items for next line.
    if ($first) {
        $result['name'] = $previous_items['name'] = array_slice($previous_items['name'], 0, $first);
        $result['tid'] = $previous_items['tid'] = array_slice($previous_items['tid'], 0, $first);
        $result['msg'][] = 683; // Previous line term.
        // Set root or previous ancestor name and id.
        $parent_name = $previous_items['name'][$first - 1];
        $parent_tid = $previous_items['tid'][$first - 1];
    }
    // Item is the first on the line: it's a root term.
    else {
        $parent_name = '';
        $parent_tid = 0;
    }
    $saved_sku = array(); // Saving items in for loop so checking if already saved it or not
    $saved_brandterm = array();
    $saved_brandvariant_term = array();
    for ($c = $first; $c < $linecount; $c++) {
        if ($line[0] == 'Business Code') // Ignore first row in the CSV headers
            break;
        $hul_term = array();
        $hul_term_des = array();
        $hul_term[0] = $line[0]; //Business Code
        $hul_term[1] = $line[2]; //Businss Group Code
        $hul_term[2] = $line[4]; //Profit Center Code
        $hul_term[3] = $line[6]; //Category Code
        $hul_term[4] = $line[8];  //Subcategory Code
        $hul_term[5] = $line[10];  //Brand Code - should be separate vocabulary
        $hul_term[6] = $line[12];  //Brand Variant - should be separate vocabulary
        $hul_term[5] = $line[14]; // Base Pack Code

        $hul_term_des[0] = $line[1];
        $hul_term_des[1] = $line[3];
        $hul_term_des[2] = $line[5];
        $hul_term_des[3] = $line[7];
        $hul_term_des[4] = $line[9];
        $hul_term_des[5] = $line[11];
        $hul_term_des[6] = $line[13];
        $hul_term_des[5] = $line[15];
        $produc_sku = $line[16];
        $term_count++;

        // Saving Hul_category Terms and its Hierarchy
        $term = new stdClass;
        $term->name = $hul_term[$c];
        $term->vid = $options['vocabulary']->vid;
        $term->vocabulary_machine_name = $options['vocabulary']->machine_name;
        $term->format = $options['filter_format'];
        $term->language = $options['language'];
        $term->parent = array($parent_tid => $parent_tid);
        $term->description = $hul_term_des[$c];
        // Weight is not set above in order to keep existing one if it exists.
        ++$terms_count;
        if ($options['keep_order']) {
            $term->weight = $terms_count;
        }

        switch ($options['import_format']) {
            case TAXONOMY_CSV_FORMAT_STRUCTURE:
            case TAXONOMY_CSV_FORMAT_TREE:
                // for this default is CSV tree structure  
                // With TAXONOMY_CSV_EXISTING_IGNORE, parent terms (so all terms but the
                // last on this line) are always updated because they are successive
                // parents of a child.
                $current_result = ($options['update_or_ignore'] == TAXONOMY_CSV_EXISTING_IGNORE && ($c < $linecount - 1)) ?
                    wpc_term_import($term, TAXONOMY_CSV_EXISTING_IGNORE_PREVIOUS, $parent_tid) :
                    wpc_term_import($term, $options['update_or_ignore'], $parent_tid);
                //$child= taxonomy_get_children($current_result['tid'], $vid = $options['vocabulary']->vid, $key = 'tid'); 
                // Saving to terms table

                break;
            case TAXONOMY_CSV_FORMAT_POLYHIERARCHY:
                // Check direct duplicates: in Drupal, a term can't be its parent.
                $current_result = ($term->name == $parent_name) ?
                    wpc_term_import($term, $options['update_or_ignore'], $parent_tid) :
                    wpc_term_import($term, $options['update_or_ignore'], NULL);
                break;
        }
        $parent_name = $current_result['name'];
        $parent_tid = $current_result['tid'];
        if (_wpc_line_result($result, $current_result, $terms_count)) {
            return $result;
        }
        $basepackcode = taxonomy_get_term_by_name($line[14]);  // Column 14 Base Pack Code ,Product will be attached to this code.
        $key = (array_keys($basepackcode)); //print_r($key);
        $key_value = $key[0];
        $product_tid = $basepackcode[$key_value]->tid; // getting Term_id from 14th column in the csv
        $skuarray = array();

        if (!in_array($line[10], $saved_brandterm)) {
            $saved_brandterm[] = $line[10];
            $module_dir = drupal_get_path('module', 'wpc');
            $file = $module_dir . '/import' . '/brand.txt';
            $myFile = $file;
            $fh = fopen($myFile, 'r') or die("can't open file");
            $stringData = "$line[10] 1\n";
            fwrite($fh, $stringData);
            fclose($fh);
            //Saving Brand Taxonomy
            $brandmachine_name = 'brand';
            $vocabular_brand = taxonomy_vocabulary_load_multiple(NULL, array('machine_name' => $brandmachine_name));
            $key_brand = (array_keys($vocabular_brand)); //print_r($key);
            $keybrand_value = $key_brand[0];
            $brand_vid = $vocabular_brand[$keybrand_value]->vid;
            $termbrand = new stdClass;
            $termbrand->name = $line[10];
            $termbrand->vid = $brand_vid;
            $termbrand->vocabulary_machine_name = 'brand';
            $termbrand->format = $options['filter_format'];
            $termbrand->language = $options['language'];
            $termbrand->parent = 0;
            $termbrand->description = $line[11];
            $brandsave = wpc_term_import($termbrand, $options['update_or_ignore'], 0);
        }
        // Saving Brand Variant Taxonomy
        if (!in_array($line[12], $saved_brandvariant_term)) {
            $brandparent_name = $brandsave['name'];
            $brandparent_tid = $brandsave['tid'];
            $saved_brandvariant_term[] = $line[12];
            $termbrandvariant = new stdClass;
            $termbrandvariant->name = $line[12];
            $termbrandvariant->vid = $brand_vid;
            $termbrandvariant->vocabulary_machine_name = 'brand';
            $termbrandvariant->format = $options['filter_format'];
            $termbrandvariant->language = $options['language'];
            $termbrandvariant->parent = $brandparent_tid;
            $termbrandvariant->description = $line[13];
            $brandvariantsave = wpc_term_import($termbrandvariant, $options['update_or_ignore'], $brandparent_tid);
        }


        if (!in_array($line[16], $saved_sku)) {  // column 16 is the CBU or SKU in drupal Commerce saving Commerce Product
            if ($c == 5 && !empty($current_result['tid'])) { // Basepack column after save
                //print_r($current_result);
                $proBasepacktid = $current_result['tid'];


                $product = commerce_product_load_by_sku($line[16]);
                $saved_sku[] = $line[16];
                $skuarray[] = $line[16];
                if (empty($product))
                    $product = commerce_product_new('product');
                $product->sku = $line[16];
                $skuarray[] = $line[16];
                $product->title = $line[16];
                $product->language = LANGUAGE_NONE;
                $product->uid = 1;
                /* $product->commerce_price[LANGUAGE_NONE][0] = array(
                  'amount' => 10 * 100, // $10
                  'currency_code' => "USD",
                  ); */
                $product->field_product_category[LANGUAGE_NONE][0]['tid'] = $proBasepacktid;
                $product->status = '0';
                //$brandparent_tid  = $brandsave['tid'];
                $product->field_product_brand[LANGUAGE_NONE][0]['tid'] = $brandvariantsave['tid'];
                commerce_product_save($product);  //  Saving Commerce Product


                $query = new EntityFieldQuery();
                $query->entityCondition('entity_type', 'node')
                    ->entityCondition('bundle', 'product_display')
                    ->fieldCondition('field_base_pack_code', 'value', $line[14], '=');
                $result = $query->execute();
                // Attaching Product to Node Based on Base Pack Code
                if (!empty($result)) {
                    $key_node = (array_keys($result['node'])); //print_r($key);
                    $key_value = $key_node[0];
                    $node = ($result['node'][$key_value]);
                    $last_index = count($node->field_product_ref['und']);
                    $nid = $node->nid;
                    $node = node_load($nid);
                    $node->title = $line[14];
                    $node->field_product_ref['und'][]['product_id'] = $product->product_id;
                }
                else {
                    $node = new stdClass();
                    $node->type = 'product_display';
                    node_object_prepare($node);
                    $node->title = $line[14];
                    $node->field_product_ref['und'][]['product_id'] = $product->product_id;
                }


                $node->language = LANGUAGE_NONE;
                $node->field_base_pack_code[$node->language][0]['value'] = $line[14];
                node_save($node);
                // writing to file for debug purpose
                $module_dir = drupal_get_path('module', 'wpc');
                $file = $module_dir . '/import' . '/debugsku.txt';
                $myFile = $file;
                $fh = fopen($myFile, 'r') or die("can't open file");
                $stringData = "$line[16] 1\n";
                fwrite($fh, $stringData);
                fclose($fh);
            }
        }
    }
    //print_r($skuarray);die;
    return $result;
}

/**
 * Import a fields line.
 *
 * @see wpc_line_import()
 */
function wpc_line_import_fields($line, $options, $terms_count = 0) {
    // Define default values.
    $result = array(
      'name' => array(),
      'tid' => array(),
      'msg' => array(),
      'terms_count' => $terms_count,
    );

    // Define a default term.
    $term = new stdClass;
    $term->vid = $options['vocabulary']->vid;
    $term->vocabulary_machine_name = $options['vocabulary']->machine_name;
    $term->format = $options['filter_format'];
    $term->language = $options['language'];
    // Weight is not set above in order to keep existing one if it exists.
    ++$terms_count;
    if ($options['keep_order']) {
        $term->weight = $terms_count;
    }

    $parents = array();

    // Import normal fields according custom format.
    foreach ($options['fields_format'] as $key => $field_name) {
        if (!isset($line[$key]) || ($line[$key] === '')) {
            continue;
        }

        if (in_array($field_name, array(
              'tid',
              'name',
              'format',
              'weight',
              'language',
              'i18n_tsid',
              'guid',
            ))) {
            $term->{$field_name} = $line[$key];
        }
        elseif ($field_name == 'vid') {
            $term->vid = $line[$key];
            $term->vocabulary_machine_name = taxonomy_vocabulary_load($line[$key])->machine_name;
        }
        elseif ($field_name == 'vocabulary_machine_name') {
            $term->vocabulary_machine_name = $line[$key];
            $term->vid = taxonomy_vocabulary_machine_name_load($line[$key])->vid;
        }
        elseif ($field_name == 'description') {
            $term->description = _wpc_set_line_break($line[$key]);
        }
        elseif ($field_name == 'parent') {
            // Just remember value: parents are imported later in the process.
            $parents[] = $line[$key];
        }
        elseif (isset($options['fields'][$field_name])) {
            // @todo To be rewritten.
            // True fields will be added later.
            // Currently, repetitive items are not managed.
            $term->fields_to_import[$field_name][] = $line[$key];
            $term->fields_to_import_instances = $options['instances'];
            $term->fields_to_import_fields = $options['fields'];
        }
        // Else there is an error.
        else {
            $current_result['msg'] = 341; // Non existing field.
            return $result;
        }
    }

    // Import parents if any.
    foreach ($parents as $value) {
        // Presume that item is a tid if it's a number.
        if (is_numeric($value)) {
            if (!$value) {
                $current_result['msg'] = 455; // Reference to term 0.
                continue;
            }
            // @todo Check if it isn't the current term id.
            // @todo Check if parent is in current vocabulary.
            $term->parent[] = $value;
        }
        // Else find the parent normally.
        else {
            $parent_term = new stdClass;
            $parent_term->name = $value;
            $parent_term->vid = $term->vid;
            $parent_term->vocabulary_machine_name = $term->vocabulary_machine_name;
            $parent_term->language = $term->language;
            $current_result = wpc_term_import($parent_term, $options['update_or_ignore']);
            if (_wpc_line_result($result, $current_result, $terms_count)) {
                return $result;
            }
            $term->parent[] = $current_result['tid'];
        }
    }

    // Import main term then store result. No check because only one term.
    $current_result = wpc_term_import($term, $options['update_or_ignore']);
    $error = _wpc_line_result($result, $current_result, $terms_count);

    return $result;
}

/**
 * Import a localization line.
 *
 * @see wpc_line_import()
 */
function wpc_line_import_localize($line, $options, $terms_count = 0) {
    // Define default values.
    $result = array(
      'name' => array(),
      'tid' => array(),
      'msg' => array(),
      'terms_count' => $terms_count,
    );

    // 1. Import main term then store and check result.
    $term = new stdClass;
    if ($options['translate_by'] == 'name') {
        $term->name = $line[0];
    }
    else {
        $term->tid = $line[0];
    }
    $term->vid = $options['vocabulary']->vid;
    $term->vocabulary_machine_name = $options['vocabulary']->machine_name;
    // With this i18n mode, language of main term is always undefined, whatever
    // the vocabulary language is.
    $term->language = 'und';
    $first_description = count($options['translate_languages']);
    if (isset($line[$first_description]) && $line[$first_description]) {
        // Other formats aren't allowed for translation of the description with this
        // i18n mode.
        $term->format = 'plain_text';
        $term->description = _wpc_set_line_break($line[$first_description]);
    }
    // Weight is not set above in order to keep existing one if it exists.
    ++$terms_count;
    if ($options['keep_order']) {
        $term->weight = $terms_count;
    }

    // Import term then store result.
    $current_result = wpc_term_import($term, $options['update_or_ignore']);
    if (_wpc_line_result($result, $current_result, $terms_count)) {
        break;
    }

    // 2. Create localizations.
    foreach ($options['translate_languages'] as $key => $language) {
        // Don't import main item.
        if ($key == 0) {
            continue;
        }

        $result_translation = i18n_string_translation_update(
            array('taxonomy', 'term', $current_result['tid'], 'name'), $line[$key], $language, $line[0]
        );
        if (!$result_translation) {
            $result['msg'][] = 360; // Translation error.
            return $result;
        }

        if (isset($line[$first_description + $key]) && $line[$first_description] && $line[$first_description + $key]
        ) {
            $result_translation = i18n_string_translation_update(
                array('taxonomy', 'term', $current_result['tid'], 'description'), _wpc_set_line_break($line[$first_description + $key]), $language, _wpc_set_line_break($line[$first_description])
            );
            if (!$result_translation) {
                $result['msg'][] = 360; // Translation error.
                return $result;
            }
        }
    }

    return $result;
}

/**
 * Import a translation line.
 *
 * @see wpc_line_import()
 */
function wpc_line_import_translate($line, $options, $terms_count = 0) {
    // Define default values.
    $result = array(
      'name' => array(),
      'tid' => array(),
      'msg' => array(),
      'terms_count' => $terms_count,
    );

    // 1. Prepare main term.
    $term = new stdClass;
    if ($options['translate_by'] == 'name') {
        $term->name = $line[0];
    }
    else {
        $term->tid = $line[0];
    }
    $term->vid = $options['vocabulary']->vid;
    $term->vocabulary_machine_name = $options['vocabulary']->machine_name;
    $term->format = $options['filter_format'];
    $term->language = $options['translate_languages'][0];
    // Weight is not set above in order to keep existing one if it exists.
    ++$terms_count;
    if ($options['keep_order']) {
        $term->weight = $terms_count;
    }

    // Import term then store result.
    $current_result = wpc_term_import($term, $options['update_or_ignore']);
    if (_wpc_line_result($result, $current_result, $terms_count)) {
        return $result;
    }
    // Need to get term with tid and eventual tsid.
    $term = taxonomy_term_load($current_result['tid']);

    // 2. Use translation set of the term if it exists, else create it.
    $translation_set = ($term->i18n_tsid) ?
        i18n_translation_set_load($term->i18n_tsid) :
        i18n_translation_set_create('taxonomy_term', $options['vocabulary']->machine_name);

    $translation_set->add_item($term, $term->language);

    // 3. Check if the term is already translated in order to update or create it.
    $existing_terms = $translation_set->get_translations();

    foreach ($options['translate_languages'] as $key => $language) {
        // Don't import main item.
        if ($key == 0) {
            continue;
        }

        // Don't import an empty item.
        if (isset($line[$key]) && $line[$key]) {
            $translated_term = new stdClass;
            if (isset($existing_terms[$language])) {
                switch ($options['update_or_ignore']) {
                    case TAXONOMY_CSV_EXISTING_UPDATE:
                        $translated_term = $existing_terms[$language];
                        // As the term is already loaded and we have the tid, we simply save
                        // it without to find it a new time (avoid search).
                        $options['update_or_ignore'] = TAXONOMY_CSV_EXISTING_IGNORE;
                        break;
                    case TAXONOMY_CSV_EXISTING_IGNORE:
                        break;
                }
            }

            // 4. Complete or create the translated term, then import it.
            $translated_term->name = $line[$key];
            $translated_term->vid = $options['vocabulary']->vid;
            $translated_term->vocabulary_machine_name = $options['vocabulary']->machine_name;
            $translated_term->format = $options['filter_format'];
            $translated_term->language = $language;

            // Import term then store result.
            $current_result = wpc_term_import($translated_term, $options['update_or_ignore']);
            if (_wpc_line_result($result, $current_result, $terms_count)) {
                return $result;
            }
            // Needed to get a term with tid.
            $translated_term = taxonomy_term_load($current_result['tid']);

            // 5. Update or create the translation set.
            // Remove is needed to avoid errors when a translated term is updated.
            $translation_set->remove_language($translated_term->language);
            $translation_set->add_item($translated_term, $translated_term->language);
        }
    }

    $translation_set->save(TRUE);

    return $result;
}

/**
 * Helper to merge results with current result.
 *
 * @return
 *   TRUE if the result contains an error. Result is passed by reference.
 */
function _wpc_line_result(&$result, $current_result, $terms_count) {
    $result['name'][] = isset($current_result['name']) ? $current_result['name'] : '';
    $result['tid'][] = isset($current_result['tid']) ? $current_result['tid'] : 0;
    $result['msg'] = isset($result['msg']) ?
        array_merge($result['msg'], $current_result['msg']) :
        $current_result['msg'];
    $result['terms_count'] = $terms_count;
    return (_wpc_worst_message($current_result['msg']) < TAXONOMY_CSV_PROCESS_NOTICE);
}

/**
 * Update/create a term with the given name and parent in the given vocabulary.
 *
 * @param $term
 *   A term object to import. Term contains either:
 *   - 'name'        => term name string,
 *   - 'tid'         => term id,
 *   and eventually, matching options:
 *   - 'vid'         => the vocabulary id where to import,
 *   - 'description' => description string,
 *   - 'format'      => the filter format of the description,
 *   - 'language'    => the language id of the term,
 *   - 'weight'      => weight integer,
 *   - 'parent'      => array of first level parent tids,
 * @param $update_or_ignore
 *   (Optional) Type of import on existing terms. Default to ignore and create.
 * @param $parent_tid
 *   (Optional) The direct parent term id where to restrict search.
 *   Used for structure import. Default to NULL (no parent restriction).
 *
 * @return array
 *    'name' => term name,
 *    'tid'  => term id,
 *    'msg'  => messages array.
 *
 * @todo Include true update/replace/ignore for fields.
 */
function wpc_term_import($term, $update_or_ignore = TAXONOMY_CSV_EXISTING_IGNORE, $parent_tid = NULL) {
    $messages = array();

    // Basic check to avoid notices when "Check lines" option is disabled.
    if ((!isset($term->name) && !isset($term->tid)) || (isset($term->name) && !$term->name) || (isset($term->tid) && !$term->tid)
    ) {
        $term->tid = 0;
        $messages[] = 432; // Warning line contains an empty term.
        return array(
          'name' => '',
          'tid' => 0,
          'msg' => $messages,
        );
    }

    switch ($update_or_ignore) {
        case TAXONOMY_CSV_EXISTING_UPDATE:
            $existing_term = wpc_term_find($term, FALSE, $parent_tid);
            if ($existing_term) {
                // Update only fields that are set. Other fields are not changed.
                foreach ($term as $key => $value) {
                    // Arrays: merge existing and new items.
                    // Only used for parent in standard taxonomy of Drupal 7.
                    if (is_array($value) && isset($existing_term->{$key})) {
                        $term->{$key} = array_unique(array_merge($existing_term->{$key}, $value));
                    }
                    // Else simply use new key: no merge is possible and useful for
                    // string, numeric or boolean fields.
                    // Description was an exception in previous version, but this
                    // exception is removed for simplicity and real use of this module.
                    // An option may be added if needed.
                }
                // Existing fields of existing term should be kept even if they have
                // not been set in new term.
                $term = (object) array_merge((array) $existing_term, (array) $term);
            }
            break;

        case TAXONOMY_CSV_EXISTING_IGNORE_PREVIOUS:
            // Doesn't ignore, but use previous parents.
            $existing_term = wpc_term_find($term, FALSE, $parent_tid);
            if ($existing_term) {
                // All fields are replaced by new ones. Other existing fields of
                // existing term should be kept even if they are not set in new term.
                $term = (object) array_merge((array) $existing_term, (array) $term);
            }
            break;

        case TAXONOMY_CSV_EXISTING_IGNORE:
            // Nothing to do: keep the term.
            break;
    }

    // Finish to set the term to avoid NULL. Format and language are set before.
    if (!isset($term->format)) {
        $term->format = 'plain_text';
    }
    if (!isset($term->description)) {
        $term->description = '';
    }
    // Currently, custom fields are managed by an external function.
    if (isset($term->fields_to_import)) {
        $messages = _wpc_term_field_import($term, $update_or_ignore);
        unset($term->fields_to_import);
        unset($term->fields_to_import_instances);
        unset($term->fields_to_import_fields);
        if (_wpc_worst_message($messages) < TAXONOMY_CSV_PROCESS_NOTICE) {
            return array(
              'name' => '',
              'tid' => 0,
              'msg' => $messages,
            );
        }
    }

    // Save regularly formatted term.
    // Return either SAVED_NEW, SAVED_UPDATED or FALSE (no change).
    $result = taxonomy_term_save($term);

    $messages[] = ($result == SAVED_NEW) ? 691 : 692; // Saved or updated.

    return array(
      'name' => $term->name,
      'tid' => $term->tid,
      'msg' => $messages,
    );
}

/**
 * Helper to import attached fields.
 *
 * @see wpc_term_import()
 *
 * @todo Integrate with wpc_line_import and wpc_term_import.
 * @todo True update/replace/ignore.
 */
function _wpc_term_field_import($term, $update_or_ignore = TAXONOMY_CSV_EXISTING_IGNORE) {
    $messages = array();

    // Currently, translatable fields are unmanaged.
    $language = 'und'; // Undefined.

    foreach ($term->fields_to_import as $field_name => $values) {
        $instance = &$term->fields_to_import_instances[$field_name];
        $field = &$term->fields_to_import_fields[$field_name];

        foreach ($values as $value) {
            switch ($field['type']) {
                case 'taxonomy_term_reference':
                    // Get machine name of referenced vocabulary.
                    $referenced_vocabulary = $field['settings']['allowed_values'][0]['vocabulary'];
                    $referenced_vocabulary = taxonomy_vocabulary_machine_name_load($referenced_vocabulary);

                    $referenced_term = new stdClass;
                    // Presume that item is a tid if it's a number.
                    if (is_numeric($value)) {
                        if (!$value) {
                            $messages['notice'] = t('Unable to make a reference to term "0".');
                            continue 2;
                        }
                        // @todo Check if the term is in allowed vocabularies.
                        $referenced_term->tid = $value;
                    }
                    // Else find the parent normally.
                    else {
                        $referenced_term->name = $value;
                        $referenced_term->vid = $referenced_vocabulary->vid;
                        $referenced_term->vocabulary_machine_name = $referenced_vocabulary->machine_name;
                        // @todo Use undefined language or term language?
                        $referenced_term->language = $term->language;
                        // @todo Increase term count.
                        $current_result = wpc_term_import($referenced_term, $update_or_ignore);
                        if (!$current_result['tid']) {
                            $messages[] = 402; // Unable to import a field term.
                            return $messages;
                        }
                        $referenced_term->tid = $current_result['tid'];
                    }

                    $value = array(
                      'tid' => $referenced_term->tid,
                    );
                    switch ($instance['widget']['type']) {
                        case 'taxonomy_autocomplete':
                            // Need tid, vid, name and vocabulary_machine_name (use cache).
                            $referenced_term = taxonomy_term_load($referenced_term->tid);
                            $value['name'] = $referenced_term->name;
                            $value['vid'] = $referenced_term->vid;
                            $value['vocabulary_machine_name'] = $referenced_term->machine_name;
                            break;
                    }

                    // Complete term if there are already items.
                    if (is_array($term->{$field_name}) && !empty($term->{$field_name})) {
                        foreach ($term->{$field_name} as $field_language => &$delta_array) {
                            $delta_array[] = $value;
                        }
                    }
                    // Else complete term directly.
                    else {
                        $term->{$field_name}[$language][0] = $value;
                    }
                    break;

                case 'file':
                    $uri_scheme = $field['settings']['uri_scheme'];
                    $file_directory = $instance['settings']['file_directory'];
                    $file_extensions = $instance['settings']['file_extensions'];

                    $source = $value;
                    $destination = $uri_scheme . '://' . $file_directory;

                    // Import source if it exists.
                    if (file_exists($source)) {
                        $filepath = file_unmanaged_copy($source, $destination, FILE_EXISTS_RENAME);
                        if ($filepath !== FALSE) {
                            $file = new stdClass();
                            $file->uid = 1;
                            $file->filename = basename($filepath);
                            $file->uri = $filepath;
                            $file->filemime = file_get_mimetype($filepath);
                            $file->filesize = filesize($filepath);
                            $file->status = 1;
                            $file->timestamp = time();

                            $file = file_save($file);

                            // Complete term if there are already items.
                            if (is_array($term->{$field_name}) && !empty($term->{$field_name})) {
                                foreach ($term->{$field_name} as $field_language => &$delta_array) {
                                    $delta_array[] = array(
                                      'fid' => $file->fid,
                                      'display' => 1,
                                      'description' => '',
                                    );
                                }
                            }
                            // Else complete term directly.
                            else {
                                $term->{$field_name}[$language][0] = array(
                                  'fid' => $file->fid,
                                  'display' => 1,
                                  'description' => '',
                                );
                            }
                        }
                    }
                    elseif ($source != '') {
                        $messages['notice'] = t('Unable to load attached file.');
                    }
                    // Else nothing to import.
                    break;

                case 'list_boolean':
                    $value = (($value == '0') || (drupal_strtolower($value) == 'false')) ? 0 : 1;
                    $term->{$field_name}[$language][0] = array('value' => $value);
                    break;

                // Managed by default:
                // case 'number_decimal':
                // case 'number_integer':
                // case 'number_float':
                // case 'text':
                // case 'text_long':
                // case 'text_with_summary': // ?
                // Currently doesn't manage default value.
                default:
                    switch ($field['type']) {
                        case 'number_decimal':
                            $value = (float) $value;
                            break;
                        case 'number_integer':
                            $value = (integer) $value;
                            break;
                        case 'number_float':
                            $value = (float) $value;
                            break;
                    }

                    // Complete term if there are already items.
                    if (is_array($term->{$field_name}) && !empty($term->{$field_name})) {
                        foreach ($term->{$field_name} as $field_language => &$delta_array) {
                            $delta_array[] = array('value' => $value);
                        }
                    }
                    // Else complete term directly.
                    else {
                        $term->{$field_name}[$language][0] = array('value' => $value);
                    }
                    break;
            }
        }
    }

    return $messages;
}

/**
 * Helper to build a standard text field from a simple string or an array.
 *
 * @todo To be removed (as other helpers) by using of Field api (but slower).
 * @todo Fields internationalization. Should be compatible with the i18n mode of
 *   the vocabulary.
 *
 * @param $value
 *   A string or an array to convert.
 * @param $language
 *   (Optional). Language to use. Default is 'und' (undefined).
 * @param $format
 *   (Optional). Format of the field. Default to NULL (fixed plain text).
 *
 * @return
 *   Formatted field array.
 */
function _wpc_field_create_text($value, $language = 'und', $format = NULL) {
    // Currently, i18n of term custom fields is not supported.
    $language = 'und';

    $field = array();
    if (!is_array($value) && !empty($value)) {
        $value = array($value);
    }

    if ($format == 'none') {
        $format = NULL;
    }

    foreach ($value as $item) {
        if ($item !== '') {
            $field[] = array(
              'value' => $item,
              'format' => $format,
              'safe_value' => ($format) ?
                  check_markup($item, $format, $language) :
                  check_plain($item),
            );
        }
    }

    return ($field) ?
        array($language => $field) :
        array();
}

/**
 * Helper to build a standard term reference field from a string or an array.
 *
 * @param $value
 *   A string or an array to convert.
 * @param $language
 *   (Optional). Language to use. Default is 'und' (undefined).
 *
 * @return
 *   Formatted field array.
 */
function _wpc_field_create_taxonomy_term_reference($value, $language = 'und') {
    // Currently, i18n of term custom fields is not supported.
    $language = 'und';

    $field = array();
    if (!is_array($value) && !empty($value)) {
        $value = array($value);
    }

    foreach ($value as $item) {
        if ($item) {
            $field[]['tid'] = $item;
        }
    }

    return ($field) ?
        array($language => $field) :
        array();
}

/**
 * Helper to convert an internal array field to a standard text field.
 *
 * @param $field_name
 *   Field to update.
 * @param $term
 *   Term object.
 * @param $existing_term
 *   Previous term object.
 *
 * @return
 *   Nothing: $term object is passed by reference.
 */
function _wpc_field_update_text($field_name, $term, $existing_term) {
    // Complete term if there are already items.
    if (is_array($existing_term->{$field_name}) && !empty($existing_term->$field_name)) {
        $new_field = $term->{$field_name};
        $term->{$field_name} = $existing_term->$field_name;
        $field = &$term->{$field_name};
        foreach ($new_field as $field_language => $array) {
            // Don't update if new field is empty.
            if (!empty($array)) {
                // Update the value for that language.
                if (isset($field[$field_language])) {
                    // Don't use array_merge or array_merge_recursive to avoid duplicates.
                    $existing = array();
                    foreach ($field[$field_language] as &$value) {
                        $existing[] = &$value['value'];
                    }
                    foreach ($array as &$new_value) {
                        // Avoid duplicates and avoid to append an empty item.
                        if (!in_array($new_value['value'], $existing) && ($new_value['value'] !== '')) {
                            $field[$field_language][] = $new_value;
                        }
                    }
                }
                // Value for this language is not set, so complete term directly.
                else {
                    foreach ($array as &$new_value) {
                        // Avoid to append an empty item.
                        if ($new_value['value'] !== '') {
                            $field[$field_language][] = $new_value;
                        }
                    }
                }
            }
        }
    }
    // Else nothing to do: term contains already new field.
}

/**
 * Helper to convert an internal array field to a standard term reference field.
 *
 * @param $field_name
 *   Field to update.
 * @param $term
 *   Term object.
 * @param $existing_term
 *   Previous term object.
 *
 * @return
 *   Nothing: $term object is passed by reference.
 */
function _wpc_field_update_taxonomy_term_reference($field_name, $term, $existing_term) {
    // Complete term if there are already items.
    if (is_array($existing_term->{$field_name}) && !empty($existing_term->$field_name)) {
        $new_field = $term->{$field_name};
        $term->{$field_name} = $existing_term->$field_name;
        $field = &$term->{$field_name};
        foreach ($new_field as $field_language => $array) {
            // Don't update if new field is empty.
            if (!empty($array)) {
                // Update the value for that language.
                if (isset($field[$field_language])) {
                    // Don't use array_merge or array_merge_recursive to avoid duplicates.
                    $existing = array();
                    foreach ($field[$field_language] as &$value) {
                        $existing[] = &$value['tid'];
                    }
                    foreach ($array as &$new_value) {
                        // Avoid duplicates and avoid to append an empty item.
                        if (!in_array($new_value['tid'], $existing) && ($new_value['tid'] != 0)) {
                            $field[$field_language][] = $new_value;
                        }
                    }
                }
                // Value for this language is not set, so complete term directly.
                else {
                    foreach ($array as &$new_value) {
                        // Avoid to append an empty item.
                        if ($new_value['tid'] != 0) {
                            $field[$field_language][] = $new_value;
                        }
                    }
                }
            }
        }
    }
    // Else nothing to do: term contains already new field.
}

/**
 * Helper to convert an internal repetitive field to a standard text field.
 *
 * @param $term
 *   A taxonomy term object with custom fields.
 * @param $field_name
 *   Field to update.
 * @param $language
 *   (Optional) Language code.
 *
 * @return
 *   Nothing: $term object is passed by reference.
 */
function _wpc_line_replace_field_text($term, $field_name, $language = 'und') {
    if (isset($term->{$field_name}) && is_array($term->$field_name)) {
        $field = array();
        foreach ($term->{$field_name} as $value) {
            $field[] = array(
              'value' => $value,
              'format' => NULL,
              'safe_value' => check_plain($value),
            );
        }
        $term->{$field_name}[$language] = $field;
    }
}
