<?php

class KolesaShipperFactory
{
    protected static $instances;
    public static function build($controllerClass)
    {

        if (class_exists($controllerClass)) {
            if(empty($instances[$controllerClass])) {
                $instances[$controllerClass]=new $controllerClass();
            }
            return $instances[$controllerClass];
        } else {
            throw new \Exception("Несуществующий контроллер");
        }
    }
}


interface ShipperControllerInterface
    extends DrupalEntityControllerInterface {
    public function create();
    public function save($entity);
    public function delete($entity);
}
interface ShipperStockControllerInterface
    extends DrupalEntityControllerInterface {
    public function create();
    public function save($entity);
    public function delete($entity);
}


class ShipperController
    extends DrupalDefaultEntityController
    implements ShipperControllerInterface {

    public function create() {
        $entity = new stdClass();
        $entity->id = 0;
        $entity->type='shipper';
        $entity->machine_key='';
        $entity->b2b=0;
        $entity->status=0;
        $entity->name = '';
        $entity->params=[];
        $entity->setup=[];
        $entity->stocks=[];
        return $entity;
    }
    public function load($ids = array(), $conditions = array()){

        $entities=parent::load($ids,$conditions);

        foreach($entities as &$entity){
            $entity->params=unserialize($entity->params);
            $entity->setup=unserialize($entity->setup);
            $entity->stocks=entity_load('shipper_stock',array(),array('sid'=>$entity->id));

            if(isset($entity->params['controller class'])){
                $controllerName=$entity->params['controller class'];

                if(class_exists($controllerName)) {
                    try {
                       $controller = new $controllerName;
                       $entity->allowedPrices=$controller->getAllowedPrices();
                    } catch (Exception $e){
                        throw new Exception("FUCK");
                    }

                }
            }



        }

        return $entities;
    }
    public function save($entity){
        $primary_keys = $entity->id ? 'id' : array();

      #  if(is_array($entity->params)) $entity->params=serialize($entity->params);
      #  if(is_array($entity->setup)) $entity->setup=serialize($entity->setup);
        if(!empty($entity->stocks)){
            $stocks=$entity->stocks;
            unset($entity->stocks);
            foreach($stocks as $stock) {
                $stock->sid=$entity->id;
                entity_get_controller('shipper_stock')->save($stock);
            }
        }
        drupal_write_record('pricer_shipper', $entity, $primary_keys);
        $invocation = 'entity_insert';
        module_invoke_all($invocation, $entity, 'shipper');
        return $entity;
    }
    public function delete($entity){
        return TRUE;
    }
}

class ShipperStockController
    extends DrupalDefaultEntityController
    implements ShipperStockControllerInterface {

    protected function buildQuery($ids, $conditions = array(), $revision_id = FALSE) {
        $query=parent::buildQuery($ids, $conditions, $revision_id);
        $query->join('pricer_shipper','ship','ship.id=base.sid');
        $query->addField('ship','machine_key','shipperName');
        return $query;
    }

    public function load($ids = array(), $conditions = array()){

        $entities=parent::load($ids,$conditions);

        foreach($entities as &$entity){
            $categories=db_query("select category, count(cae) from {pricer_price} where stock=:id and qty>0 group by category",[':id'=>$entity->id])->fetchAllKeyed(0,1);
            $entity->categories=$categories;
        }

        return $entities;
    }

    public function create() {
        $entity = new stdClass();
        $entity->id = 0;
        $entity->sid= 0;
        $entity->machine_key='';
        $entity->name = '';
        $entity->delivery_days = 0;
        $entity->description = '';

        $entity->b2b=0;
        $entity->params=[];
        $entity->setup=[];
        $entity->status=0;
        return $entity;
    }
    public function save($entity){
     #   parent::save($entity);
        module_invoke_all('entity_presave', $entity, 'shipper_stock');
        $primary_keys = $entity->id ? 'id' : array();
        drupal_write_record('pricer_shipper_stocks', $entity, $primary_keys);
        $invocation = 'entity_insert';
        module_invoke_all($invocation, $entity, 'shipper_stock');
        return $entity;
    }
    public function delete($entity){
        return TRUE;
    }
}

function kolesa_pricer_get_all_shippers(){
    $active_shippers=entity_load('shipper');
    foreach($active_shippers as $shipper){
        $ids[$shipper->id]=$shipper;
    }
    return $ids;
}
function kolesa_pricer_get_active_shippers(){
    $active_shippers=entity_load('shipper',array(),array('status'=>1));
    #$active_shippers=entity_load('shipper');
    foreach($active_shippers as $shipper){
        $ids[$shipper->id]=$shipper;
    }
    return $ids;
}
function kolesa_pricer_get_mrc_shippers(){
    $active_shippers=entity_load('shipper',array(),array('status'=>1));
    $ids=[];
    foreach($active_shippers as $shipper){
        $mrc_params=$shipper->params['mrc'];
        if($mrc_params['mrc_enable']) $ids[$shipper->id]=$shipper;
    }
    return $ids;
}
function kolesa_pricer_get_stocks($type=''){
    static $stocks;
    if(empty($stocks)) {
        if ($cache = cache_get('pricer_stocks')) {
            $stocks = $cache->data;
        } else {
            $all_stocks=entity_load('shipper_stock');
            foreach($all_stocks as $stock){
                $stock->shipper=current(entity_load('shipper',array($stock->sid)));
                $stocks['all'][$stock->id]=$stock;
                if($stock->status==0) $stocks['disabled'][$stock->id]=$stock;
                if($stock->status==1) $stocks['instock'][$stock->id]=$stock;
                if($stock->status==2) $stocks['order'][$stock->id]=$stock;
                if($stock->status==3) $stocks['own'][$stock->id]=$stock;
            }
        }
    }
    if(!empty($type)) return $stocks[$type];
    return $stocks;
}

function kolesa_pricer_entity_info() {
    $info['shipper'] = array(
        // A human readable label to identify our entity.
        'label' => t('Shipper Entity'),
        'module' => 'kolesa_pricer',
        'controller class' => 'ShipperController',
        'base table' => 'pricer_shipper',
        'fieldable' => FALSE,
        'entity keys' => array(
            'id' => 'id' ,
            'machine_key'=>'machine_key',
        ),
        'static cache' => FALSE,
        'view modes' => array(
            'full' => array(
                'label' => t('Full'),
                'custom settings' =>  FALSE,
            ),
        )
    );
    $info['shipper_stock'] = array(
        // A human readable label to identify our entity.
        'label' => t('Shipper Stock Entity'),
        'module' => 'kolesa_pricer',
        'controller class' => 'ShipperStockController',
        'base table' => 'pricer_shipper_stocks',
        'fieldable' => FALSE,
        'entity keys' => array(
            'id' => 'id' ,

        ),
        'static cache' => FALSE,
        'view modes' => array(
            'full' => array(
                'label' => t('Full'),
                'custom settings' =>  FALSE,
            ),
        )
    );

    return $info;
}

