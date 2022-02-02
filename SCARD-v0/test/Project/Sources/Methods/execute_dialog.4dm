//%attributes = {"invisible":true}
#DECLARE($params : Object)

Case of 
	: (Count parameters:C259=0)
		
		CANCEL:C270  //Close Box Method (Open window)
		
	Else 
		
		$title:=$params.title
		
		$dialog:=cs:C1710.Dialog.new()
		
		$window:=$dialog.find_user_window($title)
		
		If ($window#0)
			$dialog.activate_window($window)
		Else 
			
			$windowType:=$params.type
			
			C_OBJECT:C1216($formRect; $status)
			C_POINTER:C301($table)
			
			$dialog:=cs:C1710.Dialog.new()
			
			$status:=$dialog.split_form_identifier($params.name)
			
			If ($status.table="{projectForm}")
				var $name : Text
				$name:=$params.name
				FORM GET PROPERTIES:C674($name; $width; $height)
				$formRect:=$dialog.get_window_position($params.name; ""; $width; $height)
				$window:=$dialog.open_window($formRect; $windowType; $title)
				DIALOG:C40($params.name; $params.form; *)
			Else 
				$formRect:=$dialog.get_window_position($params.name)
				$window:=$dialog.open_window($formRect; $windowType; $title)
				$table:=Table:C252($status.tableNumber)
				DIALOG:C40($table->; $status.form; $params.form; *)
			End if 
			
		End if 
		
End case 