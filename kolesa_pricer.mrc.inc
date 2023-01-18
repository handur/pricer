<?php

Class KolesaPricerMRCUpdate extends Pricer\Core\PricerUpdate
{

    protected function parseItem($item,$parseOptions){
        $parsedItem=array();
        $groupOptions=$this->prepareOptions($parseOptions);

        $groupOptions['row']=$item;
        $groupOptions['Shipper']=$parsedItem['Shipper']=$this->shipperName;

        foreach($parseOptions->children() as $field){
            $parseOptions=$this->prepareOptions($field);
            $parseOptions=array_merge($groupOptions,$parseOptions);
            $parseOptions['element']=$field->getName();
            $parseOptions['processed_row']=$parsedItem;
            if(!isset($item[$parseOptions['field']])){
                if(isset($parseOptions['no'])){
                    $curElField=array_keys($item)[$parseOptions['no']];
                    $curEl=$item[$curElField];
                }
            } else {
                $curEl=$item[$parseOptions['field']];
            }
            if(!empty($curEl)) $parsedElement = $this->parseElement($curEl, $parseOptions);
            else return NULL;
            $parsedItem[$parseOptions['element']]=$parsedElement;
        }
        return $parsedItem;
    }

    public function updateMRC(){
        $shipper=reset(entity_load('shipper',array(),array('machine_key'=>$this->shipperName)));
        db_delete('pricer_cost')->condition('shipper', $shipper->id)->execute();

        $data=$this->parsedData;
        $chunks = array_chunk($data, 500);
        foreach ($chunks as $chunk) {
            $query = db_insert('pricer_cost')->fields(['cae', 'shipper', 'cost_type', 'cost']);
            foreach ($chunk as $cost) {
                if(!empty($cost['VendorCode'])) {
                    $query->values(array(
                        'cae' => $cost['VendorCode'],
                        'shipper' => $shipper->id,
                        'cost_type' => 'mrc',
                        'cost' => $cost['MRC'],
                    ));
                }
            }
            $query->execute();
        }

        db_query("update {pricer_price} p set p.mrc=IFNULL((select cost from {pricer_cost} r where r.cae=p.cae and r.cost_type='mrc' limit 1),0)");

    }




}