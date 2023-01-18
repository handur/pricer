<?php
namespace Pricer\Read;

Class ReadXML extends ReadDefault{
    public $name='XML';
    public function read($file){
        $xml = new \SimpleXMLElement($file);
        return $xml;

    }
}