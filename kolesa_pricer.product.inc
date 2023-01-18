<?php

function kolesa_pricer_product_cost_alter(&$entities,$product_type){
    foreach($entities as &$entity){
        $costs=kolesa_pricer_get_cost_by_cae($entity->cae);
        $prices=array();
        foreach($costs as $c){
            if(!empty($c['retail']))     $prices[]=$c['retail'];
        }
        $entity->product_type=$product_type;
        $entity->cost = $costs;
        if(!empty($prices)) {
            $entity->min_cost = min($prices);
            $entity->max_cost = max($prices);
        }
        kolesa_pricer_check_cost($entity);
    }
}

function kolesa_pricer_get_cost_by_cae($cae){
    $cost = db_query("select * from {shop_price} p where p.cae=:id and p.qty>=1", array(":id" => $cae))->fetchAllAssoc('stock', PDO::FETCH_ASSOC);
    return $cost;
}

function kolesa_pricer_get_product_by_cae($cae){
    static $products;
    if(empty($products)){
        if($cache=cache_get('product_by_cae')){
           $products=$cache->data;
        } else {
            $entityTypes = array('shina_razmer'=>'Tyres', 'disk_razmer'=>'Wheels');
            foreach($entityTypes as $type=>$category){
                $entities_by_types[$category]=entity_load($type);
            }
            $query=db_query("select * from {pricer_mapping_caes}");
            foreach($query as $row){
                $entities=$entities_by_types[$row->category];
                if(!empty($entities[$row->product_id])) $products[$row->cae]=$entities[$row->product_id];
            }
        }
        cache_set('product_by_cae',$products,'cache',time()+24*60*60);
    }
    return $products[$cae];
}


function kolesa_pricer_get_addon_cost($cae){
    static $addon_cost;
    if(empty($addon_cost)){
        $result=db_query("select * from {shop_cost}")->fetchAllAssoc('id');
        foreach($result as $cost){
            $addon_cost[$cost->cae][$cost->cost_type]=$cost->cost;
        }
    }
    if(!empty($addon_cost[$cae])) return $addon_cost[$cae];
    else return NULL;
}

function kolesa_pricer_calc_retail_cost($entity=NULL,$cost){
    if(is_object($cost)) $cost=(array) $cost;
    $stocks=kolesa_pricer_get_stocks('all');
    $stock_object=$stocks[$cost['stock']];
    $stock_setup=$stock_object->shipper->setup;
    $global_setup=$stock_setup['global'];
    $addon_cost = kolesa_pricer_get_addon_cost($cost['cae']);
    if(!empty($addon_cost['spec'])) {
        $log['spec']='Взяли спеццену';
        return array('cost'=>$addon_cost['spec'],'log'=>$log);
    }

        if ($type_setup = array_filter($stock_setup['global'][$cost['category']])) {
            $cost_setup = array_merge($global_setup['All'], $type_setup);
        }
        if (!$entity) $entity = kolesa_pricer_get_product_by_cae($cost['cae']);

        if ($brand_setup = array_filter($stock_setup['brand'][$entity->brand])) {
            $cost_setup = array_merge($cost_setup, $brand_setup);
        }
        if ($cost_setup['rec'] && !empty($cost['mrc'])) {
            $log['mrc']='Взяли РРЦ';
            return array('cost'=>$cost['mrc'],'log'=>$log);
        }
        if($cost_setup['skid']) {
            $process_cost = $cost['stock_price'] - $cost['stock_price'] * $cost_setup['skid'] / 100;
            $log['skid']='Вычли скидку';
        } else {
            $process_cost = $cost['stock_price'];
        }

        $percentMarg = $process_cost * $cost_setup['nac'] / 100;
        $log['marg']='Производим расчёт наценки';
        $rubMarg = $cost_setup['nac_rub'];
        if ($percentMarg > $rubMarg) {
            $process_cost += $percentMarg;
            $log['percmarg']='Наценка в процентах больше - взяли её';
        }
        else {
            $process_cost += $rubMarg;
            $log['rubmarg']='Наценка в рублях больше - взяли её';
        }
        $process_cost = ceil($process_cost / 5) * 5;
        $log['ceil']='Округлили цену';




    return array('cost'=>$process_cost,'log'=>$log);
}

