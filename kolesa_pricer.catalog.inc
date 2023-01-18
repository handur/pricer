<?php
/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 27.04.2021
 * Time: 15:25
 */

function kolesa_pricer_catalog_page($mode,$brand_id=NULL,$model_id=NULL,$product_id=NULL){
    drupal_add_css(drupal_get_path('module','kolesa_pricer').'/kolesa_pricer.catalog.css');
    switch($mode){
        case 'main':
            $content=kolesa_pricer_catalog_main();
            break;
        case 'brand':
            $content=kolesa_pricer_catalog_brand($brand_id);
            break;
        case 'model':
            $content=kolesa_pricer_catalog_model($brand_id,$model_id);
            break;
    }
    return $content;


}

function kolesa_pricer_catalog_main(){
    $build=[];
    $query=kolesa_pricer_catalog_get_brands();

    foreach($query as $row){
        $brands[$row->category][$row->id]=$row;
    }
    $cats=array_keys($brands);
    foreach($cats as $cat){
        $brand_rows=[];
        foreach($brands[$cat] as $id=>$brand){
            $brand_rows[]=[l($brand->name,'catalog/'.$brand->id),$brand->modelCount,$brand->productCount];
        }
        $build[$cat]=[
          '#prefix'=>'<h2>'.$cat.'</h2>',
        ];
        $header=['Название','Моделей','Размеров'];
        $build[$cat]['brands']['#markup']=theme('table',['rows'=>$brand_rows,'header'=>$header]);
    }


    return $build;
}

function kolesa_pricer_catalog_brand($brand_id){
    $build=[];
    $brand=current(kolesa_pricer_catalog_get_brands(['brand'=>$brand_id]));
    drupal_set_title($brand->name);

    $models=kolesa_pricer_catalog_get_models(['brand'=>$brand_id]);
    $model_list=[];
    foreach($models as $model){
        $model=kolesa_pricer_catalog_load_full_model($model,['image'=>TRUE,'imagesLimit'=>1]);
        $model_list[]=theme('kolesa_pricer_catalog_model',['model'=>$model]);
    }
    $build['model_list']['#markup']=theme('kolesa_pricer_catalog_model_list',['list'=>$model_list]);

    return $build;
}

function kolesa_pricer_catalog_model($brand_id,$model_id){
    $build=[];

    $model=current(kolesa_pricer_catalog_get_models(['model'=>$model_id]));
    $model=kolesa_pricer_catalog_load_full_model($model,['image'=>TRUE,'brand'=>TRUE,'products'=>TRUE]);
    drupal_set_title($model->brandObject->name.' '.$model->name);
    $caes=[];
    foreach($model->products as $product){
        $shippers=db_query("select distinct ps.machine_key from {pricer_price} p left join {pricer_shipper_stocks} ss on ss.id=p.stock left join {pricer_shipper} ps on ps.id=ss.sid where cae=:cae",[":cae"=>$product->cae])->fetchCol();
        $caes[$product->cae]=$shippers;
        $shippers+=$shippers;

    }
    $build['model']['#prefix']="<p>Поставщики: ".implode(",",$shippers)."</p>";

    $build['model']['#markup']=theme('kolesa_pricer_catalog_model',['model'=>$model,'view'=>'full']);
/*
    $models=kolesa_pricer_catalog_get_models(['brand'=>$brand_id]);
    $model_list=[];
    foreach($models as $model){
        $model=kolesa_pricer_catalog_load_full_model($model,['imagesLimit'=>1]);
        $model_list[]=theme('kolesa_pricer_catalog_model',['model'=>$model]);
    }
    $build['model_list']['#markup']=theme('kolesa_pricer_catalog_model_list',['list'=>$model_list]);
*/
    return $build;
}

function kolesa_pricer_catalog_load_full_model($model,$options=[]){
    if(!empty($options['image'])) {
        if ($images = explode(",", $model->imageFids)) {

            $model_images=[];
            foreach ($images as $fid) {
                if($file = file_load($fid)) {
                    $uri = $file->uri;
                    $model_images[$fid] = $uri;
                }
            }
            if (!empty($options['imagesLimit'])) $model_images = array_slice($model_images, $options['imagesLimit']);
            $model->images=$model_images;
        }
    }

    if(!empty($options['brand'])){
        $model->brandObject=kolesa_pricer_catalog_get_brand($model->brand_id);
    }

    if(!empty($options['products'])){
        $model->products=kolesa_pricer_catalog_get_products(['model'=>$model->id]);
    }


#    $productCount=count(kolesa_pricer_catalog_get_products(['model'=>$model->id]));
  #  $model->productCount=$productCount;
    return $model;
}

function kolesa_pricer_catalog_get_images($options=[]){
    $query=db_select('pricer_products_images','i');
    $query->fields('i',['fid','source']);
    if(isset($options['model'])){
        $product_ids=array_keys(kolesa_pricer_catalog_get_products(['model'=>$options['model']]));
        if(!empty($product_ids)) $query->condition('product_id',$product_ids,'IN');
        else return NULL;
    }
    $result=$query->execute()->fetchAllAssoc('source');
    if(!empty($result)) return $result;
    return NULL;
}

function kolesa_pricer_catalog_get_brand($id){
    $brands=kolesa_pricer_catalog_get_brands();

    if(isset($brands[$id])) return $brands[$id];
    return NULL;
}
function kolesa_pricer_catalog_get_brands($options=[]){
    static $brands;
    if(empty($options)&&!empty($brands)) return $brands;
    $query=db_select('pricer_products_brands','b');
    $query->fields('b');
    if(isset($options['brand'])){
        $query->condition('b.id',$options['brand']);
    }
    $query->addExpression('count(p.id)','productCount');
    $query->addExpression('count(distinct p.model)','modelCount');
    $query->leftJoin('pricer_products','p','p.brand=b.id');
    $query->groupBy('b.id');
    $query->orderBy('name','ASC');
    $result=$query->execute()->fetchAllAssoc('id');
    $brands=$result;
    return $result;
}
function kolesa_pricer_catalog_get_models($options=[]){
    $query=db_select('pricer_products_models','m');
    $query->fields('m');
    if(isset($options['brand'])){
        $query->condition('brand_id',$options['brand']);
    }
    if(isset($options['model'])){
        $query->condition('model',$options['model']);
    }
    $query->leftJoin('pricer_products','p','p.model=m.id');
    $query->leftJoin('pricer_products_images','im','im.product_id=p.id');
    $query->addExpression('count(p.id)','productCount');
    $query->addExpression("GROUP_CONCAT(DISTINCT im.fid SEPARATOR ', ')",'imageFids');
        #GROUP_CONCAT(DISTINCT b.book ORDER BY b.book ASC SEPARATOR ', ')
    $query->groupBy('m.id');
    $query->orderBy('productCount','desc');
    $result=$query->execute()->fetchAllAssoc('id');

    return $result;
}

function kolesa_pricer_catalog_get_products($options=[]){
    $query=db_select('pricer_products','p');
    $query->fields('p');
    if(isset($options['brand'])){
        $query->condition('brand',$options['brand']);
    }
    if(isset($options['model'])){
        $query->condition('model',$options['model']);
    }
    $query->leftJoin('pricer_products_images','im','im.product_id=p.id');
    $query->addExpression("GROUP_CONCAT(DISTINCT im.fid SEPARATOR ', ')",'imageFids');
    $query->groupBy('p.id');
    $result=$query->execute()->fetchAllAssoc('id');
   # print strtr((string) $query, $query->arguments());
   # exit;
    return $result;
}