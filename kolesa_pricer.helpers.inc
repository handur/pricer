<?php

function kolesa_pricer_get_params($name){


}

function kolesa_pricer_get_shippers(){

    $shippers=array();
    drupal_alter('pricer_shippers',$shippers);
    foreach($shippers as $shipper=>&$data){
        if($stocks=kolesa_pricer_get_stocks($shipper)){
            $data['stocks']=$stocks;
        }
    }
    return $shippers;
}

function kolesa_pricer_get_shipper($shipper_id){
    $shippers=kolesa_pricer_get_shippers();
    if(isset($shippers[$shipper_id])) return $shippers[$shipper_id];
    else return FALSE;

}

function kolesa_pricer_get_stocks($shipper=NULL){
    $stocks=array();
    drupal_alter('pricer_stocks',$stocks);
    if(!empty($shipper)&&isset($stocks[$shipper])) return $stocks[$shipper];
    else return $stocks;
}
function kolesa_pricer_get_stock($stock,$shipper){
    $stocks=kolesa_pricer_get_stocks($shipper);

    if(isset($stocks[$stock])) return $stocks[$stock];


}
function kolesa_pricer_get_cae_by_source($shipper,$cae=NULL){
    static $products;

    if(!isset($products[$shipper])) {
        $result = db_query("select * from {pricer_products} where source=:source", array(":source" => $shipper))->fetchAllAssoc('cae', PDO::FETCH_ASSOC);
        foreach ($result as $i => $row) {
            $products[$shipper][$i]=kolesa_pricer_get_product_from_row($row);
        }
    }

    if(isset($cae)) return $products[$shipper][$cae];
    else return $products[$shipper];
}
function kolesa_pricer_get_products_from_rows($rows,$parse=TRUE){
    $addon_data=array();
    drupal_alter('addon_data',$addon_data,$rows);
    foreach($rows as $row){
        $addon=isset($addon_data[$row->cae])?$addon_data[$row->cae]:array();
        kolesa_pricer_get_product_from_row($row,$addon,$parse);
    }
}
function kolesa_pricer_full_product_info_from_row($row){
    $addon=unserialize($row->addon);
    $data=unserialize($row->data);


    $info=array();
   # drupal_alter('get_goods_info_by_cae',$info,$row->cae);
    drupal_alter('get_goods_info_from_addon',$info,$addon,$row->source);
    drupal_alter('get_goods_info_from_data',$info,$data,$row->source);
    $brand=reset($info['brand']);
    $model=reset($info['model']);
    return array('brand'=>$brand,'model'=>$model,'images'=>$info['image'],'color'=>$info['color']);

}
function kolesa_pricer_get_product_from_row($row,$addon=NULL,$parse=TRUE){
    if(!is_array($row)) $row=(array)$row;

    $product=array();
    $product['data']=unserialize($row['data']);
    $product['addon']=$addon;
    $product['source']=$row['source'];
    $product['type']=$row['type'];
    if($parse) drupal_alter("product_parse",$product);
    return $product;
}

function kolesa_pricer_get_number_from_string($str){
    return filter_var($str, FILTER_SANITIZE_NUMBER_FLOAT,FILTER_FLAG_ALLOW_FRACTION);
}

function kolesa_pricer_get_brands($vid=NULL){
    static $brands;
    if(empty($brands)) {
        $query = db_query("select tid,vid,LCASE(name) name from {taxonomy_term_data} where vid IN(2,3,8)");
        foreach ($query as $row) {
            $brands[$row->vid][$row->tid]=$row->name;
        }
    }
    if(!empty($vid)) return $brands[$vid];
    return $brands;
}

function kolesa_pricer_get_models($type){
    static $models;

    if(empty($models[$type])){
        $nodes=node_load_multiple(array(),array('type'=>$type));
        foreach($nodes as $node){
            $models[$type][$node->nid]=$node->field_shortname[und][0]['value'];
        }
    }

    return $models[$type];
}
function _pricer_string_prepare(&$str){
    $str=mb_strtolower($str);
    $str=preg_replace('/\s*/', '', $str);
    $str=str_replace("(","",$str);
    $str=str_replace(")","",$str);
    $str=str_replace("-","",$str);
    $str=str_replace("*","",$str);
    $str=str_replace("/","",$str);
}


function kolesa_pricer_find_text_in_list($str, $list, $fuzzy=FALSE, $returnId=TRUE){

    _pricer_string_prepare($str);
    $shortest=-1;
    foreach ($list as $id=>$item) {
        _pricer_string_prepare($item);
        $lev=levenshtein($str, $item);
        if($lev==0){
            $closest=$id;
            $shortest=0;
            break;
        }
        if($fuzzy==TRUE) {
             # поиск среди ближайшего похожего
            if($lev<=$shortest || $shortest<0){
                $closest=$id;
                $shortest=$lev;
            }

        }
    }
    if(!empty($closest)){
        if($returnId) return $closest;
        else return $list[$closest];
    } else return NULL;

}

