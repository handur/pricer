<?php
$title=$model->name;
$images=$model->images;
if(empty($images)) $image='public://products/dummy.png';
else $image=$images[0];
$image_url=image_style_url("medium", $image);
$link='/catalog/'.$model->brand_id.'/'.$model->id;
?>
<div class="model teaser">
    <div class="content">
        <div class="image">
        <img src="<?php print $image_url;?>" class="img-responsive"/>
        </div>
        <div class="title">
        <?php print $title;?>
        </div>
        <div class="data">
            <p>Кол-во размеров: <?php print $model->productCount;?></p>
        </div>
        <a href="<?php print $link;?>" class="overlink"></a>
    </div>
</div>
