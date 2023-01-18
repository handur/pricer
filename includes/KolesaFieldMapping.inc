<?php

/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 05.02.2021
 * Time: 11:53
 */
class KolesaFieldMapping extends Pricer\Core\FieldMapping
{
    private static $maps=[
      'brandMap'=>[],
      'modelMap'=>[],
      'productMap'=>[],
      'caeMap'=>[],
      'entityMap'=>[],
      'nodeBrandMap'=>[],
      'nodeMap'=>[],
      'termMap'=>[],
    ];
    private static $productType = ['Wheels' => 'disk_razmer', 'Tyres' => 'shina_razmer'];

    private static $productParams = [
        'Wheels' => ['BrandObject', 'ModelObject', 'Width', 'Diametr', 'Krep', 'PCD', 'DIA','ColorTerm'],
        'Tyres' => ['BrandObject' , 'ModelObject', 'Width', 'Height' , 'Diametr', 'Season', 'Pin', 'IndexN', 'IndexS', 'Runflat'],
    ];

    public static function searchInMappingList($type,$row){
        switch($type){
            case 'brands':
                return db_query("select id from {pricer_mapping_brands} where shipper=:shipper and category=:category and brand=:brand",array(
                    ":shipper"=>$row['Shipper'],
                    ":category"=>$row['Category'],
                    ":brand"=>$row['Brand'],
                ))->fetchField();
                break;
            case 'models':
                return db_query("select id from {pricer_mapping_models} where shipper=:shipper and category=:category and brand=:brand and model=:model",array(
                    ":shipper"=>$row['Shipper'],
                    ":category"=>$row['Category'],
                    ":brand"=>$row['Brand'],
                    ":model"=>$row['Model']
                ))->fetchField();
                break;
            case 'caes':
                return db_query("select product_id from {pricer_mapping_caes} where  cae=:cae and brand=:brand",array(
                    ":cae"=>$row['VendorCode'],":brand"=>$row['Brand'],
                ))->fetchField();
                break;
        }
        return FALSE;

    }
    public static function updateMappingList($type,$row,$id){
         switch($type){
            case 'brands':
                if(empty($row['Brand'])) return;
                db_merge("pricer_mapping_brands")->key(array(
                    'shipper'=>$row['Shipper'],
                    'category'=>$row['Category'],
                    'brand'=>$row['Brand'],
                ))->fields(array(
                    'shipper'=>$row['Shipper'],
                    'category'=>$row['Category'],
                    'brand'=>$row['Brand'],
                    'id'=>!empty($id)?(int)$id:NULL,
                ))->execute();
                break;
            case 'models':
                if(empty($row['Model'])) return;
                    db_merge("pricer_mapping_models")->key(array(
                        'shipper' => $row['Shipper'],
                        'category' => $row['Category'],
                        'brand' => $row['Brand'],
                        'model' => trim($row['Model']),
                    ))->fields(array(
                        'shipper' => $row['Shipper'],
                        'category' => $row['Category'],
                        'brand' => $row['Brand'],
                        'model' => trim($row['Model']),
                        'id' => !empty($id)?(int)$id:NULL,
                    ))->execute();
                break;
            case 'caes':
                if(empty($row['VendorCode'])) return;
                    db_merge("pricer_mapping_caes")->key(array(
                        'category' => $row['Category'],
                        'cae' => $row['VendorCode'],
                        'brand' => $row['Brand'],
                    ))->fields(array(
                        'category' => $row['Category'],
                        'cae' => $row['VendorCode'],
                        'brand' => $row['Brand'],
                        'product_id' =>(int) $id,
                    ))->execute();
                break;
        }
    }

    public static function getBrand($row){
        $brand_id=static::searchInMappingList('brands',$row);
        $updateMap=FALSE;
        if(empty($brand_id)){
            if(!empty($row['Product']))  $brand_id=$row['Product']->brand;
            if(empty($brand_id)) {
                $findValue=$row['Brand'];
                $findValue=KolesaFieldConvert::convert($findValue, array('removespace', 'upper', 'clean'));
                $brand_id=self::searchKeyInMap($findValue,static::brandMap($row['Category']));
            }
            if(!empty($brand_id)) $updateMap=TRUE;
        }
        if($updateMap) self::updateMappingList('brands',$row,$brand_id);
        if(!empty($brand_id)) {
            return $brand_id;
        }
        return NULL;
    }

