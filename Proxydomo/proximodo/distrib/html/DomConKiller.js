//
// Proxomitron DOM based container killer v0.007
// See "DomConKiller.txt" for details and usage
// By Scott R. Lemmon
//

var KillScore=25;
var PrxDebug=0;

function PrxOut(msg){
  if(PrxDebug){if(!confirm(msg)){PrxDebug=0;}}
}

function PDomHide(){
  var a=document.getElementsByName("PDomTarget");
  while(a[0]){
    var x=a[0].parentNode;
    a[0].parentNode.removeChild(a[0]);
    var d=6;
    while(d-- && x){
      var y=x;
      var tag=y.nodeName;
      x=x.parentNode;
      if(x && tag.match(/^(TD|TR|TABLE|CENTER|TBODY|FONT|IFRAME|A|DIV|P|SPAN)$/)){
        var score=PDomSize(y);
        if(1){PrxOut(tag + ":" + score + "\\n" + y.innerHTML);}
        if(score > KillScore){d=0;}
        else{
          y.parentNode.removeChild(y);
        }
      }
    }
  }
}

function PDomSize(t){
  var k=t.childNodes;
  var pl=k.length;
  var tot=pl;
  var score=1;
  while(pl--){tot+=PDomSize(k[pl]);}
  switch(t.nodeName){
  case "#text": 
    if(t.nodeValue.match(/(advert|banner|sponsor|promo)/ig)){score= -10;}
    else{score=(t.length/40);}
    if(t.parentNode.nodeName.match(/^(SCRIPT|STYLE|#comment)$/)){score=0;}
    break;
  case "P": score=2; break;
  case "#comment": score=0; break;
  case "SCRIPT": score=3; break;
  case "A": score=5; break;
  case "FORM": score=20; break;
  }

  return(tot+score);
}

PDomHide();
