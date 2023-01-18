<?php
Class YokohamaSportPricerUpdate extends KolesaPricerUpdate{
    /*
     * https://b2b.tyres.spb.ru/price/download?file_key=aa79be7b96b2ea37c5c95207379b1f95&good_alias=tyre&format=xml&settings[id_store]=IP,BA,2I,2V,P1,49,IL,2H,G1,G6,G2,2W,G5,2R,IT,38,PP,2M,2N,IS,G4,2O
     * */
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXLSX';
    protected $options=[
        'mapping'=>[
            'load url'=>'http://pricer.kolesa812.ru/prices/yokohama_sport.xlsx',
            'file id'=>'yokohama_sport_full',
            'options file'=>'yokohama_sport.xml',
            'header'=>TRUE,
            'category'=>'Tyres',
            'queued'=>'mapping',
            'skiprows'=>2,
        ],
        'products'=>[
            'load url'=>'http://pricer.kolesa812.ru/prices/yokohama_sport.xlsx',
            'file id'=>'yokohama_sport_full',
            'options file'=>'yokohama_sport.xml',
            'header'=>TRUE,
            'skiprows'=>2,
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'products',

        ],
        'price'=>[
            'load url'=>'http://pricer.kolesa812.ru/prices/yokohama_sport.xlsx',
            'file id'=>'yokohama_sport_full',
            'options file'=>'yokohama_sport.xml',
            'header'=>TRUE,
            'skiprows'=>2,
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'price',

        ],
    ];

    protected $shipperName='yokohama_sport';

    public function parseData(){
        $xls=parent::getData($this->type);

        $category = $this->curOptions['category'];
        $price = array();
        $parseOptions = $this->parseOptions->{$category};

        $count = 0;
        foreach ($xls as $item) {
            $count++;

            if($parseItem=$this->parseItem($item, $parseOptions)) {
                $price[$category][] =$parseItem;
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
            'qty' => 16,
            'cost' => $cost,
        );

        return $stockInfo;
    }

}