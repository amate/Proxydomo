/* ===================================
　　　　　　　Proxydomo　　　　　　　　　　　　
 ==================================== */
 
■はじめに
このソフトはローカルで動くプロクシフィルタリングソフトです
作成にあたりオープンソースのProximodoを使って作られています。

■使い方
起動して　127.0.0.1:6060(プロクシポートに表示されてる数値)をプロクシとして指定すると
プロクシフィルタとして機能します。
詳しい使い方はProxomitronなどを参考にしてください。

$LSTの指定方法は listsフォルダ以下にあるテキストから拡張子を消したものです
※例: lists\Kill.txt -> $LST(Kill)

■既知のバグ
・一部実装していないコマンドがあります($ADDLSTBOXなど)

■免責
作者(原著者＆改変者)は、このソフトによって生じた如何なる損害にも、
修正や更新も、責任を負わないこととします。
使用にあたっては、自己責任でお願いします。
 
何かあれば下記のURLにあるメールフォームにお願いします。
http://www31.atwiki.jp/lafe/pages/33.html
 
■著作権表示
Copyright (C) 2004 Antony BOUCHER
Copyright (C) 2013-2016 amate
 
 画像の一部に「VS2010ImageLibrary」の一部を使用しています。
 
■ビルドについて
Visual Studio 2015 Communityが必要です
ビルドには boost(1.60~)と zlib(v1.2.8~) と WTL(v91_5321_Final) と ICU(v55.1~) と wolfSSL(v3.9.6~) が必要なのでそれぞれ用意してください。

◆boost
http://www.boost.org/

◆zlib
http://zlib.net/

◆ICU
http://site.icu-project.org/

◆WTL
http://sourceforge.net/projects/wtl/

◆wolfSSL
http://www.wolfssl.com/yaSSL/Home.html

□コンパイル済みdll
zlibのコンパイル済みdllを下記のURLで公開しています
http://1drv.ms/1vqvcaG


zlibのヘッダの場所
$(SolutionDir)zlib\x86\include
$(SolutionDir)zlib\x64\include
zlibのライブラリの場所
$(SolutionDir)zlib\x86\lib
$(SolutionDir)zlib\x64\lib
を以下の場所にすればとくに設定はいらないはずです
これ以外の場所にzlibを置いているなら適当にzlibbuffer.h/cppを修正してください

ICU は
$(SolutionDir)icu\Win32 or Win64 フォルダに include と lib があればコンパイル通るようになっています

boost::shared_mutexを使用するのでboost::threadのライブラリが必要になります
 Boostライブラリのビルド方法
 https://sites.google.com/site/boostjp/howtobuild
コマンドライン
// x86
b2.exe install -j 4 --prefix=lib toolset=msvc-14.0 define=BOOST_USE_WINAPI_VERSION=0x0501 runtime-link=static --with-thread --with-date_time --with-timer --with-log
// x64
b2.exe install -j 4 --prefix=lib64 toolset=msvc-14.0 define=BOOST_USE_WINAPI_VERSION=0x0501 runtime-link=static address-model=64 --with-thread --with-date_time --with-timer --with-log

□wolfssl
$(SolutionDir)wolfssl\wolfssl.vcxproj
のプロジェクトを読み込むようにしているので
あとはプリプロセッサの定義の変更だけすればビルドが通るようにしています

それ以外の場合
wolfsslがソリューションエクスプローラーに読み込まれていないとビルドが通らないので
一度消してプロジェクトを追加する必要があります

Proxydomoのソリューションエクスプローラーからwolfsslのプロジェクトを削除して
既存のプロジェクトの追加で wolfssl\wolfssl.vcxproj を追加してください

wolfsslのプロパティページでプリプロセッサ->プリプロセッサの定義の内容を以下の内容に書き換える

※プリプロセッサの定義の変更
// for Debug/Release Win32
WOLFSSL_LIB
WOLFSSL_CERT_GEN
WOLFSSL_KEY_GEN
HAVE_AESGCM
HAVE_AESCCM
HAVE_CAMELLIA
HAVE_ECC
HAVE_OCSP
HAVE_TLS_EXTENSIONS
HAVE_SESSION_TICKET
HAVE_SNI
HAVE_SECURE_RENEGOTIATION
HAVE_SUPPORTED_CURVES
HAVE_CERTIFICATE_STATUS_REQUEST_V2
WOLFSSL_RIPEMD
WOLFSSL_SHA384
WOLFSSL_SHA512
WOLFSSL_STATIC_RSA
NO_RC4
NO_HC128
NO_PSK

// for Debug/Release x64
WOLFSSL_LIB
WOLFSSL_CERT_GEN
WOLFSSL_KEY_GEN
HAVE_AESGCM
HAVE_AESCCM
HAVE_CAMELLIA
HAVE_ECC
HAVE_OCSP
HAVE_TLS_EXTENSIONS
HAVE_SESSION_TICKET
HAVE_SNI
HAVE_SECURE_RENEGOTIATION
HAVE_SUPPORTED_CURVES
HAVE_CERTIFICATE_STATUS_REQUEST_V2
WOLFSSL_RIPEMD
WOLFSSL_SHA384
WOLFSSL_SHA512
WOLFSSL_STATIC_RSA
NO_RC4
NO_HC128
NO_PSK
WOLFSSL_AESNI

wolfsslのプロパティページ C/C++ -> コード生成
ランタイムライブラリを 構成がDebug なら"マルチスレッド デバッグ (/MTd)" へ変更
Releaseなら"マルチスレッド (/MT)" へ変更してください

プロジェクトの依存関係の設定で プロジェクト:Proxydomo 依存先のwolfssl にチェックを入れてください
ソリューションエクスプローラーにあるProxydomoの"参照"に wolfssl を追加してください

これでビルドが通るはずです


v1.66の以下の修正はwolfSSL側のソースを修正する必要があります
・サーバーから送られてくるルートCA証明書がおかしいとき"ASN no signer error to confirm failure"の警告が出るのを修正
internal.cの 6271行目あたりを

// before
WOLFSSL_MSG("Failed to verify CA from chain");

// after
WOLFSSL_MSG("Failed to verify CA from chain");

if (count > 2) {
	if (ret == ASN_NO_SIGNER_E || ret == ASN_SIG_CONFIRM_E) {
		ret = 0;	// 最上位のみ見逃す
	}
}
// ==============================================

■開発支援
https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=D4WWS368DZKPS
