<?php

function kolesa_pricer_sync_check_page($type){
    $sync_count=0;
    $del_count=0;
    $add_count=0;
    $newMode=FALSE;
    switch($type){
        case NULL:
            $type='shina';
            break;
        case 'newshina':
            $type='shina';
            $newMode=TRUE;
            break;
        case 'newdisk':
            $type='disk';
            $newMode=TRUE;
            break;
    }


    $new_models=_kolesa_pricer_sync_get_models_with_addon();
    $old_models=_kolesa_pricer_sync_get_old_models();

    $cur_new_models=$new_models[$type];
    $cur_old_models=$old_models[$type];
    $cur_old_models_keys=array_keys($cur_old_models);
    $cur_new_models_keys=array_keys($cur_new_models);
    $old_images=_kolesa_pricer_sync_get_old_images();
    $cur_old_images=$old_images[$type];

    if($newMode){
        $list=$cur_new_models;
        $check_list=$cur_old_models_keys;
        $header=array('Поставщики','Новая модель','CAE','Новое фото',  'Старая модель','Старое фото');
    } else {
        $list=$cur_old_models;
        $check_list=$cur_new_models_keys;
        $header=array('NID', 'Старая модель', 'Новая модель');
    }

    foreach($list as $model=>$info){
        $find=kolesa_pricer_find_text_in_list($model,$check_list,FALSE,FALSE);
        $find_name="";
        if(!empty($find)){
            $sync_count++;
            if($newMode) {
                $id=$cur_old_models[$find]['nid'];
                $find_name=$id.":".$find;
                $sources="<small>".implode("<br/>",$info['sources'])."</small>";
            }
            else {
                $id=$info['nid'];
                $find_name=$find;
            }
        } else {
            $id=NULL;
            $del_count++;
        }
        if($newMode) {
            $img="";
            foreach($info['images'] as $url) {
                $img.= "<IMG SRC='" . $url . "' width=80px/>";
            }
            $old_img="";
            if(!empty($cur_old_images[$find])){
                foreach($cur_old_images[$find] as $url){
                    $old_img.=theme('image_style',array('path'=>$url,'style_name'=>'tyres_micro'));
                }
            }
            $cae=implode(", ",$info['cae']);
            $rows[]=array($sources,$model,$cae,$img,$find_name,$old_img);
        }
        else $rows[]=array($id,$model,$find_name);
    }

    $build=array();
    if($newMode){
        $stat_rows = array(
            array('Всего старых моделей:', count($cur_old_models)),
            array('Всего новых моделей:', count($cur_new_models)),
            array('Сохранится моделей:', $sync_count),
            array('Исчезнет моделей:', count($cur_old_models)-$sync_count),
            array('Добавится моделей:', count($list) - $sync_count),
        );
    } else {
        $stat_rows = array(
            array('Всего старых моделей:', count($cur_old_models)),
            array('Всего новых моделей:', count($cur_new_models)),
            array('Сохранится моделей:', $sync_count),
            array('Исчезнет моделей:', $del_count),
            array('Добавится моделей:', (count($list) - $sync_count)),
        );
    }

    $build['stat']['#markup'] = theme('table', array('rows' => $stat_rows));
    if($newMode){
        $build['table']['#markup'] = theme('table', array('rows' => $rows, 'header' => $header));
    } else {
        $build['table']['#markup'] = theme('table', array('rows' => $rows, 'header' => $header));
    }
    return $build;
}
function _kolesa_pricer_sync_get_old_images(){
    static $images;
    if(empty($images)) {
        $query = db_query("SELECT  fi.bundle,n.title, fi.entity_id, fm.uri FROM {field_data_field_image} fi
LEFT JOIN {file_managed} fm ON fm.fid=fi.field_image_fid
LEFT JOIN {node} n ON n.nid=fi.entity_id
WHERE fi.entity_type='node' AND NOT ISNULL(n.title) AND fi.bundle IN ('shina','disk') AND NOT ISNULL(fm.uri)");
        foreach ($query as $row) {
            $images[$row->bundle][$row->title][] = $row->uri;
        }
    }
    return $images;
}
function _kolesa_pricer_sync_get_old_models(){
    $query=db_query("select * from {shop_old_models} order by title ASC");
    foreach($query as $row){
        $models[$row->type][$row->title]=array('name'=>$row->title,'nid'=>$row->nid);
    }
    return $models;
}

function kolesa_pricer_sync_image_form($form,&$form_state){
    $form['mode']=array(
      '#type'=>'radios',
        '#options'=>array('diskoptim'=>'Обновление фото с ДискОптим','sh_models'=>'Обновление фото моделей шин','d_models'=>'Обновление фото моделей дисков','d_razmers'=>'Обновление фото дисков по цветам'),
        '#default_value'=>'diskoptim',
    );
    $form['submit']=array(
      '#type'=>'submit',
        '#value'=>'Запустить обновление',
    );

    return $form;

}

function kolesa_pricer_sync_image_form_submit($form,&$form_state){
    $values=$form_state['values'];
    $mode=$values['mode'];
    $d_models=db_query("select distinct nid,title from {node} 
RIGHT JOIN {shop_disk_list} l ON l.mid=node.nid
where node.type='disk'")->fetchAllKeyed(0,1);
    if($mode=='diskoptim'){

        $models=kolesa_pricer_sync_preprocess_models_list('Replay',$d_models);

        $rows=db_query("select id from {diskoptim}")->fetchCol();
        $chunks=array_chunk($rows,50);
        foreach($chunks as $chunk){
            $operations[]=array('kolesa_pricer_sync_image_process_batch_diskoptim',array($chunk,$models));
        }
    }
    if($mode=='d_razmers'){
        $rows=db_query("select * from {shop_disk_info} order by brand,model")->fetchAll(PDO::FETCH_ASSOC);
        $chunks=array_chunk($rows,50);
        foreach($chunks as $chunk){
            $operations[]=array('kolesa_pricer_sync_image_process_batch',array($chunk,'disk',$d_models));
        }
    }

    if($mode=='d_models'){
        $rows=db_query("SELECT mid,color, image  FROM {shop_disk_list} WHERE not ISNULL(image) and not ISNULL(color) GROUP BY `mid`,color")->fetchAll(PDO::FETCH_ASSOC);
        foreach($rows as $row){

            $images[$row['mid']][$row['color']]=$row['image'];
        }

        $chunks=array_chunk($images,20,TRUE);
        foreach($chunks as $chunk){
            $operations[]=array('kolesa_pricer_sync_image_process_batch_models',array($chunk));
        }
    }

    if($mode=='sh_models'){
        $rows=db_query("SELECT mid,image  FROM {shop_shina_info} WHERE not ISNULL(image) and not ISNULL(mid)")->fetchAll(PDO::FETCH_ASSOC);
        foreach($rows as $row){
            $images[$row['mid']][]=$row['image'];
        }

        $chunks=array_chunk($images,20,TRUE);
        foreach($chunks as $chunk){
            $operations[]=array('kolesa_pricer_sync_image_process_batch_models',array($chunk));
        }
    }
/*
    $sh_models=db_query("select distinct nid,title from {node} 
RIGHT JOIN {shop_shina_list} l ON l.mid=node.nid
where node.type='shina' ")->fetchAllKeyed(0,1);
    $d_models=db_query("select distinct nid,title from {node} 
RIGHT JOIN {shop_disk_list} l ON l.mid=node.nid
where node.type='disk'")->fetchAllKeyed(0,1);

    $rows=db_query("select * from {shop_shina_info} order by brand,model")->fetchAll(PDO::FETCH_ASSOC);
    foreach($rows as $row){
        $sh_rows_models[$row['brand'].' '.$row['model']][]=$row;
    }

*/
    /*
    $chunks=array_chunk($rows,50);
    foreach($chunks as $chunk){
        $operations[]=array('kolesa_pricer_sync_image_process_batch',array($chunk,'shina',$sh_models));
    }
    */
/*

*/
    if(!empty($operations)) {
        $batch = array(
            'title' => 'Обновляем изображения',
            'init_message' => 'Начали процесс',
            'error_message' => t('Batch has encountered an error.'),
            'finished' => 'kolesa_pricer_shipper_batch_finished',
            'file' => drupal_get_path('module', 'kolesa_pricer') . '/kolesa_pricer.sync.inc',
            'operations' => $operations,
        );
        batch_set($batch);
    }
    #kolesa_pricer_sync_image_batch_setup();
}
function kolesa_pricer_sync_image_process_batch_models($rows,&$context){
    foreach($rows as $mid=>$images){
        $node=node_load($mid);
        $type=$node->type;
        $field=array();
        if($type=='disk') {
            foreach ($images as $color => $fid) {
                $color = taxonomy_term_load($color);
                $field[] = array(
                    'fid' => $fid,
                    'alt' => $node->title . " " . $color->name,
                    'title' => $node->title . " " . $color->name,
                );
            }
        } elseif($type=='shina'){
            foreach ($images as $image) {
                $options=array(
                  'name'=>$node->title,
                );
                $field[]=kolesa_pricer_sync_add_image($image,'shina',$options);
            }

        }
        if(!empty($field)) {
            $context['message'].="Обновляем изображения для ".$node->title."<br/>";
            $node->field_image[und] = $field;
        }
        node_save($node);
    }
}
function kolesa_pricer_sync_image_process_batch_diskoptim($rows,$models,&$context){

    foreach($rows as $id){
        $row=db_query("select * from {diskoptim} where id=:id",array(":id"=>$id))->fetchAssoc(PDO::FETCH_ASSOC);
        if(strpos($row['brand'],"REPLAY")!==false){
            $isReplica=TRUE;
        }

        if(empty($row['bid'])){
            $brands=kolesa_pricer_get_brands(3);
            $bid=kolesa_pricer_find_text_in_list($row['brand'],$brands);
            if(!empty($bid)){
                db_update("diskoptim")->fields(array("bid"=>$bid))->condition('id',$id)->execute();
            } else {
                if($isReplica==TRUE){
                    $term = new stdClass();
                    $term->name = $row['brand'];
                    $term->vid = 3;
                    $term->parent=27995;
                    taxonomy_term_save($term);
                    $bid=$term->tid;
                    db_update("diskoptim")->fields(array("bid"=>$bid))->condition('brand',$row['brand'])->execute();
                }
            }
        } else {
            $bid=$row['bid'];
        }

        if(empty($row['mid'])){
            if($isReplica==TRUE) {
                $name = 'Replay ' . $row['model'];
            } else $name = $row['brand'] . ' ' . $row['model'];

            $mid=kolesa_pricer_find_text_in_list($name,$models);
            if(!empty($mid)){
                db_update("diskoptim")->fields(array("mid"=>$mid))->condition('model',$row['model'])->execute();
            }

        } else {
            $mid=$row['mid'];
        }

        if(empty($row['cid'])){
            $colors=kolesa_pricer_get_brands(8);
            $cid = kolesa_pricer_find_text_in_list($row['color'], $colors);
            if(!empty($cid)){
                db_update("diskoptim")->fields(array("cid"=>$cid))->condition('id',$id)->execute();
            }
        } else $cid=$row['cid'];

        if(!empty($bid)&&!empty($mid)&&!empty($cid)){
            if(!empty($row['image'])) {
                $options = array(
                    'mid' => $mid,
                    'color' => $cid,
                    'name' => $row['brand'] . ' ' . $row['model'] . ' ' . $row['color'],
                );
                $image_field = kolesa_pricer_sync_add_image($row['image'], 'disk', $options);
            }
        }

        if(!empty($mid)&&!empty($bid)&&$isReplica){
            db_update("shop_disk_list")->condition('mid',$mid)->fields(array('replica'=>$bid))->execute();
        }

        $context['message'].="Обновляем ".$row['model']."<br/>";
    }
}

function kolesa_pricer_sync_image_batch_setup(){
    $query=db_query("select * from {shop_products} where not isnull(addon) group by type,cae");
    foreach($query as $row){
        if ($row->type == 'shina') $shina[] = $row;
        elseif ($row->type == 'disk') $disk[] = $row;
    }
    $shina_chunk=array_chunk($shina,100);
    $disk_chunk=array_chunk($disk,100);
    foreach($shina_chunk as $chunk){
        $operations[]=array('kolesa_pricer_sync_image_batch',array($chunk,'shina'));
    }
    foreach($disk_chunk as $chunk){
        $operations[]=array('kolesa_pricer_sync_image_batch',array($chunk,'disk'));
    }
    $batch = array(
        'title' => 'Обновляем ' . $shipper['name'],
        'init_message' => 'Начали процесс',
        'error_message' => t('Batch has encountered an error.'),
        'finished' => 'kolesa_pricer_shipper_batch_finished',
        'file' => drupal_get_path('module', 'kolesa_pricer') . '/kolesa_pricer.sync.inc',
        'operations' => $operations,
    );

    batch_set($batch);
}

function kolesa_pricer_sync_preprocess_model($row,$alternate=FALSE){
    if($alternate){
        $name = $row['brand'];
        $name .= " ";
        if($row['brand']=='LS Wheels') $name.="LS";
        if($row['brand']=='SDT') $row['model']=str_replace('Ü','U',$row['model']);
        $name .= $row['model'];
    } else {
        $name = $row['brand'];
        $name .= " ";
        $name .= $row['model'];
    }
    return $name;
}
function kolesa_pricer_sync_preprocess_models_list($brand,$models){

    if($brand=='Replay'){
        foreach($models as $mid=>&$model){
            $model=preg_replace("[Acura|Audi|BMW|Cadillac|Chery|Chevrolet|Chrysler|Citroen|Datsun|Daewoo|Dongfeng|FAW|Fiat|Ford|Geely|Great Wall|Honda|HAVAL|Hyundai|Infiniti|Jaguar|Jeep|Kia|Land Rover|Lexus|Lifan|Mazda|Mercedes|Mini|Mitsubishi|Nissan|Opel|Peugeot|Porsche|Ravon|Renault|Seat|Skoda|Ssang Yong|Subaru|Suzuki|Toyota|Volkswagen|Volvo|Volkswagen|UAZ]","",$model);
            $model=str_replace("  "," ",$model);
            $model=str_replace("(","",$model);
            $model=str_replace(")","",$model);
        }
    }
    return $models;
}
function kolesa_pricer_sync_image_process_batch($rows,$type,$models,&$context){
    foreach($rows as $row){
        $name=kolesa_pricer_sync_preprocess_model($row);
        $vid=$type=='shina'?2:3;
        $brands=kolesa_pricer_get_brands($vid);
        $colors=kolesa_pricer_get_brands(8);
        $bid=kolesa_pricer_find_text_in_list($row['brand'],$brands);
        $mid=kolesa_pricer_find_text_in_list($name,$models);
        if(empty($mid)){
            $name=kolesa_pricer_sync_preprocess_model($row,TRUE);
            $models=kolesa_pricer_sync_preprocess_models_list($row['brand'],$models);
            $mid=kolesa_pricer_find_text_in_list($name,$models);
        }
        $context['message'].="Смотрим ".$name."<br/>";
        if($type=='disk') {
            $cid = kolesa_pricer_find_text_in_list($row['color'], $colors);
        }
        $fields=array();
        if(!empty($bid)) {
            $fields['bid']=$bid;
        }
        if(!empty($mid)) {
            $fields['mid'] = $mid;
            $node=node_load($mid);

        }
        if(!empty($cid)) {
            $fields['cid']=$cid;
        }

        if(!empty($mid)&&!empty($cid)&&!empty($row['image'])){
            $options=array(
                'mid'=>$mid,
                'color'=>$cid,
                'name'=>$node->title,
            );
            if($type=='disk'){
                $options['name'].=' '.$row['color'];
            }
            $image_field=kolesa_pricer_sync_add_image($row['image'],$type,$options);

        }

        if(!empty($fields)){
            if($type=='shina') $query=db_update('shop_shina_info');
            else  $query=db_update('shop_disk_info');
            $query->fields($fields);
            $query->condition('id',$row['id']);
            $query->execute();
        }

    }
}
function kolesa_pricer_sync_image_batch($rows,$type,&$context){
    foreach($rows as $row){
        $info=kolesa_pricer_full_product_info_from_row($row);
        $table=$type=='shina'?'shop_shina_info':'shop_disk_info';

        if(!empty($info['images'])){
            foreach($info['images'] as $delta=>$image) {
                if($type=='shina') {
                    db_merge("shop_shina_info")->
                        key(array('brand' => $info['brand'],'model'=>$info['model'],'image'=>$image))->
                    fields(array(
                        'brand' => $info['brand'],
                        'model' => $info['model'],
                        'delta' => $info['delta'],
                        'image' => $image
                    ))->execute();
                }
                if($type=='disk') {
                    db_merge("shop_disk_info")->
                    key(array('brand' => $info['brand'],'model'=>$info['model'],'image'=>$image))->
                    fields(array(
                        'brand' => $info['brand'],
                        'model' => $info['model'],
                        'color'=>$info['color'],
                        'image' => $image
                    ))->execute();
                }
            }
            $context['message'].="Добавили изображение для ".$info['brand']." ".$info['model'];
        }


    }


    $context['finished']=1;
}

function _kolesa_pricer_sync_get_empty_image_nodes(){
    $query="SELECT DISTINCT n.nid,n.title,n.type FROM {node} n
WHERE n.type IN ('shina','disk') AND n.nid NOT IN (SELECT im.entity_id FROM {field_data_field_image} im WHERE im.bundle IN ('shina','disk'))";
    $result=db_query($query);
    foreach($result as $row){
        $models[$row->type][$row->nid]=$row->title;
    }
    return $models;
}

function _kolesa_pricer_sync_get_models_with_addon(){
    $query="SELECT * FROM {shop_products} order by name asc";
    $result=db_query($query);

    foreach($result as $row){
        $info=kolesa_pricer_full_product_info_from_row($row);
        $model_name=implode(" ",array($info['brand'],$info['model']));

        if(!isset($models[$row->type][$model_name])){
            $models[$row->type][$model_name] = $info;
        }

        $models[$row->type][$model_name]['cae'][$row->cae]=$row->cae;
        $models[$row->type][$model_name]['sources'][$row->source]=$row->source;
    }

    return $models;
}

function kolesa_pricer_sync_model_page($form,&$form_state,$type){
    if(!empty(arg(4))) $default_brand=arg(4);
    else $default_brand=NULL;

    if($type=='disk') $query=db_select('shop_disk_info','i');
    else $query=db_select('shop_shina_info','i');
    $query->fields('i');
    $query->addExpression('count(i.model)','mcount');
    $query->isNull('mid');
    $query->isNotNull('bid');
    $query->condition('not_found',0);
    $query->orderBy('brand');
    $query->orderBy('model');
    $query->groupBy('model');

    $brand_query=clone $query;
    if($default_brand)    $query->condition('bid',$default_brand);
    $result=$query->execute()->fetchAll();



    $brand_result=$brand_query->execute()->fetchAll();
    $brands=array();
    foreach($brand_result as $row){
        $brands[$row->bid]=$row->brand;
    }

    $header=array('Производитель','Модель','Кол-во вариантов','Модель у нас','Не найдена');

    $limit=50;

    pager_default_initialize(count($result), $limit);

    $page=$_GET['page']?$_GET['page']:0;

    $result=array_slice($result,$limit*$page,$limit);
    $form['filter']=array(
      '#type'=>'fieldset',
        '#title'=>'Фильтр',
        '#weight'=>0,
    );
    $form['filter']['brand']=array(
        '#type'=>'select',
        '#options'=>$brands,
        '#default_value'=>$default_brand,
        '#empty_option'=>'- укажите бренд -',
        '#title'=>'Производитель',
    );
    $form['filter']['find']=array(
        '#type'=>'submit',
        '#submit'=>array('kolesa_pricer_sync_model_page_filter_submit'),
        '#value'=>'Отобрать',
    );
    $form['table']=array(
        '#theme'=>'kolesa_form_table',
        '#header'=>$header,

        'rows'=>array(
            '#tree'=>TRUE,
        ),
    );
    foreach($result as $row){
        $models=kolesa_pricer_sync_get_models_by_brand($type,$row->bid);

        $form['table']['rows'][$row->model]['brand']=array(
            '#type'=>'markup',
            '#markup'=>$row->brand,
        );
        $form['table']['rows'][$row->model]['model']=array(
            '#type'=>'markup',
            '#markup'=>$row->model,
        );
        $form['table']['rows'][$row->model]['mcount']=array(
            '#type'=>'markup',
            '#markup'=>$row->mcount,
        );
        $form['table']['rows'][$row->model]['select_model']=array(
            '#type'=>'select',
            '#options'=>$models,
            '#empty_option'=>'- Укажите модель '.$row->brand." -",
        );
        $form['table']['rows'][$row->model]['not_found']=array(
            '#type'=>'checkbox',
            '#title'=>'Не найдена',

        );

    }
    $form['pager']=array(
        '#type'=>'markup',

        '#markup'=>theme("pager"),
    );
    $form['submit']=array(
      '#type'=>'submit',
        '#value'=>'Сохранить',
    );

    $form['type']=array('#type'=>'value','#value'=>$type);
    return $form;

}
function kolesa_pricer_sync_model_page_filter_submit($form,&$form_state){
    $brand=$form_state['values']['brand'];
    $type=$form_state['values']['type'];

    $url=$type=='disk'?"admin/price/sync/disk_models":"admin/price/sync/shina_models";
    $url.="/".$brand;
    $form_state['redirect']=$url;
}
function kolesa_pricer_sync_model_page_submit($form,&$form_state){
    $values=$form_state['values'];
    $type=$values['type'];
    foreach($values['rows'] as $model=>$info){
        $fields=array();
        if(!empty($info['select_model'])){
            $fields['mid']=$info['select_model'];
        }
        if(!empty($info['not_found'])){
            $fields['not_found']=1;
        }
        if(!empty($fields)){
            if($type=='shina') $query=db_update('shop_shina_info');
            else  $query=db_update('shop_disk_info');
            $query->condition('model',$model);
            $query->fields($fields);
            $query->execute();
        }
    }

}
function kolesa_pricer_sync_get_models_by_brand($type,$bid){
    static $models;
    if(empty($models[$type][$bid])){
        if($type=='disk') $nids=db_query("select entity_id from {field_revision_field_disk_brand} where field_disk_brand_tid=:tid",array(":tid"=>$bid))->fetchCol();
        if($type=='shina') $nids=db_query("select entity_id from {field_revision_field_brand} where field_brand_tid=:tid",array(":tid"=>$bid))->fetchCol();
        $nodes=array();
        if(!empty($nids)) {
            $query = db_select('node', 'n')
                ->fields('n', array('nid', 'title'))
                ->condition('n.nid', $nids, 'IN')
                ->orderBy('title')
                ->execute();
            $nodes = $query->fetchAllKeyed(0, 1);
        }
        $models[$type][$bid] = $nodes;
    }
    return $models[$type][$bid];
}

function kolesa_pricer_sync_add_image($link,$type,$options=array()){
    module_load_include('inc', 'pathauto');
    if($type=='shina') $dir="public://shini";
    else $dir="public://diski";
    $clean_title = pathauto_cleanstring($options['name']);
    $ext = pathinfo($link, PATHINFO_EXTENSION);
    if (empty($ext)) $ext = "jpg";
    $newpath = strtolower($clean_title . "." . $ext);

    $encodedPath = rawurlencode($link);
    $encodedPath = str_replace('%3A', ':', str_replace('%2F', '/', $encodedPath));

    $file_info = system_retrieve_file($encodedPath, $dir . "/" . $newpath, true, FILE_EXISTS_RENAME);
    if ($file_info->fid) {
        $image_field=array(
            'fid'=>$file_info->fid,
            'alt'=>$options['name'],
            'title'=>$options['name'],
        );

        if($type=='disk') {
            $query=db_update("shop_disk_list")->fields(array('image'=>$file_info->fid))->condition('mid',$options['mid'])->condition('color',$options['color']);
            $query->isNull('image');
            $query->execute();

        }



        return $image_field;
    }
}