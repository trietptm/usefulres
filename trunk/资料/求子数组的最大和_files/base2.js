String.prototype.trim = function(){return this.replace(/(^[ |��]*)|([ |��]*$)/g, "");}
function $(s){return document.getElementById(s);}
function $$(s){return document.frames?document.frames[s]:$(s).contentWindow;}
function $c(s){return document.createElement(s);}
function initSendTime(){
	SENDTIME = new Date();
}
var err;
var cnt=0;
function commentSubmit(theform,refurlk){
	
	if(document.readyState=="complete"){

	var sDialog = new dialog();
	sDialog.init();
	if(!theform){
		sDialog.event("������ <a href='http://home.51cto.com/index.php?reback="+ encodeURIComponent(encodeURIComponent(refurlk)) +"' style='color:BLUE;'>��¼</a> �� <a href='http://passport.51cto.com/reg.php?reback=" + refurlk + "' style='color:BLUE;'>ע��</a> ���ٽ��д��������",'');
		sDialog.button('dialogOk','void 0');
		$('dialogOk').focus();
		return false;
	}

	var smsg =theform.content.value;
	var susername = theform.username.value;
	var sauthnum = theform.authnum.value;
	var sartID = theform.tid.value;

	if(smsg == ''){
		sDialog.event('��������������!','');
		sDialog.button('dialogOk','void 0');
		$('dialogOk').focus();
		return false;
	}
	if(sauthnum == ''){
		sDialog.event('��������֤��!','');
		sDialog.button('dialogOk','void 0');
		$('dialogOk').focus();
		return false;
	}
	if(sartID == ''){
		sDialog.event('������Ч������','');
		sDialog.button('dialogOk','void 0');
		$('dialogOk').focus();
		return false;
	}

	
	var url = "/commentcheckform.php?authnum="+sauthnum;
	var ajax = new ActiveXObject("MSXML2.XMLHTTP.3.0");
	ajax.open("GET", url, false);
	ajax.send();
	err=ajax.responseText;
	if(!err){
		var ajax = new new XMLHttpRequest();
		ajax.open("GET", url, false);
		ajax.send();
		err=ajax.responseText;
	}
	if(err == "-1"){
		sDialog.event('��֤���������!','');
		sDialog.button('dialogOk','void 0');
		$('dialogOk').focus();
		return false;
	}
	//initSendTime();
	//$("src_title").value = $("commentText").innerHTML;
	//$("src_uname").value = $('feedback').submit();
	}
	cnt++;
	if (cnt!=1){
		return false;
	}
}