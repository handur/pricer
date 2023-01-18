<?php
namespace Pricer\Load;

Class LoadFTP extends LoadDefault {
    public $name='FTP';
    protected $url;
    protected $requiredOptions=array('load url','login','pass','host');
    public function __construct(array $options){
        parent::__construct($options);
        $this->url=$options['load url'];
        $this->ftp=['host'=>$options['host'],'login'=>$options['login'],'pass'=>$options['pass']];
    }
    public function load(){

        $ftpUrl="ftp://".$this->ftp['login'].":".$this->ftp['pass']."@".$this->ftp['host'].'/'.$this->url;
        $data=file_get_contents($ftpUrl);
        if(!empty($data)) return $data;
        return FALSE;
    }
}