function kolesa_pricer_entity_property_info() {
    $info = array();
    $shipper_properties = &$info['shipper']['properties'];
    $shipper_properties['id'] = array(
        'label' => t("Shipper ID"),
        'type' => 'integer',
    );
    $shipper_properties['machine_key'] = array(
        'label' => t("Shipper Key"),
        'type' => 'varchar',
    );
    $shipper_properties['name'] = array(
        'label' => t("Shipper Name"),
        'type' => 'varchar',
    );

    $shipper_stock_properties = &$info['shipper_stock']['properties'];
    $shipper_stock_properties['id'] = array(
        'label' => t("Shipper`s Stock ID"),
        'type' => 'integer',
    );
    $shipper_stock_properties['sid'] = array(
        'label' => t("Shipper`s Stock"),
        'type' => 'integer',
    );
    $shipper_stock_properties['machine_key'] = array(
        'label' => t("Shipper`s Stock Machine Key"),
        'type' => 'varchar',
    );
    $shipper_stock_properties['name'] = array(
        'label' => t("Shipper`s Stock Name"),
        'type' => 'varchar',
    );
    $shipper_stock_properties['status'] = array(
        'label' => t("Shipper`s Stock Status"),
        'type' => 'integer',
    );
    $shipper_stock_properties['description'] = array(
        'label' => t("Shipper`s Stock Description"),
        'type' => 'varchar',
    );

    return $info;
}


function kolesa_pricer_schema_alter(&$schema) {

    $schema['pricer_shipper'] = array(
        'description' => 'Shippers list',
        'fields' => array(
            'id' => array(
                'description' => 'Shipper ID',
                'type' => 'serial',
                'not null' => FALSE,
            ),
            'machine_key' => array(
                'description' => 'Shipper Key',
                'type' => 'varchar',
                'length' => '50',
                'not null' => FALSE,
            ),
            'name' => array(
                'description' => 'Shipper Name',
                'type' => 'varchar',
                'length' => '255',
                'not null' => FALSE,
            ),
            'params' => array(
                'description' => 'Params serialize',
                'type' => 'blob',
                'serialize'=>TRUE,
                'not null' => FALSE,
            ),
            'setup' => array(
                'description' => 'Setup serialize',
                'type' => 'blob',
                'serialize'=>TRUE,
                'not null' => FALSE,
            ),
            'b2b' => array(
                'description' => 'B2B enable',
                'type' => 'int',
                'not null' => FALSE,
                'default' => 0,
            ),
            'status' => array(
                'description' => 'Status',
                'type' => 'int',
                'not null' => FALSE,
                'default' => 1,
            ),
            'weight'=>array(
                'description' => 'Weight',
                'type' => 'int',
                'not null' => FALSE,
                'default' => 0,
            ),
            'updated'=>array(
                'mysql_type' => 'timestamp',
                'not null' => FALSE,
                'default' => NULL,
            ),
        ),
        'unique keys' => array(
            'id' => array('id')
        ),
        'indexes' => array(
            'key' => array('machine_key'),
            'status' => array('status'),
        ),
    );

    $schema['pricer_shipper_stocks'] = array(
        'description' => 'TODO: пожалуйста, опишите эту таблицу!',
        'fields' => array(
            'id' => array(
                'description' => 'TODO: пожалуйста, опишите это поле!',
                'type' => 'serial',
                'not null' => FALSE,
            ),
            'sid' => array(
                'description' => 'TODO: пожалуйста, опишите это поле!',
                'type' => 'int',
                'not null' => FALSE,
            ),
            'machine_key' => array(
                'description' => 'TODO: пожалуйста, опишите это поле!',
                'type' => 'varchar',
                'length' => '50',
                'not null' => FALSE,
            ),
            'name' => array(
                'description' => 'TODO: пожалуйста, опишите это поле!',
                'type' => 'varchar',
                'length' => '255',
                'not null' => FALSE,
            ),
            'delivery_days' => array(
                'description' => 'TODO: пожалуйста, опишите это поле!',
                'type' => 'int',
                'not null' => FALSE,
            ),
            'description' => array(
                'description' => 'TODO: пожалуйста, опишите это поле!',
                'type' => 'text',
                'not null' => FALSE,
            ),
            'b2b' => array(
                'description' => 'B2B enable',
                'type' => 'int',
                'not null' => FALSE,
                'default' => NULL,
            ),
            'status' => array(
                'description' => 'Status',
                'type' => 'int',
                'not null' => FALSE,
                'default' => 0,
            ),
            'weight'=>array(
                'description' => 'Weight',
                'type' => 'int',
                'not null' => FALSE,
                'default' => 0,
            ),
        ),
        'unique keys' => array(
            'id' => array('id')
        ),
        'indexes' => array(
            'delivery_days' => array('delivery_days'),
            'sid' => array('sid'),
        ),
    );
}



function kolesa_pricer_shipper_page(){
    $build=array();
    $build['shipper_update']=array(
      'form'=>  drupal_get_form('kolesa_pricer_shipper_action_form'),
    );
    $build['shipper_list']=array(
        'form'=>drupal_get_form('kolesa_pricer_shipper_list_form'),
    );
    return $build;
}

function kolesa_pricer_get_shipper_brands($catName,$shipperKey){
    $result=db_query("select id,brand from {pricer_mapping_brands} where shipper=:shipper and category=:category and id is not null order by brand asc",array(":shipper"=>$shipperKey,":category"=>$catName))->fetchAllKeyed(0,1);
    return $result;
}
function kolesa_pricer_get_shipper_categories($shipperKey){
    $result=db_query("select distinct category from {pricer_mapping_brands} where shipper=:shipper",array(":shipper"=>$shipperKey))->fetchAllKeyed(0,0);
    return $result;
}

function kolesa_pricer_shipper_action_form($form,&$form_state){
    $form['actions']=[
      '#type'=>'actions',
    ];
    $form['actions']['update_source']=[
      '#type'=>'submit',

      '#value'=>'Обновить источники данных',
        '#submit'=>['kolesa_pricer_shipper_action_form_submit'],
    ];
    $form['actions']['update_products']=[
        '#type'=>'submit',
        '#value'=>'Пакетное обновление позиций',
        '#op'=>'products',
        '#submit'=>['kolesa_pricer_shipper_action_form_products_submit'],
    ];
    $form['actions']['update_price']=[
        '#type'=>'submit',
        '#value'=>'Пакетное обновление цен',
        '#op'=>'price',
        '#submit'=>['kolesa_pricer_shipper_action_form_products_submit'],
    ];
    return $form;
}

function kolesa_pricer_shipper_action_form_submit($form,&$form_state){
    $shippers=kolesa_pricer_get_active_shippers();
    foreach($shippers as $shipper) {
        $operations[]=['kolesa_pricer_shipper_batch_update_shipper_source',[$shipper]];
        #kolesa_pricer_update_shipper_source($shipper);
    }
    $batch = array(
        'title' => 'Обновляем источники данных',
        'init_message' => 'Начали процесс',
        'error_message' => t('Batch has encountered an error.'),
        'finished' => 'kolesa_pricer_shipper_batch_finished',
        'file' => drupal_get_path('module', 'kolesa_pricer') . '/kolesa_pricer.shipper.inc',
        'operations' => $operations,
    );
    batch_set($batch);
}
function kolesa_pricer_shipper_batch_update_shipper_source($shipper,&$context){
    kolesa_pricer_update_shipper_source($shipper, TRUE);
    $context['message']='Обновляем '.$shipper->name;
    $context['finished']=1;
}


