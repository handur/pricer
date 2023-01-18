<?php
//http://webmim.svrauto.ru/api/v1/catalog/unload?access-token=d_isRyjVBf-k7fBFtvNVVdNIZzZIuUxh&format=xml&types=1%3B2%3B3%3B4&sterritoryid=88431040556%3B88431040558%3B85859921835


class Kolesa812PricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[
        #http://webmim.svrauto.ru/api/v1/catalog/unload?access-token=d_isRyjVBf-k7fBFtvNVVdNIZzZIuUxh&format=xml&types=1%3B2%3B3%3B4&sterritoryid=85858701837%3B85858701785%3B85859922199%3B91285579565%3B86179049108%3B85977251372%3B86253221543%3B87177593317%3B86920786711%3B85859921835
        'mapping'=>[
            'load url'=>'https://kolesa812.ru/sites/default/files/1c_import/kolesa_stock_new.xml',
            'file id'=>'kolesa812_full',
            'options file'=>'kolesa812.xml',
            'queued'=>'mapping',

        ],
        'products'=>[
            'load url'=>'https://kolesa812.ru/sites/default/files/1c_import/kolesa_stock_new.xml',
            'file id'=>'kolesa812_full',
            'options file'=>'kolesa812.xml',
            'exclude'=>['Stocks'],
            'create'=>['models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
         #   'required'=>['Brand','Model'],
            'queued'=>'products',

        ],
        'price'=>[
            'load url'=>'https://kolesa812.ru/sites/default/files/1c_import/kolesa_stock_new.xml',
            'file id'=>'kolesa812_full',
            'options file'=>'kolesa812.xml',
            'queued'=>'price',
        ],



    ];
    protected $shipperName='kolesa812';


    public function parseData(){
        $xml=parent::getData($this->type);

        $price=array();
        $groups=array();

        foreach($this->parseOptions as $category=>$parseOptions){
            $groups[$category]=(string)$parseOptions->attributes()->group;
        }

        foreach($groups as $catName=>$group){
            if($groupXml=$xml->$group){
                $parsedItems=[];
                foreach($groupXml as $items){
                    foreach($items as $item){
                        if($parsedItem=$this->parseItem((array)$item, $this->parseOptions->$catName)) {

                            if(!empty($parsedItem['Category']))                           $parsedItems[] = $parsedItem;
                        }
                    }
                }
                $price[$catName]=$parsedItems;
            }
        }




        $this->parsedData=$price;
        $this->process($price);
        return $price;
    }
    protected function parseStockInfo($item,$options){
        $stock=$item[$options['field_stock']];

        $stockReplace=[
            'Балканская_склад'=>'balk',
            'Дальневосточный_склад'=>'daln',
            'Комендантский_склад'=>'kom',
            'Балканская_ПАУЕР_СОХ'=>'balk_oh',
            'Дальневосточный_ПАУЕР_СОХ'=>'daln_oh',
            'Комендантский_ПАУЕР_СОХ'=>'kom_oh',
            'Балканская_контейнер_ПАУЕР_СОХ'=>'balk_cont',
            'Балканская_контейнер'=>'balk_cont',
            'БОЛЬШЕВИКОВ_склад'=>'bolsh',
            'БОЛЬШЕВИКОВ_ПАУЕР_СОХ'=>'bolsh',
            'БОЛЬШЕВИКОВ_СМ_склад'=>'bolsh',
            'БОЛЬШЕВИКОВ_ИП_склад'=>'bolsh',
            'Балканская_склад_нокиан'=>'balk',
            'Дальневосточный_склад_нокиан'=>'daln',
            'Комендантский_склад_нокиан'=>'kom',
            'Контейнер'=>'balk',
            'Контейнер_нокиан'=>'balk',
            'Придорожная_склад'=>'prid',
            'Придорожная_склад_нокиан'=>'prid',
            'Придорожная_ПАУЕР_СОХ'=>'prid',
        ];
        if(isset($stockReplace[$stock])) $stock=$stockReplace[$stock];
        $cost=$item[$options['field_cost']];
        KolesaFieldConvert::convert($cost,['digit','removecomma']);
        $cost=ceil($cost);
        $stockInfo=array(
            'stock'=>$stock,
            'stock_id'=>KolesaFieldMapping::getStock($options,$stock),
            'qty'=>$item[$options['field_qty']],
            'cost'=>$cost,
        );

        return $stockInfo;
    }
}