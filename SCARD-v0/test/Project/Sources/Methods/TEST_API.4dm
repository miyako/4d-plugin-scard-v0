//%attributes = {}
var $options : Object
$status:=SCARD Get readers($options)

If ($status.readers.length#0)
	$reader:=$status.readers.shift()
	
	$reader.services:=New collection:C1472
	
	$reader.services.push(New object:C1471("code"; "090f"; "size"; 20))
	
	$status:=SCARD Read tag($reader)
	
End if 