# Proxydomo　　　　　　　　　　　　
 
## ■はじめに
このソフトはローカルで動くプロクシフィルタリングソフトです  
作成にあたりオープンソースのProximodoを使って作られています。  

## ■動作環境
・Windows10 home 64bit バージョン 20H2  
※v2.0 からは 64bit版でしか動作しません

事前に、vc_redist.x64.exe のインストールが必要になるかもしれません

## ■使い方
起動して　127.0.0.1:6060(プロクシポートに表示されてる数値)をプロクシとして指定すると
プロクシフィルタとして機能します。  
詳しい使い方はProxomitronなどを参考にしてください。  

$LSTの指定方法は listsフォルダ以下にあるテキストから拡張子を消したものです  
※例: lists\Kill.txt -> $LST(Kill)

## ■既知のバグ
・一部実装していないコマンドがあります($ADDLSTBOXなど)

## ■免責
作者(原著者＆改変者)は、このソフトによって生じた如何なる損害にも、  
修正や更新も、責任を負わないこととします。  
使用にあたっては、自己責任でお願いします。  
 
何かあれば下記のURLにあるメールフォームにお願いします。  
https://ws.formzu.net/fgen/S37403840/
 
## ■著作権表示
Copyright (C) 2004 Antony BOUCHER  
Copyright (C) 2013-2021 amate
 
画像の一部に「VS2010ImageLibrary」の一部を使用しています。
 
## ■ビルドについて
Visual Studio 2019 が必要です
ビルドには boost(1.60~)と zlib(v1.2.8~) と WTL(v91_5321_Final) と ICU(v55.1~) と OpenSSL(v3.0.0~) が必要なのでそれぞれ用意してください。

◆boost  
http://www.boost.org/

◆zlib  
http://zlib.net/

◆ICU  
http://site.icu-project.org/  
自前でビルドする場合は  
common,i18n,makedataをビルドすれば  
icudtXX.dll,icuinXX.dll,icuucXX.dllができるっぽい？  
事前にC++ ->コード生成->ランタイム ライブラリを マルチスレッド(/MT)に変更するのを忘れずに

◆WTL  
http://sourceforge.net/projects/wtl/

◆OpenSSL  
https://www.openssl.org/  
https://kb.firedaemon.com/support/solutions/articles/4000121705

◇brotli  
https://github.com/google/brotli
ソース組み込み済み

◇zstd  
https://github.com/facebook/zstd

□コンパイル済みdll  
zlibのコンパイル済みdllを下記のURLで公開しています  
http://1drv.ms/1vqvcaG


zlibのヘッダの場所  
\$(SolutionDir)zlib\x86\include  
\$(SolutionDir)zlib\x64\include  
zlibのライブラリの場所  
\$(SolutionDir)zlib\x86\lib  
\$(SolutionDir)zlib\x64\lib  
を以下の場所にすればとくに設定はいらないはずです  
これ以外の場所にzlibを置いているなら適当にzlibbuffer.h/cppを修正してください  

ICU は  
$(SolutionDir)icu\Win32 or Win64 フォルダに include と lib があればコンパイル通るようになっています

boost::shared_mutexを使用するのでboost::threadのライブラリが必要になります
 Boostライブラリのビルド方法  
 https://sites.google.com/site/boostjp/howtobuild
コマンドライン  
// x86  
b2.exe install --prefix=lib toolset=msvc-14.2 runtime-link=static --with-thread --with-date_time --with-timer --with-log  

// x64  
b2.exe install  --prefix=lib64 toolset=msvc-14.2 runtime-link=static address-model=64 --with-thread --with-date_time --with-timer --with-log

## ■更新履歴

<pre>

v2.4
・[add] "Content-Encoding: zstd" に対応
・[misc] 接続モニターのタイトルバーに表示されるアップロードとダウンロード速度のテキストにパッティングを追加

v2.3
・[fix] SSL接続中にCPU使用率が異常に高くなるバグを修正
・[add] SSL接続に30秒のタイムアウトを設定
・[add] 接続モニターへアップロードとダウンロード速度を表示するようにした
・[add] 接続モニターで選択したURLをDeleteキーで切断できるようにした
・[update] OpenSSLを v3.0.1 から v3.4.0 へ更新

・[add] 接続モニターのタイトルバーに、全接続のアップロードとダウンロード速度の合計を表示するようにした
・[change] アップロード、ダウンロード速度の計算に、CConnectionManagerが持っている s_connectionDataList を利用しないようにした(UpdateNotifyを利用する方法に変更)

v2.2
・[update] OpenSSLを v3.0 から v3.0.1 へ更新

v2.1
・[fix] 一部のSECURE RENEGOTIATIONに対応していないサイトに繋がらなかったのを修正
・[change] CAList.pemを生成せずにCA証明書を証明書ストアから読み込むように変更

v2.0
・[change] SSL/TLSライブラリを wolfSSL から OpenSSL へ変更
・[change] CA証明書の生成では必ずECC暗号を利用するようにした

</pre>


