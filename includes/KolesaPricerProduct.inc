<?php

/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 15.02.2021
 * Time: 14:02
 */

class KolesaProductFactory
{
    protected static $instances;
    public static function build($catName)
    {
        $product = "KolesaPricerProduct_" . ucfirst($catName);
        if (class_exists($product)) {
            if(empty($instances[$catName])) {
                $instances[$catName]=new $product();
            }
            return $instances[$catName];
        } else {
            throw new \Exception("Несуществующий тип продукта");
        }
    }
}

class KolesaPricerProduct
{
    protected static  $imageDir;
    protected static $resetBrands=FALSE;
    protected static $resetModels=FALSE;
    protected static $resetProducts=FALSE;
    protected static $resetAddons=FALSE;
    protected static $resetImages=FALSE;
    protected static $category="";
    public static function createBrand($data){
        $data['Category']=static::$category;
        if(empty($data['Category'])||empty($data['Brand'])) return;
        $brand=new stdClass();
        $brand->name=$data['Brand'];
        $brand->category=$data['Category'];
        static::saveBrand($brand);
        static::$resetBrands=TRUE;
        return $brand;
    }

    public static function createModel($data){
        $data['Category']=static::$category;
        if(empty($data['Category'])||empty($data['BrandObject'])||empty($data['Model'])) return;
        $model=new stdClass();
        $model->name=static::getModelName($data);
        $model->category=$data['Category'];
        $model->brand_id=$data['BrandObject']->id;
        static::updateModel($model,$data);
        static::$resetModels=TRUE;
        return $model;
    }

    public static function createProduct($data){
        $data['Category']=static::$category;

        if(empty($data['Category'])||empty($data['BrandObject'])||empty($data['ModelObject'])||empty($data['VendorCode'])) return;
        $product=new stdClass();
        $product->cae=$data['VendorCode'];
        $product->category=$data['Category'];
        static::updateProduct($product,$data);
        static::$resetProducts=TRUE;
        return $product;
    }

    public static function updateModel(&$model,$data){
        if(empty($data['Shipper'])) $data['Shipper']='kolesa812';
        if(!empty($data['Shipper'])) $model->source=$data['Shipper'];
        static::saveModel($model);
    }
    public static function updateProduct(&$product,$data){
        if(empty($data['Category'])) $data['Category']=static::$category;
        if(empty($product->category)) $product->category=$data['Category'];
        $product->name=static::getProductName($data);
        if(empty($data['Shipper'])) $data['Shipper']='kolesa812';
        if(!empty($data['Shipper'])) $product->source=$data['Shipper'];
        $product->brand=$data['BrandObject']->id;
        $product->model=$data['ModelObject']->id;
        if(!empty($data['Image'])&&empty($product->images)) {
            static::addImage($product,$data);
            if(!empty($product->images)) {
                foreach ($product->images as $source => $image) {
                    db_merge('pricer_products_images')->key(['product_id' => $product->id, 'source' => $source])->fields(['product_id' => $product->id, 'fid' => $image, 'source' => $source])->execute();
                }
            }
        }
        static::saveProduct($product);
    }
    protected static function saveBrand(&$brand){
        if(empty($brand->id)){
            $id=db_insert('pricer_products_brands')->fields((array)$brand)->execute();
            $brand->id=$id;
        } else {
            db_update('pricer_products_brands')->condition('id',$brand->id)->fields((array)$brand)->execute();
            $brand=static::loadBrand($brand->id);
        }
        return $brand;
    }
    protected static function saveModel(&$model){
            if(empty($model->id)){
                $id=db_insert('pricer_products_models')->fields((array)$model)->execute();
                $model->id=$id;
            } else {
                db_update('pricer_products_models')->condition('id',$model->id)->fields((array)$model)->execute();
                $model=static::loadModel($model->id);
            }
            return $model;
    }
    protected static function saveProduct(&$product){

        if(!empty($product->images)) $images=$product->images;
        unset($product->addons);
        unset($product->images);
        if(empty($product->id)){
            $id=db_insert('pricer_products')->fields((array)$product)->execute();
            $product->id=$id;

        } else {
            db_update('pricer_products')->condition('id',$product->id)->fields((array)$product)->execute();
        }

        return $product;
    }

