
<div class="row">
    <div class="col-md-3">
        <fieldset>
            <legend>Данные поставщика</legend>
        <?php print render($form['name']);?>
        <?php print render($form['machine_key']);?>
            <?php print render($form['b2b']);?>
        <?php print render($form['params']);?>
        </fieldset>
    </div>
    <div class="col-md-9">
        <?php print render($form['stocks_container']);?>
    </div>
</div>
<fieldset>
    <legend>Настройки цен</legend>
    <table><tr>
<?php
foreach(element_children($form['setup_container']['group']) as $key):
?>
    <td><div class="setup-wrapper">
            <?php print render($form['setup_container']['group'][$key]);?></div>
    </td>
    <?php endforeach;?>
        </tr>
    </table>
</fieldset>
<fieldset>
    <legend>Настройка брендов</legend>
    <table><tr>
            <?php
            foreach(element_children($form['brands']) as $key):
                ?>
                <td>
                    <div class="setup-wrapper">
                    <h3><?php print $form['brands'][$key]['#title'];?></h3>
                    <?php print render($form['brands'][$key]);?>
                    </div>
                </td>
            <?php endforeach;?>
        </tr>
    </table>
</fieldset>
<?php
print drupal_render_children($form);