    public static function getModel($row,$fullSearch=FALSE){
        $model_id=self::searchInMappingList('models',$row);
        $updateMap=FALSE;
        if(empty($model_id)){
            if($fullSearch) {
                if (!empty($row['Product'])) $model_id = $row['Product']->model;
                if (empty($model_id)) {
                    if (!empty($row['BrandObject'])) {
                        $findValue = $row['BrandObject']->name . ' ' . $row['Model'];
                    } elseif (!empty($row['Brand'])) {
                        $findValue = $row['Brand'] . ' ' . $row['Model'];
                    }

                    KolesaFieldConvert::convert($findValue, array('cleanword', 'removespace', 'upper', 'clean'));
                    $model_id = self::searchKeyInMap($findValue, self::modelMap($row['Category']));
                }
                if(!empty($model_id)) $updateMap=TRUE;
            }

        }
        if($updateMap) self::updateMappingList('models',$row,$model_id);
        if(!empty($model_id)) {
            return $model_id;
        }
        return NULL;
    }

    public static function getProduct($row){
        $product_id=self::searchInMappingList('caes',$row);

        if(empty($product_id)) {
            $findValue=$row['VendorCode'];
            $findValue=KolesaFieldConvert::convert($findValue,array('removespace','upper','clean'));
            $findValue=$row['BrandObject']->id.'-'.$findValue;

            if($product_id=self::searchKeyInMap($findValue, static::caeMap($row['Category']))){
                $updateMap=TRUE;
            }
        }
        if(!empty($product_id)) {
            if($updateMap) self::updateMappingList('caes',$row,$product_id);
            return $product_id;
        }
        return NULL;
    }
    public static function getCae($row){
        static $changeCae;
        $curCae=$row['VendorCode'];
        /* Проверяем на множественный артикул*/
        if($caeSplit=preg_split("[=]",$curCae,0,PREG_SPLIT_DELIM_CAPTURE)){
            if(count($caeSplit)>1) {
                foreach ($caeSplit as $cae) {
                    $cae=KolesaFieldConvert::convert($cae,'cleancode');
                    $trow=$row;
                    $trow['VendorCode']=$cae;
                    $trow['VendorCode']=KolesaFieldMapping::getCae($trow);
                    if($productId=KolesaFieldMapping::getProduct($row)){
                        return $cae;
                    }
                }
            }

        }

        /* Ищем возможную замену артикула */
        if(empty($changeCae)){
            $changeCae=db_query("select old_cae, new_cae, brand from {pricer_change_cae}")->fetchAllAssoc('old_cae',PDO::FETCH_ASSOC);
        }
        $newCae=$curCae;
        if($caeData=$changeCae[$curCae]) {

           # $newCae=$caeData['new_cae'];

            if(!empty($caeData['brand'])){
                if(strcasecmp($caeData['brand'],$row['Brand'])==0) $newCae=$caeData['new_cae'];
            } else {
                $newCae=$caeData['new_cae'];
            }

        }
        return $newCae;
    }
    public static function getProductByParams($row)
    {
        $logMsg='Ищем позицию '.$row['Category'].' по параметрам';
        $catName = $row['Category'];
        $entityType = self::$productType[$catName];
        $curParams = self::$productParams[$catName];
        $fields = array();


        array_walk($curParams, function ($value) use (&$fields, $row) {
            if($value=='BrandObject'||$value=='ModelObject') $fields[$value]=$row[$value]->id;
            else $fields[$value]=$row[$value];
        });
        #print_r($curParams);
        #print_r($fields);

        if (!$diff = array_diff_key(array_flip($curParams), array_filter($fields))) {

            $product_entities = entity_load($entityType, array(), $fields);
            if(!empty($product_entities)) {
                $product = reset($product_entities);
                $logMsg.=' - нашли '.$product->id;
                KolesaPricerLog::addLog('message','getProductByParams',$logMsg,$row);

                return $product;
            }
        } else {
            print_r($diff);
        }#
        $logMsg.=' - не нашли';
        KolesaPricerLog::addLog('message','getProductByParams',$logMsg,$row);
        return NULL;
    }

    public static function getStock($options,$rawValue,$mode='key'){
        $shipper=$options['Shipper'];
        switch($mode){
            case 'key':
                $stock_id=static::stockMap($shipper)[$rawValue];
                break;
            case 'name':
                $stock_id=static::stockMapName($shipper)[$rawValue];
                break;
        }
        return $stock_id;
    }