function kolesa_pricer_create_param($fields){
    $id=db_insert("pricer_sync")->fields($fields)->execute();
    $param=kolesa_pricer_load_param($id);
    return $param;
}
function kolesa_pricer_save_param(&$param){
    if(isset($param['id'])) {
        db_update("pricer_sync")->condition('id',$param['id'])->fields($param)->execute();
    } else {
        $param=kolesa_pricer_create_param($param);
    }
    return $param;
}
function kolesa_pricer_load_param($id){
    $param=db_query("select * from {pricer_sync} where id=:id",array(":id"=>$id))->fetchAssoc();
    return $param;
}

function kolesa_pricer_update_param($param,$options){
    switch($param['param']){
        case 'brand':
            $update_func='kolesa_pricer_update_brand';
            break;
        case 'model':
            $update_func='kolesa_pricer_update_model';
            break;
    }
    if (function_exists($update_func)) {
        $update_func($param, $options);
    } else {
        /* ругаемся на несуществующую функцию обновления параметра */
    }

}

function kolesa_pricer_sync_param(&$param,$options=array()){

    switch($param['param']){
        case 'brand':
            $vid=$param['type']=='shina'?2:3;
            $check_list=kolesa_pricer_get_brands($vid);#vid==2 для шин
            $options['vid']=$vid;
            $create_func='kolesa_pricer_create_brand';
            break;
        case 'model':
            $check_list=kolesa_pricer_get_models($param['type']);
            $create_func='kolesa_pricer_create_model';
            break;

    }

    $find_id=kolesa_pricer_find_text_in_list($param['src_name'],$check_list);


    if(empty($find_id)){
        if(!empty($options['create_param_entity'])) {
            if (function_exists($create_func)) {
                $create_func($param, $options);
                $param['status']='created';
            } else {
                /* ругаемся на несуществующую функцию создания параметра */
            }
        }

    } else {
        $param['status']='load';
        $param['dest_name']=$check_list[$find_id];
        $param['dest_id']=$find_id;
    }

    kolesa_pricer_save_param($param);

    return $param;
}
function kolesa_pricer_check_cae_exist($cae,$type){
    static $caes;
    if(empty($caes[$type])){
        if($type=='shina'){
            $query=db_select("pricer_shina_list","l");
        } else{
            $query=db_select("pricer_disk_list","l");
        }
        $query->fields("l",array("id","cae"));
        $caes[$type]=$query->execute()->fetchAllKeyed(1,0);
    }
    if(isset($caes[$type][$cae])) return TRUE;
    else return FALSE;
}
function kolesa_pricer_save_products($products,$type){


    if($type=='shina') $query=db_insert("pricer_shina_list")->fields(
        array(
            'cae','name','mid','sh','p','d','index_n','index_s','brand','sezon','ship','runflat','type'
        )
    );
    else $query=db_insert("pricer_disk_list")->fields(
        array(
            'cae','name','mid','sh','d','krep','pcd','et','dia','brand','color','replica','tipdiska'
        )
    );

    foreach($products as $product) {
        if(kolesa_pricer_check_cae_exist($product['cae'],$type)) continue;

        $values = array(
            'cae' => $product['cae'],
            'name' => kolesa_pricer_get_name_product($product),
            'mid' => $product['mid'],
            'sh' => $product['sh'],
            'p' => $product['p'],
            'd' => $product['d'],
            'krep'=>$product['krep'],
            'pcd'=>$product['pcd'],
            'et'=>$product['et'],
            'dia'=>$product['dia'],

            'color'=>$product['color'],
            'replica'=>$product['replica'],
            'tipdiska'=>$product['tipdiska'],
            'index_n' => $product['in'],
            'index_s' => $product['is'],
            'brand' => $product['brand'],
            'sezon' => $product['sezon'],
            'ship' => $product['ship'],
            'runflat' => $product['runflat'],
            'type' => $product['ts'],
            'active'=>1,
        );
        $query->values($values);
    }
    $query->execute();
}
function kolesa_pricer_get_name_product($product){
    return $product['name'];
}
function kolesa_pricer_create_brand(&$param,$options){

    $vid=$options['vid'];
    $term = new stdClass();
    $term->name = $param['src_name'];
    $term->vid = $vid;
    taxonomy_term_save($term);
    $param['dest_name']=$term->name;
    $param['dest_id']=$term->tid;
    return $param;
}
function kolesa_pricer_update_brand(&$param,$options){
    $tid=$param['dest_id'];
    $term=taxonomy_term_load($tid);
    return $param;
}
function kolesa_pricer_create_model(&$param,$options,$node=NULL){
    if(empty($node)) $mode='create';
    else $mode='update';

    $product=$options['product'];
    if(isset($product['brand'])){
        $brand=taxonomy_term_load($product['brand']);
    } else {
        return FALSE;
    }
    $vehicle=28422;
    switch($product['ts']){
        case 'car':
            if($product['subtype']=='suv') $vehicle=28418;
            break;
        case 'moto':
            $vehicle=28428;
            break;
        case 'truck':
            $vehicle=28419;
            break;
    }
    if(empty($node)) {
        $node = new stdClass();
        $node->type = $param['type'];
        node_object_prepare($node);
        $node->uid = 0;
        $node->created = strtotime("now");
        $node->changed = strtotime("now");
        $node->status = 1;
        $node->comment = 0;
        $node->promote = 0;
        $node->moderate = 0;
        $node->sticky = 0;
        $node->language = 'und';
    }
    $node->title = $brand->name.' '.$param['src_name'];
    $node->field_shortname[LANGUAGE_NONE][]['value'] = $param['src_name'];

    if($param['type']=='shina') {
        $node->field_brand[LANGUAGE_NONE][]['tid'] = $brand->tid;
        $node->field_sezon[LANGUAGE_NONE][]['tid'] = $product['sezon'];
        $node->field_tipts[LANGUAGE_NONE][]['tid'] = $vehicle;
        $image_dir="public://shini";
    } elseif($param['type']=='disk'){
        $node->field_disk_brand[LANGUAGE_NONE][]['tid'] = $brand->tid;
        $node->field_tipd[LANGUAGE_NONE][]['tid'] = $product['tipdiska'];
        $image_dir="public://diski";
    }
    if(isset($product['image_url'])){
        $file=kolesa_pricer_get_image_from_url($product['image_url'],$node->title,$image_dir);
        if(isset($file->fid)){
            $node->field_image[und][0]['fid']=$file->fid;
        }
    }
    node_save($node);
    if($mode=='create') {
        kolesa_pricer_log('Создаём новую модель для ' . $param['src_name'] . ' - nid:' . $node->nid, 'add model');
    } else {
        kolesa_pricer_log('Обновляем модель для ' . $param['src_name'] . ' - nid:' . $node->nid, 'update model');
    }

    $param['dest_name']=$node->title;
    $param['dest_id']=$node->nid;
    return $param;
}

