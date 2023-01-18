<?php
Class TyresspbPricerUpdate extends KolesaPricerUpdate{
    /*
     * https://b2b.tyres.spb.ru/price/download?file_key=aa79be7b96b2ea37c5c95207379b1f95&good_alias=tyre&format=xml&settings[id_store]=IP,BA,2I,2V,P1,49,IL,2H,G1,G6,G2,2W,G5,2R,IT,38,PP,2M,2N,IS,G4,2O
     * */
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[

//      https://b2b.tyres.spb.ru/price/download?file_key=aa79be7b96b2ea37c5c95207379b1f95&good_alias=tyre&format=xml

        'mapping'=>[
            'load url'=>'https://b2b.tyres.spb.ru/price/download?file_key=aa79be7b96b2ea37c5c95207379b1f95&good_alias=tyre&format=xml&settings[id_store]=IP,BA,2I,2V,P1,49,IL,2H,G1,G6,G2,2W,G5,2R,IT,38,PP,2M,2N,IS,G4,2O',
            'file id'=>'tyresspb_full',
            'options file'=>'tyresspb.xml',
            'category'=>'Tyres',
            'queued'=>'mapping',
        ],
        'products'=>[
            'load url'=>'https://b2b.tyres.spb.ru/price/download?file_key=aa79be7b96b2ea37c5c95207379b1f95&good_alias=tyre&format=xml&settings[id_store]=IP,BA,2I,2V,P1,49,IL,2H,G1,G6,G2,2W,G5,2R,IT,38,PP,2M,2N,IS,G4,2O',
            'file id'=>'tyresspb_full',
            'options file'=>'tyresspb.xml',
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
          #  'update'=>['products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'products',
            'unique'=>['VendorCode'],
        ],
        'price'=>[
            'load url'=>'https://b2b.tyres.spb.ru/price/download?file_key=aa79be7b96b2ea37c5c95207379b1f95&good_alias=tyre&format=xml&settings[id_store]=IP,BA,2I,2V,P1,49,IL,2H,G1,G6,G2,2W,G5,2R,IT,38,PP,2M,2N,IS,G4,2O',
            'file id'=>'tyresspb_full',
            'options file'=>'tyresspb.xml',
            'category'=>'Tyres',
            'queued'=>'price',
        ],
    ];

    protected $shipperName='tyresspb';


    public function parseData(){
        $xml=parent::getData($this->type);

        $category = $this->curOptions['category'];
        $price = array();
        $parseOptions = $this->parseOptions->{$category};
        $itemName = (string)$parseOptions->attributes()->item;
        $count = 0;

        foreach ($xml->goods->tyre as $item) {

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