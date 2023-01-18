(function ($) {
    Drupal.behaviors.mappingUpdate={
        attach: function (context,settings){
            $('select[data-method=update-mapping]').once(function(){
                $(this).on('change',function(){
                    var send_data=$(this).data();
                    send_data['id']=$(this).val();

                    $.ajax({
                        type: 'POST',
                        url: '/ajax_pricer/update_mapping',
                        data: send_data,
                        success: function(){
                            $(this).removeClass('warning');
                        }
                    });

                });
            });

        }

    };
})(jQuery);
