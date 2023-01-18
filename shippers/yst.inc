<?php

class YSTPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[
        'tyres_mapping'=>[
            'load url'=>'https://terminal.yst.ru/api/xml/tyre/xml/7f325fcd-dd02-4b23-9911-2d2954fd9d1e',
            'file id'=>'yst_tires_full',
            'options file'=>'yst.xml',
            'category'=>'Tyres',
            'queued'=>'mapping',
        ],
        'wheels_mapping'=>[
            'load url'=>'https://terminal.yst.ru/api/xml/disk/xml/7f325fcd-dd02-4b23-9911-2d2954fd9d1e',
            'file id'=>'yst_wheels_full',
            'options file'=>'yst.xml',
            #  'update'=>['products'=>TRUE],
            'category'=>'Wheels',
            'queued'=>'mapping',
        ],
        'tyres_products'=>[
            'load url'=>'https://terminal.yst.ru/api/xml/tyre/xml/7f325fcd-dd02-4b23-9911-2d2954fd9d1e',
            'file id'=>'yst_tires_full',
            'options file'=>'yst.xml',
            'exclude'=>['Stocks'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'products',
        ],
        'tyres_price'=>[
            'load url'=>'https://terminal.yst.ru/api/xml/tyre/xml/7f325fcd-dd02-4b23-9911-2d2954fd9d1e',
            'file id'=>'yst_tires_full',
            'options file'=>'yst.xml',
            'category'=>'Tyres',
            'type'=>'price',
            'queued'=>'price',
        ],
        'wheels_products'=>[
            'load url'=>'https://terminal.yst.ru/api/xml/disk/xml/7f325fcd-dd02-4b23-9911-2d2954fd9d1e',
            'file id'=>'yst_wheels_full',
            'options file'=>'yst.xml',
            'exclude'=>['Stocks'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
            'category'=>'Wheels',
            'queued'=>'products',
        ],
        'wheels_price'=>[
            'load url'=>'https://terminal.yst.ru/api/xml/disk/xml/7f325fcd-dd02-4b23-9911-2d2954fd9d1e',
            'file id'=>'yst_wheels_full',
            'options file'=>'yst.xml',
            'category'=>'Wheels',
            'type'=>'price',
            'queued'=>'price',
        ],


    ];
    protected $shipperName='yst';


    public function parseData(){
        $xml=parent::getData($this->type);
        $category=$this->curOptions['category'];
        $price=array();
        $parseOptions=$this->parseOptions->{$category};
        $itemName=(string)$parseOptions->attributes()->item;
        $count=0;
        foreach($xml as $item){
            $count++;
            $item->current_no=$count;

            if($item->getName()==$itemName) {

                if($parsedItem=$this->parseItem((array)$item, $parseOptions)) {

                    $price[$category][]=$parsedItem;
                }
            }
        }

        $this->parsedData=$price;
        $this->process($price);
        return $price;
    }

}