    public static function getModelById($bundle,$rawValue){
        return self::searchKeyInMap($rawValue,self::modelMap($bundle));
    }
    public static function getProductById($bundle,$rawValue){
        return self::searchKeyInMap($rawValue,self::entityMap($bundle));
    }
    public static function getBrandByNid($vid,$rawValue){
        return self::searchKeyInMap($rawValue,self::nodeBrandMap($vid));
    }
    public static function getNodeByNid($nid){
        return reset(node_load_multiple(array($nid)));

    }
    public static function getTermByTid($tid){
        return reset(taxonomy_term_load_multiple(array($tid)));
    }
    public static function brandMap($category){
        if(empty(self::$maps['brandMap'][$category])){
            $result=db_query("SELECT name, id FROM {pricer_products_brands} WHERE category=:cat",array(":cat"=>$category));
            foreach($result as $row){
                $findValue=$row->name;
                $findValue=KolesaFieldConvert::convert($findValue,array('removespace','upper','clean'));
                $map[$findValue]=$row->id;
            }
            self::$maps['brandMap'][$category]=$map;
        }
        return self::$maps['brandMap'][$category];
    }

    public static function modelMap($category){
        if(empty(self::$maps['modelMap'][$category])){
            $result=db_query("SELECT models.id, models.category, brands.NAME brand,models.name model FROM {pricer_products_models} models
LEFT JOIN {pricer_products_brands} brands ON brands.id=models.brand_id where models.category=:cat",array(":cat"=>$category));
            foreach($result as $row){
                if(in_array($row->brand,['Replay','Replica'])){
                    $modelvalue=$row->model;
                    $row->model=KolesaFieldConvert::convert($modelvalue,array('replica'));
                }
                $findValue=$row->brand.' '.$row->model;

                $findValue=KolesaFieldConvert::convert($findValue,array('removespace','upper','clean'));
                $map[$findValue]=$row->id;
            }

            self::$maps['modelMap'][$category]=$map;
        }

        return self::$maps['modelMap'][$category];
    }



    public static function caeMap($category){
        if(empty(static::$maps['caeMap'][$category])){
            $products=db_query("select brand,cae,id from {pricer_products} where category=:cat",[":cat"=>$category]);

            $result=[];
            foreach($products as $product){
                if(empty($product->cae)) continue;
                $cae=$product->cae;
                $brand=$product->brand;

                KolesaFieldConvert::convert($cae,array('removespace','upper','clean'));
                $key=$cae;
                $key=$brand.'-'.$cae;
                $result[$key]=$product->id;
            }
            static::$maps['caeMap'][$category]=$result;
        }
        return static::$maps['caeMap'][$category];
    }

    public static function stockMap($shipper){
        if(empty(static::$maps['stockMap'])){
            $stocks=entity_load('shipper_stock');
            foreach($stocks as $stock){
                static::$maps['stockMap'][$stock->shipperName][$stock->machine_key]=$stock->id;
            }
        }
        return static::$maps['stockMap'][$shipper];
    }
    public static function stockMapName($shipper){
        if(empty(static::$maps['stockMapName'])){
            $stocks=entity_load('shipper_stock');
            foreach($stocks as $stock){
                static::$maps['stockMapName'][$stock->shipperName][$stock->name]=$stock->id;
            }
        }
        return static::$maps['stockMapName'][$shipper];
    }
    public static function entityMap($bundle){
        if(empty(self::$maps['entityMap'][$bundle])){
            self::$maps['entityMap'][$bundle]=entity_load($bundle);
        }
        return self::$maps['entityMap'][$bundle];
    }

    public static function nodeBrandMap($vid){
        if(empty(self::$maps['nodeBrandMap'][$vid])){
            $result=db_query("SELECT distinct ti.nid, ti.tid FROM {taxonomy_index} ti 
LEFT JOIN {taxonomy_term_data} td ON td.tid=ti.tid
WHERE td.vid=:vid
ORDER BY nid",array(":vid"=>$vid))->fetchAllKeyed(0,1);
            self::$maps['nodeBrandMap'][$vid]=$result;
        }
        return self::$maps['nodeBrandMap'][$vid];
    }

    public static function nodeMap(){
        if(empty(self::$maps['nodeMap'])){
            $nids=db_query("SELECT nid from {node} order by nid asc")->fetchCol();
            $nodes=node_load_multiple($nids);
            self::$maps['nodeMap']=$nodes;
        }

        return self::$maps['nodeMap'];
    }
    public static function termMap(){
        if(empty(self::$maps['termMap'])){
            $tids=db_query("SELECT tid from {taxonomy_term_data} order by tid asc")->fetchCol();
            $terms=taxonomy_term_load_multiple($tids);
            self::$maps['termMap']=$terms;
        }
        return self::$maps['termMap'];
    }
}