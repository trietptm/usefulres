function add_to_wz() {
    var width = 480;
    var height = 480;
    var leftVal = (screen.width - width) / 2;
    var topVal = (screen.height - height) / 2;
    var d = document;
    var t = d.selection ? (d.selection.type != 'None' ? d.selection.createRange().text : '') : (d.getSelection ? d.getSelection() : '');
    window.open('http://wz.cnblogs.com/create?t=' + escape(d.title) + '&u=' + escape(d.location.href) + '&c=' +
     escape(t) + '&i=0', '_blank', 'width=' + width + ',height=' + height + ',toolbars=0,resizable=1,left=' + leftVal + ',top=' + topVal);
}
function cnblogs_code_show(id) {
    if ($('#cnblogs_code_open_' + id).css('display') == 'none') {
        $('#cnblogs_code_open_' + id).show();
        $('#code_img_opened_' + id).show();
        $('#code_img_closed_' + id).hide();
    }
}
function cnblogs_code_hide(id, event) {
    if ($('#cnblogs_code_open_' + id).css('display') != 'none') {
        $('#cnblogs_code_open_' + id).hide();
        $('#code_img_opened_' + id).hide();
        $('#code_img_closed_' + id).show();
        if (event.stopPropagation) {
            event.stopPropagation();
        }
        else if (window.event) {
            window.event.cancelBubble = true;
        }
    }
}


