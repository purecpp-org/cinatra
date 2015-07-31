/**
 * Created by tornodo on 14-9-10.
 */
/**
 * Created by tornodo on 14-6-14.
 */

(function(window,module){
    var module = window[module] = {};
    var loginDialogId = 'login-dialog';
    var dlgMaskId = 'dialog-mask';
    var innerText = '';

    window.onresize = function(width,height){
        var loginDialog = document.getElementById(loginDialogId);
        if(loginDialog){
            loginDialog.style.width = width + 'px';
            loginDialog.style.height = height + 'px';
            loginDialog.style.marginLeft = -Math.floor(width / 2) + 'px';
            loginDialog.style.marginTop = -Math.floor(height / 2) + 'px';
            loginDialog.style.visibility = 'hidden';
            loginDialog.style.visibility = 'visible';
        }
    };
    module.login = function(){
        try{
            document.body.removeChild(document.getElementById(loginDialogId));
            document.body.removeChild(document.getElementById(dlgMaskId));
        }catch (error){

        }
        var loginDialog = document.createElement("div");
        loginDialog.setAttribute('id',loginDialogId);
        document.body.appendChild(loginDialog);
        //loginDialog.innerHTML = '<iframe frameborder="0" scrolling="no" width="100%" height="100%" src="login"></iframe>';
        loginDialog.innerHTML = innerText;
        module.alert(loginDialogId,'422','287');
    };
    module.alert = function(id,width,height){
        var setCenter = function(obj,width,height){
            var x = Math.max(Math.round(width / 2),0),y = Math.max(Math.round(height / 2),0);
            var scrollLeft = document.body.scrollLeft,scrollTop = document.body.scrollTop;
            obj.style.zIndex = 65535;
            obj.style.background = '#fff';
            obj.style.position = 'absolute';
            obj.style.left = '50%';
            obj.style.top = '50%';
            obj.style.height = height + 'px';
            obj.style.width = width + 'px';
            obj.style.marginLeft = -x + scrollLeft + 'px';
            obj.style.marginTop = -y + scrollTop + 'px';
            obj.style.filter = 'Alpha(opacity=100)';
            obj.style.opacity = 1;

        },floatDiv = function(){
            var dlgMask = document.createElement('div');
            dlgMask.setAttribute('id',dlgMaskId);
            dlgMask.style.background = '#000';
            dlgMask.style.width = '100%';
            dlgMask.style.height = document.body.scrollHeight + 'px';
            dlgMask.style.position = 'absolute';
            dlgMask.style.top = '0';
            dlgMask.style.left = '0';
            dlgMask.style.zIndex = '5000';
            dlgMask.style.opacity = 0.3;
            dlgMask.style.filter = 'Alpha(opacity=30)';
            document.body.appendChild(dlgMask);

        };
        var dlg = document.getElementById(id);
        floatDiv();
        dlg.setAttribute('class','float');
        setCenter(dlg,width,height);
    };
    module.init = function(){
        var flag = getCookie('flag');
        if(flag !== '1') {
            this.login();
            return;
        }
    };
    module.load = function(){
        var a = ajaxInit();
        if (a === false) {
            alert("您当前浏览器不支持该功能，请使用其它浏览器重试！");
            return false;
        }
        a.open("get", "template/login", true);
        a.onreadystatechange = function() {
            if (a.readyState == 4) {
                if (a.status == 200) {
                    innerText = a.responseText;
                    module.init();
                }
            }
        }
        a.send(null);
    };
    module.load();
})(window,'account');