function kolesa_pricer_get_cost($entity,&$cost){
    $calc_cost=kolesa_pricer_calc_retail_cost($entity,$cost);
    $cost['retail']=$calc_cost['cost'];
    $cost['retail_log']=$calc_cost['log'];
}
function kolesa_pricer_check_cost(&$entity){
    $stocks=kolesa_pricer_get_stocks();
    $costs=&$entity->cost;

    if(empty($costs)) return TRUE;


    $get_another_cost=TRUE; #По умолчанию - если одна из позиций подтверждена для показа, то следующие цены уже не проверяем.
    /* Расчёт цен, проверка наличия акций и проверка на наличие позиции в наших складах */
    $checked_positions=array();

    array_walk($costs,function(&$el) use ($entity,$stocks,&$get_another_cost,&$checked_positions){
        /* Предварительная подготовка позиций - рассчитываем розничную цену, ищем акции по позициям */
        kolesa_pricer_get_cost($entity,$el);

        if($checkRules = kolesa_addon_check_rules($entity, $el,TRUE)){
            $el['spec_rules']=$checkRules;
            $el['spec_rules_weight']=kolesa_addon_weight_rules($checkRules);
        }
        /* Просчитываем вес склада*/
        $el['stock_weight']=$stocks['all'][$el['stock']]->weight;
        $el['shipper_weight']=$stocks['all'][$el['stock']]->shipper->weight;
          /* Если среди позиций есть позиции с нашего склада - отдаём им приоритет, если кол-во товара на них <4, то даём шанс другим позициям */
        if(isset($stocks['own'][$el['stock']])) {
            $el['check']=1;
            $el['check_rule']['own']='Наш склад';
            if($el['qty']>=4)  $get_another_cost=FALSE;
            $checked_positions[$el['stock']]=$el;
        }
    });
    /* Ищем другие возможные для выдачи позиции */
    if($get_another_cost){
        /* Подтверждаем все позиции с акциями*/
        $checked_positions=array();
        array_walk($costs,function(&$el) use ($stocks,&$get_another_cost,&$checked_positions){
            if(!empty($el['spec_rules'])&&!isset($stocks['order'][$el['stock']])&&$el['qty']>=4){
                $el['check']=1;
                $el['check_rule']['has_spec']='Есть акции';
                $checked_positions[$el['stock']]=$el;

            }
        });
        if(count($checked_positions)>1){
            /* Если акционных позиций больше 1й, то отбираем из них самую жирную */
            /* Сортируем подтвержденные позиции по весу акций и весу складов */
            uasort($checked_positions,"_kolesa_pricer_sort_spec_stock");
            _kolesa_pricer_get_fat_position($checked_positions,$costs,'has_spec',', но акции имеют низкий вес');
            #    $get_another_cost=FALSE;
        }
#        if($razmer->id==36950){$GLOBALS['debug']['rules'][]=print_r($checked_positions,TRUE);}
        /* Если среди отобранных позиций есть хоть одна с кол-вом >=4, то запрещаем отбирать другие позиции */
        foreach($checked_positions as $key=>$curPosition){
            if($curPosition['qty']>=4) $get_another_cost=FALSE;
        }

    }

    /* Ищем другие возможные для выдачи позиции */
    if($get_another_cost){
        $checked_positions=array();
        /* Подтверждаем все позиции в наличии с кол-вом >=4 */
        array_walk($costs,function(&$el) use ($stocks,&$get_another_cost,&$checked_positions){
            if($el['qty']>=4&&!isset($stocks['order'][$el['stock']])){
                $el['check']=1;
                $el['check_rule']['qty']='Не под заказ, кол-во больше 4';

                $checked_positions[$el['stock']]=$el;
                $get_another_cost=FALSE;
            }
        });
        /* Сортируем позиции по цене и приоритету склада*/
        uasort($checked_positions,"_kolesa_pricer_sort_cost_stock");
        /*Оставляем одну позицию с минимальной ценой и приоритетным складом*/
        _kolesa_pricer_get_fat_position($checked_positions,$costs,'qty',', но приоритет склада ниже (в наличии)');

    }

    if($get_another_cost) {
        $checked_positions=array();
        /* Подтверждаем все позиции под заказ с кол-вом >=4 */
        array_walk($costs, function (&$el) use ($stocks,&$get_another_cost, &$checked_positions) {
            if ($el['qty'] >= 4 && !empty($el['spec_rules'])&& isset($stocks['order'][$el['stock']])) {
                $el['check'] = 1;
                $el['check_rule']['qty'] = 'Под заказ, кол-во больше 4';
                $el['check_rule']['has_spec']='Есть акции';
                $checked_positions[$el['stock']] = $el;
                $get_another_cost = FALSE;
            }
        });

        /*Сортируем позиции по цене и приоритету склада */
        uasort($checked_positions,"_kolesa_pricer_sort_cost_stock");
        /*Оставляем одну позицию с минимальной ценой и приоритетным складом*/
        _kolesa_pricer_get_fat_position($checked_positions,$costs,'qty',', но приоритет склада ниже (под заказ)');
    }

    if($get_another_cost){
        array_walk($costs, function (&$el) use (&$get_another_cost, &$checked_positions) {
            $el['check'] = 1;
            $el['check_rule']['another'] = 'Оставшиеся позиции';
            $checked_positions[$el['stock']] = $el;
        });
        /*Сортируем позиции по цене и приоритету склада */
        uasort($checked_positions,"_kolesa_pricer_sort_cost_stock");
        /*Оставляем одну позицию с минимальной ценой и приоритетным складом*/
        _kolesa_pricer_get_fat_position($checked_positions,$costs,'qty',', но приоритет склада ниже');
    }
    if($entity->id==9907){
      #   $GLOBALS['debug']['costs']=print_r($costs,TRUE);
        # $GLOBALS['debug']['rules'][10]=print_r($costs[10],TRUE);
        # $GLOBALS['debug']['rules'][1333]=print_r($costs[1333],TRUE);
    }
    $all_rules=array();
    $costs_array=array();
    foreach($costs as &$cost){
        if(!isset($cost['check'])) $cost['check']=0;
        if($cost['check']==1){
            $checkRules = kolesa_addon_check_rules($razmer, $cost);
            $all_rules+=array_flip(array_keys($checkRules));
            $cost['spec_rules']=$checkRules;
            $costs_array[]=$cost['cost'];
        }
    }
    $entity->spec_rules=array_keys($all_rules);
    $entity->cost=$costs;


}



