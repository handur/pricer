<?php
function kolesa_pricer_block_info(){
    $blocks = array();

        $blocks["pricer:shippers"] = array(
            'info' => "Pricer: Поставщики",
            'cache'=> DRUPAL_CACHE_PER_PAGE,
        );

    return $blocks;
}

function kolesa_pricer_block_view($delta = ''){
    $block=[];
    if($delta=='pricer:shippers'){
        $block['subject']="Поставщики";
        $block['content']=kolesa_pricer_shipper_list_table();
    }
    return $block;
}