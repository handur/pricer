<?php
Class KolesaDaromPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXML';

    protected  $options=[
        'mapping'=>[
            'load url'=>'https://kolesa812.ru/import/kolesadarom/price.xml',
            'file id'=>'kolesadarom_full',
            'options file'=>'kolesa_darom.xml',
            'queued'=>'mapping',
        ],
        'products'=>[
            'load url'=>'https://kolesa812.ru/import/kolesadarom/price.xml',
            'file id'=>'kolesadarom_full',
            'options file'=>'kolesa_darom.xml',
            'unique'=>['VendorCode'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
          #  'update'=>['products'=>TRUE],
            'queued'=>'products',
        ],
        'price'=>[
            'load url'=>'https://kolesa812.ru/import/kolesadarom/price.xml',
            'file id'=>'kolesadarom_full',
            'options file'=>'kolesa_darom_price.xml',
            'queued'=>'price',
        ],
    ];
    protected $shipperName='kolesa_darom';



    public function parseData(){
        $xml=parent::getData($this->type);
        $price=array();
        $groups=array();

        foreach($this->parseOptions as $category=>$parseOptions){
            $groups[(string)$parseOptions->attributes()->group]=$category;
        }

        foreach($xml as $category=>$items){
            $parsedItems=array();
            if($catName=$groups[$category]){
                foreach($items as $item){
                    $parsedItems[]=$this->parseItem((array)$item,$this->parseOptions->$catName);
                }
                $price[$catName]=$parsedItems;
            } else continue;
        }

        $this->parsedData=$price;
        $this->process($price);
        return $price;
    }

    protected function parseStockInfo($item,$options){
        $stockInfo=array(
            'stock'=>$item[$options['field_stock']],
            'stock_id'=>KolesaFieldMapping::getStock($options,$item[$options['field_stock']],'name'),
            'qty'=>$item[$options['field_qty']],
            'cost'=>$item[$options['field_cost']],
        );

        return $stockInfo;
    }

}