function kolesa_pricer_update_model($param,$options){
    $nid=$param['dest_id'];
    $node=node_load($nid);
    kolesa_pricer_create_model($node);
}

function kolesa_pricer_get_image_from_url($url,$name,$dir){
    module_load_include('inc', 'pathauto');
    $file_content = file_get_contents($url);
    $filename = pathauto_cleanstring($name);
    $filename.=".".pathinfo($url,PATHINFO_EXTENSION);
    kolesa_pricer_log('Берём '.$url.' и сохраняем в '.$filename,'get image');
    $file_temp = file_save_data($file_content, $dir."/".$filename, FILE_EXISTS_RENAME);
    kolesa_pricer_log('Создан FILE fid:'.$file_temp->fid,'save file');
    return $file_temp;
}

function kolesa_pricer_get_param_by_source($param_type,$value,$product,$options){
    $product_type=$product['type'];
    $source=$product['source'];
    $options['product']=$product;

    $fields=array(
        "param"=>$param_type,
        "type"=>$product_type,
        "src_name"=>$value,
        "source"=>$source,
    );
    $param=db_query("select * from {pricer_sync} where param=:param and type=:type and  src_name=:src_name and source=:source",$fields)->fetchAssoc();
    if(empty($param)) $param=kolesa_pricer_create_param($fields);

    if (empty($param['dest_id'])) kolesa_pricer_sync_param($param, $options);
     elseif(!empty($options['update_param_entity'])) kolesa_pricer_update_param($param, $options);

    return $param;


/*    if(empty($check)){
        if($param=='brand'){
            $vid=$type=='shina'?2:4;
            $check_list=kolesa_pricer_get_brands($vid);#vid==2 для шин
        }
        if($param=='model'){
            $check_list=kolesa_pricer_get_models($type);
        }
        $check=kolesa_pricer_find_text_in_list($value,$check_list);
        if(empty($check)){
            # Create new brand
        }
        if(!empty($check)) {
            $fields = array(
                'param' => $param,
                'type' => $type,
                'src_name' => $value,
                'source' => $source,
                'dest_name' => $brands[$check],
                'dest_id' => $check,
            );
            db_insert("pricer_sync")->fields($fields)->execute();
            return $check;
        }
    } else {
        return $check;
    }*/
}

function kolesa_pricer_preprocess_info_value($value,$ignore_list=array()){
    $new_value=trim(str_replace($ignore_list,"",$value));

    return $new_value;
}