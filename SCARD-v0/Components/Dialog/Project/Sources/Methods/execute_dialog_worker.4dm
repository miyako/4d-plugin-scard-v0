//%attributes = {"invisible":true,"shared":true,"preemptive":"capable"}
#DECLARE($data : Object)

Case of 
	: (Count parameters:C259=1)
		
		execute_task($data.method; $data)
		
		If ($data.interval#Null:C1517)
			
			If ($data.interval>0)
				
				$signal:=New signal:C1641
				$signal.wait($data.interval)
				
				CALL WORKER:C1389(Current process name:C1392; Current method name:C684; $data)
				
			End if 
		End if 
		
End case 