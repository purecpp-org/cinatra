/**
 * Created by tornodo on 14-9-8.
 */


function hideElement(id){
    var element = document.getElementById(id);
    if(element){
        element.style.display = 'none';
    }
}

function showElement(id){
    var element = document.getElementById(id);
    if(element){
        element.style.display = '';
    }
}

var g_timer;
function showError(divId,msgId,text){
    var tagID = divId;
    var msgID = msgId;
    var tag=document.getElementById(divId);
    var msg=document.getElementById(msgId);
    clearError(divId,msgId);
    msg.innerHTML=text;
    tag.style.display="block"
    g_timer = setTimeout('clearError("' + tagID + '","' + msgID + '")',2000);

}

function hideError(tagID,msgID){
    var tag=document.getElementById(tagID);
    var msg=document.getElementById(msgID);
    msg.innerHTML="";
    tag.style.display="none"
}

function clearError(tagID,msgID){
    if(g_timer !== undefined){
        try{
            hideError(tagID,msgID);
            clearTimeout(g_timer);
        }catch (error){
        }
    }
}

function setClassName(id,name,attribute){
    var tag = document.getElementById(id);
    if(tag){
        tag.className = attribute;
    }
}

function setInnerHtml(id,text){
    var tag = document.getElementById(id);
    if(tag){
        tag.innerHTML = text;
    }
}

function bodyremove(id){
    document.body.removeChild(document.getElementById(id));
}

function setValue(id,value){
    var tag = document.getElementById(id);
    if(tag){
        tag.value = value;
    }
}

function  validate_name(n){
    if(n == "" || n.length > 32){
        return false;
    }
    return true;
}
function validate_pass(a) {
    if (a == "" || a.length < 6) {
        return false
    }
    return true
}

function getCookie(objName){//获取指定名称的cookie的值
    var arrStr = document.cookie.split("; ");
    for(var i = 0;i < arrStr.length;i ++){
        var temp = arrStr[i].split("=");
        if(temp[0] == objName) return unescape(temp[1]);
    }
}

function delCookie(name){//为了删除指定名称的cookie，可以将其过期时间设定为一个过去的时间
    var date = new Date();
    date.setTime(date.getTime() - 10000);
    document.cookie = name + "=a; expires=" + date.toGMTString();
}