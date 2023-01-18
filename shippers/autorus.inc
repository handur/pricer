<?php

/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 18.05.2021
 * Time: 11:32
 */
class AutorusPricerUpdate extends KolesaPricerUpdate{
    /*
     * https://b2b.tyres.spb.ru/price/download?file_key=aa79be7b96b2ea37c5c95207379b1f95&good_alias=tyre&format=xml&settings[id_store]=IP,BA,2I,2V,P1,49,IL,2H,G1,G6,G2,2W,G5,2R,IT,38,PP,2M,2N,IS,G4,2O
     * */
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXLSX';
    protected $options=[
        'mapping'=>[
            'load url'=>'public://process/AUTORUSshina.xlsx',
            'file id'=>'autorus_full',
            'options file'=>'autorus.xml',
            'header'=>TRUE,
            'category'=>'Tyres',
            'queued'=>'mapping',
#            'skiprows'=>2,
        ],
        'products'=>[
            'load url'=>'public://process/AUTORUSshina.xlsx',
            'file id'=>'autorus_full',
            'options file'=>'autorus.xml',
            'header'=>TRUE,
            'unique'=>['VendorCode'],
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
       #     'update'=>['products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'products',
        ],
        'price'=>[
            'load url'=>'public://process/AUTORUSshina.xlsx',
            'file id'=>'autorus_full',
            'options file'=>'autorus.xml',
            'header'=>TRUE,
  #          'skiprows'=>2,
         #   'create'=>['products'=>TRUE],
            'category'=>'Tyres',
            'queued'=>'price',
        ],
    ];

    protected $shipperName='autorus';

    public function preprocess(){
        $mails=KolesaGetMail::getMail('price@autorus.ru');
        foreach($mails['price@autorus.ru'] as $mail){
            foreach($mail['attachments'] as $at) {
                $saved = $at['saved'];

                $check_string=str_replace("public://email/","",$saved);
                if(preg_match('/AUTORUSshina\.zip/', $check_string)){
                    $zip = new ZipArchive;
                    $res=$zip->open(drupal_realpath($saved));
                    if ($res === TRUE) {
                        $zip->extractTo("public://process/");
                        $zip->close();

                    }
                }
            }
        }

        KolesaPricerLog::addLog('message','Load Preprocess Function','Autorus preprocessed - get mail');
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