function kolesa_pricer_shipper_action_form_products_submit($form,&$form_state){
    $trigger=$form_state['triggering_element'];
    $shippers=kolesa_pricer_get_active_shippers();
    $operations=[];
    foreach($shippers as $shipper){
        $controller=new $shipper->params['controller class'];
        $options=$controller->getAllowedPrices();
        foreach($options as $key=>$option){
            if($option['queued']==$trigger['#op']||$key==$trigger['#op']){
                $operations[]=array('kolesa_pricer_batch_get_products',array($shipper,$key));
                $operations[]=array('kolesa_pricer_batch_update_products',array($shipper,$key));
                #$operations+=kolesa_pricer_shipper_batch_get_operations($shipper,$key);
              #  $operations[]=array('kolesa_pricer_batch_test',array($shipper,$key));
            }
        }
    }

    $batch = array(
        'title' => 'Обновляем товарные позиции - '.$trigger['#value'],
        'init_message' => 'Начали процесс',
        'error_message' => t('Batch has encountered an error.'),
        'finished' => 'kolesa_pricer_shipper_batch_finished',
        'file' => drupal_get_path('module', 'kolesa_pricer') . '/kolesa_pricer.shipper.inc',
        'operations' => $operations,
    );
    batch_set($batch);
}
function kolesa_pricer_batch_get_products($shipper,$key,&$context){
    db_delete('pricer_process_products')->condition('shipper',$shipper->machine_key)->condition('option',$key)->execute();
    $price=kolesa_pricer_shipper_get_data($shipper,$key);
   # kolesa_pricer_shipper_update_source($shipper,$price,TRUE);
    foreach($price as $catName=>$catPrice){
        $priceChunk=array_chunk($catPrice,500);
        foreach($priceChunk as $no=>$chunk){
            $query=db_insert("pricer_process_products")->fields(array('shipper', 'option', 'category','row'));
            foreach($chunk as $row) {
                $query->values(['shipper' => $shipper->machine_key, 'option'=>$key,'category'=>$catName,'row' => serialize($row)]);
            }
            $query->execute();
        }
    }

    $context['finished']=1;
    $context['message'].='Забираем прайс у '.$shipper->machine_key.' - '.$key;
}


function kolesa_pricer_batch_update_products($shipper,$option,&$context){
    $limit=20;
    if(empty($context['sandbox'])){
            $context['sandbox'] = [];
            $context['sandbox']['id']=0;
            $context['sandbox']['count']=db_query("select count(id) from {pricer_process_products} where shipper=:shipper and option=:option",[':shipper'=>$shipper->machine_key,':option'=>$option])->fetchField();
            $context['sandbox']['no']=0;
    }
    $controller=KolesaShipperFactory::build($shipper->params['controller class']);
    $importOptions=array(
        'shipper'=>$shipper,
        'controller'=>$controller,
        'option'=>$option,
    );
    $price=[];
    $query=db_query("select id,row from {pricer_process_products} where shipper=:shipper and option=:option and id>:id  limit ".$limit,[':shipper'=>$shipper->machine_key,':option'=>$option,':id'=>$context['sandbox']['id']]);
    foreach($query as $row){
        $price[]=unserialize($row->row);
        $context['sandbox']['id']=$row->id;
        $context['sandbox']['no']++;
    }
    KolesaPricerLog::addLog('message','pricer_update','Update '.$shipper->machine_key.' option '.$option);


    $processed_rows=$controller->updateData($importOptions['option'],$price);

    $context['message'].='Обновляем '.$shipper->machine_key.' - '.$option;
   # $context['message'].=theme('table',array('rows'=>$controller->getAllowedPrices()[$option]));
    $context['message'].='<br/>'.$context['sandbox']['no'].' из '.$context['sandbox']['count'];
    if(!empty($processed_rows)) {
        $context['message'] .= theme('table', array('rows' => $processed_rows, 'header' => array_keys($processed_rows[0])));
    }
    if ($context['sandbox']['no'] != $context['sandbox']['count']) {
        $context['finished'] = ($context['sandbox']['no'] >= $context['sandbox']['count']);
    }


}

