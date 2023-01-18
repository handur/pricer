<?php
/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 05.02.2021
 * Time: 11:18
 */

namespace Pricer\Core;


class FieldConvert
{
    public static function convert(&$fieldValue,$convertRule, $convertOptions=[])
    {
              if (is_array($convertRule)) {
                foreach ($convertRule as $rule) {
                    static::convert($fieldValue, $rule, $convertOptions);
                }
            } else {
                static::process($fieldValue, $convertRule, $convertOptions);
            }
            return $fieldValue;


    }

    protected static function process(&$fieldValue,$convertRule, $convertOptions=[]){
        $fieldValue=trim($fieldValue);
        switch ($convertRule) {
            case 'custom':

                $fieldValue = (string)$fieldValue;
                foreach ($convertOptions['convert'] as $convert) {
                    if ($convert['from'] == $fieldValue) {
                        $fieldValue = $convert['to'];
                        return;
                    }
                }
                $fieldValue =NULL;
                break;
            case 'trim':
                $fieldValue=trim($fieldValue);
                break;
            case 'float':
                $fieldValue = (float)$fieldValue;
                break;
            case 'integer':
                $fieldValue = (integer)$fieldValue;
                break;
            default:
                $fieldValue = (string)$fieldValue;
                break;
        }

    }

}

