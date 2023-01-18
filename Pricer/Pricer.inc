<?php
define("PRICER_UPLOAD_DIR",$_SERVER['DOCUMENT_ROOT']."/prices");

require_once("Autoload.inc");

$classLoader = new SplClassLoader('Pricer',__DIR__);
$classLoader->register();