function kolesa_pricer_add_shipper_form($form,&$form_state,$sid){
    if(empty($form_state['storage']['stocks_count'])) $form_state['storage']['stocks_count']=1;
    $def_values=array(
        'name'=>'',
        'machine_key'=>'',
        'update_type'=>'',
    );
    if(!empty($sid)){
        $shipper=reset(entity_load('shipper',array($sid),array(),TRUE));

        drupal_set_title('Настройка поставщика '.$shipper->name);
        $def_values=[
            'name'=>$shipper->name,
            'machine_key'=>$shipper->machine_key,
            'params'=>$shipper->params,
            'b2b'=>$shipper->b2b,
            'setup'=>$shipper->setup
        ];
        $form['shipper']=array(
            '#type'=>'value',
            '#value'=>$shipper,
        );

        $shipperCats=kolesa_pricer_get_shipper_categories($shipper->machine_key);

    }

    if(!empty($form_state['values'])) {
        $def_values = isset($form_state['values']) ? $form_state['values'] : [];
    }
    $form['#theme']='kolesa_pricer_add_shipper_form';

    $form['name']=array(
      '#type'=>'textfield',
      '#default_value'=>!empty($def_values['name'])?$def_values['name']:NULL,
      '#title'=>'Название поставщика',
      '#size'=>25,
    );
    $form['machine_key']=array(
        '#type'=>'textfield',
        '#default_value'=>!empty($def_values['machine_key'])?$def_values['machine_key']:NULL,
        '#title'=>'Key',
        '#size'=>25,
        '#description'=>'Только английские буквы, цифры и знак _',
    );
    $form['b2b']=array(
        '#type'=>'checkbox',
        '#default_value'=>!empty($def_values['b2b'])?$def_values['b2b']:NULL,
        '#title'=>'B2B',
    );
    $form['params']=array(
        '#type'=>'container',
        '#attributes'=>array('id'=>'shipper-params'),
        '#title'=>'Параметры',
        '#tree'=>TRUE,
    );
    $ajax=array(
      'callback'=>'kolesa_pricer_add_shipper_form_callback',
      'wrapper'=>'shipper-params',
    );
    $update_types=array();

    drupal_alter('shipper_update_types',$update_types);

    $form['params']['update type']=array(
        '#type'=>'select',
        '#title'=>'Тип импорта',
        '#options'=>$update_types,
        '#empty_option'=>'- Выберите тип импорта -',
        '#ajax'=>$ajax,
        '#default_value'=>!empty($def_values['params']['update type'])?$def_values['params']['update type']:NULL,
    );

    if(isset($def_values['params']['update type'])){
        if($def_values['params']['update type']=='custom'){
            $form['params']['controller class']=array(
                '#title'=>'Класс обработки',
                '#type'=>'textfield',
                '#size'=>25,
                '#default_value'=>$def_values['params']['controller class'],

            );
        }
    }

    $form['params']['mrc']=array(
      '#type'=>'container',
        '#tree'=>TRUE,
    );
    $form['params']['mrc']['mrc_enable']=array(
        '#type'=>'checkbox',
        '#title'=>'Грузить МРЦ',
        '#default_value'=>!empty($def_values['params']['mrc']['mrc_enable']) ? $def_values['params']['mrc']['mrc_enable'] : NULL,
        '#ajax'=>$ajax,
    );
    if(isset($def_values['params']['mrc']['mrc_enable'])) {
        $form['params']['mrc']['update type'] = array(
            '#type' => 'select',
            '#title' => 'Тип импорта для МРЦ',
            '#options' => $update_types,
            '#empty_option' => '- Выберите тип импорта -',
            '#ajax' => $ajax,
            '#default_value' => !empty($def_values['params']['mrc']['update type']) ? $def_values['params']['mrc']['update type'] : NULL,
        );

        if (isset($def_values['params']['mrc']['update type'])) {
            if ($def_values['params']['mrc']['update type'] == 'custom') {
                $form['params']['mrc']['controller class'] = array(
                    '#title' => 'Класс обработки для МРЦ',
                    '#type' => 'textfield',
                    '#size' => 25,
                    '#default_value' => $def_values['params']['mrc']['controller class'],
                );
            }
        }
    }
    $form['stocks_container']=array(
        '#type'=>'fieldset',
        '#title'=>'Склады',
    );
    $form['stocks_container']['stocks']=array(
        '#type'=>'container',
        '#attributes'=>array('id'=>'edit-stocks'),
        '#tree'=>TRUE,
        '#theme'=>'kolesa_pricer_form_table',
        '#id'=>'stocks-list-table',
        '#header'=>array(
            'key'=>'Key','name'=>'Название','delivery_days'=>'Срок доставки','description'=>'Описание','b2b'=>'B2B','status'=>'Включен','weight'=>'Вес'
        ),
    );
    $form['stocks_container']['new_stocks']=array(
      '#type'=>'container',
        '#attributes'=>array('id'=>'edit-new-stocks'),
        '#tree'=>TRUE,
        'stocks'=>[
            '#theme'=>'kolesa_pricer_form_table',
            '#header'=>array(
                'key'=>'Key','name'=>'Название','delivery_days'=>'Срок доставки','description'=>'Описание','b2b'=>'B2B','status'=>'Включен'
            ),
        ],
    );
    $form['stocks_container']['new_stocks']['stocks_add']=array(
        '#type'=>'submit',
        '#value'=>'Добавить склад',
        '#submit' => array('kolesa_pricer_add_shipper_stock_submit'),
        '#ajax'=>array(
          'callback'=>'kolesa_pricer_add_shipper_stock_ajax',
          'wrapper'=>'edit-new-stocks',
        ),
    );
    if(!empty($shipper)) {
        $b2b_options=[];
        $b2b_shippers=kolesa_pricer_shipper_load_multiple([],['b2b'=>1]);
        foreach($b2b_shippers as $b2b_id=>$b2b_object){
            $b2b_options[$b2b_id]=$b2b_object->name;
        }
        uksort($shipper->stocks,function($a,$b){
            if($a->weight==$b->weight) return 0;
            return $a->weight<$b->weight?-1:1;
        });
        foreach ($shipper->stocks as $stock) {
            $stock_row['id'] = array(
                '#type' => 'value',
                '#value' => $stock->id,
            );
            $stock_row['machine_key'] = array(
                '#type' => 'textfield',
                '#size' => 10,
                '#default_value' => $stock->machine_key,
            );
            $stock_row['name'] = array(
                '#type' => 'textfield',
                '#size' => 25,
                '#default_value' => $stock->name,
            );
            $stock_row['delivery_days'] = array(
                '#type' => 'textfield',
                '#size' => 5,
                '#default_value' => $stock->delivery_days,
            );
            $stock_row['description'] = array(
                '#type' => 'textarea',
                '#cols' => 30,
                '#rows' => 1,
                '#resizable' => FALSE,
                '#default_value' => $stock->description,
            );
            $stock_row['b2b']=array(
                '#type'=>'select',
                '#options'=>$b2b_options,
                '#default_value'=>$stock->b2b,
                '#empty_option'=>'- не брать B2B -',

            );
            $stock_row['status'] = array(
                '#type' => 'select',
                '#options' => array(0 => 'отключен', 1 => 'в наличии', 2 => 'под заказ'),
                '#default_value' => $stock->status,
            );

            $stock_row['weight'] = array(
                '#type' => 'weight',
                '#title'=>'Вес',
                '#default_value' => $stock->weight,
                '#delta' => 20,
            );
            $form['stocks_container']['stocks']['rows'][] = $stock_row;
        }
    }
    for ($i = 0; $i < $form_state['storage']['stocks_count']; $i++) {
        $form['stocks_container']['new_stocks']['stocks']['rows'][]=array(
            'machine_key'=>array('#type'=>'textfield','#size'=>10),
            'name'=>array('#type'=>'textfield','#size'=>20),
            'delivery_days'=>array('#type'=>'textfield','#size'=>10),
            'description'=>array('#type'=>'textarea','#rows'=>1,'#resizable'=>FALSE),
            'b2b'=>array('#type'=>'select','#options'=>$b2b_options,'#empty_option'=>'- не брать B2B -'),
            'status'=>array('#type'=>'select', '#options'=>array(0=>'отключен',1=>'в наличии',2=>'под заказ'),
            ),
        );
    }

    $form['setup_container']=array(
        '#type'=>'fieldset',
        '#title'=>'Настройки цен поставщика',
        '#printed'=>TRUE,
    );
    $form['setup_container']['group']=array(
        '#type'=>'container',
        '#tree'=>TRUE,

    );
    $setup_elements=array(
        'nac'=>array('textfield','Наценка в %'),
        'nac_rub'=>array('textfield','Наценка в руб.'),
        'skid'=>array('textfield','Скидка на опт в %'),
        'rec'=>array('checkbox','РРЦ'),
    );
    $setup_groups=array(
      'All'=>'Общие',
      'Tyres'=>'Шины',
      'Wheels'=>'Диски',
    );

    foreach($setup_groups as $group=>$name){
        if($group=='All'||isset($shipperCats[$group])) {
            $form['setup_container']['group'][$group] = array(
                '#type' => 'container',
                '#prefix' => "<h3>" . $name . ' - настройки</h3>',
            );
            foreach ($setup_elements as $el_key => $el_data) {
                $form['setup_container']['group'][$group][$el_key] = array(
                    '#type' => $el_data[0],
                    '#title' => $el_data[1],
                    '#default_value'=>$def_values['setup']['global'][$group][$el_key]??NULL,
                );
            }
        }
    }
    if(isset($shipperCats)) {
        $form['brands']=array(
            '#type'=>'container',
            '#tree'=>TRUE,
            '#title'=>'Настройка цен для брендов',
            '#printed'=>TRUE,

        );
        foreach ($shipperCats as $catName) {
            $catBrands=kolesa_pricer_get_shipper_brands($catName, $shipper->machine_key);
            $rows=array();
            foreach($catBrands as $tid=>$name){
                $row=array(
                  'name'=>array('#markup'=>l($name,'admin/price/shipper/'.$sid.'/pricelist/'.$tid)),
                );
                foreach($setup_elements as $el_key=>$el_data){
                    $row[$el_key]=array(
                        '#type'=>$el_data[0],
                        '#title'=>$el_data[1],
                        '#size'=>2,
                        '#default_value'=>$def_values['setup']['brand'][$tid][$el_key]??NULL,
                    );
                }
                $rows[$tid]=$row;
            }

            $form['brands'][$catName]=array(
                '#theme'=>'kolesa_pricer_form_table',
                '#title'=>$setup_groups[$catName],
                'rows'=>$rows,
            );
        }
    }

    $form['actions']=array(
      '#type'=>'actions',
    );
    $form['actions']['submit']=array(
      '#type'=>'submit',
      '#default_value'=>$sid?'Обновить':'Добавить',
    );
    return $form;
}
function kolesa_pricer_add_shipper_stock_submit($form,&$form_state){
    if (!isset($form_state['storage']['stocks_count'])) {
        $form_state['storage']['stocks_count'] = 1;
    }
    $form_state['storage']['stocks_count']++;
    $form_state['rebuild'] = TRUE;
}
function kolesa_pricer_add_shipper_stock_ajax($form,&$form_state){

    return $form['stocks_container']['new_stocks'];
}

