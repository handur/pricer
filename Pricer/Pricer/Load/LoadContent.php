<?php
namespace Pricer\Load;

Class LoadContent extends LoadUrl {
    public function load(){
        return file_get_contents($this->url);
    }
}