function _kolesa_pricer_get_fat_position(&$checked_positions, &$costs,$type,$type_msg=""){
    $fat_position_key=key($checked_positions);
    foreach(array_keys($checked_positions) as $key){
        if($key!=$fat_position_key) {
            $costs[$key]['check']=0;
            $costs[$key]['check_rule'][$type].=$type_msg;
            unset($checked_positions[$key]);
        }
    }

}
function _kolesa_pricer_sort_spec_stock($a,$b){
    if($a['spec_rules_weight']==$b['spec_rules_weight']) $rule=0;
    else $rule=$a['spec_rules_weight']<$b['spec_rules_weight']?-1:1;

    if($rule==0) {
        if ($a['stock_weight'] == $b['stock_weight']) $rule = 0;
        else $rule = $a['stock_weight'] < $b['stock_weight'] ? -1 : 1;
    }

    return $rule;
}
function _kolesa_pricer_sort_cost_stock($a,$b){
    if($a['cost']==$b['cost']) $rule=0;
    else $rule=$a['cost']<$b['cost']?-1:1;
    if($rule==0){
        if($a['stock_weight']==$b['stock_weight']) $rule=0;
        else $rule=$a['stock_weight']<$b['stock_weight']?-1:1;
    }
    return $rule;
}


function kolesa_pricer_get_products_from_options($options=[]){
    $query=db_select('pricer_products','pr');
    $query->fields('pr');
    $query->join('pricer_price','p','p.cae=pr.cae');

    if(!empty($options['stocks'])){
        $query->condition('p.stock',$options['stocks'],'IN');
    }
    $result=$query->execute()->fetchAllAssoc('id');

    return $result;

}