function kolesa_pricer_shipper_update_types_alter(&$update_types){
    $update_types['custom']='Свой класс обработки';
}
function kolesa_pricer_add_shipper_form_submit($form,&$form_state){
    $values=$form_state['values'];
    if(!empty($values['shipper'])){
        $shipper=$values['shipper'];
    } else {
        $shipper=entity_get_controller('shipper')->create();
    }

    $shipper->name=trim($values['name']);
    $shipper->machine_key=trim($values['machine_key']);
    $shipper->params=$values['params'];
    $shipper->status=1;
    $shipper->b2b=$values['b2b'];
    $shipper_setup=[];
    foreach($values['group'] as $groupName=>$group){
        $group=array_filter($group);
        if(!empty($group)) $shipper_setup['global'][$groupName]=$group;
    }
    foreach($values['brands'] as $catName=>$brands){
        foreach($brands['rows'] as $tid=>$bSetup)
        {
            $bSetup=array_filter($bSetup);
            if(!empty($bSetup)) $shipper_setup['brand'][$tid]=$bSetup;
        }
    }
    $shipper->setup=$shipper_setup;
    $shipper->stocks=array();
    foreach($values['stocks']['rows'] as $stock){
                if(!empty($stock['machine_key']))   $shipper->stocks[]=(object) $stock;
    }
    foreach($values['new_stocks']['stocks']['rows'] as $stock){
        if(!empty($stock['machine_key']))   $shipper->stocks[]=(object) $stock;
    }
    entity_get_controller('shipper')->save($shipper);

   # $shipper=new Shipper();
}

function kolesa_pricer_add_shipper_form_callback($form,&$form_state){
    $form_state['rebuild'] = TRUE;

    return $form['params'];
}
function kolesa_pricer_shipper_list_table(){
    $shippers=entity_load('shipper');
    uksort($shippers,function($a,$b){
        if($a->weight==$b->weight) return 0;
        return $a->weight<$b->weight?-1:1;
    });
    $rows=[];
    $countCats=[];
    foreach($shippers as $id=>$shipper){
        $getProducts=kolesa_pricer_shipper_products($shipper);
        $productsList=[];
        foreach($getProducts as $category=>$products){
            $productsList[]=$category.": ".count($products);
            $countCats[$category]+=count($products);
        }
        $data=array(
            'name'=>'<b>'.$shipper->name.'</b><br/><small>'.$shipper->machine_key.'</small>',
            'products'=>theme('item_list',['items'=>$productsList]),
            'stocks'=>kolesa_pricer_shipper_get_stocks_table($shipper->stocks),
        );
        $rows[$shipper->id]=['data'=>$data];
    }
    $sumCats=[];

    foreach($countCats as $category=>$count){
        $sumCats[]=$category.": ".$count;
    }

    $rows[]=['Итого',theme('item_list',['items'=>$sumCats]),''];
    $header=['Название','Товары','Склады'];
    return theme('table',['rows'=>$rows,'header'=>$header]);
}

function kolesa_pricer_shipper_list_form($form,&$form_state){
    $form['add_shipper']=array(

    );
    $form['table']=array(
        '#id'=>'shipper-list-table',
        '#theme'=>'kolesa_pricer_form_table',
        '#header'=>array(
            'key'=>'Key','name'=>'Название','stocks'=>'Склады','edit'=>'Управление','update'=>'Обновление','weight'=>'Вес'
        ),
        'rows'=>array(
          '#tree'=>TRUE,
        ),
    );
    $form['table']['rows']=array(
        '#tree'=>TRUE,
    );

    $shippers=entity_load('shipper');

    uasort($shippers,function($a,$b){

       if($a->weight==$b->weight) return 0;
       return $a->weight<$b->weight?-1:1;
    });

    foreach($shippers as $id=>$shipper){
        $links=array(
            l('Протоколы загрузки','admin/price/reports/load_protocols/'.$id),
            l('Настройка','admin/price/shipper/'.$id.'/edit'),
            l('Остатки','admin/price/shipper/'.$id.'/pricelist'),
          #  l('Импорт номенклатуры','admin/price/shipper/'.$id.'/products_update'),
          #  l('Импорт цен','admin/price/shipper/'.$id.'/price_update'),
        );
        $operations=array();
        if($shipper->params['mrc']['mrc_enable']){
            $operations['mrc_update']=array(
              '#type'=>'submit',
                '#name'=>'mrc-update-'.$shipper->machine_key,
                '#value'=>t('Update MRC'),
                '#shipper'=>$shipper,
                '#submit'=>array('kolesa_pricer_shipper_mrc_update_submit'),
            );
        }
        $operations['update_source']=array(
            '#type'=>'submit',
            '#name'=>'update-source-'.$shipper->machine_key,
            '#value'=>t('Update source'),
            '#shipper'=>$shipper,
            '#submit'=>array('kolesa_pricer_shipper_source_update_submit'),
        );
        if(!empty($shipper->allowedPrices)){
            foreach($shipper->allowedPrices as $key=>$options){
                $links[]=l(t('View '.$key),'admin/price/shipper/'.$id.'/'.$key.'/view',array('attributes'=>array('target'=>'_blank')));
           #     $links[]=l(t('Excel '.$key),'admin/price/shipper/'.$id.'/'.$key.'/excel',array('attributes'=>array('target'=>'_blank')));
                $operations[$key.'_batch_update']=array(
                    '#type'=>'submit',
                    '#name'=>$key.'-update-submit-'.$shipper->machine_key,
                    '#value'=>t('Update '.$key),
                    '#shipper'=>$shipper,
                    '#option'=>$key,
                    '#submit'=>array('kolesa_pricer_shipper_batch_update_submit'),
                );
            }
        }
        $data=array(
            'key'=>array('#markup'=>$shipper->machine_key),
            'name'=>array('#markup'=>$shipper->name),
            'stocks'=>array('#markup'=>kolesa_pricer_shipper_get_stocks_table($shipper->stocks)),
            'edit'=>array('#markup'=>theme('item_list',array('items'=>$links))),
            'operations'=>array(
                '#type'=>'container',
                '#attributes'=>array('class'=>array('shipper-ops')),
                'operations'=>$operations,
            ),
            'weight' => array(
                '#type' => 'weight',
                '#title' => t('Weight'),
                '#default_value' => $shipper->weight,
                '#delta' => 20,
            ),
        );
        $form['table']['rows'][$shipper->id]=$data;

    }
    $form['submit']=array(
      '#type'=>'submit',
        '#value'=>'Обновить всё',
    );

    return $form;
}

