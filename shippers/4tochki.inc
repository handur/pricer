<?php
Class FortochkiPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[
        'tyres_mapping'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/Tires.xml',
            'file id'=>'4tochki_tires_full',
            'options file'=>'4tochki_full.xml',
            #'unique'=>['Brand','Model'],
            #'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            #'update'=>['models'=>TRUE,'products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'mapping',
        ],
        'wheels_mapping'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/Rims.xml',
            'file id'=>'4tochki_wheels_full',
            'options file'=>'4tochki_full.xml',
         #   'unique'=>['Brand','Model'],
            #'exclude'=>['Name','VendorCode','ShipperCode','Diametr','Width','Krep','PCD','ET','DIA','Color','ColorTerm'],
            'category'=>'Wheels',
            'queued'=>'mapping',
        ],
        'tyres_full_products'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/Tires.xml',
            'file id'=>'4tochki_tires_full',
            'options file'=>'4tochki_full.xml',
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
            'category'=>'Tyres'
        ],
        'wheels_full_products'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/Rims.xml',
            'file id'=>'4tochki_wheels_full',
            'options file'=>'4tochki_full.xml',
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
            'category'=>'Wheels',
        ],
        'tyres_current_products'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/M24934.xml',
            'file id'=>'4tochki_tires_current',
            'options file'=>'4tochki_current.xml',
     #       'unique'=>['Brand','Model','VendorCode'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
           'update'=>['products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'products',
            'update time'=>120
        ],
        'wheels_current_products'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/M24934.xml',
            'file id'=>'4tochki_wheels_current',
            'options file'=>'4tochki_current.xml',
       #     'unique'=>['Brand','Model','VendorCode'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
            'category'=>'Wheels',
            'queued'=>'products',
            'update time'=>120
        ],
        'krep_current_products'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/M24934.xml',
            'file id'=>'4tochki_tires_current',
            'options file'=>'4tochki_current.xml',
            #      'unique'=>['Brand','Model','VendorCode'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
            'category'=>'Krep',
            'queued'=>'products',
            'update time'=>120
        ],
        'tyres_price'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/M26016.xml',
            'file id'=>'4tochki_tires_price',
            'options file'=>'4tochki_price.xml',
            'type'=>'price',
            'category'=>'Tyres',
            'queued'=>'price',
            'update time'=>15
        ],
        'wheels_price'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/M26016.xml',
            'file id'=>'4tochki_wheels_price',
            'options file'=>'4tochki_price.xml',
            'type'=>'price',
            'category'=>'Wheels',
            'queued'=>'price',
            'update time'=>15
        ],
        'tyres_qty'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/rest_spb_pish.xml',
            'file id'=>'4tochki_tyres_qty',
            'options file'=>'4tochki_qty.xml',
            'type'=>'qty',
            'category'=>'Tyres',
            'queued'=>'qty',
            'update time'=>10
        ],
        'wheels_qty'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/rest_spb_pish.xml',
            'file id'=>'4tochki_wheels_qty',
            'options file'=>'4tochki_qty.xml',
            'type'=>'qty',
            'category'=>'Wheels',
            'queued'=>'qty',
            'update time'=>10
        ]

    ];
    protected static $parsedElements=[];
    protected $shipperName='4tochki';


    public function parseData(){
            $xml=parent::getData($this->type);

            $category = $this->curOptions['category'];
            $price = array();
            $parseOptions = $this->parseOptions->{$category};
            $itemName = (string)$parseOptions->attributes()->item;
            $count = 0;
            foreach ($xml as $item) {
                $count++;
                $item->current_no = $count;
                if ($item->getName() == $itemName) {
                    if($parseItem=$this->parseItem((array)$item, $parseOptions)) {
                        $price[$category][] =$parseItem;
                    }
                }
            }
            $this->parsedData = $price;
            $this->process($price);
            return $price;
    }
    public function parseData2(){
        $xml=parent::getData($this->type);

        $category = $this->curOptions['category'];
        $price = array();
        $parseOptions = $this->parseOptions->{$category};
        $itemName = (string)$parseOptions->attributes()->item;
        $items=$xml->xpath($itemName);

        $count = 0;

        foreach ($items as $item) {
            $count++;
            $item->current_no = $count;
                if($parseItem=$this->parseItem((array)$item, $parseOptions)) {
                    $price[$category][] =$parseItem;
                }
        }
        /*foreach ($items as $item) {
            $count++;
            $item->current_no = $count;
            if($parseItem=$this->parseItem((array)$item, $parseOptions)) {
                $price[$category][] =$parseItem;
            }
        }
        */
        $this->parsedData = $price;
        $this->process($price);
        return $price;
    }

    protected function parseStockInfo($item, $options)
    {

        $cost=$item[$options['field_cost']];
        if(empty($cost)&&isset($options['field_cost_if_empty'])) $cost=$item[$options['field_cost_if_empty']];
        $qty=$item[$options['field_qty']];
        $stock_id=KolesaFieldMapping::getStock($options, $options['stock']);
        if($stock_id==7) return [];
        KolesaFieldConvert::convert($cost,['removecomma','digit']);
        KolesaFieldConvert::convert($qty,['digit']);
        if($stock_id) {
            $stockInfo = array(
                'stock' => $options['stock'],
                'stock_id' => $stock_id,
                'qty' => (int)$qty,
                'cost' => (float)$cost,
            );
        }

        return $stockInfo;
    }


    public function updateStocks($rows)
    {
        $costs = [];

        array_walk($rows, function (&$row, $key) use (&$costs) {
            $product_id=NULL;
            $error=FALSE;
            if(empty($row['VendorCode'])){
                KolesaPricerLog::addLog('error', 'updateStocks','Empty Vendor Code for position '.$row['Name'],$row);
                $error=TRUE;
            }
            $productFactory=KolesaProductFactory::build($row['Category']);
            if(empty($row['BrandObject'])){
                if($brand_id=KolesaFieldMapping::getBrand($row)) $row['BrandObject']=$productFactory::loadBrand($brand_id);
            }

            if(empty($row['Product'])&&!empty($row["VendorCode"])) {
                $product_id=KolesaFieldMapping::getProduct($row);
            }
            if(empty($row['Stocks'])){
                KolesaPricerLog::addLog('error', 'updateStocks','Empty Stocks for position '.$row['Name'],$row);
                $error=TRUE;
            }

            if ($error==FALSE) {

                foreach ($row['Stocks'] as $cost_data) {

                    if(empty($cost_data['stock_id'])){
                        KolesaPricerLog::addLog('error', 'updateStocks','Empty Stock id for position '.$row['Name'],$row);
                    }

                    if (!empty($cost_data['stock_id']) && !empty($cost_data['cost'])) {

                        if($checkQty=$this->getQty($row['VendorCode'],$cost_data['stock_id'])){
                            $qty=$checkQty;
                        } else {
                            $qty=KolesaFieldConvert::convert($cost_data['qty'], 'digit');
                        }

                        $costs[] = array(
                            'id'=>!empty($product_id)?$product_id:NULL,
                            'cae' => $row['VendorCode'],
                            'category'=>$row['Category'],
                            'brand'=>!empty($row['BrandObject'])?$row['BrandObject']->id:NULL,
                            'stock' => $cost_data['stock_id'],
                            'qty' => $qty,
                            'stock_price' => $cost_data['cost'],
                            'active' => 1,
                        );
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


    }

}