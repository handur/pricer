<?php

/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 05.02.2021
 * Time: 11:34
 */
class KolesaFieldConvert extends Pricer\Core\FieldConvert
{
    protected static function process(&$fieldValue,$convertRule, $convertOptions=[]){
        parent::process($fieldValue,$convertRule, $convertOptions);
        if(empty($fieldValue)) return;
        switch ($convertRule) {
            case 'findbrand':
                $brands=preg_split("[,]",$convertOptions['brands']);

                $findFlag=FALSE;
                foreach($brands as $brand){
                    if(stripos($fieldValue, $brand)!== false) {
                        $fieldValue=$brand;
                        $findFlag=TRUE;
                        break;
                    }
                }
                if(!$findFlag) $fieldValue="";

                break;
            case 'getrazmer':
                preg_match_all($convertOptions['pattern'],$fieldValue,$match);

                if(!empty($match[0]))  {
                    $razmer=$match[0][0];
                    preg_match_all("/[0-9.\,]+/",$razmer,$r_match);
                    $fieldValue=$r_match[0][$convertOptions['razmer']];
                    $fieldValue = preg_replace("/[,]/", '.', $fieldValue);
                } else {
                    $fieldValue="";
                }



                break;
            case 'cleanbrand':
                $brand=$convertOptions['processed_row']['Brand'];
                $fieldValue=preg_replace("/(".$brand.")/i","",$fieldValue);
                $fieldValue=trim($fieldValue);

             #   $fieldValue=print_r($convertOptions,TRUE);
                break;
            case 'cleanfirst':
                $string = preg_split( '/\s+/', $fieldValue);
                $fieldValue=implode( ' ', array_slice( $string, $convertOptions['firstcount']));
                break;
            case 'lastword':
                $fieldArr=explode(" ",$fieldValue);
                $fieldValue=end($fieldArr);
                break;
            case 'removespace':
                $fieldValue=preg_replace("/\s+/","",$fieldValue);
                break;
            case 'removecomma':
                $fieldValue = preg_replace("/[,]/", '', $fieldValue);
                break;
            case 'removecomma2':
                $fieldValue = preg_replace("/[,]/", '.', $fieldValue);
                break;
            case 'digit':
                $fieldValue = preg_replace("/[^0-9.,]/", '', $fieldValue);
                $fieldValue = preg_replace("/[,]/", '.', $fieldValue);
                $fieldValue=(float)$fieldValue;
                break;
            case 'ceil':
                $fieldValue=ceil($fieldValue);
                break;
            case 'indexn':
                $fieldValue = preg_replace("/[^0-9\/]/", '', $fieldValue);
                break;
            case 'word':
                $fieldValue = preg_replace("/[0-9.,\/]/", '', $fieldValue);

                break;
            case 'pregmatch':
                preg_match_all($convertOptions['pattern'],$fieldValue,$match);
                if(!empty($match[0])) {
                    $fieldValue = $match[0][$convertOptions['delta']];
                } else $fieldValue="";
                break;
            case 'checkold':

                preg_match_all($convertOptions['pattern'],$fieldValue,$match);
                if(!empty($match[0])) {
                    $fieldValue = 1;
                } else $fieldValue=0;
                break;
            case 'upper':
                $fieldValue = mb_convert_case($fieldValue, MB_CASE_UPPER, "UTF-8");
                break;
            case 'lower':
                setlocale (LC_ALL, "ru_RU.UTF-8");
                $fieldValue = mb_convert_case($fieldValue, MB_CASE_LOWER, "UTF-8");
                break;
            case 'cleanword':
                $filter="Лето|лето|Зима|зима|Noise cancelling system|Seal-Inside|ContiSeal|SSR|SoundComfort|Run flat|Run Flat|Stock|stock|Runflat|RunFlat|RFT|Sealant|moto|old|old[0-9]+";
                if(!empty($convertOptions['pattern'])){
                    $filter.="|".$convertOptions['pattern'];
                }
                $fieldValue = preg_replace("/($filter)/", '', $fieldValue);
                $fieldValue=ltrim($fieldValue,'0');
                break;

            case 'remove':
                $filter=$convertOptions['filter'];
                $fieldValue = preg_replace("/($filter)/", '', $fieldValue);
                break;
            case 'replica':
                if(preg_match( '!\(([^\)]+)\)!', $fieldValue, $match))         $fieldValue = $match[1];
                break;
            case 'cleancode':
                $filter="\\n|_Нк|moto|BR|old|T000STD|old[0-9]+";
                $fieldValue = preg_replace("/($filter)/", '', $fieldValue);
                $fieldValue=ltrim($fieldValue,'0');
                break;
            case 'clean':
                $fieldValue = preg_replace("/[^А-ЯeЁьЬъЪа-яA-Za-z0-9]/", '', $fieldValue);
                break;
            case 'divide':
                $divider=$convertOptions['divider'];
                $delta=$convertOptions['delta'];

                $divideValue=preg_split("[".$divider."]",$fieldValue);
                $fieldValue=$divideValue[$delta];
                KolesaFieldConvert::process($fieldValue,$convertOptions['convertType'],$convertOptions);
                break;
            case 'taxonomy':
                $fieldValue=KolesaFieldMapping::getTerm($convertOptions,$fieldValue);
                break;
            case 'brand':
                $fieldValue=KolesaFieldMapping::getBrand($convertOptions,$fieldValue);
                break;
            case 'node':
                $fieldValue=KolesaFieldMapping::getNode($convertOptions,$fieldValue);
                break;
            case 'model':
                $fieldValue=KolesaFieldMapping::getModel($convertOptions,$fieldValue);
                break;
            case 'product':
               $fieldValue=KolesaFieldMapping::getProduct($convertOptions,$fieldValue);
                break;
            case 'stock':
                $fieldValue=KolesaFieldMapping::getStock($convertOptions,$fieldValue);
                break;
        }
    }
}