    public function addImage(&$product,$data){
        if($images=static::loadImages($product->id)&&isset($images[$data['Shipper']])) return;
        if(empty($data['Image'])) return;
        if($image=static::loadImageByUrl($data['Image'],$product->name)){
            $product->images[$data['Shipper']]=$image['fid'];
        }
    }
    public function loadBrand($id,$reset=FALSE){
        static $brands;
        if($reset||static::$resetBrands||empty($brands)) {
            $brands=db_query("select * from {pricer_products_brands}")->fetchAllAssoc('id');
            static::$resetBrands=FALSE;
        }
        if(isset($brands[$id])) return $brands[$id];
        return NULL;
    }
    public function loadModel($id,$reset=FALSE){
        static $models;
        if($reset||static::$resetModels||empty($models)) {
            $models = db_query("select * from {pricer_products_models}")->fetchAllAssoc('id');
            static::$resetModels=FALSE;
        }
        if(isset($models[$id])) return $models[$id];
        return NULL;
    }
    public function loadProduct($id,$reset=FALSE){
        $product = db_query("select * from {pricer_products} where id=:id",[":id"=>$id])->fetchObject();
        if(isset($product)) {
            $product->addons=static::loadAddons($id);
            $product->images=static::loadImages($id);
            return $product;
        }
        KolesaPricerLog::addLog('error','loadProduct','Грузим товар по коду '.$id.' - не нашли');
        return NULL;
    }

    public function getModelName($data){
        return "Model";
    }
    public function getProductName($data){
        return "Product";
    }
    public function loadImageByUrl($link,$name){
        $dir=static::$imageDir;
        module_load_include('inc', 'pathauto');
        $clean_title = pathauto_cleanstring($name);
        $ext = pathinfo($link, PATHINFO_EXTENSION);
        if (empty($ext)) $ext = "jpg";
        $newPath = strtolower($clean_title . "." . $ext);

        $encodedPath = rawurlencode($link);
        $encodedPath = str_replace('%3A', ':', str_replace('%2F', '/', $encodedPath));
        $fileInfo = system_retrieve_file($encodedPath, $dir . "/" . $newPath, true, FILE_EXISTS_RENAME);
        if ($fileInfo->fid) {
            $imageField=array(
                'fid'=>$fileInfo->fid,
                'alt'=>$name,
                'title'=>$name,
            );
            return $imageField;
        }
        return NULL;
    }
    public function loadAddons($id,$reset=TRUE){

        $result=db_query("select * from {pricer_products_addons} where id=:id",[":id"=>$id]);
        foreach($result as $row){
            $addons[$row->id][$row->param_key]=$row->param_value;
        }
        if(isset($addons[$id])) return $addons[$id];
        return NULL;
    }
    public function loadImages($id,$reset=TRUE){
        $result=db_query("select * from {pricer_products_images} where product_id=:id",[":id"=>$id]);
        foreach($result as $row){
            $images[$row->product_id][$row->source]=$row->fid;
        }
        if(isset($images[$id])) return $images[$id];
        return NULL;
    }
    public function updateAddons($keys,&$product,$data){
        foreach($keys as $key){
            if(isset($data[$key])) {
                db_merge('pricer_products_addons')
                    ->key([
                        'id' => $product->id,
                        'param_key' => $key,
                    ])
                    ->fields([
                        'id' => $product->id,
                        'param_key' => $key,
                        'param_value' => $data[$key]])
                    ->execute();
                $product->addons[$key] = $data[$key];
            }
        }
    }

}

class KolesaPricerProduct_Tyres extends KolesaPricerProduct{
    protected static $imageDir='public://products/tyres';
    protected static $category='Tyres';


