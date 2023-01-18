<?php

/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 28.05.2021
 * Time: 12:03
 */
class SimbiomPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXLSX';
    protected $options=[
        #http://webmim.svrauto.ru/api/v1/catalog/unload?access-token=d_isRyjVBf-k7fBFtvNVVdNIZzZIuUxh&format=xml&types=1%3B2%3B3%3B4&sterritoryid=85858701837%3B85858701785%3B85859922199%3B91285579565%3B86179049108%3B85977251372%3B86253221543%3B87177593317%3B86920786711%3B85859921835
#/var/www/www-root/data/www/pricer.kolesa812.ru/sites/default/files/email/buywheel_2805.xls
        'products'=>[
            'load url'=>'public://process/simbiom.xlsx',
            'file id'=>'simbiom_full',
            'options file'=>'simbiom.xml',
            'exclude'=>['Stocks'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
            'skiprows'=>10,
            'category'=>'Tyres',
            'queued'=>'products'

        ],
        'price'=>[
            'load url'=>'public://process/simbiom.xlsx',
            'file id'=>'simbiom_full',
            'options file'=>'simbiom.xml',
            'header'=>TRUE,
            'skiprows'=>10,
            #  'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'price',
        ],

    ];
    protected $shipperName='simbiom';

    public function preprocess(){
        $mails=KolesaGetMail::getMail('e.alymova@simbiom.ru');
        foreach($mails['e.alymova@simbiom.ru'] as $mail){
            foreach($mail['attachments'] as $at) {
                $saved = $at['saved'];
                $check_string=str_replace("public://email/","",$saved);
                if(strpos($check_string,".xlsx")){
                    copy($saved,"public://process/simbiom.xlsx");
                }
            }
        }

        KolesaPricerLog::addLog('message','Load Preprocess Function','simbiom preprocessed - get mail');
    }

    public function parseData(){
        $xls=parent::getData($this->type);

        $category = $this->curOptions['category'];

        $price = array();
        $parseOptions = $this->parseOptions->{$category};

        $count = 0;
        foreach ($xls as $item) {
            $count++;
                if ($parseItem = $this->parseItem($item, $parseOptions)) {
                        if(!empty($parseItem['Name']))                          $price[$category][] = $parseItem;
                }

        }
        $this->parsedData = $price;
        $this->process($price);
        return $price;
    }
    protected function parseStockInfo($item, $options)
    {
        $costColumn=array_keys($item)[$options['field_cost']];
        $cost=$item[$costColumn];
        $cost=preg_replace("/[^0-9.,]/", '', $cost);
        $cost=preg_replace("/[,]/", '', $cost);

        KolesaFieldConvert::convert($cost,'digit');
        $cost=str_replace(".","",$cost);
        $qtyColumn=array_keys($item)[$options['field_qty']];
        $qty=KolesaFieldConvert::convert($item[$qtyColumn], 'digit');
        $stockInfo = array(
            'stock' => $options['stock'],
            'stock_id' => KolesaFieldMapping::getStock($options, $options['stock']),
            'qty' => (int)$qty,
            'cost' => (float)$cost,
        );

        return $stockInfo;
    }

}