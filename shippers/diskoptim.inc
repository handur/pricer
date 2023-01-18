<?php

class DiskoptimPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadFTP';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[
        'mapping'=>[
            'load url'=>'Diskoptim Ostatki Sklad.xml',
            'file id'=>'diskoptim_full',
            'login'=>'00-00000515',
            'pass'=>'onaMI32h',
            'host'=>'diskoptim.ru',
            'options file'=>'diskoptim.xml',
            'category'=>'Wheels',
            'queued'=>'mapping',
        ],
        'products'=>[
            'load url'=>'Diskoptim Ostatki Sklad.xml',
            'file id'=>'diskoptim_full',
            'login'=>'00-00000515',
            'pass'=>'onaMI32h',
            'host'=>'diskoptim.ru',
            'options file'=>'diskoptim.xml',
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
            'category'=>'Wheels',
            'queued'=>'products',
            'unique'=>['VendorCode'],
        ],
        'price'=>[
            'load url'=>'Diskoptim Ostatki Sklad.xml',
            'file id'=>'diskoptim_full',
            'login'=>'00-00000515',
            'pass'=>'onaMI32h',
            'host'=>'diskoptim.ru',
            'options file'=>'diskoptim.xml',
            'category'=>'Wheels',
            'queued'=>'price',
        ],
    ];
    protected $shipperName='diskoptim';


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
                $price[$category][] = $this->parseItem((array)$item, $parseOptions);
            }
        }

        $this->parsedData = $price;
        $this->process($price);
        return $price;
    }

}