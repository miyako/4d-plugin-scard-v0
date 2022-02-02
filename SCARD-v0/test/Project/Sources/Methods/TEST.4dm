//%attributes = {}
/*

ダイアログを表示するための汎用クラス

プリエンプティブモードでバックグラウンド処理を実行する（ローカルまたはサーバー側）

スレッドと通信するローカルワーカー：TEST#{windowID}
バックグラウンド処理：テスト画面@{UUID}

*/

$dialog:=cs:C1710.Dialog.new()

$params:=New object:C1471

$params.title:="テスト画面"
$params.type:=Plain window:K34:13
$params.name:="TEST"
$params.form:=New object:C1471("executeOnServer"; False:C215; "method"; Formula:C1597(test_background_worker); "interval"; 1)

$dialog.run($params)