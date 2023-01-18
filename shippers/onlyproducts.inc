<?php

/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 28.05.2021
 * Time: 12:03
 */
class OnlyProductsPricerUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXLSX';
    protected $options=[
        #http://webmim.svrauto.ru/api/v1/catalog/unload?access-token=d_isRyjVBf-k7fBFtvNVVdNIZzZIuUxh&format=xml&types=1%3B2%3B3%3B4&sterritoryid=85858701837%3B85858701785%3B85859922199%3B91285579565%3B86179049108%3B85977251372%3B86253221543%3B87177593317%3B86920786711%3B85859921835
#/var/www/www-root/data/www/pricer.kolesa812.ru/sites/default/files/email/buywheel_2805.xls
        'mapping'=>[
            'load url'=>'public://process/products.xlsx',
            'file id'=>'onlyproducts_full',
            'options file'=>'onlyproducts.xml',
            'header'=>TRUE,
            'category'=>'Tyres',
            #'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
         #   'queued'=>'mapping',
#            'skiprows'=>2,
        ],
        'products'=>[
            'load url'=>'public://process/products.xlsx',
            'file id'=>'onlyproducts_full',
            'options file'=>'onlyproducts.xml',
            'header'=>TRUE,
            'category'=>'Tyres',
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
            'update'=>['products'=>TRUE],
            #   'queued'=>'mapping',
#            'skiprows'=>2,
        ],

    ];
    protected $shipperName='onlyproducts';



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



}