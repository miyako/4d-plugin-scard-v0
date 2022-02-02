//%attributes = {"invisible":true,"shared":true,"preemptive":"capable"}
#DECLARE($context : Variant; $params : Object)

Case of 
	: (Count parameters:C259=0)
		
		//ON ERR CALL
		
	: (Count parameters:C259=2)
		
		//entry point 
		
		If (Value type:C1509($context)=Is text:K8:3)
			
			$o:=New object:C1471
			
			$o.CLIENT_ID:=Generate UUID:C1066
			$o.method:=$context
			$o.params:=$params
			$o.name:=String:C10($params.name)
			
			If ($o.name="")
				$o.name:="#"+$o.method
			End if 
			
			CALL WORKER:C1389($o.method; Current method name:C684; $o)
			
		End if 
		
	: (Count parameters:C259=1)
		
		var $returnValue : Blob
		
		If ($context.params.executeOnServer)
			$returnValue:=execute_method_remote($context)
		Else 
			$returnValue:=execute_method_local($context)
		End if 
		
		ON ERR CALL:C155(Current method name:C684)
		EXPAND BLOB:C535($returnValue)
		ON ERR CALL:C155("")
		
		var $data : Variant
		
		If (OK=1)
			BLOB TO VARIABLE:C533($returnValue; $data)
		End if 
		
		If (OB Instance of:C1731($context.params.formula; 4D:C1709.Function))
			$context.params.formula.call($context; $data)
		Else 
			KILL WORKER:C1390
		End if 
		
End case 