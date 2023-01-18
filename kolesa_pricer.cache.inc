<?php
/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 20.04.2021
 * Time: 10:50
 */

function kolesa_pricer_cache_set($key,$data,$expired){
    db_delete('pricer_cache')->condition('key',$key)->execute();
    $fields=[
        'key'=>$key,
        'data'=>serialize($data),
        'expired'=>$expired,
    ];
    db_merge('pricer_cache')->key(['key'=>$key])->fields($fields)->execute();
}

function kolesa_pricer_cache_get($key){

}