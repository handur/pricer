<?php
/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 24.02.2021
 * Time: 11:58
 */
namespace Pricer\Read;

Class ReadXLS2 extends ReadDefault{
    public $name='XLS2';
    public function read($content){
        require_once "Includes/PHPExcel-1.8/Classes/PHPExcel.php";
        $options=$this->options;
        $objReader = \PHPExcel_IOFactory::createReader('Excel5');
        $temp=tmpfile();
        fwrite($temp,$content);
        $tmpfile_path = stream_get_meta_data($temp)['uri'];

        if($objPHPExcel = $objReader->load($tmpfile_path)) {
            $objWriter = \PHPExcel_IOFactory::createWriter($objPHPExcel, 'CSV');
            $csvFileName=$tmpfile_path.".csv";
            $objWriter->save($csvFileName);

            $file = fopen($csvFileName, 'r');

            $rows=[];
            if(empty($header)&&$options['header']) {
                $header=fgetcsv($file);
                foreach($header as &$h_col){
                    $h_col=trim($h_col);
                }
            }
            if(!empty($options['skiprows'])){
                for($n=0; $n<$options['skiprows']; $n++){
                    fgetcsv($file);
                }
            }
            while (($line = fgetcsv($file)) !== FALSE) {
                if($options['header']) {
                    $rows[] = array_combine($header, $line);
                } else {
                    $rows[]=$line;
                }
            }
            fclose($file);
            return $rows;
        }
        fclose($temp);
    }
}