<?php
/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 20.04.2021
 * Time: 13:13
 */

function kolesa_pricer_theme(){
    return array(
        'kolesa_pricer_add_shipper_form' => array(
            'render element' => 'form',
            'template' => 'kolesa_pricer_add_shipper_form',
            'file'=>'kolesa_pricer.theme.inc',
        ),
        'kolesa_pricer_log_form'=>array(
            'render element' => 'form',
            'template' => 'kolesa_pricer_log_form',
            'file'=>'kolesa_pricer.theme.inc',
        ),
        'pricer_log_element'=>array(
            'file'=>'kolesa_pricer.theme.inc',
        ),
        'kolesa_pricer_form_table' => array(
            // The renderable element is the form.
            'render element' => 'form',
            'file'=>'kolesa_pricer.theme.inc',
        ),
        'kolesa_pricer_catalog_model'=>array(
         #   'file'=>'kolesa_pricer.theme.inc',
            'template'=>'kolesa_pricer_catalog_model'
        ),
        'kolesa_pricer_catalog_model_list'=>array(
           # 'file'=>'kolesa_pricer.theme.inc',
            'template'=>'kolesa_pricer_catalog_model_list'
        ),
        'kolesa_pricer_catalog_model_products'=>array(
            # 'file'=>'kolesa_pricer.theme.inc',
            'template'=>'kolesa_pricer_catalog_model_products'
        ),
    );

}
function kolesa_pricer_preprocess_kolesa_pricer_catalog_model(&$vars){
    if(!empty($vars['view'])){
        $vars['theme_hook_suggestions'][]='kolesa_pricer_catalog_model__'.$vars['view'];
    }

}
function kolesa_pricer_preprocess_kolesa_pricer_catalog_model_products(&$vars){
    if(!empty($vars['view'])){
        $vars['theme_hook_suggestions'][]='kolesa_pricer_catalog_model_products__'.$vars['view'];
    }

}
function theme_kolesa_pricer_form_table(&$variables){
    // Get the useful values.
    $form = $variables['form'];
    $rows = $form['rows'];
    $header = $form['#header'];

    $table_id = $form['#id'];
    // Setup the structure to be rendered and returned.
    $content = array(
        '#theme' => 'table',
        '#header' => $header,
        '#rows' => array(),
        '#attributes'=>array('id'=>$table_id)
    );

    // Traverse each row.  @see element_chidren().
    foreach (element_children($rows) as $row_index) {
        $rows[$row_index]['weight']['#attributes']['class']=array('item-weight');
        $row = array();
        // Traverse each column in the row.  @see element_children().
        foreach (element_children($rows[$row_index]) as $col_index) {
            // Render the column form element.
            if($rows[$row_index][$col_index]['#type']=='value') continue;
            $row[] = array('data'=>drupal_render($rows[$row_index][$col_index]),'class'=>array($col_index));
        }
        $class=array('draggable');
        if($rows[$row_index]['disabled']['#default_value']) $class[]='disabled';
        // Add the row to the table.
        $content['#rows'][] = array('data'=>$row,'class'=>$class);
    }
    drupal_add_tabledrag($table_id, 'order', 'sibling', 'item-weight');
    // Redner the table and return.
    return drupal_render($content);
}
function theme_pricer_log_element(&$vars){
    $row=$vars['row'];
    $element=$vars['element'];
    $elementData=$row[$element];
    switch($element){
        case 'type':
            $elementData="<span class='element-$element value-$elementData'>".$elementData."</span>";
            break;
        case 'data':

            $elementData='<small>'.print_r(unserialize($elementData),TRUE).'</small>';
            break;
    }
    return $elementData;
}
