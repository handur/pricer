<?php

/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 20.02.2021
 * Time: 9:50
 */
class KolesaPricerLog
{
    public static function addLog($type='',$process='',$msg='',$data=[]){
       db_insert("pricer_log")->fields(array(
            'type'=>$type,
            'process'=>$process,
            'message'=>$msg,
            'data'=>serialize((array)$data),
        ))->execute();

    }
    public static function viewLog($type='',$time_from='',$time_to=''){
        $query=db_select('pricer_log','l')->fields('l');
        if(!empty($type)) $query->condition('type',$type);
        if(!empty($time_from)) $query->condition('created',$time_from,'>=');
        if(!empty($time_to)) $query->condition('created',$time_to,'<=');
        $query->orderBy('created','DESC');
        $result=$query->execute()->fetchAllAssoc('id',PDO::FETCH_ASSOC);
        return $result;
    }

    public static function addPositionLog($shipper='',$type='',$reason='',$session_id,$data=[]){
        db_insert("pricer_log_position")->fields(array(
            'shipper_name'=>$shipper,
            'type'=>$type,
            'reason'=>$reason,
            'session_id'=>$session_id,
            'data'=>serialize((array)$data),
        ))->execute();
    }
    public static function cleanPositionLog($shipper){
      #  db_delete('pricer_log_position')->condition('shipper_name',$shipper)->execute();
    }

    public static function addSessionLog($session_id, $update_type, $mode){
        db_insert("pricer_log_session")->fields(array(
            'session_id'=>$session_id,
            'type'=>$update_type,
            'mode'=>$mode
        ))->execute();
    }


}