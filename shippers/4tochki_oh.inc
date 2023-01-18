<?php
Class FortochkiOhPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[
        /*
        'tyres_products'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/M26924.xml',
            'file id'=>'4tochki_oh_tires_products',
            'options file'=>'4tochki_oh_products.xml',
            #       'unique'=>['Brand','Model','VendorCode'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            #'update'=>['products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'products',
            'update time'=>120
        ],
        'wheels_products'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/M26924.xml',
            'file id'=>'4tochki_oh_tires_products',
            'options file'=>'4tochki_oh_products.xml',
            #     'unique'=>['Brand','Model','VendorCode'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            #'update'=>['products'=>TRUE],
            'category'=>'Wheels',
            'queued'=>'products',
            'update time'=>120
        ],
        */
        'tyres_price'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/M26924.xml',
            'file id'=>'4tochki_oh_tires_current',
            'options file'=>'4tochki_oh_price.xml',
            'type'=>'price',
            'category'=>'Tyres',
            'queued'=>'price',
            'update time'=>30
        ],
        'wheels_price'=>[
            'load url'=>'https://b2b.4tochki.ru/export_data/M26924.xml',
            'file id'=>'4tochki_oh_wheels_current',
            'options file'=>'4tochki_oh_price.xml',
            'type'=>'price',
            'category'=>'Wheels',
            'queued'=>'price',
            'update time'=>30
        ],
    ];
    protected $shipperName='4tochki_oh';


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


    protected function parseStockInfo($item, $options)
    {

        $cost=$item[$options['field_cost']];
        if(empty($cost)&&isset($options['field_cost_if_empty'])) $cost=$item[$options['field_cost_if_empty']];
     #   $b2b_cost=$item[$options['field_b2b']];
     #   if(empty($b2b_cost)) $b2b_cost=0;

        $qty=$item[$options['field_qty']];
        $stock_id=KolesaFieldMapping::getStock($options, $options['stock']);

        KolesaFieldConvert::convert($cost,['removecomma','digit']);
        KolesaFieldConvert::convert($qty,['digit']);
        if($stock_id) {
            $stockInfo = array(
                'stock' => $options['stock'],
                'stock_id' => $stock_id,
                'qty' => (int)$qty,
                'cost' => (float)$cost,
            #    'b2b'=>(float)$b2b_cost,
            );
        }

        return $stockInfo;
    }


    public function updateStocks($rows)
    {
        $costs = [];

        array_walk($rows, function (&$row, $key) use (&$costs) {
            $error=FALSE;
            $productFactory=KolesaProductFactory::build($row['Category']);
            if(empty($row['BrandObject'])){
                if($brand_id=KolesaFieldMapping::getBrand($row)) $row['BrandObject']=$productFactory::loadBrand($brand_id);
            }

            if(empty($row['Product'])&&!empty($row["VendorCode"])) {
                $product_id=KolesaFieldMapping::getProduct($row);
            }
            if(empty($row['VendorCode'])){
                KolesaPricerLog::addLog('error', 'updateStocks','Empty Vendor Code for position '.$row['Name'],$row);
                $error=TRUE;
            }
            if(empty($row['Stocks'])){
                KolesaPricerLog::addLog('error', 'updateStocks','Empty Stocks for position '.$row['Name'],$row);
                $error=TRUE;
            }

            if ($error==FALSE) {
                foreach ($row['Stocks'] as $cost_data) {
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
                            'stock' => $cost_data['stock_id'],
                            'qty' => $qty,
                            'stock_price' => $cost_data['cost'],
                         #   'b2b'=>$cost_data['b2b'],
                            'active' => 1,
                        );
                    }
                }
            }
        });

        /* Обновление цен*/


        $costs_chunk = array_chunk($costs, 200);
        foreach ($costs_chunk as $chunk) {
            $query = db_insert('pricer_price')->fields(['id','cae','category', 'stock', 'qty', /*'b2b',*/'stock_price', 'active']);
            foreach ($chunk as $cost) {
                $query->values($cost);
            }
            $query->execute();
        }


    }

}