<?php

/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 28.05.2021
 * Time: 12:03
 */
class KamaPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXLS2';
    protected $options=[
        #http://webmim.svrauto.ru/api/v1/catalog/unload?access-token=d_isRyjVBf-k7fBFtvNVVdNIZzZIuUxh&format=xml&types=1%3B2%3B3%3B4&sterritoryid=85858701837%3B85858701785%3B85859922199%3B91285579565%3B86179049108%3B85977251372%3B86253221543%3B87177593317%3B86920786711%3B85859921835
#/var/www/www-root/data/www/pricer.kolesa812.ru/sites/default/files/email/buywheel_2805.xls
        'mapping'=>[
            'load url'=>'public://process/kama2.xls',
            'file id'=>'kama_full',
            'options file'=>'kama.xml',
            'header'=>TRUE,
            'category'=>'Tyres',
            'queued'=>'mapping',
#            'skiprows'=>2,
        ],
        'products'=>[
            'load url'=>'public://process/kama2.xls',
            'file id'=>'kama_full',
            'options file'=>'kama.xml',
            'header'=>TRUE,
            #           'skiprows'=>2,
       #     'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'products',
        ],
        'price'=>[
            'load url'=>'public://process/kama2.xls',
            'file id'=>'kama_full',
            'options file'=>'kama.xml',
            'header'=>TRUE,
            #          'skiprows'=>2,
           # 'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'price',
        ],

    ];
    protected $shipperName='kama';

    public function preprocess(){
        $mails=KolesaGetMail::getMail('spb-kama@yandex.ru');
        foreach($mails['spb-kama@yandex.ru'] as $mail){
            foreach($mail['attachments'] as $at) {
                $saved = $at['saved'];
                $check_string=str_replace("public://email/","",$saved);
                if(strpos($check_string,".xls")){
                    copy($saved,"public://process/kama.xls");
                }
            }
        }

        KolesaPricerLog::addLog('message','Load Preprocess Function','Kama preprocessed - get mail');
    }

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
            'qty' => $item[$options['field_qty']],
            'cost' => $cost,
        );

        return $stockInfo;
    }

}