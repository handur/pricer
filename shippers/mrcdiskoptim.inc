<?php

class MrcDiskoptimPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadFTP';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[

        'price'=>[
            'load url'=>'Diskoptim Ostatki Sklad.xml',
            'file id'=>'mrcdiskoptim_full',
            'login'=>'00-00000515',
            'pass'=>'onaMI32h',
            'host'=>'diskoptim.ru',
            'options file'=>'mrcdiskoptim.xml',
            'category'=>'Wheels',
            'queued'=>'price',
        ],


    ];
    protected $shipperName='mrcdiskoptim';


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