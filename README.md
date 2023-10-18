![version](https://img.shields.io/badge/version-17%2B-3E8B93)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-scard-v0)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-scard-v0/total)

# 4d-plugin-scard-v0
macOSの標準APIでFeliCaを読む

## SCARD Get readers

```4d
status:=SCARD Get readers(options)
```

|パラメーター|データ型|説明|
|-|-|-|
|options|Object|使用していません|
|status|Object||
|status.success|Boolean||
|status.warning|Text|macOSで必要なentitlementsが付与されていない場合などに返されます|
|status.readers[]|Collection|
|status.readers[].slotName|Text|カードリーダーの名称|

## SCARD Read tag

```4d
status:=SCARD Read tag(reader)
```

|パラメーター|データ型|説明|
|-|-|-|
|reader|Object||
|reader.slotName|Text|カードリーダーの名称|
|reader.services[]|Collection|
|reader.services[].code|Text|16進数4桁のサービスコード|
|reader.services[].size|Integer|ブロックサイズ|
|status|Object||
|status.uuid|Text|セッション識別子|

## SCARD Get status

```4d
status:=SCARD Get status(uuid)
```

|パラメーター|データ型|説明|
|-|-|-|
|uuid|Text|セッション識別子|
|status|Object||
|status.uuid|Text|セッション識別子|
|status.success|Boolean|少なくともIDmとPMmが読み取れたらtrue|
|status.complete|Boolean|読み取りの成否に関係なく通信セッションが完了するとtrue|
|status.slotName|Text|カードリーダーの名称（complete=falseのときは返されない）|
|status.IDm|Text|カードのIDm（success=falseのときは返されない）|
|status.PMm|Text|カードのPMm（success=falseのときは返されない）|
|status.services[]|Collection||
|status.services[].code|Text|16進数文字列|
|status.services[].data|Text|16進数文字列|

#### 仕組み

`IDm`と`PMm`については必ずAPDUを送受信して取得します。

その他のサービスコードについては，指定したコードを指定したブロック数まで*read without encryption*で取得します。途中でカードを動かした場合など，不完全なデータが返される場合もあります（`0x0`が返されるかもしれません）。

#### macOSプラットフォーム使用上の注意

プラグインは，内部的に[`TKSmartCardSlotManager`](https://developer.apple.com/documentation/cryptotokenkit/tksmartcardslotmanager?language=objc)クラスを使用しています。このAPIまたはPCSC Framworkを公証サンドボックスアプリで使用するためには，[`com.apple.security.smartcard`](https://developer.apple.com/documentation/bundleresources/entitlements/com_apple_security_smartcard?language=objc)エンタイトルメント付きでアプリがコード署名されていなければなりません。エンタイトルメントは，プラグインではなく，アプリ本体側のコード署名に含まれている必要があります。エンタイトルメント付きでアプリをコード署名する方法については[コード署名ツール](https://github.com/miyako/4d-class-build-application)を参照してください。

#### TODO

* https://github.com/treastrain/TRETJapanNFCReader
