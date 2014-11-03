jQuery(document).ready(function ($, Drupal)
{

    jQuery("td.views-field-nothing").click(function () {
        var value = jQuery(this).closest('tr').children('td:first').text();
        id = jQuery.trim(value);
        jQuery.ajax({
            type: "POST",
            url: "download_csv/" + id,
            data: value,
            success: function (data) {
                window.location = 'download_csv/' + id;
            }
        });
    });
});