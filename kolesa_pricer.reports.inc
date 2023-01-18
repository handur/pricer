<?php
function kolesa_pricer_reports_load_session_info($session_id){
    $query=db_query("select * from {pricer_log_session} where session_id=:session",[":session"=>$session_id]);
    $session_info=[];
    foreach($query as $row){
        if(empty($session_info)) {
            $session_info = ['id' => $row->session_id, 'shipper_name' => $row->shipper_name, 'type' => $row->type];
        }
        if($row->mode=='start'){
            $session_info['start']=date("d.m.Y H:s:i",strtotime($row->date));
        } elseif($row->mode=='finish'){
            $session_info['finish']=date("d.m.Y H:s:i",strtotime($row->date));
        }
    }
    return $session_info;
}
function kolesa_pricer_reports_load_protocols($shipper_id=NULL,$session_id=NULL){
    if(empty($shipper_id)) return kolesa_pricer_reports_load_protocols_all();
    if(!empty($session_id)) return kolesa_pricer_reports_load_protocols_session($shipper_id,$session_id);


    $shipper=reset(entity_load('shipper',array($shipper_id)));
    drupal_set_title('Протоколы загрузки '.$shipper->name);

    $query=db_query("select session_id, type, date from {pricer_log_session} where shipper_name=:shipper and mode='finish' group by session_id order by date desc",[":shipper"=>$shipper->name]);
    $table_rows=[];
    foreach($query as $row){

        $table_rows[]=[
            $row->session_id,
            $row->type,
            l('Ссылка на протокол загрузки','admin/price/reports/load_protocols/'.$shipper_id.'/'.$row->session_id),
            date("d.m.Y H:s:i",strtotime($row->date)),
        ];
    }



    $build=[];
    $header=[
      '# Сессии',
      'Тип операции',
        'Ссылка на протокол',
      'Время окончания',
    ];
    $build['table']=[
      '#markup'=>theme('table',['rows'=>$table_rows,'header'=>$header])
    ];
    return $build;

}

function kolesa_pricer_reports_load_protocols_all(){

}

function kolesa_pricer_reports_load_protocols_session($shipper_id,$session_id){
    $session=kolesa_pricer_reports_load_session_info($session_id);
    drupal_set_title($session_id.': Протокол загрузки '.$session['shipper_name'].' '.$session['type']);
    $build=[];
    $build['times']=[
      '#markup'=>'<b>Время начала:</b>&nbsp;'.$session['start']."<br/><b>Время окончания:</b>&nbsp;".$session['finish']."<hr/>",
    ];

    $query=db_query("select * from {pricer_log_position} where session_id=:session order by created asc",[":session"=>$session_id]);

    foreach($query as $row){
        $data=unserialize($row->data);
        $type=$row->type;
        $reason=$row->reason;
        $table_rows[]=[
          $type,$reason,print_r($data,TRUE)
        ];
    }

    $build['table']=[
        '#markup'=>theme('table',['rows'=>$table_rows])
    ];
    return $build;
}


function kolesa_pricer_reports_check_prices(){
    $shippers=kolesa_pricer_get_active_shippers();
    $rows=[];
    $checkPrice=[];
    foreach($shippers as $shipperId=>$shipper){
        foreach($shipper->allowedPrices as $priceKey=>$priceType){
            if($priceKey=='price'||$priceType['queued']=='price'){
                $checkPrice[$shipperId][]=$priceKey;
            }
        }
        /*
        $data=array(
            'key'=>$shipper->machine_key,
            'name'=>$shipper->name,
            'stocks'=>array('data'=>kolesa_pricer_shipper_get_stocks_table($shipper->stocks)),
        );
        */
        #$rows[$shipper->id]=$data;
    }
    foreach($checkPrice as $shipperId=>$priceKeys){
        foreach($priceKeys as $priceKey) {
            $curData=kolesa_pricer_shipper_get_data($shippers[$shipperId],$priceKey);
            foreach($curData as $catName=>$catPrice){
                $stat[$shipperId][$catName]=_pricer_analyze_price($catPrice,$catName);
                $row=[
                  'shipper'=>$shippers[$shipperId]->name,
                    'cat'=>$catName,
                    'products_count'=>$stat[$shipperId][$catName]['products_count'],
                    'stocks_count'=>'',
                    'errors'=>'',
                ];
                if(!empty($stat[$shipperId][$catName]['errors'])){
                    $errors_rows=[];
                    foreach($stat[$shipperId][$catName]['errors'] as $errorType=>$errorInfo){
                        $errors_rows[]=[$errorType,join(", ",$errorInfo)];
                    }
                    $row['errors']=theme('table',['rows'=>$errors_rows]);
                }
                $stock_rows=[];
                foreach($stat[$shipperId][$catName]['stocks_count'] as $stockId=>$counts){
                    $stock_rows[]=[$stockId.".".$shippers[$shipperId]->stocks[$stockId]->name,'Принято:'.$counts['source'],'Отдано:'.$counts['export']];
                }
                $row['stocks_count']=theme('table',['rows'=>$stock_rows]);
                $rows[]=$row;

            }
        }

    }

    $build=[];
    $header=[];
    $build['shippers']['#markup']=theme('table',['rows'=>$rows,'header'=>$header]);
    return $build;
}

function _pricer_analyze_price($price,$cat){
    $stat=[];
    $stat['products_count']=count($price);
    foreach($price as $row){
        if(isset($row['Stocks'])){
            foreach($row['Stocks'] as $stockName=>$stockInfo){
                if($stockInfo['qty']!=0&&isset($stockInfo['stock_id'])) {
                    $stat['stocks_count'][$stockInfo['stock_id']]['source']++;
                } elseif(empty($stockInfo['stock_id'])){
                    $stat['errors']['no_stock_id'][$stockName]=$stockName;
                }
            }
        }
    }

    foreach($stat['stocks_count'] as $stock_id=>$stock_count){
        $priceCount=db_query("select count(*) FROM {pricer_price} price 
          left JOIN {pricer_products} products on products.cae=price.cae  
  WHERE NOT ISNULL(products.id) and price.stock=:id and products.category=:cat and price.category=:cat and price.qty>0",[":id"=>$stock_id,":cat"=>$cat])->fetchField();
        $stat['stocks_count'][$stock_id]['export']=$priceCount;
    }

    return $stat;
}

function _pricer_analyze_row(&$row){
    $errors=[];

    if(!empty($row['Stocks'])){
        $hasPrice=FALSE;
        foreach($row['Stocks'] as $stockName=>$stockInfo){
            $stockId=$stockInfo['stock_id'];
            $check=db_query("select * from {pricer_price} where cae=:cae and category=:cat and stock=:stock",[":cae"=>$row['VendorCode'],":cat"=>$row['Category'],":stock"=>$stockId])->fetchAll();
            if($check){
                $hasPrice=TRUE;
            }
            if($hasPrice) break;
        }
        if(!$hasPrice) {
            $errors[]='Нет цены';
            if(empty($row['VendorCode'])){
                $errors[]='Нет CAE';
            }
            if(empty($row['Brand'])){
                $errors[]='Нет производителя';
            }
            if(empty($row['Model'])){
                $errors[]='Нет модели';
            }
        }
    } else {
        $errors[]='Нет складов';
    }
    if(!empty($errors)){
        return implode(", ",$errors);
    }

    return FALSE;
}