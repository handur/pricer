<?php
function kolesa_pricer_export_get_export_data($mode='json'){
    $query=db_query("select pp.id, pr.* from {pricer_price_export} pr
LEFT JOIN {pricer_products} pp ON pp.id=pr.id AND pp.category=pr.category
WHERE NOT ISNULL(pp.id)");
    $price=[];
    foreach($query as $row){
        $price[$row->category][$row->id][$row->stock]=['article'=>$row->id,'cae'=>$row->cae,'qty'=>$row->qty,'b2b'=>$row->b2b,'stock_price'=>$row->stock_price];
    }
    if($mode=='json'){
        drupal_json_output($price);
        exit;
    }
}

function kolesa_pricer_export_get_export_file($stock_id,$category,$format,$mode='csv'){
    $data=kolesa_pricer_export_get_export($stock_id,$category,$format);
#    drupal_add_http_header('Content-Type', 'text/csv; utf-8');

    //instead of writing down to a file we write to the output stream
    if($mode=='csv'&&!empty($data)) {
        $fh = fopen('php://memory', 'w');
        $delimiter = ";";
        //form header
        fputcsv($fh, array_keys(current($data)), $delimiter);

        //write data in the CSV format
        foreach ($data as $row) {
            fputcsv($fh, $row, $delimiter);
        }
        rewind($fh);
        print stream_get_contents($fh);
        exit;
    } else {

        $build=[
            '#prefix'=>'<h4>Количество: '.count($data).'</h4>',
          '#markup'=>theme('table',array('rows'=>$data,'header'=>array_keys($data[0]))),
        ];
        return $build;
    }
}
function kolesa_pricer_export_get_export($stock_id,$category,$format){
    if($format=='onlyproducts'){
        $price = kolesa_pricer_export_prepare_data($stock_id, $category,TRUE);
        $format='marketmixer';
    } else {
        $price = kolesa_pricer_export_prepare_data($stock_id, $category);
    }

    $formatPrice=kolesa_pricer_export_process_data($price,$category,$format);
    return $formatPrice;
}
function kolesa_pricer_export_prepare_data($stock_id,$category,$noprice=FALSE){
    $stock=kolesa_pricer_stock_load($stock_id);
    global $user;
        if(!empty($stock->b2b)){
            $b2b_stocks=entity_load('shipper_stock',[],['sid'=>$stock->b2b]);
            if(!empty($b2b_stocks)){
                $b2b_stocks_ids=array_keys($b2b_stocks);
            }
        }

    $stock_updated=$stock->shipper->updated;
    if(!$noprice) {
        $sql="select price.cae cae,
        products.id product_id,
        products.name product,
        products.category category,
        brands.id brand_id,
        brands.name brand,
        models.id model_id,
        models.name model,
        SUM(price.qty) qty,
        price.stock_price stock_price,";

        if(!empty($b2b_stocks_ids)){
            $sql.="price_b2b.stock_price b2b_price,";
        }
        $sql.="images.fid imageFid,
        addons.params addons
        FROM {pricer_price} price ";
        if(!empty($b2b_stocks_ids)){
            $sql.="left JOIN pricer_price price_b2b ON price.cae=price_b2b.cae  and price_b2b.stock IN (".implode(",",$b2b_stocks_ids).") ";
        }
        $sql.="left JOIN {pricer_products} products on products.id=price.id
          left JOIN {pricer_products_brands} brands on brands.id=products.brand
          left JOIN {pricer_products_models} models on models.id=products.model
          left JOIN {pricer_products_images} images ON images.product_id=products.id
          LEFT JOIN (SELECT id,GROUP_CONCAT(param_key,':',param_value SEPARATOR '#') params FROM {pricer_products_addons} GROUP BY id)  addons ON addons.id=products.id    
  WHERE NOT ISNULL(products.id) and price.stock=:id and products.category=:cat and price.category=:cat and price.qty>0";
        if(!empty($b2b_stocks_ids)){
            $sql.=" AND price_b2b.stock_price!=0 ";
        }

        $sql.=" GROUP BY products.id";
        $params=[":id" => $stock_id, ":cat" => $category];

        $price = db_query($sql, $params)->fetchAllAssoc('cae', PDO::FETCH_ASSOC);
    } else {
        $price = db_query("SELECT  products.cae cae,
products.id product_id,
        products.name product,
        products.category category,
        brands.id brand_id,
        brands.name brand,
        models.id model_id,
        models.name model,
        images.fid imageFid,
 (SELECT GROUP_CONCAT(param_key,':',param_value SEPARATOR '#') params FROM {pricer_products_addons} WHERE pricer_products_addons.id=products.id) addons       
        FROM {pricer_products} products 
          left JOIN {pricer_products_brands} brands on brands.id=products.brand
          left JOIN {pricer_products_models} models on models.id=products.model
          left JOIN {pricer_products_images} images ON images.product_id=products.id
WHERE NOT ISNULL(products.id) and products.category='Tyres' 
ORDER BY brand_id, model_id, product_id
", [":cat" => $category])->fetchAllAssoc('cae', PDO::FETCH_ASSOC);
    }
    $imageFids=array_filter(array_column($price,'imageFid'));
    $images=file_load_multiple($imageFids);
    foreach($price as &$row){
        if(!empty($row['b2b_price'])){
            $row['stock_price']=$row['b2b_price'];
            unset($row['b2b_price']);
        }
        if(!empty($row['imageFid'])) $row['image']=$images[$row['imageFid']]->uri;
        else $row['image']='';
        $addons=explode("#",$row['addons']);
        foreach($addons as $addon){
            $addon_param=explode(":",$addon);
            $row[$addon_param[0]]=$addon_param[1];
        }
        $row['created']=$stock_updated;
        if(!empty($stock_id))        $row['stock_id']=$stock_id;
        unset($row['addons']);
        unset($row['imageFid']);

    }

    return $price;
}



function kolesa_pricer_export_process_data($data,$category,$format){
    $curFormat=kolesa_pricer_export_get_format($category,$format);
    $curConvertRules=kolesa_pricer_export_get_convert($format);
    $processData=[];
    foreach($data as $row){
        $processRow=[];
        foreach($curFormat as $fKey=>$pKey){
            if(isset($row[$pKey])) $processRow[$fKey]=kolesa_pricer_export_process_row($pKey,$row[$pKey],$curConvertRules);
            else $processRow[$fKey]="";
        }
        $processData[]=$processRow;
    }
    return $processData;
}

function kolesa_pricer_export_process_row($pKey,&$rowEl,$curConvertRules){
    if(isset($curConvertRules[$pKey])){
        $rowEl=str_replace(array_keys($curConvertRules[$pKey]),array_values($curConvertRules[$pKey]),$rowEl);
    }
    if($pKey=='image'&&!empty($rowEl)){
        $rowEl=file_create_url($rowEl);
    }
    if($pKey=='Runflat'){
        if($rowEl==1) $rowEl="да";
    }
    if($pKey=='Pin'){
        if($rowEl==1) $rowEl="да";
    }
    return $rowEl;
}

function kolesa_pricer_export_get_format($category,$format){
    $allFormats=kolesa_pricer_export_format();
    $currentFormat=$allFormats[$format];
    $getFormat=$currentFormat['Main'];
    if($categoryData=$currentFormat[$category]) $getFormat+=$categoryData;
    return $getFormat;

}
function kolesa_pricer_export_get_convert($format){
    $allConvert=kolesa_pricer_export_convert();
    if(isset($allConvert[$format])) return $allConvert[$format];
    return NULL;
}
function kolesa_pricer_export_convert(){
    $converts=[
      'marketmixer'=>[
        #  'Vehicle'=>['sport'=>'спортивные','car'=>'легковые','truck'=>'грузовые','moto'=>'мото'],
      ],
        'onlyproducts'=>[

        ]
    ];
    return $converts;
}

function kolesa_pricer_export_format(){
    #PRODUCT_ID	PRODUCT_NAME	BRAND_ID	BRAND_NAME	MODEL_ID	MODEL_NAME	Wheel type	Width	Diametr	LZ	PCD	LZ 2	PCD 2	ET	DIA	Color	QUANTITY	Image MM 1	Image MM 2	Image MM 3	STOCK	RETAIL_PRICE	STOCK_PRICE	SPEC_PRICE	MIC	MRC	VIANOR_PRICE	SPEC_PRICE2	RETAIL_PRICE3	TIME	B2B_ID	Артикул товара поставщика	TYREINDEX_ID	MANUFACTER_ID	Артикул товара производителя	Тест2	Тест3
    #cae	product	category	brand	model	qty	stock_price	image	Width	Diametr	Krep	PCD	ET	DIA	Type	Color

#PRODUCT_ID	PRODUCT_NAME	BRAND_ID	BRAND_NAME	MODEL_ID	MODEL_NAME	Сезон	Тип ТС	Ширина	Профиль	Диаметр	Индекс нагрузки	Индекс скорости	Тип усиленности	Шипованность	Run on Flat	QUANTITY	Image MM 1	Image MM 2	Image MM 3	STOCK	RETAIL_PRICE	STOCK_PRICE	SPEC_PRICE	MIC	MRC	VIANOR_PRICE	SPEC_PRICE2	RETAIL_PRICE3	TIME	B2B_ID	Артикул товара поставщика	TYREINDEX_ID	MANUFACTER_ID	Артикул товара производителя	Тест2	Тест3
    #cae	product	category	brand	model	qty	stock_price	image	Width	Height	Diametr	Season	IndexN	IndexS	Vehicle	GTIN	Omologation
    $formats=[
        'marketmixer'=>[
            'Main'=>[
              'PRODUCT_ID'=>'product_id',
                'PRODUCT_NAME'=>'product',
                'BRAND_ID'=>'brand_id',
                'BRAND_NAME'=>'brand',
                'MODEL_ID'=>'model_id',
                'MODEL_NAME'=>'model',
                'Image'=>'image',
                'STOCK'=>'stock_id',
                'QUANTITY'=>'qty',
                'STOCK_PRICE'=>'stock_price',
                'TIME'=>'created',
                'Артикул товара производителя'=>'cae',
                'Old'=>'Old',
                'Производство'=>'Country'
            ],
            'Tyres'=>[
                'Ширина'=>'Width',
                'Профиль'=>'Height',
                'Диаметр'=>'Diametr',
                'Сезон'=>'Season',
                'Индекс нагрузки'=>'IndexN',
                'Индекс скорости'=>'IndexS',
                'Тип усиленности'=>'Usil',
                'Шипованность'=>'Pin',
                'Run on Flat'=>'Runflat',
                'Омологация'=>'Omologation',
                'XL'=>'XL',
                'GTIN'=>'GTIN',
                'Тип ТС'=>'Vehicle',

            ],
            'Truck'=>[
                'Ширина'=>'Width',
                'Профиль'=>'Height',
                'Диаметр'=>'Diametr',
                'Сезон'=>'Season',
                'Индекс нагрузки'=>'IndexN',
                'Индекс скорости'=>'IndexS',
                'Тип усиленности'=>'Usil',
                'Шипованность'=>'Pin',
                'Run on Flat'=>'Runflat',
                'Омологация'=>'Omologation',
                'XL'=>'XL',
                'GTIN'=>'GTIN',
                'Тип ТС'=>'Vehicle',

            ],
            'Moto'=>[
                'Ширина'=>'Width',
                'Профиль'=>'Height',
                'Диаметр'=>'Diametr',
                'Сезон'=>'Season',
                'Индекс нагрузки'=>'IndexN',
                'Индекс скорости'=>'IndexS',
                'Тип усиленности'=>'Usil',
                'Шипованность'=>'Pin',
                'Run on Flat'=>'Runflat',
                'Омологация'=>'Omologation',
                'XL'=>'XL',
                'GTIN'=>'GTIN',
                'Тип ТС'=>'Vehicle',
                'Применение'=>'Use',
                'Ось применения'=>'Axis',

            ],
            'Wheels'=>[
                'Ширина'=>'Width',
                'Диаметр'=>'Diametr',
                'Wheel type'=>'Type',
                'LZ'=>'Krep',
                'PCD'=>'PCD',
                'ET'=>'ET',
                'DIA'=>'DIA',
                'Color'=>'Color',
                'GTIN'=>'GTIN',
            ],
            'Krep'=>[
                'Тип крепежа'=>'TypeKrep',
                'Описание'=>'Description',
                'Длина'=>'Length',
                'Длина резьбы'=>'Thread',
                'Размер под ключ'=>'KeySize',
                'Форма'=>'Form',
                'Тип упаковки'=>'Package',
                'Кол-во в упаковке'=>'Amount',
                'Color'=>'Color',
                'SubCategory'=>'SubCategory'
            ]
        ],

    ];
    return $formats;
}