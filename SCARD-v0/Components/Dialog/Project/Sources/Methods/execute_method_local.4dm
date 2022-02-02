//%attributes = {"invisible":true,"preemptive":"capable"}
#DECLARE($status : Object)->$returnValue : Blob

var $signal : 4D:C1709.Signal

$signal:=New signal:C1641

CALL WORKER:C1389($status.name; $status.method; $status; $signal)

If ($signal.wait())
	
	var $data : Variant
	
	$data:=$signal.data
	
	VARIABLE TO BLOB:C532($data; $returnValue)
	COMPRESS BLOB:C534($returnValue; GZIP fast compression mode:K22:19)
	
End if 