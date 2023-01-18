<fieldset>
    <legend>Фильтр</legend>
<div class="row">
    <div class="col-md-2">
        <?php print render($form['time_from']);?>
    </div>
    <div class="col-md-2">
        <?php print render($form['time_to']);?>
    </div>
    <div class="col-md-4">
        <?php print render($form['type']);?>
    </div>
    <div class="col-md-4">
        <?php print render($form['process']);?>
    </div>
    <div class="col-md-12">
        <?php print render($form['submit']);?>
    </div>
</div>
<?php
    print drupal_render_children($form);
?>
</fieldset>
