//%attributes = {"invisible":true,"shared":true,"preemptive":"capable"}
#DECLARE($data : Object; $signal : 4D:C1709.Signal)  //何も返してはいけない

Case of 
	: (This:C1470#Null:C1517)
		
/*
		
このブロックはバックグラウンドプロセスとの通信をするプロセスです。
バックグラウンドプロセスからSignalで結果が返れると実行されます。
プリエンプティブスレッドなので必要であればCALL FORMでダイアログを更新します。
		
*/
		
		CALL FORM:C1391(This:C1470.params.window; This:C1470.params.method; $data)
		
		KILL WORKER:C1390
		
	: (Count parameters:C259=2)
		
/*
		
このブロックがバックグラウンドプロセスです。
executeOnServerがTrueであればサーバー側プロセスとなります。
intervalで指定した秒間隔で呼ばれます。
プリエンプティブスレッドです。
		
*/
		
		ASSERT:C1129(OB Instance of:C1731($signal; 4D:C1709.Signal))
		
		var $returnValue : Object
		
		$returnValue:=New shared object:C1526
		
		Use ($signal)
			$signal.data:=$returnValue
		End use 
		
		var $options : Object
		var $info : Collection
		
		$information:=New collection:C1472
		
		$status:=SCARD Get readers($options)
		
		If ($status.success)
			If ($status.readers.length#0)
				$reader:=$status.readers.shift()
				
/*
				
カードリーダーの名称で対応の可否を判定します。
				
*/
				
				If ($reader.slotName="SONY FeliCa RC-S300/@") | ($reader.slotName="VMware Virtual USB CCID@") | ($reader.slotName="SONY FeliCa Port/PaSoRi@")
					
/*
					
読み取りたいサービスとブロック数をオブジェクト型で渡します。
					
*/
					
					$reader.services:=New collection:C1472
					
					$reader.services.push(New object:C1471("code"; "090f"; "size"; 20))
					
					$status:=SCARD Read tag($reader)
					
					If ($status.uuid#Null:C1517)
						
						Repeat 
							DELAY PROCESS:C323(Current process:C322; 6)
							$status:=SCARD Get status($status.uuid)
							If (OB Is empty:C1297($status))
								$status:=SCARD Read tag($reader)
							Else 
								If ($status.complete)
									If ($status.success)
										
/*
										
ここでデータを読み取ります。
										
*/
										
										$stationCode:=cs:C1710.StationCode.new()
										
										For each ($service; $status.services)
											
											If ($service.code="090f")
												
												$information:=$stationCode.parse($service.data)
												
											End if 
											
										End for each 
										
									End if 
								End if 
							End if 
						Until ($status.complete)
						
					End if 
				End if 
			End if 
		End if 
		
		Use ($returnValue)
			
			$returnValue.data:=$information.copy(ck shared:K85:29; $returnValue)
			
		End use 
		
		If (Not:C34(Process aborted:C672))
			$signal.trigger()
		End if 
		
		KILL WORKER:C1390
		
	: (Count parameters:C259=1)
		
		$dialog:=cs:C1710.Dialog.new()
		
		If (Not:C34($dialog.is_preemptive()))
			
			//%T-
			If (Form:C1466#Null:C1517)
				
				Form:C1466.data:=New object:C1471("col"; $data.data; "sel"; Null:C1517; "item"; Null:C1517; "pos"; Null:C1517)
				
			End if 
			//%T+
		End if 
		
	: (Count parameters:C259=0)
		
/*
		
このブロックは予備です。
使用していません。
		
*/
		
		$dialog:=cs:C1710.Dialog.new()
		
		If (Not:C34($dialog.is_preemptive()))
			
			//%T-
			If (Form:C1466#Null:C1517)
				
				TRACE:C157
				
			End if 
			//%T+
		End if 
		
End case 