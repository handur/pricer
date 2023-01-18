<?php
Class YstVendorPriceUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadCSV';
    protected $options=[

        'tyres_products'=>[
            'load url'=>'http://pricer.kolesa812.ru/prices/yst_vendor.csv',
            'file id'=>'yst_vendor_current',
            'options file'=>'yst_vendor.xml',
     #       'unique'=>['Brand','Model','VendorCode'],
            'create'=>['brands'=>FALSE,'models'=>FALSE,'products'=>FALSE],
            'update'=>['products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'products',
            'update time'=>120
        ],
    ];

    protected $shipperName='yst_vendor';


    public function parseData(){
        $csv=parent::getData($this->type);

        $category = $this->curOptions['category'];
        $price = array();
        $parseOptions = $this->parseOptions->{$category};

        $count = 0;
        foreach ($csv as $item) {
            $count++;
            if($parseItem=$this->parseItem($item, $parseOptions)) {
                $price[$category][] =$parseItem;
            }
        }

        $this->parsedData = $price;
        $this->process($price);
        return $price;


    }



}