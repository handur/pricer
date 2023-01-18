<?php

function _kolesa_pricer_helper_get_brands($cat=NULL){
    static $brands;
    if(empty($brands)){
        $query=db_query("select * from {pricer_products_brands} pb WHERE EXISTS (SELECT * FROM {pricer_mapping_brands} mb WHERE mb.id=pb.id)");
        foreach($query as $row){
            $brands[$row->category][$row->id]=$row->name;
        }
    }
    if(!empty($cat)) return $brands[$cat];
    return $brands;
}

function _kolesa_pricer_helper_get_models_by_brand($brand_id){
    static $models;
    if(empty($models[$brand_id])){
        $result=db_query("SELECT id,CONCAT(name,' :',source) name FROM {pricer_products_models}
WHERE brand_id=:brand order by name ASC",array(":brand"=>$brand_id))->fetchAllKeyed(0,1);
        $models[$brand_id]=$result;
    }
    return $models[$brand_id];
}

function kolesa_pricer_mapping_brands_form($form,&$form_state){
    drupal_add_css(drupal_get_path('module','kolesa_pricer').'/kolesa_pricer.css');
    drupal_add_js(drupal_get_path('module','kolesa_pricer').'/kolesa_pricer.js');

    $all_brands=_kolesa_pricer_helper_get_brands();

    $shippers=kolesa_pricer_get_active_shippers();
    foreach($shippers as $shipper) {
        $shipper_options[$shipper->machine_key] = $shipper->name;
    }
    $def=[];
    $allowed=['emptyonly','shipper'];
    foreach($allowed as $key){
        if(isset($_GET[$key])) $def[$key]=$_GET[$key];
    }

    $form['#attributes']=array('class'=>array('mapping-table'));
    $form['filter']=array(
        '#type'=>'container',
        '#title'=>'Фильтр',
        '#tree'=>TRUE,
    );
    $form['filter']['shipper']=array(
        '#type'=>'select',
        '#title'=>'Поставщик',
        '#empty_option'=>'- Все -',
        '#options'=>$shipper_options,
        '#default_value'=>$def['shipper']??$def['shipper'],
    );
    $form['filter']['emptyonly']=array(
        '#type'=>'checkbox',
        '#title'=>'Только пустые',
        '#default_value'=>$def['emptyonly']??$def['emptyonly'],
    );
    $form['filter']['submit']=array(
        '#type'=>'submit',
        '#value'=>'Отобрать',
    );
    $form['mapping']=array(
        '#theme'=>'kolesa_pricer_form_table',
        '#header'=>array(
            'shipper'=>'Поставщик','category'=>'Категория','brand'=>'Бренд у поставщика','tid'=>'Бренд у нас'
        ),
        'rows'=>array(
          '#tree'=>TRUE,
        ),
    );

    $query=db_select('pricer_mapping_brands','m')->fields('m');
    if(!empty($def['shipper'])) $query->condition('shipper',$def['shipper']);
    if(!empty($def['emptyonly'])) $query->isNull('id');

    $query->orderBy('category')->orderBy('brand')->orderBy('shipper');
    $result=$query->execute();

    foreach($result as $row){
        $table_row=array();
        $table_row['shipper']=array(
          '#markup'=>$row->shipper,
        );
        $table_row['category']=array(
            '#markup'=>$row->category,
        );

        $table_row['brand']=array(
            '#markup'=>$row->brand,
        );
        $attributes=array();
        $attributes['data-method']='update-mapping';
        $attributes['data-type']='brands';
        $attributes['data-shipper']=$row->shipper;
        $attributes['data-category']=$row->category;
        $attributes['data-brand']=$row->brand;
        if(empty($row->id)) {
            $attributes['class']=array('warning');
        }
        $brands_options=[];
        if(empty($all_brands[$row->category])){
            foreach($all_brands as $catBrands){
                $brands_options+=$catBrands;
            }
        } else {
            $brands_options=$all_brands[$row->category];
        }
        asort($brands_options);

        foreach($brands_options as $id=>&$brand){
            $models=_kolesa_pricer_helper_get_models_by_brand($id);
            $brand.=" (".count($models).")";
        }
        $table_row['id']=array(
            '#attributes'=>$attributes,
            '#type'=>'select',
            '#empty_option'=>'- Нет -',
            '#options'=>$brands_options,
            '#value'=>$row->id,


        );
        $form['mapping']['rows'][]=$table_row;

    }


    return $form;
}

function kolesa_pricer_mapping_brands_form_submit($form,&$form_state){
    $filterValues=$form_state['values']['filter'];
    $filterValues=array_filter($filterValues);
    unset($filterValues['submit']);
    drupal_goto('admin/price/mapping/brands',['query'=>$filterValues]);
    #   $form_state['rebuild']=TRUE;
}

function kolesa_pricer_mapping_models_form($form,&$form_state){
    drupal_add_css(drupal_get_path('module','kolesa_pricer').'/kolesa_pricer.css');
    drupal_add_js(drupal_get_path('module','kolesa_pricer').'/kolesa_pricer.js');
    $shippers=kolesa_pricer_get_active_shippers();
    foreach($shippers as $shipper){
        $shipper_options[$shipper->machine_key]=$shipper->name;
    }

    $allowed=['emptyonly'=>NULL,'cat'=>'Tyres','brand'=>NULL,'shipper'=>NULL];
    foreach($allowed as $key=>$a_def){
        if(isset($_GET[$key])) $def[$key]=$_GET[$key];
         elseif(!empty($a_def)) $def[$key]=$a_def;
    }

    $query = db_select('pricer_mapping_models', 'm');

    if(empty($def['brand'])) $paging=TRUE;
    else $paging=FALSE;

    $query->fields('m');
    if(!empty($def['cat'])) $query->condition('category',$def['cat']);
    if(!empty($def['shipper'])) $query->condition('shipper',$def['shipper']);
    if(!empty($def['emptyonly'])) $query->isNull('id');
    if(!empty($def['brand'])) {

        $map_brands=db_query("select distinct brand from {pricer_mapping_brands} mm where id=:brand",[":brand"=>$def['brand']])->fetchCol();
        if(!empty($map_brands))  $query->condition('brand',$map_brands,'IN');

    }

    $query->orderBy('brand')->orderBy('model')->orderBy('shipper')->orderBy('category');
    $result=$query->execute()->fetchAll();

    $brands_options=_kolesa_pricer_helper_get_brands($def['cat']);

    asort($brands_options);
    $table_rows=array();

    $form['#attributes']=array('class'=>array('mapping-table'));

    if($paging){
        $limit=50;
        $all_count=count($result);
        $cur_page=$_GET['page'];
        $result=array_slice($result, $limit*$cur_page,$limit);
        pager_default_initialize($all_count,$limit);
        $form['pager']=array(
            '#weight'=>10,
            '#markup'=>theme('pager'),
        );
    }

    foreach($result as $row){
        if(empty($row->shipper)) continue;
        $table_row=array();
        $table_row['shipper']=array(
            '#markup'=>$row->shipper,
        );
        $table_row['category']=array(
            '#markup'=>$row->category,
        );



        $processed_row=['Shipper'=>$row->shipper,'Category'=>$row->category,'Brand'=>$row->brand];

        $findbrand=KolesaFieldMapping::getBrand($processed_row);

        if(!empty($findbrand)) $node_options=_kolesa_pricer_helper_get_models_by_brand($findbrand);
        else $node_options=array();
        $table_row['brand']=array(
            '#markup'=>$row->brand,
        );

        if($findbrand){
            $table_row['brand']['#markup'].="&nbsp;<span class='exist'>есть</span>";
        }

        $table_row['model']=array(
            '#markup'=>$row->model,
        );
        $attributes=array();
        $attributes['data-method']='update-mapping';
        $attributes['data-type']='models';
        $attributes['data-shipper']=$row->shipper;
        $attributes['data-brand']=$row->brand;
        $attributes['data-category']=$row->category;
        $attributes['data-model']=$row->model;
        if(empty($row->id)) {
            $attributes['class']=array('warning');
        }


        $table_row['id']=array(
            '#attributes'=>$attributes,
            '#type'=>'select',
            '#empty_option'=>'- Нет -',
            '#options'=>$node_options,
            '#value'=>$row->id,
        );

        $table_rows[]=$table_row;
    }


    $form['filter']=array(
        '#type'=>'container',
        '#title'=>'Фильтр',
        '#tree'=>TRUE,

    );
    $form['filter']['cat']=array(
      '#type'=>'select',
        '#title'=>'Категория',
        '#options'=>['Tyres'=>'Шины','Moto'=>'Мотошины','Truck'=>'Грузовые шины','Wheels'=>'Диски'],
        '#default_value'=>$def['cat']?$def['cat']:'Tyres',
    );
    $form['filter']['shipper']=array(
        '#type'=>'select',
        '#title'=>'Поставщик',
        '#options'=>$shipper_options,
        '#empty_option'=>'- все -',
        '#default_value'=>$def['shipper']??$def['shipper'],
    );
    $form['filter']['brand']=array(
        '#type'=>'select',
        '#title'=>'Производитель',
        '#options'=>$brands_options,
        '#empty_option'=>'- все -',
        '#default_value'=>$def['brand']??$def['brand'],
    );
    $form['filter']['emptyonly']=array(
        '#type'=>'checkbox',
        '#title'=>'Только пустые',
        '#default_value'=>$def['emptyonly']??$def['emptyonly'],
    );
    $form['filter']['submit']=array(
      '#type'=>'submit',
        '#value'=>'Отобрать',
    );
    $form['mapping']=array(

        '#theme'=>'kolesa_pricer_form_table',
        '#header'=>array(
            'shipper'=>'Поставщик','category'=>'Категория','brand'=>'Бренд','model'=>'Модель у поставщика','nid'=>'Модель у нас'
        ),
        'rows'=>array(
            '#tree'=>TRUE,
        ),
    );



    $form['mapping']['rows']=$table_rows;



    return $form;
}
function kolesa_pricer_mapping_models_form_submit($form,&$form_state){
    $filterValues=$form_state['values']['filter'];
    $filterValues=array_filter($filterValues);
    unset($filterValues['submit']);
    drupal_goto('admin/price/mapping/models',['query'=>$filterValues]);
 #   $form_state['rebuild']=TRUE;
}

function kolesa_pricer_mapping_update_ajax(){
    $data=$_POST;

    $row=array('Shipper'=>$data['shipper'],'Category'=>$data['category']);
    if(isset($data['brand'])) $row['Brand']=$data['brand'];
    if(isset($data['model'])) $row['Model']=$data['model'];
    if(empty($data['id'])) $data['id']=NULL;

    switch($data['type']){
        case 'brands':
            KolesaFieldMapping::updateMappingList('brands',$row,$data['id']);
            break;
        case 'models':
            KolesaFieldMapping::updateMappingList('models',$row,$data['id']);
            break;
    }
    $result=$data;
    return $result;
}