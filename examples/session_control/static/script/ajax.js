/**
 * Created by tornodo on 14-6-14.
 */
function ajaxInit() {
    var c = false;
    if (window.XMLHttpRequest) {
        c = new XMLHttpRequest();
        if (c.overrideMimeType) {
            c.overrideMimeType("text/xml");
        }
    } else {
        if (window.ActiveXObject) {
            var a = ["Microsoft.XMLHTTP", "MSXML.XMLHTTP", "Msxml2.XMLHTTP.7.0", "Msxml2.XMLHTTP.6.0", "Msxml2.XMLHTTP.5.0", "Msxml2.XMLHTTP.4.0", "MSXML2.XMLHTTP.3.0", "MSXML2.XMLHTTP"];
            for (var b = 0; b < a.length; b++) {
                try {
                    c = new ActiveXObject(a[b]);
                    if (c) {
                        return c;
                    }
                } catch (d) {
                    c = false;
                }
            }
        }
    }
    return c;
}

function asyncGet(url,callback){
    var ajax = ajaxInit();
    if (ajax === false) {
        return false;
    }
    ajax.open("get", url, true);
    ajax.onreadystatechange = function() {
        if (ajax.readyState == 4) {
            if (ajax.status == 200) {
                callback(ajax.responseText);
            }
        }
    }
    ajax.send(null);
    return true;
}

function asyncPost(url,data,callback){
    var ajax = ajaxInit();
    if(ajax === false){
        return false;
    }
    ajax.open("post", url, true);
    ajax.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
    ajax.onreadystatechange = function() {
        var result = '{"result":-1}';
        if (ajax.readyState == 4) {
            if (ajax.status == 200) {
                result = ajax.responseText;
            }
            callback(result);
        }
    };
    ajax.send(data);
    return true;
}