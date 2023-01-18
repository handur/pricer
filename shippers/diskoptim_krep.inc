<?php

class DiskoptimKrepPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXLS2';
    protected $options=[
        'test_products'=>[
            'load url'=>'public://process/diskoptim_krep.xls',
            'file id'=>'diskoptim_krep_full',
            'options file'=>'diskoptim_krep.xml',
            'header'=>TRUE,
            'unique'=>['VendorCode'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['models'=>TRUE,'products'=>TRUE],
            #'queued'=>'products',
            'category'=>'Krep',
            'skiprows'=>0,
        ],
        'price'=>[
            'load url'=>'public://process/diskoptim_krep.xls',
            'file id'=>'diskoptim_krep_full',
            'options file'=>'diskoptim_krep.xml',
            'header'=>TRUE,
            'category'=>'Krep',
            'queued'=>'price',
        ],


    ];
    protected $shipperName='diskoptim_krep';


    public function parseData(){
        $xls=parent::getData($this->type);

        $price = array();

        $category = $this->curOptions['category'];

        $parseOptions = $this->parseOptions->{$category};

        $count = 0;
        $subCategory='';
        foreach ($xls as $item) {
            $count++;

            if($parseItem=$this->parseItem($item, $parseOptions)) {

                if(empty($parseItem['Name'])&&!empty($parseItem['Model'])){
                    $subCategory=$parseItem['Model'];
                }
                if(!empty($parseItem['Name'])) {
                    $parseItem['SubCategory']=$subCategory;
                    $price[$category][]=$parseItem;
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


        KolesaFieldConvert::convert($cost,['removecomma','digit']);
        $stockInfo = array(
            'stock' => $options['stock'],
            'stock_id' => KolesaFieldMapping::getStock($options, $options['stock']),
            'qty' => !empty($item[$options['field_qty']])?$item[$options['field_qty']]:0,
            'cost' => $cost*2,
        );

        return $stockInfo;
    }

}