function kolesa_pricer_shipper_source_update_submit($form,&$form_state)
{
    $trigger = $form_state['triggering_element'];
    $shipper = $trigger['#shipper'];

    kolesa_pricer_update_shipper_source($shipper,TRUE);

   drupal_set_message($shipper->name.' обновил исходники');
}
function kolesa_pricer_shipper_list_form_submit($form,&$form_state){


    foreach($form_state['values']['rows'] as $id=>$values){
        $shipper=reset(entity_load('shipper',array($id)));
        $shipper->weight=$values['weight'];
        kolesa_pricer_shipper_save($shipper);
    }

}

function kolesa_pricer_shipper_get_data($shipper,$option,$filters=[]){

    $params=$shipper->params;
    $update_type=$params['update type'];
    $price=[];
    if($update_type=='custom'){
        $controller_class=$params['controller class'];

        if(!empty($controller_class)&&class_exists($controller_class)){
            $updater=new $controller_class;
            $updater_options=$updater->getAllowedPrices()[$option];
            $fileUrl="public://pricer/".$updater_options['file id'].".raw";
            if(file_exists($fileUrl)&&$fileContent=file_get_contents($fileUrl)){
                $updater->setFileFromContent($fileContent);
                $updater->getData($option);
                $price=$updater->parseData();
            } else {

                kolesa_pricer_update_shipper_source($shipper,TRUE);
                return kolesa_pricer_shipper_get_data($shipper,$option,$filters);
            }

            if(!empty($filters)){
                $filtered_price=[];
                foreach($price as $cat=>$rows){
                    foreach($rows as $row){
                        foreach($filters as $f_key=>$f_value){
                            if($row[$f_key]==$f_value) $filter_check=TRUE;
                            else $filter_check=FALSE;
                        }
                        if($filter_check==TRUE) {
                            $filtered_price[$cat][]=$row;
                        }

                    }
                }

                return $filtered_price;
            }
            return $price;

        }
    }
    return NULL;
}
function kolesa_pricer_shipper_update_source($shipper,$price,$reset=TRUE){

    if($reset){
        db_delete('pricer_source')->condition('shipper',$shipper->machine_key)->condition('category',array_keys($price),'IN')->execute();
    }
    $existsName=db_query('select name from {pricer_source} where shipper=:shipper',array(':shipper'=>$shipper->machine_key))->fetchAllKeyed(0,0);

    $fields=[];
    $counts=[];

    foreach($price as $catName=>$rows){
        foreach($rows as $row){
            $counts[$row['Category']]++;
            if(!isset($existsName[$row['Name']])) {
                $fields[] = [
                    'category' => $catName,
                    'shipper' => $shipper->machine_key,
                    'name' => $row['Name'],
                    'brand' => $row['Brand'],
                    'model' => $row['Model'],
                    'row' => serialize($row),
                ];
            }
        }
    }

    $fieldsChunks=array_chunk($fields,500);
    foreach($fieldsChunks as $chunk){
        $query=db_insert("pricer_source");
        $query->fields(['name','shipper','category','brand','model','row']);
        foreach($chunk as $fields) {
            $query->values($fields);
        }
        $query->execute();
    }
    foreach($counts as $catName=>$count){
        db_merge('pricer_source_info')
            ->key([
                'shipper'=>$shipper->machine_key,
                'category'=>$catName])
            ->fields([
                'shipper'=>$shipper->machine_key,
                'category'=>$catName,
                'products_count'=>$count,
                'updated'=>date('Y-m-d H:i:s'),
            ])->execute();
    }

    // Update map for mapping
    db_query("INSERT INTO {pricer_mapping_models} (shipper,category,brand,model) 
SELECT DISTINCT s.shipper, s.category,s.brand,s.model FROM {pricer_source} s WHERE NOT EXISTS (SELECT * FROM {pricer_mapping_models} mm
      WHERE mm.shipper=s.shipper AND mm.category=s.category AND mm.brand=s.brand AND mm.model=s.model)  AND s.shipper=:shipper",[':shipper'=>$shipper->machine_key]);

    db_query("INSERT INTO {pricer_mapping_brands} (shipper,category,brand)
SELECT DISTINCT s.shipper, s.category,s.brand FROM {pricer_source} s
WHERE NOT EXISTS (SELECT * FROM {pricer_mapping_brands} mm
      WHERE mm.shipper=s.shipper AND mm.category=s.category AND mm.brand=s.brand)  AND s.shipper=:shipper",[':shipper'=>$shipper->machine_key]);

}

function kolesa_pricer_shipper_get_source($options){
    $query=db_select('pricer_source','s');
    $query->fields('s');
    if(isset($options['shipper'])) $query->condition('shipper',$options['shipper']);
    if(isset($options['category'])) $query->condition('category',$options['category']);
    if(isset($options['brand'])) $query->condition('brand',$options['brand']);
    if(isset($options['limit'])) $query->range(0,$options['limit']);
    $result=$query->execute();
    foreach($result as $row){
        $out[]=unserialize($row->row);
    }
    return $out;
}

function kolesa_pricer_get_unique_fields($rows){
    $ignored=['GTIN','Brand','Model','Name','VendorCode','ShipperCode','IndexN','IndexS','Width','Height','Diametr','Stocks','Image'];

    $unique=[];
    foreach($rows as $row){
        foreach($row as $key=>$el) {
            if(in_array($key,$ignored)) continue;
            if (is_array($unique[$key])&&!in_array($el, $unique[$key])&&!empty($el)) $unique[$key][] = $el;
        }
    }
    return $unique;
}

function kolesa_pricer_shipper_view_data($shipperId,$type){

    $shipper=reset(entity_load('shipper',array($shipperId)));
    $filters=[];
    if(isset($_GET['cae'])) $filters['VendorCode']=$_GET['cae'];
    if(isset($_GET['brand'])) $filters['Brand']=$_GET['brand'];


    $data=kolesa_pricer_shipper_get_data($shipper,$type,$filters);

    drupal_set_title('Загрузка данных для '.$shipper->name.': '.$type);
    drupal_add_css('table.price-table tr td{vertical-align:top};', 'inline');


    foreach($data as $catName=>$rows) {
        $countRows = count($rows);
        $header=array_keys(reset($rows));
        $limit=!empty($filters)?0:100;
        $uniqueFields=kolesa_pricer_get_unique_fields($rows);
        $uniqueFieldsRows=[];
        foreach($uniqueFields as $key=>$fields){
            $uniqueFieldsRows[]=[$key,implode(", ",$fields)];
        }
        $uniqueFieldsTable=theme('table',array('rows'=>$uniqueFieldsRows));

        if($limit) {
            $page = $_GET['page'] ? $_GET['page'] : 0;
            pager_default_initialize($countRows, $limit);
            $rows = array_slice($rows, $limit * $page, $limit);
        }

        $tableRows=[];
        foreach($rows as &$row){

            array_walk($row,function(&$item,$key){
                if(is_array($item)) $item=theme('table',array('rows'=>$item));
                elseif(is_object($item)) {
                    switch ($key) {
                        case 'ModelNode':
                            $item=l($item->nid.": ".$item->title,"node/".$item->nid);
                            break;
                        case 'BrandTerm':
                            $item=l($item->tid.": ".$item->name,"taxonomy/term/".$item->tid);
                            break;
                        case 'ColorTerm':
                            $item=$item->tid.": ".$item->name;
                            break;
                        case 'Product':
                            $item=$item->name;
                            break;
                        default:
                            $item='Object...';
                            break;
                    }
                } else {
                    switch($key){
                        case 'Image':
                            $item="<img src='".$item."' style='width:50px;height:auto;'/>";
                            break;
                    }
                }

            });
            $tableRows[]=$row;
          #  if(empty($cae)||(!empty($cae)&&$row['VendorCode']==$cae)) $tableRows[]=$row;
          #  if(empty($brand)||(!empty($brand)&&$row['Brand']==$brand)) $tableRows[]=$row;
        }


        $build[$catName]['#prefix']="<h3>".$catName."</h3>";
        $build[$catName]['count']['#markup']="Загружено: ".$countRows."<hr/>";
        $build[$catName]['fields']['#markup']="Поля: ".$uniqueFieldsTable."<hr/>";
        $build[$catName]['rows']['#markup'] = theme('table', array('rows' => $tableRows, 'header' => $header, 'attributes' => array('class' => array('price-table'))));
    }
    $build['pager']['#markup']=theme('pager');
    return $build;
}


function kolesa_pricer_shipper_excel_data($shipperId,$type){
    $shipper=current(entity_load('shipper',array($shipperId)));
    $data=kolesa_pricer_shipper_get_data($shipper,$type);
    foreach($data as $catName=>$rows) {
        $f = fopen('public://pricer_export/'.$shipper->name.'_'.$catName.'.csv', 'w');
        $csv=[];
        $header=array_keys(reset($rows));
        $csv[]= fputcsv($f, $header, ";");
        foreach($rows as &$row){
            array_walk($row,function(&$item,$key){
                if(is_array($item)) $item="Array...";
                elseif(is_object($item)) {
                    switch ($key) {
                        case 'ModelNode':
                            $item=$item->title;
                            #$item=l($item->nid.": ".$item->title,"node/".$item->nid);
                            break;
                        case 'BrandTerm':
                            $item=$item->name;
                            #$item=l($item->tid.": ".$item->name,"taxonomy/term/".$item->tid);
                            break;
                        case 'ColorTerm':
                            $item=$item->name;
                            #$item=$item->tid.": ".$item->name;
                            break;
                        case 'Product':
                            $item=$item->name;
                            break;
                        default:
                            $item='Object...';
                            break;
                    }
                } else {
                    switch($key){
                        case 'Image':

                            #$item="<img src='".$item."' style='width:50px;height:auto;'/>";
                            break;
                    }
                }
            });
            $csv[]= fputcsv($f, $row, ";");
        }

        fclose($f);
    }
}


function kolesa_pricer_shipper_mrc_update_submit($form,&$form_state){
    $trigger=$form_state['triggering_element'];
    $shipper=$trigger['#shipper'];
    if($shipper->params['mrc']['update type']=='custom'){
        $class=$shipper->params['mrc']['controller class'];

        if(class_exists($class)){
            $controller=new $class;
            $mrcData=$controller->getMRC();
            $controller->updateMRC();
        }
    }


}
function kolesa_pricer_shipper_batch_update_submit($form,&$form_state){
    $trigger=$form_state['triggering_element'];
    $shipper=$trigger['#shipper'];


    $option=$trigger['#option'];
    $operations=kolesa_pricer_shipper_batch_get_operations($shipper,$option,$fromData=TRUE);

    if(!empty($operations)) {
        $batch = array(
            'title' => 'Обновляем ' . $shipper->name.' - '.$option,
            'init_message' => 'Начали процесс',
            'error_message' => t('Batch has encountered an error.'),
            'finished' => 'kolesa_pricer_shipper_batch_finished',
            'file' => drupal_get_path('module', 'kolesa_pricer') . '/kolesa_pricer.shipper.inc',
            'operations' => $operations,
        );
        batch_set($batch);
    } else {
        drupal_set_message('Пул операций пуст!');
    }
}

function kolesa_pricer_shipper_batch_get_operations($shipper,$option,$fromData=FALSE){
    $price=[];

    if($fromData==TRUE) {
        $getData=kolesa_pricer_shipper_get_data($shipper,$option);
        foreach($getData as $catName=>$rows){
            foreach($rows as $row) {
                $price[]=$row;
            }

        }
    }
    else {
        $query = db_query("select row from {pricer_process_products} where shipper=:shipper and option=:option", [':shipper' => $shipper->machine_key, ':option' => $option]);
        foreach ($query as $row) {
            $price[] = unserialize($row->row);
        }
    }

    $controller=new $shipper->params['controller class'];
    $importOptions=array(
        'shipper'=>$shipper,
        'controller'=>$controller,
        'option'=>$option,
    );
    $operations[]=array('kolesa_pricer_shipper_batch_start',array($importOptions));
    if($option=='products'){
        $importOptions['override']=['update'=>['products'=>TRUE]];
    }
    $options=$controller->getAllowedPrices()[$option];
    if(!empty($options['type'])){
        $option=$options['type'];
        $importOptions['option']=$option;
    }

    $priceChunk=array_chunk($price,20);
    foreach($priceChunk as $no=>$chunk){
        $operations[]=array('kolesa_pricer_shipper_batch_process',array($chunk,$importOptions));
    }
    $operations[]=array('kolesa_pricer_shipper_batch_finish',array($importOptions));

    return $operations;
}

function kolesa_pricer_shipper_batch_start($importOptions,&$context){
    $controller=$importOptions['controller'];
 #   $options=$controller->getAllowedPrices()[$importOptions['option']];
    KolesaPricerLog::cleanPositionLog($importOptions['shipper']->machine_key);

    if($importOptions['option']=='price') $controller->updateStocks_before($importOptions['option']);
    $context['message']="Подготовка к обновлению";
}
function kolesa_pricer_shipper_batch_finish($importOptions,&$context){
    $controller=$importOptions['controller'];
  #  $options=$controller->getAllowedPrices()[$importOptions['option']];

    if($importOptions['option']=='price') $controller->updateStocks_after($importOptions['option']);
    $context['message']="Завершение обновления";
}
function kolesa_pricer_shipper_batch_process($rows,$importOptions,&$context){
    #$process=kolesa_pricer_import_price_process($rows,$importOptions);
    $controller=$importOptions['controller'];
    if(!empty($importOptions['override'])) $controller->setOptions($importOptions['option'],$importOptions['override']);

    $processed_rows=$controller->updateData($importOptions['option'],$rows);

    $context['message']=theme('table',array('rows'=>$processed_rows,'caption'=>'LOG'));
    $context['finished']=1;
}



function kolesa_pricer_shipper_batch_finished($success, $results, $operations){
    if ($success) {
        // Here we could do something meaningful with the results.
        // We just display the number of nodes we processed...
        if($results) drupal_set_message(t('Обновлено позиций: %count', array('%count' => $results)));
    }
    else {
        // An error occurred.
        // $operations contains the operations that remained unprocessed.
        $error_operation = reset($operations);
        drupal_set_message(t('An error occurred while processing @operation with arguments : @args', array('@operation' => $error_operation[0], '@args' => print_r($error_operation[0], TRUE))));
    }
}

function kolesa_pricer_shipper_get_stocks_table($stocks){
    $rows=array();
    $all_count=[];
    foreach($stocks as $id=>$stock){
        $categories=$stock->categories;
        $caeCount="";
        $links="";
        foreach($categories as $catName=>$caeCatCount){
            $caeCount.=$catName.": ".$caeCatCount."<br/>";
            $all_count[$catName]+=$caeCatCount;
            $links.=l('Ссылка на '.$catName,'export_price/'.$id.'/'.$catName.'/marketmixer')."&nbsp;&nbsp;";
            $links.=l("♥",'admin/price/'.$id.'/'.$catName.'/marketmixer/view')."<br/>";
        }
        #$count_cae=db_query("select count(cae) from {pricer_price} where  stock=:stock and qty>0",array(":stock"=>$id))->fetchField();

        $rows[]=array($stock->id.'.'.$stock->name,$stock->delivery_days,$stock->description,$caeCount,$links);
    }
    $allCount="";
    foreach($all_count as $catName=>$allCatCount){
        $allCount.=$catName.": ".$allCatCount."<br/>";
    }
    $rows[]=array(array('data'=>"<b>Всего:</b><br/>".$allCount,'colspan'=>4,'align'=>'right'),'');
    $header=array('Название','Срок доставки','Описание','Кол-во позиций','Ссылки на прайсы в CSV');
    $table=theme('table',array('rows'=>$rows,'header'=>$header,'sticky'=>FALSE,'attributes'=>array('class'=>array('stock-list-table'))));
    return $table;
}

function kolesa_pricer_add_stocks_form_submit($form,&$form_state){
    $values=$form_state['values'];

    $shipper=kolesa_pricer_shipper_load($values['sid']);
    $shipper->stocks[]=array(
        'sid'=>$values['sid'],
        'name'=>$values['name'],
        'description'=>$values['description'],
        'delivery_days'=>$values['delivery_days'],
    );
    entity_get_controller('shipper')->save($shipper);
}


function kolesa_pricer_shipper_save(&$entity){
    return entity_get_controller('shipper')->save($entity);
}
function kolesa_pricer_shipper_load_multiple($ids = FALSE, $conditions = array(), $reset = FALSE) {

    return entity_load('shipper', $ids, $conditions, $reset);
}




function kolesa_pricer_shipper_price_list($shipper_id){
    require_once('kolesa_pricer.reports.inc');
    $shipper=reset(entity_load('shipper',array($shipper_id)));
    drupal_set_title('Остатки для поставщика '.$shipper->name);


    $tableRows=[];
    $errorRows=[];
    $no=0;
    $header=[
        'Поставщик',
        'Категория',
        'CAE',
        'Название',
        'Склады',
        'Ошибки',
    ];
    foreach($shipper->allowedPrices as $priceKey=>$priceOption){
        if($priceKey=='price'||$priceOption['queued']=='price'){
            $curData = kolesa_pricer_shipper_get_data($shipper, $priceKey);
            foreach($curData as $catName=>$catPrice){
                foreach($catPrice as $row){
                    $no++;
                    $stocksInfo='';
                    if(is_array($row['Stocks'])){

                        foreach($row['Stocks'] as $stockKey=>$stockInfo){
                            $stocksInfo.=$stockKey.": ".$stockInfo['qty'].": ".$stockInfo['cost']."<br/>";
                        }
                    }
                    $table_row=[
                      'Поставщик'=>$row['Shipper'],
                        'Категория'=>$row['Category'],
                        'CAE'=>$row['VendorCode'],
                        'Название'=>$row['Name'],
                        'Склады'=>$stocksInfo,
                        'Ошибки'=>'',
                    ];
                   # $row['Stocks']=kolesa_pricer_shipper_get_stocks_table($row['Stocks'])
                    if($check=_pricer_analyze_row($row)){
                        $table_row['Ошибки']=$check;
                        $tableRows[]=['class'=>['error'],'data'=>$table_row];
                        $errorRows[]=['class'=>['error'],'data'=>$table_row+['row'=>print_r($row,TRUE)]];
                    } else {
                        $tableRows[]= $table_row;
                    }
                }

            }
        }
    }





    $build=array();
    $info=[
        "Ошибок: ".count($errorRows),
        "Всего позиций: ".count($tableRows),
    ];
    $build['info']['#markup']=theme('item_list',['items'=>$info]).'<hr/>';
    $build['errors']['#prefix']="<h2>Ошибки</h2>";
    $build['errors']['#markup']=theme('table',array('rows'=>$errorRows,'header'=>$header));
    $build['products']['#prefix']="<h2>Прайс-лист</h2>";
    $build['products']['#markup']=theme('table',array('rows'=>$tableRows,'header'=>$header));
    $build['pager']['#markup']=theme('pager');

    return $build;
}

function _kolesa_pricer_shipper_get_price($shipper_id,$stock_id,$category){

    $shipper=reset(entity_load('shipper',array($shipper_id)));

    foreach($shipper->allowedPrices as $priceKey=>$priceOption) {

        if ($priceKey == 'price' || $priceOption['queued'] == 'price') {

            $curData = kolesa_pricer_shipper_get_data($shipper, $priceKey);

            foreach ($curData as $catName => $catPrice) {

                foreach($catPrice as $row) {
                    if (is_array($row['Stocks'])) {
                        foreach ($row['Stocks'] as $stockInfo) {
                            $cur_row=$row;
                            unset($cur_row['Stocks']);
                            $cur_row+=$stockInfo;
                            $price[$stockInfo['stock_id']][$catName][]=$cur_row;
                        }
                    }
                }
            }
        }
    }

    if(isset($price[$stock_id][$category])) return $price[$stock_id][$category];
    else return FALSE;

}

function kolesa_pricer_stock_load($id){
    $entity=current(entity_load('shipper_stock',array($id)));
    $entity->shipper=current(entity_load('shipper',array($entity->sid)));
    return $entity;
}


function kolesa_pricer_shipper_products($shipper){
    $stocks=$shipper->stocks;

    $options=['stocks'=>array_keys($stocks)];
    $getProducts=kolesa_pricer_get_products_from_options($options);
    $outProducts=[];
    foreach($getProducts as $id=>$product){
        $outProducts[$product->category][$id]=$product;
    }
    return $outProducts;




}