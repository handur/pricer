<?php
Class KolradPricerUpdate extends KolesaPricerUpdate{
    /*
     * https://b2b.tyres.spb.ru/price/download?file_key=aa79be7b96b2ea37c5c95207379b1f95&good_alias=tyre&format=xml&settings[id_store]=IP,BA,2I,2V,P1,49,IL,2H,G1,G6,G2,2W,G5,2R,IT,38,PP,2M,2N,IS,G4,2O
     * */
    public $loadMethod='Pricer\Load\LoadContent';
    public $priceFormat='Pricer\Read\ReadXLSX';
    protected $options=[
        'mapping'=>[
            #'load url'=>'https://kolrad.us16.list-manage.com/track/click?u=a87f27239ca841ee6b98104a5&id=545518ed8d&e=ce1fdf1a4f',
            'load url'=>'public://process/kolrad.xlsx',
            'file id'=>'kolrad_full',
            'options file'=>'kolrad.xml',
            'header'=>TRUE,
            'category'=>'Wheels',
            'queued'=>'mapping',
        ],
        'products'=>[
          #  'load url'=>'https://kolrad.us16.list-manage.com/track/click?u=a87f27239ca841ee6b98104a5&id=545518ed8d&e=ce1fdf1a4f',
            'load url'=>'public://process/kolrad.xlsx',
            'file id'=>'kolrad_full',
            'options file'=>'kolrad.xml',
            'header'=>TRUE,
            'category'=>'Wheels',
            'create'=>['brands'=>TRUE,'models'=>TRUE,'products'=>TRUE],
     #       'update'=>['products'=>TRUE],
            'queued'=>'products',
        ],
        'price'=>[
            #'load url'=>'https://kolrad.us16.list-manage.com/track/click?u=a87f27239ca841ee6b98104a5&id=545518ed8d&e=ce1fdf1a4f',
            #'load url'=>'https://drive.google.com/uc?export=download&id=1GaQCAnrdntwuLdn53kX2FhGrXNhmQNw4',
            'load url'=>'public://process/kolrad.xlsx',
            'file id'=>'kolrad_full',
            'header'=>TRUE,
            'category'=>'Wheels',
            'options file'=>'kolrad.xml',
            'queued'=>'price',
        ],
    ];

    protected $shipperName='kolrad';

    public function preprocess()
    {
        $url = "https://drive.google.com/uc?export=download&id=1GaQCAnrdntwuLdn53kX2FhGrXNhmQNw4";
        $result = drupal_http_request($url);
        if(!empty($result->headers['location'])){
            system_retrieve_file($result->headers['location'],"public://process/kolrad.xlsx",FALSE,FILE_EXISTS_REPLACE);
            KolesaPricerLog::addLog('message','Load Preprocess Function','KolRad preprocessed - get file',$result);
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
            if(empty($item['ТМЦ'])) continue;
            if($parseItem=$this->parseItem($item, $parseOptions)) {
                $price[$category][] =$parseItem;
            }
        }
        $this->parsedData = $price;
        $this->process($price);
        return $price;
    }



}