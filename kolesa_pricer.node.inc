<?php
/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 26.02.2021
 * Time: 14:06
 */


function kolesa_pricer_node_load($nodes, $types) {
    // Decide whether any of $types are relevant to our purposes.
    if(count(array_intersect(array('shina','disk'), $types))) {
        foreach($nodes as $nid=>&$node){
            $products=[];
            if($node->type=='shina') $products=kolesa_razmer_shina_load_multiple(FALSE, array('mid' => $nid),FALSE);
            if($node->type=='disk') $products=kolesa_razmer_disk_load_multiple(FALSE, array('mid' => $nid),FALSE);
            $node->products=$products;
            $costs=array();
            foreach($products as $product){
                if(!empty($product->min_cost)) $costs[]=$product->min_cost;
                if(!empty($product->max_cost)) $costs[]=$product->max_cost;
            }

            $node->min_cost=min($costs);
            $node->max_cost=max($costs);
        }
    }
}
