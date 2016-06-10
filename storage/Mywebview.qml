import QtQuick 2.5
import QtQuick.Window 2.2
//import QtWebEngine 1.1
import QtWebKit 3.0
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.4

Window {
    id:webview
    visible: true
    width: 600
    height: 400



    function parsing() {
        var http = new XMLHttpRequest();
        var json , parse , text , rev_id;

        http.onreadystatechange = function(){
            if(http.readyState == 4 && http.status == 200)
            { json = http.responseText;

                parse = JSON.parse(json);
                rev_id = parse.parse.revid;
                console.log(rev_id);

                text = parse.parse.text["*"];
                //console.log(text);
                 // <-- STRIP ME (o.O)
                while(text.match(/&#39;\/index.php/)){
                text = text.replace(/&#39;\/index.php/, "http://en.wikitolearn.org/index.php");
                text = text.replace(/&amp;/,"&");
                text = text.replace(/MathShowImage&amp;/, "MathShowImage&")
                text = text.replace(/mode=mathml&#39;/, "mode=mathml\"");
                text = text.replace(/<meta class="mwe-math-fallback-image-inline" aria-hidden="true" style="background-image: url\(/ ,"<img style=\"background-repeat: no-repeat; background-size: 100% 100%; vertical-align: -0.838ex;height: 2.843ex; \" src=\"");
                text = text.replace(/<meta class="mwe-math-fallback-image-display" aria-hidden="true" style="background-image: url\(/ ,"<img style=\"background-repeat: no-repeat; background-size: 100% 100%; vertical-align: -0.838ex;height: 2.843ex; \" src=\"");
                text = text.replace(/&amp;mode=mathml\"/ , "&mode=mathml>\"");

                }
                console.log(text); // after strip :p .

                webEngineView.loadHtml(text);
                return(text);
            }
        };
        http.open('GET','http://en.wikitolearn.org/api.php?action=parse&page=Linear%20Algebra/Sets&format=json');
        http.send();
    }


    WebView{
        id: webEngineView
        anchors.fill: parent
    }


    Component.onCompleted: parsing()

}
