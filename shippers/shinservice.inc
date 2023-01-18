<?php
Class ShinservicePricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[
        'mapping'=>[
            'load url'=>'https://duplo.shinservice.ru/xml/shinservice-b2b-15.xml?id=10001600&t=1612866696232',
            'file id'=>'shinservis_full',
            'options file'=>'shinservice.xml',
            'queued'=>'mapping',
        ],
        'products'=>[
            'load url'=>'https://duplo.shinservice.ru/xml/shinservice-b2b-15.xml?id=10001600&t=1612866696232',
            'file id'=>'shinservis_full',
            'options file'=>'shinservice.xml',
            'exclude'=>['Stocks'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
            'queued'=>'products',
            'unique'=>['VendorCode'],
        ],
        'price'=>[
            'load url'=>'https://duplo.shinservice.ru/xml/shinservice-b2b-15.xml?id=10001600&t=1612866696232',
            'file id'=>'shinservis_full',
            'options file'=>'shinservice.xml',
            'queued'=>'price',
        ],
    ];
    protected $shipperName='shinservice';

    public function parseData(){
        $data=parent::getData($this->type);
        $price=array();

        foreach($this->parseOptions as $category=>$parseOptions){
            $groups[(string)$parseOptions->attributes()->group]=$category;
        }

        foreach($data as $category=>$items){
            $parsedItems=array();
            if ($catName = $groups[$category]) {
                foreach ($items as $item) {
                    $attributes = $this->prepareOptions($item);
                    $parsedItems[] = $this->parseItem($attributes, $this->parseOptions->$catName);
                }
                $price[$catName] = $parsedItems;
            } else continue;
        }
        $this->parsedData=$price;
        $this->process($price);
        return $price;
    }



}