<?php
/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 24.02.2021
 * Time: 11:58
 */
namespace Pricer\Read;

Class ReadCSV extends ReadDefault{
    public $name='CSV';
    public function read($file){
        $data=str_getcsv($file, "\n");
        $header=str_getcsv(array_shift($data),";");
        foreach ($data as $i=>$row) {
            $csv_string=str_getcsv($row,";");
            $csv[$i] = array_combine($header, $csv_string);
        }

        return $csv;
    }
}