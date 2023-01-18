<?php

/**
 * Created by PhpStorm.
 * User: lushanov
 * Date: 18.05.2021
 * Time: 12:37
 */
class KolesaGetMail
{
    private static $mail="price@kolesa812.ru";
    private static $server="{imap.yandex.ru:993/imap/ssl}";
    private static $login="price@kolesa812.ru";
    private static $pass='prodam$kolesa';

    private function connection(){
        $server=self::$server;
        $login=self::$login;
        $pass=self::$pass;

        $connection = imap_open($server."INBOX", $login, $pass);

        return $connection;
    }

    public static function getMail($from='',$fromdate='',$limit=1){

        $connection=self::connection();
        $criteria="";
        if(!empty($from)) $criteria.='FROM "'.$from.'"';

        $mails=imap_sort($connection,SORTDATE,TRUE,0,$criteria);
        if($limit){
            $mails=array_slice($mails,0,$limit);
        }


        /*if(!empty($from)){
            $mails = imap_search($connection,'FROM "'.$from.'"');
        } else $mails = imap_search($connection,'NEW');
*/
        if($mails){
            $get_mails=[];
            foreach($mails as $num_mail){
                $header = imap_header($connection, $num_mail);
                // достаем ящик отправителя письма
                #if(!empty($fromdate)) if($header->udate<$fromdate) continue;
                $mail_from = $header->sender[0]->mailbox . "@" . $header->sender[0]->host;
                $attachments=self::getAttachments($connection,$num_mail);
                $get_mails[$mail_from][]=[
                    'date'=>$header->date,
                    'subject'=>$header->subject,
                    'attachments'=>$attachments,
                ];
            }

            return $get_mails;
        }
        return NULL;
    }

    private static function getAttachments($connection,$num_mail){
        $structure = imap_fetchstructure($connection, $num_mail);
        $attachments = array();
        if(isset($structure->parts) && count($structure->parts)) {
            for($i = 0; $i < count($structure->parts); $i++) {
                $attachments[$i] = array(
                    'is_attachment' => false,
                    'filename' => '',
                    'name' => '',
                    'attachment' => ''
                );
                if($structure->parts[$i]->ifdparameters) {
                    foreach($structure->parts[$i]->dparameters as $object) {
                        if(strtolower($object->attribute) == 'filename') {
                            $attachments[$i]['is_attachment'] = true;
                            $attachments[$i]['filename'] = imap_utf8($object->value);
                        }
                    }
                }
                if($structure->parts[$i]->ifparameters) {
                    foreach($structure->parts[$i]->parameters as $object) {
                        if(strtolower($object->attribute) == 'name') {
                            $attachments[$i]['is_attachment'] = true;
                            $attachments[$i]['name'] = $object->value;
                        }
                    }
                }
                if($attachments[$i]['is_attachment']) {
                    $attachments[$i]['attachment'] = imap_fetchbody($connection, $num_mail, $i+1);
                    if($structure->parts[$i]->encoding == 3) { // 3 = BASE64
                        $attachments[$i]['attachment'] = base64_decode($attachments[$i]['attachment']);
                    }
                    elseif($structure->parts[$i]->encoding == 4) { // 4 = QUOTED-PRINTABLE
                        $attachments[$i]['attachment'] = quoted_printable_decode($attachments[$i]['attachment']);
                    }
                }
            }
        }
        if(count($attachments)!=0){
            foreach($attachments as &$at){
                if($at['is_attachment']==1){
                    file_put_contents("public://email/".$at['filename'], $at['attachment']);
                    $at['saved']="public://email/".$at['filename'];
                }
            }
        }
        return $attachments;
    }

}