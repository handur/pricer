<?php
Class TrekTyrePricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[
        'mapping'=>[
            'load url'=>'https://www.spb.trektyre.ru/load-price-xml?url=862fc2e25b1b328e430333728c7cb177&oplata=1',
            'file id'=>'trektyre_full',
            'options file'=>'trektyre.xml',
            'category'=>'Tyres',
            'queued'=>'mapping',
        ],
        'products'=>[
            'load url'=>'https://www.spb.trektyre.ru/load-price-xml?url=862fc2e25b1b328e430333728c7cb177&oplata=1',
            'file id'=>'trektyre_full',
            'options file'=>'trektyre.xml',
            'exclude'=>['Image','Stocks'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
         #   'update'=>['products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'products',
            'unique'=>['VendorCode'],
        ],
        'price'=>[
            'load url'=>'https://www.spb.trektyre.ru/load-price-xml?url=862fc2e25b1b328e430333728c7cb177&oplata=1',
            'file id'=>'trektyre_full',
            'options file'=>'trektyre.xml',
            'category'=>'Tyres',
            'queued'=>'price',
        ],


    ];
    protected $shipperName='trektyre';


    public function parseData(){
        $xml=parent::getData($this->type);

        $price=array();
        $groups=array();

        foreach($this->parseOptions as $category=>$parseOptions){
            $groups[(string)$parseOptions->attributes()->group]=$category;
        }

        foreach($xml->product as $item){
            $itemType=(string)$item->attributes()->type;
            if($catName=$groups[$itemType]){
                if($parsedItem=$this->parseItem((array)$item,$this->parseOptions->$catName))  $price[$catName][]=$parsedItem;

            } else continue;
        }

        $this->parsedData=$price;
        $this->process($price);
        return $price;
    }
}