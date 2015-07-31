/**
 * Created by tornodo on 14-9-9.
 */
var isLog = false;
function infoSelect(){
    setClassName('tongji','class','current');
    setClassName('huodong','class','');
    setClassName('guanggao','class','');
    showElement('base-info');
    hideElement('guangguao-control');
    hideElement('huodong-control');
    setInnerHtml('usercount', document.cookie);
}

function huodongSelect(){
    setClassName('tongji','class','');
    setClassName('huodong','class','current');
    setClassName('guanggao','class','');
    showElement('huodong-control');
    hideElement('base-info');
    hideElement('guangguao-control');
}

function guanggaoSelect(){
    setClassName('tongji','class','');
    setClassName('huodong','class','');
    setClassName('guanggao','class','current');
    showElement('guangguao-control');
    hideElement('base-info');
    hideElement('huodong-control');

    var callback = function(json) {
        var result = eval("("+json+")");
        setInnerHtml('show_uid', result.uid);
        setInnerHtml('show_nick', result.nick);
        setInnerHtml('show_pwd', result.pwd);
    };

    connectServer('queryInfo', 'user=' + document.getElementById('user').value, callback);
}

function readCookie() {
    var flag = getCookie('flag');
    if(flag === '1'){
        setValue('flag', '1');
        isLog = true;
    }
    var uid = getCookie('uid');
    if(uid != null) {
        setValue('user', uid);
        setInnerHtml('userid', uid);
    }
}

function loginOut(){
    var callback = function(response){
        window.location.href=window.location.href;
    };
    asyncPost('loginOut','login_out=',callback);
    delCookie('flag');
    delCookie('uid');
    isLog = false;
}

function onLoad(){
    readCookie();
    if(isLog === true){
        loginSuccess();
        infoSelect();
    }
}

function showQueryErr(text){
    showError('payerror','errormsg',text);
}

function showQuerySucc(text){
    showError('paysuccess','successmsg',text);
}

//改变用户信息
function changeInfo(){
    var nick = document.getElementById('nick').value;
    var password = document.getElementById('new_password').value;
    var password_again = document.getElementById('new_password_again').value;
    if (validate_name(nick) == false) {
        showQueryErr( "无效的用户名！");
        return false;
    }
    if (validate_pass(password) == false) {
        showQueryErr("无效的密码，最小6位长度！");
        return false;
    }
    if(password != password_again) {
        showQueryErr("两次输入密码不一致！");
        return false;
    }

    var callback = function(json) {
        setValue('nick', '');
        setValue('new_password', '');
        setValue('new_password_again', '');
    };

    connectServer('change', 'user=' + document.getElementById('user').value + '&nick=' + nick + '&password=' + hex_md5(password), callback);
}

function guanggao(){
    connectServer('action=3','3');
}

function connectServer(url,data, callback){
    var hideImg = function(){
        document.getElementById('loading_pay').style.display = "none";
    }
    var errInfo = function(errmsg){
        hideImg();
        showQueryErr(errmsg);
    }
    var setInfo = function(json){
        hideImg();
        callback(json);
    }
    var result = function(response){
        if(response == 0) {
            hideImg();
            showQuerySucc('操作成功');
        }
    }
    document.getElementById('loading_pay').style.display = "block";
    queryServer(url, data, setInfo, errInfo, result);
}

function queryServer(url, data, setInfo, errInfo, result){
    var errmsg = '连接服务端失败！';
    var callback = function(response){
        var ret = eval("("+response+")");
        var resultNO = parseInt(ret.result);
        if(isNaN(resultNO)){
            errInfo(errmsg);
            return;
        }
        if(resultNO == -1){
            errInfo(errmsg);
        }else if(resultNO == -2){
            errInfo('请重新登陆！');
            delCookie('flag');
            delCookie('uid');
            window.location.href=window.location.href;
        }else if(resultNO == -3){
            errInfo('登陆失败！');
        }else{
            setInfo(response);
            result(resultNO);
        }
    }
    asyncPost(url,data,callback);
}

function showLoginErr(text){
    showError('error_tips','err_m', text);
}

function loginend(){
    bodyremove('login-dialog');
    bodyremove('dialog-mask');
    loginSuccess();
}

function loginSuccess(){
    document.getElementById('weclome').style.display = 'inline';
    document.getElementById('logout').style.display = 'inline';
    document.getElementById('login').style.display = 'none';
    readCookie();
    setInnerHtml('usercount', document.cookie);
}

function validate_required() {
    var form = document.getElementById("loginform");
    var uid = form.elements["uin"];
    if (validate_name(uid.value) == false) {
        showLoginErr( "无效的用户名！");
        return false;
    }
    var pwd = form.elements["pwd"];
    if (validate_pass(pwd.value) == false) {
        showLoginErr("无效的密码，最小6位长度！");
        return false;
    }
    async_login("user=" + uid.value + "&password=" + hex_md5(pwd.value));
    return true;
}

function async_login(data){
    var callback = function(response){
        var result = eval("("+response+")");
        var c = parseInt(result.result);
        if (isNaN(c)) {
            showLoginErr("服务器错误,请稍后再试！");
        } else {
            if (c == 0) {
                loginend();
            }else{
                switch (c){
                    case -3:
                        showLoginErr("登陆失败！");
                        break;
                }
            }
        }
    }
    asyncPost('login',data,callback);
}