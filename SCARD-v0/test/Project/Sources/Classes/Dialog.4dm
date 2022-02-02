Class constructor
	
	
	
Function run($params : Object)
	
	CALL WORKER:C1389(1; Formula:C1597(execute_dialog).source; $params)
	
Function find_user_window($title : Text)->$match : Integer
	
	$windows:=This:C1470.get_user_windows()
	
	C_LONGINT:C283($window)
	
	If (Not:C34(This:C1470.is_preemptive()))
		//%T-
		For each ($window; $windows) Until ($match#0)
			If ($title=Get window title:C450($window))
				$match:=$window
			End if 
		End for each 
		//%T+
	End if 
	
Function get_user_windows()->$user_windows : Collection
	
	$processes:=Get process activity:C1495(Processes only:K5:35)
	$userInterfaceProcess:=$processes.processes.query("type === :1"; Main process:K36:10)[0]
	
	$user_windows:=New collection:C1472
	
	If (Not:C34(This:C1470.is_preemptive()))
		//%T-
		ARRAY LONGINT:C221($windows; 0)
		WINDOW LIST:C442($windows; *)
		For ($i; 1; Size of array:C274($windows))
			$windowNumber:=$windows{$i}
			If (Window process:C446($windowNumber)=$userInterfaceProcess.number)
				$user_windows.push($windowNumber)
			End if 
		End for 
		//%T+
	End if 
	
Function is_preemptive()->$is_preemptive : Boolean
	
	C_LONGINT:C283($state; $mode)
	C_REAL:C285($time)
	
	PROCESS PROPERTIES:C336(Current process:C322; $name; $state; $time; $mode)
	
	$is_preemptive:=$mode ?? 1
	
Function activate_window($window : Integer)
	
	If (Not:C34(This:C1470.is_preemptive()))
		//%T-
		GET WINDOW RECT:C443($x; $y; $r; $b; $window)
		SET WINDOW RECT:C444($x; $y; $r; $b; $window)
		//%T+
	End if 
	
Function open_window($formRect : Object; $windowType : Integer; $title : Text)->$window : Integer
	
	If (Not:C34(This:C1470.is_preemptive()))
		//%T-
		If ($formRect.x=0) & ($formRect.y=0)
			C_LONGINT:C283($left; $top; $right; $bottom; $screen)
			$screen:=Menu bar screen:C441
			SCREEN COORDINATES:C438($left; $top; $right; $bottom; $screen)
			$cx:=($right-$left)/2
			$cy:=($bottom-$top)/2
			$formRect.x:=$cx-($formRect.width/2)
			$formRect.y:=$cy-($formRect.height/2)
		End if 
		$window:=Open window:C153($formRect.x; $formRect.y; $formRect.x+$formRect.width; $formRect.y+$formRect.height; $windowType; $title; "execute_dialog")
		//%T+
	End if 
	
Function get_settings_folder()->$prefFolder : Object
	
	$projectName:=Folder:C1567(fk database folder:K87:14).name
	$className:=OB Class:C1730(This:C1470).name
	
	$applicationSupportFolder:=Folder:C1567(fk user preferences folder:K87:10).parent
	$prefFolder:=$applicationSupportFolder.folder($projectName).folder($className)
	$prefFolder.create()
	
Function get_window_bounds_file($tableName : Text; $formName : Text)->$file : 4D:C1709.File
	
	$prefFolder:=This:C1470.get_settings_folder()
	
	$file:=$prefFolder.folder($tableName).file($formName+".json")
	
Function get_window_position($formIdentifier : Text; $contextData : Variant; $width : Integer; $height : Integer)->$formRect : Object
	
	C_TEXT:C284($windowContext)
	
	If (Count parameters:C259>1)
		C_BLOB:C604($data)
		VARIABLE TO BLOB:C532($contextData; $data)
		$windowContext:=Generate digest:C1147($data; SHA256 digest:K66:4)
	End if 
	
	$info:=This:C1470.split_form_identifier($formIdentifier; $windowContext)
	
	If ($info#Null:C1517)
		
		C_TEXT:C284($tableName; $formName; $context)
		$tableName:=$info.table
		$formName:=$info.form
		$context:=$info.context
		
		$formFile:=This:C1470.get_window_bounds_file($tableName; $formName; $context)
		
		C_OBJECT:C1216($formRect)
		
		C_LONGINT:C283($left; $top; $right; $bottom)
		
		$appVersion:=Application version:C493
		If (Not:C34(This:C1470.is_preemptive()))
			//%T-
			$screen:=Menu bar screen:C441
			SCREEN COORDINATES:C438($left; $top; $right; $bottom; $screen; Screen work area:K27:10)
			//%T+
		End if 
		
		If ($formFile.exists)
			$json:=$formFile.getText("utf-8"; Document with CR:K24:21)
			$formRect:=JSON Parse:C1218($json; Is object:K8:27)
		Else 
			
			If (Not:C34(This:C1470.is_preemptive()))
				C_LONGINT:C283($width; $height)
				//%T-
				If (Count parameters:C259<3)
					If ($tableName="{projectForm}")
						FORM GET PROPERTIES:C674($formName; $width; $height)
					Else 
						$tableNumber:=ds:C1482[$tableName].getInfo().tableNumber
						C_POINTER:C301($table)
						$table:=Table:C252($tableNumber)
						FORM GET PROPERTIES:C674($table->; $formName; $width; $height)
					End if 
				End if 
				//%T+
				$formRect:=New object:C1471("x"; 0; "y"; 0; "width"; $width; "height"; $height; "screen"; $screen; "left"; 0; "top"; 0)
			End if 
			
		End if 
		
		If ($formRect#Null:C1517)
			
			C_LONGINT:C283($x; $y; $s)
			
			If (Not:C34(This:C1470.is_preemptive()))
				//%T-
				$s:=Menu bar screen:C441
				SCREEN COORDINATES:C438($sleft; $stop; $sright; $sbottom; $s)
				$x:=$formRect.left*($sright-$sleft)
				$y:=$formRect.top*($sbottom-$stop)
				If ($s#$formRect.screen)
					SCREEN COORDINATES:C438($sleft; $stop; $sright; $sbottom; $formRect.screen)
					If ($formRect.x>=$sleft) & ($formRect.x<=$sright) & ($formRect.y>=$stop) & ($formRect.y<=$sbottom)
						$x:=$formRect.x
						$y:=$formRect.y
					End if 
				End if 
				//%T+
			End if 
			
			$formRect.x:=$x
			$formRect.y:=$y
			
		End if 
		
	End if 
	
Function set_window_position($formIdentifier : Text; $contextData : Variant)
	
	C_TEXT:C284($windowContext)
	
	If (Count parameters:C259>1)
		C_BLOB:C604($data)
		VARIABLE TO BLOB:C532($contextData; $data)
		$windowContext:=Generate digest:C1147($data; SHA256 digest:K66:4)
	End if 
	
	$info:=This:C1470.split_form_identifier($formIdentifier; $windowContext)
	
	If ($info#Null:C1517)
		
		C_TEXT:C284($tableName; $formName; $context)
		$tableName:=$info.table
		$formName:=$info.form
		$context:=$info.context
		
		$formFile:=This:C1470.get_window_bounds_file($tableName; $formName; $context)
		
		C_OBJECT:C1216($formRect)
		
		If (Not:C34(This:C1470.is_preemptive()))
			//%T-
			$window:=Current form window:C827
			//%T+
			If ($window#0)
				
				C_REAL:C285($left; $top)
				C_LONGINT:C283($x; $y; $right; $bottom; $screen; $s)
				//%T-
				GET WINDOW RECT:C443($x; $y; $right; $bottom; $window)
				C_LONGINT:C283($sleft; $stop; $sright; $sbottom)
				$screen:=Menu bar screen:C441
				SCREEN COORDINATES:C438($sleft; $stop; $sright; $sbottom; $screen)
				$left:=($x-$sleft)/($sright-$sleft)
				$top:=($y-$stop)/($sbottom-$stop)
				For ($s; 1; Count screens:C437)
					SCREEN COORDINATES:C438($sleft; $stop; $sright; $sbottom; $s)
					If ($x>=$sleft) & ($x<=$sright) & ($y>=$stop) & ($y<=$sbottom)
						$screen:=$s
						$left:=($x-$sleft)/($sright-$sleft)
						$top:=($y-$stop)/($sbottom-$stop)
					End if 
				End for 
				//%T+
				$formRect:=New object:C1471("x"; $x; "y"; $y; "width"; $right-$x; "height"; $bottom-$y; "screen"; $screen; "left"; $left; "top"; $top)
				If ($context#"")
					$contextValueType:=Value type:C1509($context)
					Case of 
						: ($contextValueType=Is boolean:K8:9)\
							 | ($contextValueType=Is real:K8:4)\
							 | ($contextValueType=Is longint:K8:6)\
							 | ($contextValueType=Is null:K8:31)\
							 | ($contextValueType=Is time:K8:8)\
							 | ($contextValueType=Is date:K8:7)\
							 | ($contextValueType=Is text:K8:3)\
							 | ($contextValueType=Is object:K8:27)\
							 | ($contextValueType=Is collection:K8:32)
							$formRect.context:=$context
						Else 
							//skip
					End case 
					
				End if 
				$json:=JSON Stringify:C1217($formRect; *)
				$formFile.setText($json; "utf-8"; Document with CR:K24:21)
				
			End if 
			
		End if 
		
	End if 
	
Function split_form_identifier($formIdentifier : Text; $windowContext : Text)->$status : Object
	
	$status:=New object:C1471("context"; $windowContext)
	
	ARRAY LONGINT:C221($pos; 0)
	ARRAY LONGINT:C221($len; 0)
	
	If (Match regex:C1019("\\[([^]]+)\\](.+)"; $formIdentifier; 1; $pos; $len))
		
		$status.table:=Substring:C12($formIdentifier; $pos{1}; $len{1})
		$status.form:=Substring:C12($formIdentifier; $pos{2}; $len{2})
		
	Else 
		
		$status.table:="{projectForm}"
		$status.form:=$formIdentifier
		
	End if 
	
Function get_form_identifier()->$formIdentifier : Text
	
	var $table : Pointer
	
	$table:=Current form table:C627
	
	var $name : Text
	
	$name:=Current form name:C1298
	
	If (Is nil pointer:C315($table))
		$formIdentifier:=$name
	Else 
		$formIdentifier:="["+Table name:C256($table)+"]"+$name
	End if 
	