<?php
/**
 * Created by PhpStorm.
 * User: lushanov
* Date: 05.02.2021
* Time: 11:46
*/

namespace Pricer\Core;


class FieldMapping
{
    private static $maps=[];

    public function __construct(){

    }



    public static function searchInMap($value,$map){
        if($id=array_search($value,$map)) return $id;
        return NULL;
    }
    public static function searchKeyInMap($key,$map){
        return isset($map[$key])?$map[$key]:NULL;
    }
    public function getMap($mapName){
        return !empty($this->maps[$mapName])??$this->maps[$mapName];
    }
    public function createMap($mapName,$mappingList=[]){
        $this->maps[$mapName]=$mappingList;
    }

}