    public static function updateProduct(&$product,$data){
        $addons_keys=['GTIN','Width','Height','Diametr','Season','Pin','IndexN','IndexS','Runflat','Vehicle','Omologation','XL','Usil','Reinforced','Old','Country'];
        parent::updateProduct($product,$data);

        if(empty($data['Season'])&&!empty($data['ModelObject'])){
            $sezon=db_query("SELECT param_value from {pricer_products_addons} addons 
              WHERE EXISTS (SELECT * FROM {pricer_products} p WHERE p.model=:model AND p.id=addons.id) AND param_key='Season'",[":model"=>$data['ModelObject']->id])->fetchField();
            if(!empty($sezon)) $data['Season']=$sezon;
        }
        /*
        if(!isset($data['Pin'])&&!empty($data['ModelObject'])){
            $pin=db_query("SELECT param_value from {pricer_products_addons} addons 
              WHERE EXISTS (SELECT * FROM {pricer_products} p WHERE p.model=:model AND p.id=addons.id) AND param_key='Pin'",[":model"=>$data['ModelObject']->id])->fetchField();
            if(!empty($pin)) $data['Pin']=$pin;
        }
        */

        parent::updateAddons($addons_keys,$product,$data);
    }
    public function getProductName($data){
        $size=(float)$data['Width'];
        if(!empty($data['Height'])) $size.="/".(float)$data['Height'];
        $size.=" R".(float)$data['Diametr'];
        $indexes=$data['IndexN'].$data['IndexS'];
        $runflat=$data['Runflat']?'Runflat':'';
        $pin='';
        if($data['Season']=='зимние') $pin=$data['Pin']?'(шип)':'(нешип)';
        $names=array($data['BrandObject']->name,$data['ModelObject']->name,$size,$indexes,$runflat,$pin);
        return implode(' ',$names);
    }

    public function getModelName($data){
        KolesaFieldConvert::convert($data['Model'], array('cleanword'));
        return $data['Model'];
    }
}
class KolesaPricerProduct_Moto extends KolesaPricerProduct_Tyres{
    protected static $category='Moto';

    public static function updateProduct(&$product,$data){
        $addons_keys=['GTIN','Width','Height','Diametr','Season','Pin','IndexN','IndexS','Runflat','Vehicle','Omologation','XL','Usil','Reinforced','Use','Axis','Old'];
        parent::updateProduct($product,$data);
        parent::updateAddons($addons_keys,$product,$data);
    }
}
class KolesaPricerProduct_Truck extends KolesaPricerProduct_Tyres{
    protected static $category='Truck';

    public static function updateProduct(&$product,$data){
        $addons_keys=['GTIN','Width','Height','Diametr','Season','Pin','IndexN','IndexS','Runflat','Vehicle','Omologation','XL','Usil','Reinforced','Old'];
        parent::updateProduct($product,$data);
        parent::updateAddons($addons_keys,$product,$data);
    }
}
class KolesaPricerProduct_Wheels extends KolesaPricerProduct{
    protected static $imageDir='public://products/wheels';
    protected static $category='Wheels';

    public static function updateProduct(&$product,$data){
        $addons_keys=['GTIN','Width','Diametr','Krep','PCD','ET','DIA','Color','ColorDescription','Type'];
        parent::updateProduct($product,$data);
        parent::updateAddons($addons_keys,$product,$data);
    }
    public function getProductName($data){
        $size=(float)$data['Width'];
        $size.=" R".(float)$data['Diametr'];
        $size.=" PCD:".$data['Krep'].'/'.(float)$data['PCD'];
        $size.=" ET:".(float)$data['ET'];
        $size.=" DIA:".(float)$data['DIA'];
        $names=array($data['BrandObject']->name,$data['ModelObject']->name,$size,$data['Color']);
        return implode(' ',$names);
    }

    public function getModelName($data){
        KolesaFieldConvert::convert($data['Model'], array('cleanword'));
        return $data['Model'];
    }
}

class KolesaPricerProduct_Krep extends KolesaPricerProduct{
    protected static $imageDir='public://products/krep';
    protected static $category='Krep';

    public static function updateProduct(&$product,$data){
        $addons_keys=['Color','TypeKrep','Length','Thread','KeySize','Form','Package','Amount','Country','SubCategory'];
        parent::updateProduct($product,$data);
        parent::updateAddons($addons_keys,$product,$data);
    }
    public function getProductName($data){
        return $data['Name'];
    }

    public function getModelName($data){
        KolesaFieldConvert::convert($data['Model'], array('cleanword'));
        return $data['Model'];
    }
}