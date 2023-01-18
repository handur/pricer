<?php

namespace Pricer\Load;

interface LoadDefaultInterface{
    public function load();
}

Class LoadDefault implements LoadDefaultInterface{
    public $name='default';
    protected $requiredOptions=array();
    protected $parseOptions=array();
    protected $options;
    public function __construct(array $options){
        if($check=self::checkParams($options, $this->requiredOptions)){
                echo ('Не заданы обязательные опции: '.implode(", ",array_keys($check)));
        }
        $this->options=array_merge(array_flip($this->requiredOptions),$options);
    }

    public function getName() {
        return $this->name;
    }

    public function load(){
    }

    protected function checkParams(array $options, array $requiredOptions){
        return array_diff_key(array_flip($requiredOptions),$options);
    }

}