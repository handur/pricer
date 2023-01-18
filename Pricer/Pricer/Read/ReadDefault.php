<?php
namespace Pricer\Read;

interface ReadDefaultInterface{
    public function read($file);
}


Class ReadDefault implements ReadDefaultInterface {
    public $name='default';
    public function __construct(array $options){
        $this->options=$options;
    }
    public function getName() {
        return $this->name;
    }
    public function read($file){

    }
}

