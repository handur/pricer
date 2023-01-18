<?php

$images=$model->images;
$image_view=[];
if(empty($images)) $images[]='public://products/dummy.png';
foreach($images as $image){
    $image_view[]="<img src='".image_style_url("big", $image)."' class='img-responsive'/>";
}
?>
<div class="model full">
    <div class="content">
        <div class="row">
            <div class="col-xs-4">
                <div class="image">
                    <?php print $image_view;?>
                </div>
            </div>
            <div class="col-xs-8">
                !!!
            </div>
        </div>



    </div>
</div>
