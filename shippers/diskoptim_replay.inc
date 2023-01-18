<?php

class DiskoptimReplayPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadFTP';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[
        'products'=>[
            'load url'=>'PodZakaz_Replay.xml',
            'file id'=>'diskoptim_replay_full',
            'login'=>'00-00000515',
            'pass'=>'onaMI32h',
            'host'=>'diskoptim.ru',
            'options file'=>'diskoptim_replay.xml',
            'category'=>'Wheels',
            'create'=>['brands'=>FALSE,'models'=>FALSE,'products'=>TRUE],
            'update'=>['products'=>FALSE],
            'queued'=>'products',
        ],
        'price'=>[
            'load url'=>'PodZakaz_Replay.xml',
            'file id'=>'diskoptim_replay_full',
            'login'=>'00-00000515',
            'pass'=>'onaMI32h',
            'host'=>'diskoptim.ru',
            'options file'=>'diskoptim_replay.xml',
            'category'=>'Wheels',
            'queued'=>'price',
        ],


    ];
    protected $shipperName='diskoptim_replay';


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

    public function parseStockInfo($item, $options)
    {

        $cost=$item[$options['field_cost']];
        KolesaFieldConvert::convert($cost,'digit');
        if($item[$options['field_qty']]=='*') $item[$options['field_qty']]=32;
        $qty=KolesaFieldConvert::convert($item[$options['field_qty']], 'digit');
        $stockInfo = array(
            'stock' => $options['stock'],
            'stock_id' => KolesaFieldMapping::getStock($options, $options['stock']),
            'qty' => (int)$qty,
            'cost' => (float)$cost,
        );

        return $stockInfo;
    }


}