<?php
namespace Pricer\Load;

Class LoadUrl extends LoadDefault {
    public $name='URL';
    protected $url;
    protected $requiredOptions=array('load url');
    public function __construct(array $options){
        parent::__construct($options);
        $this->url=$options['load url'];
    }
    public function load(){
        return $this->url;
    }
}

