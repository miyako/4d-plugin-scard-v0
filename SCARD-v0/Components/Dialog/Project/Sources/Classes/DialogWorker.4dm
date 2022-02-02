Class constructor
	
	
	
Function get_worker_identifier()->$workerIdentifier : Text
	
	var $window : Integer
	
	$window:=Current form window:C827
	
	var $table : Pointer
	
	$table:=Current form table:C627
	
	var $name : Text
	
	$name:=Current form name:C1298
	
	If (Is nil pointer:C315($table))
		$workerIdentifier:=New collection:C1472($name; "#"; $window).join()
	Else 
		$workerIdentifier:=New collection:C1472(Table name:C256($table); "."; $name; "#"; $window).join()
	End if 
	
Function startWorker($method : 4D:C1709.Function)
	
	ASSERT:C1129(OB Instance of:C1731($method; 4D:C1709.Function))
	
	If (This:C1470.worker=Null:C1517)
		
		var $o : Object
		
		$o:=New object:C1471
		
		$o.name:=Get window title:C450(Current form window:C827)
		$o.method:=$method.source
		$o.formula:=$method
		$o.window:=Current form window:C827
		$o.interval:=Num:C11(Form:C1466.interval)
		$o.executeOnServer:=Bool:C1537(Form:C1466.executeOnServer)
		$worker:=This:C1470.get_worker_identifier()
		
		If (Not:C34($o.executeOnServer))
			$o.name:=$o.name+"@"+Generate UUID:C1066
		End if 
		
		This:C1470.name:=$o.name
		This:C1470.method:=$o.method
		This:C1470.formula:=$o.formula
		This:C1470.window:=$o.window
		This:C1470.interval:=$o.interval
		This:C1470.executeOnServer:=$o.executeOnServer
		This:C1470.worker:=$worker
		
		CALL WORKER:C1389($worker; "execute_dialog_worker"; $o)
		
	End if 
	
Function stopWorker()
	
	If (This:C1470.worker#Null:C1517)
		
		$worker:=This:C1470.worker
		
		KILL WORKER:C1390($worker)
		
	End if 
	
Function callWorker($params : Object)
	
	If (This:C1470.worker#Null:C1517)
		
		var $o : Object
		
		$o:=New object:C1471
		
		$o.name:=This:C1470.name
		$o.method:=This:C1470.method
		$o.formula:=This:C1470.formula
		$o.window:=This:C1470.window
		$o.interval:=This:C1470.interval
		$o.executeOnServer:=This:C1470.executeOnServer
		$o.params:=$params
		
		$worker:=This:C1470.worker
		
		CALL WORKER:C1389($worker; "execute_dialog_worker"; $o)
		
	End if 
	