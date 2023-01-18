<?php
/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 24.02.2021
 * Time: 14:49
 */
Class ShinserviceZimaPricerMRCUpdate extends KolesaPricerUpdate{
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXLSX';
    protected $options=[
        'price'=>[
            #'load url'=>'https://www.shinservice.ru/file/shinservice_b2b_winter_121021.xlsx',
            'load url'=>'public://process/mrcshinservice_winter.xlsx',
            'header'=>TRUE,
            'options file'=>'mrc_shinservice2.xml',
            'file id'=>'mrc_shinservice2_full',
            'header'=>TRUE,
          #  'skiprows'=>2,
            'queued'=>'price',
            'category'=>'Tyres',

        ],


    ];
    protected $shipperName='mrcshinservice2';
    public function preprocess()
    {
        $url = "https://www.shinservice.ru/file/shinservice_b2b_winter_[date].xlsx";
        $starttime = time();
        $get = FALSE;
        $day = 0;
        while (!$get&&$day<10) {
            $date = date("dmy", $starttime);
            $parseUrl = str_replace("[date]", $date, $url);
            $file_headers = @get_headers($parseUrl);
            if (!$file_headers || $file_headers[0] == 'HTTP/1.1 404 Not Found') {
                $day++;
                $starttime = strtotime("-" . $day . " day");
                $get = FALSE;
            } else {
                $get = TRUE;
            }
            KolesaPricerLog::addLog('message', 'Load Preprocess Function', 'Autorus preprocessed - get mail');
        }
        if($get&&!empty($parseUrl)){
            system_retrieve_file($parseUrl,"public://process/mrcshinservice_winter.xlsx",FALSE,FILE_EXISTS_REPLACE);
            KolesaPricerLog::addLog('message','Load Preprocess Function','MRC SHINSERVICE WINTER preprocessed - get file');
        } else {
            KolesaPricerLog::addLog('error','Load Preprocess Function','MRC SHINSERVICE WINTER preprocessed - not file');
        }


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

    protected function parseStockInfo($item,$options){
        $costColumn=array_keys($item)[$options['field_cost']];

        $stockInfo=array(
            'stock'=>$options['stock'],
            'stock_id'=>KolesaFieldMapping::getStock($options,$options['stock']),
            'qty'=>4,
            'cost'=>$item[$costColumn],
        );

        return $stockInfo;
    }
}