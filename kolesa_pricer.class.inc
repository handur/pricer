<?php

Class ExecutionTime
{
    public static $start_time = [];
    public static $end_time = [];
    public static $message = [];

    public static function addStartTime($key)
    {
        self::$start_time[$key] = microtime(TRUE);
    }

    public static function addMsg($key, $msg)
    {
        self::$message[$key] = $msg;
    }

    public static function addEndTime($key)
    {
        self::$end_time[$key] = microtime(TRUE);
    }

    public static function getTimes()
    {
        $msg="";
        foreach (self::$start_time as $key => $time) {
            $end = self::$end_time[$key];
            $delta = $end - $time;
            $style = "";
            if ($delta > 0.1) {
                $style = 'background-color:red;color:#fff;font-size:16px;';
            }
            $msg.=$key . ": " . sprintf('%.5f', $delta) . " sec\n";
            $msg.=self::$message[$key] . "\n";
        }
        return $msg;
    }
}


Class KolesaPricerUpdate extends Pricer\Core\PricerUpdate
{

    public $shipperId;
    private $shipperStocks;
    private $sessionId;

    private $processedPrice;

    public function __construct(){
        parent::__construct();
    }
    public function setOptions($option,$data){
        $currentOptions=$this->options[$option];
        $this->options[$option]=array_replace_recursive($currentOptions,$data);
    }
    public function setSessionId($sid){
        $this->sessionId=$sid;
    }
    protected function rowToString($row){
        array_walk($row,function(&$el){
            $el=(string)$el;
        });
        return $row;
    }
    public function getUniqueColumnData($key)
    {
        $data = $this->parsedData;
        foreach ($data as $catName => $rows) {
            $column = array();
            array_walk($rows, function (&$item) use ($key, &$column) {
                if (is_array($key)) $index = implode(" ", array_intersect_key($item, array_flip($key)));
                else $index = $item[$key];
                if (!isset($column[$index])) $column[$index] = $item;
            });
            $columns[$catName] = $column;
        }

        return $columns;
    }

    public function updateData($type, $rows = [],$session_id='')
    {


        $options=$this->options[$type];


        if(!empty($options['type'])) $type=$options['type'];


        if($type=='price') return $this->updateStocks($rows);

        if($type=='qty') return $this->updateQty($rows);
        if($type=='mrc') return $this->updateMRC($rows);

        if (empty($rows)) {
            $price = $this->parsedData;
            foreach ($price as $catName => $rows) {
                $this->updateData($type, $rows);
            }
            return;
        }
      $alltime1=microtime(TRUE);

        $unknown_positions=[];
        $productsNotCreated=[];
        foreach($rows as &$row) {


            $productFactory=KolesaProductFactory::build($row['Category']);

          #  $getTime['product']=microtime(TRUE);

            if(empty($row['VendorCode'])){
                if($findCode=kolesa_pricer_get_custom_caes($row['Name'])) {
                    $row['VendorCode'] = $findCode;
                } else {
                    KolesaPricerLog::addLog('error', 'updateData', 'Empty Vendor Code for position ' . $row['Name'], $row);
                    $unknown_positions[$row['Name']] = $row;
                }
            }

            if(empty($row['VendorCode'])){
                $productsNotCreated['empty_vendor_code'][]=$row;
            }

            if(empty($row['BrandObject'])){
                if($brand_id=KolesaFieldMapping::getBrand($row)) $row['BrandObject']=$productFactory::loadBrand($brand_id);
            }
            if(empty($row['ModelObject'])){
                if($model_id=KolesaFieldMapping::getModel($row,TRUE)) $row['ModelObject']=$productFactory::loadModel($model_id);
            }
            if(empty($row['Product'])&&!empty($row["VendorCode"])) {
                if($product_id=KolesaFieldMapping::getProduct($row)) {
                    $row['Product']=$productFactory::loadProduct($product_id);
                    #if(empty($row['BrandObject'])) $row['BrandObject']=$productFactory::loadBrand($row['brand']);
                #    $row['ModelObject']=$productFactory::loadModel($row['model']);

                }
            }

           # $getTime['product']=microtime(TRUE)-$getTime['product'];

           # $getTime['brand']=microtime(TRUE);
          #  $getTime['brand']=microtime(TRUE)-$getTime['brand'];

          #  $getTime['model']=microtime(TRUE);

           # $getTime['model']=microtime(TRUE)-$getTime['model'];


            if (empty($row['BrandObject']) && !empty($options['create']['brands'])) {
                /* Create new brand */
                if (empty($row['Brand'])) continue;
             #   $getTime['create_brand']=microtime(TRUE);
                $row['BrandObject'] = $productFactory->createBrand($row);
                $options['update']['brands']=FALSE;
                KolesaFieldMapping::updateMappingList('brands', $row, $row['BrandObject']->id);
             #   $getTime['create_brand']=microtime(TRUE)-$getTime['create_brand'];
            }


            if (!empty($row['BrandObject']) && !empty($options['update']['brands'])) {
                /* Update current brand */
                $productFactory->updateBrand($row['BrandObject'],$row);
            }

            if (empty($row['ModelObject']) && !empty($options['create']['models'])) {
                /* Create new model */
                if(empty($row['BrandObject'])||empty($row['Model'])) continue;
             #   $getTime['create_model']=microtime(TRUE);
                $row['ModelObject'] = $productFactory->createModel($row);
                $options['update']['models']=FALSE;
                KolesaFieldMapping::updateMappingList('models', $row, $row['ModelObject']->id);
            #    $getTime['create_model']=microtime(TRUE)-$getTime['create_model'];
            }
            if (!empty($row['ModelObject']) && !empty($options['update']['models'])) {
                /* Update model */
                $productFactory->updateModel($row['ModelObject'], $row);
            }
            $product_has_create=FALSE;
            if (empty($row['Product']) && !empty($options['create']['products'])) {
                /* Create new product */
                if(empty($row['BrandObject'])||empty($row['ModelObject'])) {

                    $productsNotCreated['empty_brand_model'][]=$row;
                    continue;
                }
              #  $getTime['create_product']=microtime(TRUE);
                $row['Product'] = $productFactory->createProduct($row);
                $product_has_create=TRUE;
                KolesaFieldMapping::updateMappingList('caes', $row, $row['Product']->id);
              #  $getTime['create_product']=microtime(TRUE)-$getTime['create_product'];
            }
            if(empty($row['Product'])&& empty($options['create']['products'])){
                $productsNotCreated['disallow_created'][]=$row;
            }

            if (!$product_has_create&&!empty($row['Product']) && !empty($options['update']['products'])) {
                # $getTime['update_product']=microtime(TRUE);
                $productFactory->updateProduct($row['Product'], $row);
              #  $getTime['update_product']=microtime(TRUE)-$getTime['update_product'];
            }
    /*
            foreach($getTime as $key=>$time) {
                $getTimes[$key][]=$time;
            }
*/

        }


            if(!empty($unknown_positions)) {
                $this->saveUnknownPositions($unknown_positions);
            }

            if(!empty($productsNotCreated)){
                foreach($productsNotCreated as $reason=>$positions) {
                    $this->saveNotCreatedPositions($positions,$reason);
                }
            }


        return $rows;

    }

    public function updateData_before($option){
        db_query("delete from {pricer_unknown} where shipper=:shipper",[":shipper"=>$this->shipperName]);
    }
    public function updateData_after($option){

    }
    public function updateStocks_before($option){
        $curOptions=$this->options[$option];
        $shipper=current(entity_load('shipper',array(),array('machine_key'=>$this->shipperName)));
        $query=db_delete('pricer_price')->condition('stock', array_keys($shipper->stocks));
        if(isset($curOptions['category'])) $query->condition('category',$curOptions['category']);
        $query->execute();
    }

    public function updateStocks($rows)
    {
        $costs = [];
        $unknown_positions=[];
        array_walk($rows, function (&$row, $key) use (&$costs,&$unknown_positions) {
            $error=FALSE;
            $product_id=NULL;
            $productFactory=KolesaProductFactory::build($row['Category']);
            if(empty($row['VendorCode'])){
                if($findCode=kolesa_pricer_get_custom_caes($row['Name'])){
                    $row['VendorCode']=$findCode;
                } else {
                    $unknown_positions[$row['Name']] = $row;
                    #   KolesaPricerLog::addLog('error', 'updateStocks','Empty Vendor Code for position '.$row['Name'],$row);
                    $error = TRUE;
                }
            }
            if(empty($row['BrandObject'])){
                if($brand_id=KolesaFieldMapping::getBrand($row)) $row['BrandObject']=$productFactory::loadBrand($brand_id);
            }

            if(empty($row['Product'])&&!empty($row["VendorCode"])) {
                $product_id=KolesaFieldMapping::getProduct($row);
            }
          #  if($brand_id=KolesaFieldMapping::getBrand($row)) $row['BrandObject']=$productFactory::loadBrand($brand_id);

            if(empty($row['Stocks'])){
                KolesaPricerLog::addLog('error', 'updateStocks','Empty Stocks for position '.$row['Name'],$row);
                $error=TRUE;
            }

            if ($error==FALSE) {
                foreach ($row['Stocks'] as $cost_data) {
                    if (!empty($cost_data['stock_id']) && !empty($cost_data['cost'])) {
                        $qty = KolesaFieldConvert::convert($cost_data['qty'], 'digit');
                        if(empty($qty)) $qty=0;
                        $cost = KolesaFieldConvert::convert($cost_data['cost'], 'digit');
                        $cost_row=array(
                            'id'=>!empty($product_id)?$product_id:NULL,
                            'cae' => $row['VendorCode'],
                            'category'=>$row['Category'],
                            'brand'=>!empty($row['BrandObject'])?$row['BrandObject']->id:NULL,
                            'stock' => $cost_data['stock_id'],
                            'qty' => $qty,
                            'stock_price' => $cost,
                            'active' => 1,
                        );

                        $costs[] = $cost_row;
                    }
                }
            }
        });

        /* Обновление цен*/


        $costs_chunk = array_chunk($costs, 200);
        foreach ($costs_chunk as $chunk) {
            $query = db_insert('pricer_price')->fields(['id','cae','category', 'stock', 'qty', 'stock_price', 'active']);
            foreach ($chunk as $cost) {
                $query->values($cost);
            }
            $query->execute();
        }

        if(!empty($unknown_positions)) {
            $this->saveUnknownPositions($unknown_positions);
        }


    }
    public function getQty($cae,$stock_id){
        static $qtys;
        if(empty($qtys[$stock_id])){
            $all_qty=db_query("select * from {pricer_qty}");
            foreach($all_qty as $q){
                $qtys[$q->stock][$q->cae]=$q->qty;
            }
        }
        if(!empty($qtys[$stock_id][$cae])) return $qtys[$stock_id][$cae];
        return NULL;
    }
    public function updateQty($rows)
    {
        $qtys = [];

        array_walk($rows, function (&$row, $key) use (&$qtys) {
            $error=FALSE;

            if(empty($row['VendorCode'])){
              #  KolesaPricerLog::addLog('error', 'updateStocks','Empty Vendor Code for position '.$row['Name'],$row);
                $error=TRUE;
            }
            if(empty($row['Stocks'])){
              #  KolesaPricerLog::addLog('error', 'updateStocks','Empty Stocks for position '.$row['Name'],$row);
                $error=TRUE;
            }

            if ($error==FALSE) {
                foreach ($row['Stocks'] as $cost_data) {
                    if (!empty($cost_data['stock_id'])) {
                        $qtys[] = array(
                            'cae' => $row['VendorCode'],
                            'category'=>$row['Category'],
                            'stock' => $cost_data['stock_id'],
                            'qty' => KolesaFieldConvert::convert($cost_data['qty'], 'digit'),
                        );
                    }
                }
            }
        });

        /* Обновление кол-ва*/

        foreach ($qtys as $qty) {
            $query = db_merge('pricer_qty')->key(['cae'=>$qty['cae'],'category'=>$qty['category'],'stock'=>$qty['stock']])
                ->fields(['cae'=>$qty['cae'],'category'=>$qty['category'],'stock'=>$qty['stock'],'qty'=>$qty['qty']]);
            $query->execute();
        }



    }
    public function saveUnknownPositions($positions){
        KolesaPricerLog::addLog('error', 'update','Есть неизвестные позиции');
        $query=db_insert("pricer_unknown");
        $query->fields(['name','shipper','category','data']);
        foreach ($positions as $pos) {
            $query->values(
                ['name'=>$pos['Name'],
                    'shipper'=>$this->shipperName,
                    'category'=>$pos['Category'],
                    'data'=>serialize($pos),
                ]
            );
        }
        $query->execute();


    }
    public function saveNotCreatedPositions($positions,$reason=''){
        $sessionId=$this->sessionId;
        foreach($positions as $row){
            $logRow=['name'=>$row['Name'],'category'=>$row['Category'],'cae'=>$row['VendorCode']];
            KolesaPricerLog::addPositionLog($this->shipperName,'not_created',$reason,$sessionId,serialize($logRow));
        }
    }
    public function updateStocks_after(){
        /* do something*/
        $shipper=current(entity_load('shipper',array(),array('machine_key'=>$this->shipperName)));
        $stocks=array_keys($shipper->stocks);
        /*
        $query=db_query("select * from {pricer_qty} where stock IN (:stocks)",[":stocks"=>$stocks]);

        foreach($query as $row){
            db_merge("pricer_price")->fields(
                ['cae'=>$row->cae,
                    'stock'=>$row->stock,
                    'category'=>$row->category,
                    'qty'=>$row->qty,
                    'active'=>1]
            )->key(
                ['cae'=>$row->cae,
                    'stock'=>$row->stock,
                    'category'=>$row->category]
            )->execute();
        }
*/
    }

    protected function process(&$price)
    {
        if (!empty($this->curOptions['unique'])) {
            $uniqueKey = $this->curOptions['unique'];
            $price = $this->getUniqueColumnData($uniqueKey);
        }
        return $price;
    }

    protected function parseStockInfo($item, $options)
    {

        $cost=$item[$options['field_cost']];
        KolesaFieldConvert::convert($cost,'digit');
        $qty=KolesaFieldConvert::convert($item[$options['field_qty']], 'digit');
        $stockInfo = array(
            'stock' => $options['stock'],
            'stock_id' => KolesaFieldMapping::getStock($options, $options['stock']),
            'qty' => (int)$qty,
            'cost' => (float)$cost,
        );

        return $stockInfo;
    }

    protected function parseItem($item, $parseOptions)
    {
        $parsedItem = parent::parseItem($item, $parseOptions);
        if(!empty($parsedItem))   {
            if(isset($parseOptions->attributes()['category'])){
                $parsedItems['Category']=$parseOptions->attributes()['category'];
            }


            $parsedItem['VendorCode']=KolesaFieldMapping::getCae($parsedItem);

        }
        else return NULL;


        return $parsedItem;
    }

    protected function parseElement($elValue, $elOptions)
    {
     #   static $parsedElements;
      #  print count($parsedElements);
        $preElValue=$elValue;
/*
        $staticElements=['Brand','Model','Diametr','Width','Height','Season','Pin','Vehicle'];
        if(in_array($elOptions['element'],$staticElements)&&isset($parsedElements[$elOptions['element']][$preElValue])){
            $elValue=$parsedElements[$elOptions['element']][$preElValue];
        }

        else {
*/
            if (empty($elValue)) {
                if (isset($elOptions['ifemptyField'])) $elValue = $elOptions['row'][$elOptions['ifemptyField']];
                elseif (isset($elOptions['default'])) $elValue = $elOptions['default'];

            }
            if (isset($elOptions['preprocess'])) {
                $preprocessRules = preg_split("[,]", $elOptions['preprocess']);
                $elValue = KolesaFieldConvert::convert($elValue, $preprocessRules, $elOptions);
            }

            if (isset($elOptions['process'])) {
                foreach ($elOptions['process'] as $process) {
                    $row = $elOptions['row'];
                    if ($row[$process['conditionfield']] == $process['conditionvalue']) {
                        $elValue = KolesaFieldConvert::convert($elValue, 'remove', ['filter' => $process['remove']]);
                    }

                }
            }

            if (isset($elOptions['convertType'])) {
                $elValue = KolesaFieldConvert::convert($elValue, $elOptions['convertType'], $elOptions);
            }

            if (!isset($elValue)) $elValue = NULL;
            if (is_object($elValue)) $elValue = NULL;
          /*  $parsedElements[$elOptions['element']][$preElValue]=$elValue;
        }
          */
        return trim($elValue);
    }


}