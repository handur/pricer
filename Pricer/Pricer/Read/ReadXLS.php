<?php
/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 24.02.2021
 * Time: 11:58
 */
namespace Pricer\Read;

Class ReadXLS extends ReadDefault{
    public $name='XLS';
    public function read($fileName){
        require_once "Includes/SimpleXLSX/simplexlsx.class.php";

        if ($xlsx = \SimpleXLSX::parse($fileName)) {

            $rows=$xlsx->rows();
            if($this->options['header']) $header=array_shift($rows);
            else $header=array();

            if(!empty($header)) {
                foreach($header as &$col){
                    $col=preg_replace('[\n]','',$col);
                }
                array_walk($rows, function (&$el) use ($header) {
                    $el = array_combine($header, $el);
                });
            }
            return $rows;
        }

    }
}