<?php
//http://webmim.svrauto.ru/api/v1/catalog/unload?access-token=d_isRyjVBf-k7fBFtvNVVdNIZzZIuUxh&format=xml&types=1%3B2%3B3%3B4&sterritoryid=88431040556%3B88431040558%3B85859921835


class SeverPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXML';
    protected $options=[
        #http://webmim.svrauto.ru/api/v1/catalog/unload?access-token=d_isRyjVBf-k7fBFtvNVVdNIZzZIuUxh&format=xml&types=1%3B2%3B3%3B4&sterritoryid=85858701837%3B85858701785%3B85859922199%3B91285579565%3B86179049108%3B85977251372%3B86253221543%3B87177593317%3B86920786711%3B85859921835
        'mapping'=>[
            #'load url'=>'http://webmim.svrauto.ru/api/v1/catalog/unload?access-token=d_isRyjVBf-k7fBFtvNVVdNIZzZIuUxh&format=xml&types=1%3B2%3B3%3B4&sterritoryid=85858701837%3B85858701785%3B85859922199%3B91285579565%3B86179049108%3B85977251372%3B86253221543%3B87177593317%3B86920786711%3B85859921835',
            'load url'=>"public://process/sever.xml",
            'file id'=>'sever_full',
            'options file'=>'sever.xml',
            'queued'=>'mapping',


        ],
        'products'=>[
            #'load url'=>'http://webmim.svrauto.ru/api/v1/catalog/unload?access-token=d_isRyjVBf-k7fBFtvNVVdNIZzZIuUxh&format=xml&types=1%3B2%3B3%3B4&sterritoryid=85858701837%3B85858701785%3B85859922199%3B91285579565%3B86179049108%3B85977251372%3B86253221543%3B87177593317%3B86920786711%3B85859921835',
            'load url'=>"public://process/sever.xml",
            'file id'=>'sever_full',
            'options file'=>'sever.xml',
            'exclude'=>['Stocks'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>FALSE],
            'required'=>['Brand','Model'],
            'queued'=>'products',
          #  'unique'=>['VendorCode'],
            'update time'=>60
        ],
        'price'=>[
#            'load url'=>'http://webmim.svrauto.ru/api/v1/catalog/unload?access-token=d_isRyjVBf-k7fBFtvNVVdNIZzZIuUxh&format=xml&types=1%3B2%3B3%3B4&sterritoryid=85858701837%3B85858701785%3B85859922199%3B91285579565%3B86179049108%3B85977251372%3B86253221543%3B87177593317%3B86920786711%3B85859921835',
            'load url'=>"public://process/sever.xml",
            'file id'=>'sever_full',
            'options file'=>'sever.xml',
            'queued'=>'price',
         #   'unique'=>['VendorCode'],
            'required'=>['Brand','Model'],
            'update time'=>60
        ],



    ];
    protected $shipperName='sever';

    public function preprocess()
    {
        $url = "https://webmim.svrauto.ru/api/v1/catalog/unload?access-token=DfpR6RGi6Hm4C2EJNbL3mQQC4evmKpSd&format=xml&types=1%3B2%3B3%3B4";
        system_retrieve_file($url,"public://process/sever.xml",FALSE,FILE_EXISTS_REPLACE);
        KolesaPricerLog::addLog('message','Load Preprocess Function','Sever preprocessed - get file');
    }

    public function parseData(){
        $xml=parent::getData($this->type);
        $price=array();
        $groups=array();

        foreach($this->parseOptions as $category=>$parseOptions){
            $groups[(string)$parseOptions->attributes()->group]=$category;
        }

        foreach($xml->COMMODITIES as $group){

            $groupName=(string)$group->attributes()->NAME;
            if($catName=$groups[$groupName]){

                $parsedItems=[];
                foreach($group as $item){
                    if($item->getName()=='COMMODITY') {
                        $parsedItem=$this->parseItem((array)$item, $this->parseOptions->$catName);
                         if($parsedItem!=NULL)                       $parsedItems[] = $parsedItem;
                    }
                }
                $price[$catName]=$parsedItems;
            } else continue;
        }


        $this->parsedData=$price;
        $this->process($price);
        return $price;
    }
    protected function parseStockInfo($item,$options){
        $stock=$item[$options['field_stock']];
        $stockInfo=array(
            'stock'=>$stock,
            'stock_id'=>KolesaFieldMapping::getStock($options,$stock,'name'),
            'qty'=>$item[$options['field_qty']],
            'cost'=>$item[$options['field_cost']],
        );

        return $stockInfo;
    }
}