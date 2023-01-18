<?php
/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 24.02.2021
 * Time: 11:58
 */
namespace Pricer\Read;

Class ReadXLSX extends ReadDefault{
    public $name='XLSX';
    public function read($content){
        require_once "Includes/PHPExcel-1.8/Classes/PHPExcel.php";
        $options=$this->options;

        $objReader = \PHPExcel_IOFactory::createReader('Excel2007');
        $temp=tmpfile();
        fwrite($temp,$content);
        $tmpfile_path = stream_get_meta_data($temp)['uri'];

        if($objPHPExcel = $objReader->load($tmpfile_path)) {
            $objWriter = \PHPExcel_IOFactory::createWriter($objPHPExcel, 'CSV');
            $csvFileName=$tmpfile_path.".csv";
            $objWriter->save($csvFileName);

            $file = fopen($csvFileName, 'r');

            $rows=[];
            if(!empty($options['skiprows'])){
                for($n=0; $n<$options['skiprows']; $n++){
                    $line=fgetcsv($file);
                }
            }
            while (($line = fgetcsv($file)) !== FALSE) {
                if(empty($header)) $header=$line;
                else $rows[]=array_combine($header,$line);

            }
            fclose($file);

            return $rows;
        }
        fclose($temp);
    }
}