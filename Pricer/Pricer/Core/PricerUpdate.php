<?php
namespace Pricer\Core;

interface PricerUpdateDefaultInterface{
    public function getData($type);
}

Class PricerUpdate implements PricerUpdateDefaultInterface{
    protected $loadMethod=NULL;
    protected $priceFormat=NULL;
    protected $options=[];
    protected $curOptions=[];
    protected $type=[];
    protected $parseOptions=[];
    protected $workDir;
    protected $shipperName;
    protected $updateType;
    protected $data=[];
    protected $parsedData=[];
    protected $file;
    public $loadMethodObj;
    public $priceFormatObj;


    public function __construct(){
        $this->workDir=$this->getDir();
    }
    public function preprocess(){

    }
    public function getShipperName(){
        return $this->shipperName;
    }
    public function getAllowedPrices(){
        return $this->options;
    }

    protected function getDir() {
        $rc = new \ReflectionClass(get_class($this));
        return dirname($rc->getFileName());
    }

    protected function loadOptions($updateType){
        if(isset($this->options[$updateType])) {
            $filePath = $this->workDir . '/' . $this->options[$updateType]['options file'];
            if(file_exists($filePath)) {
                $content=file_get_contents($filePath);
                return simplexml_load_string($content);

            }
        }
    }
    public function setFileFromContent($content){
        $this->file=$content;
    }
    public function setDataFromContent($content){
        $this->data=$content;
    }
    public function loadData($type,$returnFile=FALSE){


            $this->parseOptions = $this->loadOptions($type);
            $this->curOptions = $this->options[$type];
            $loadMethodName = empty($this->curOptions['loadMethod']) ? $this->loadMethod : $this->curOptions['loadMethod'];
            $priceFormatName = empty($this->curOptions['priceFormat']) ? $this->priceFormat : $this->curOptions['priceFormat'];
            if (class_exists($loadMethodName))
                $this->loadMethodObj = new $loadMethodName($this->curOptions);
            if (class_exists($priceFormatName))
                $this->priceFormatObj = new $priceFormatName($this->curOptions);


            if(empty($this->file)) {
                $file = $this->loadMethodObj->load();
            } else {
                $file=$this->file;
            }

        if(!empty($file)){
            if($returnFile) return $file;
            $this->data=$this->priceFormatObj->read($file);
            return $this->data;
        }
    }

    public function getData($type){
        $this->type=$type;
        if(!isset($this->options[$type])) {
            throw new \Exception('Не найдены параметры для загрузки '.$type);
            return NULL;
        }
        if(!empty($this->data)) return $this->data;
        else return $this->loadData($type);
    }

    public function parseData(){

    }

    protected function process(&$price){
        /* Make something*/
        return $price;
    }

    protected function processRow(&$row,$parseOptions){

    }

    protected function parseItem($item,$parseOptions){
        $parsedItem=array();
        $groupOptions=$this->prepareOptions($parseOptions);

        $groupOptions['row']=$item;
        $groupOptions['Shipper']=$parsedItem['Shipper']=$this->shipperName;
        $groupOptions['Category']=$parsedItem['Category']=$parseOptions->getName();

        foreach($parseOptions->children() as $field){
            $parseOptions=self::prepareOptions($field);

            $parseOptions['element']=$field->getName();

            $parseOptions=array_merge($groupOptions,$parseOptions);
            $parsedElement=NULL;
            $curEl=NULL;
            if(!empty($this->curOptions['exclude'])&&in_array($parseOptions['element'],$this->curOptions['exclude'])) continue;

            if($parseOptions['element']=='Addon'){
                $addonInfo=$this->parseAddonInfo($item,$parseOptions);
                $parsedItem['Addon'][$addonInfo['name']]=$addonInfo;
            }
            elseif($parseOptions['element']=='Stock'){
                $stockInfo=$this->parseStockInfo($item,$parseOptions);
                $parsedItem['Stocks'][$stockInfo['stock']]=$stockInfo;
            } else {
                $parseOptions['processed_row']=$parsedItem;
                if(isset($parseOptions['concat'])){
                    $concat_fields=preg_split("/[#]/",$parseOptions['concat']);
                    $parsedElements=[];
                    foreach($concat_fields as $parseField){
                        if (!empty($parseField) && isset($item[$parseField])) $curEl = $item[$parseField];
                        if($curEl) $parsedElements[] = $this->parseElement($curEl, $parseOptions);
                    }
                    if(!empty($parsedElements))  $parsedElement=implode(" ",$parsedElements);
                    else $parsedElement=NULL;
                } else {
                    $parseField = $parseOptions['field'];
                    if (!empty($parseField) && isset($item[$parseField])) $curEl = $item[$parseField];
                    elseif (isset($parseOptions['no'])) {
                        $curElField = array_keys($item)[$parseOptions['no']];
                        $curEl = $item[$curElField];
                    }
                    $parsedElement = $this->parseElement($curEl, $parseOptions);
                }
                if (!empty($parseOptions['include']) && !in_array($parsedElement, $parseOptions['include'])) return NULL;
                if (!empty($parseOptions['exclude']) && in_array($parsedElement, $parseOptions['exclude'])) return NULL;
                if (!empty($this->curOptions['required']) && in_array($parseOptions['element'], $this->curOptions['required']) && empty($parsedElement)) return NULL;
                $parsedItem[$parseOptions['element']]=$parsedElement;
            }

        }

        return $parsedItem;
    }
    protected static function prepareOptions($field){
        $options=[];
        foreach($field->attributes() as $key=>$value) $options[$key]=(string)$value;
        if(isset($field->convert)){
            foreach($field->convert as $convert){
                $options['convert'][]=array(
                    'from'=>(string)$convert->attributes()['from'],
                    'to'=>(string)$convert->attributes()['to']
                );
            }
        }
        if(isset($field->process)){
            foreach($field->process as $process){
                $process_options=[];
                foreach($process->attributes() as $key=>$value){
                    $process_options[(string)$key]=(string)$value;
                }
                $options['process'][]=$process_options;
            }
        }
        if(isset($field->include)){
            foreach($field->include as $inc){
                $options['include'][]=(string)$inc;
            }
        }
        if(isset($field->exclude)){
            foreach($field->exclude as $inc){
                $options['exclude'][]=(string)$inc;
            }
        }

        return $options;
    }
    protected function parseElement($elValue,$elOptions){

        if(isset($elOptions['preprocess'])){
            $preprocessRules=preg_split("[,]",$elOptions['preprocess']);
            FieldConvert::convert($elValue, $preprocessRules,$elOptions);
        }
        if(isset($elOptions['process'])){

        }
        if(isset($elOptions['convertType'])){
            FieldConvert::convert($elValue,$elOptions['convertType'],$elOptions);
        }
        if(empty($elValue)) $elValue=$elOptions['default'];
        return trim($elValue);
    }
    protected function parseStockInfo($item,$options){
        if(empty($options['stock'])){
            $stockName=$item[$options['field_stock']];
        } else $stockName=$options['stock'];
        $qty=$item[$options['field_qty']];
        $cost=$item[$options['field_cost']];
        $stockInfo=array(
            'stock'=>$stockName,
            'qty'=>$qty,
            'cost'=>$cost,
        );

        return $stockInfo;
    }
    protected function parseAddonInfo($item,$options){
        $addonInfo = array(
            'name' => $options['name'],
            'value'=>$item[$options['field']],
        );

        return $addonInfo;
    }
    protected function preprocessValue($rule,&$value){
        return $value;
    }

}
