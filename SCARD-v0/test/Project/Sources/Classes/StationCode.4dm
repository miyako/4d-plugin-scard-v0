Class constructor
	
	var $stationCode : Collection
	
	If (Storage:C1525.stationCode=Null:C1517)
		
		Use (Storage:C1525)
			$stationCode:=New shared collection:C1527
			Storage:C1525.stationCode:=$stationCode
		End use 
		
		$csvFile:=Folder:C1567(fk resources folder:K87:11).folder("csv").file("stationCode.csv")
		$csv:=$csvFile.getText("windows-31j"; Document with CR:K24:21)
		
		ARRAY TEXT:C222($lines; 0)
		ARRAY TEXT:C222($values; 0)
		
		ARRAY LONGINT:C221($pos; 0)
		ARRAY LONGINT:C221($len; 0)
		
		$i:=0x0001
		//read lines
		While (Match regex:C1019("([^\\r]+)"; $csv; $i; $pos; $len))
			$line:=Substring:C12($csv; $pos{1}; $len{1})
			APPEND TO ARRAY:C911($lines; $line)
			$i:=$pos{1}+$len{1}
		End while 
		//read CSV line
		Use ($stationCode)
			$props:=New collection:C1472
			For ($i; 1; Size of array:C274($lines))
				$line:=$lines{$i}
				CLEAR VARIABLE:C89($values)
				$j:=0x0001
				While (Match regex:C1019("(([^,]*),)"; $line; $j; $pos; $len))
					$value:=Substring:C12($line; $pos{2}; $len{2})
					APPEND TO ARRAY:C911($values; $value)
					$j:=$pos{1}+$len{1}
				End while 
				$value:=Substring:C12($line; $j)
				APPEND TO ARRAY:C911($values; $value)
				If ($props.length=0)
					ARRAY TO COLLECTION:C1563($props; $values)
				Else 
					$elem:=0
					$stationObject:=New shared object:C1526
					$stationCode.push($stationObject)
					Use ($stationObject)
						For each ($prop; $props)
							$elem:=$elem+1
							$stationObject[$prop]:=$values{$elem}
						End for each 
					End use 
				End if 
			End for 
		End use 
	End if 
	
	This:C1470.stationCode:=Storage:C1525.stationCode
	
	var $端末種 : Collection
	
	$端末種:=New collection:C1472
	$端末種[3]:="精算機"
	$端末種[4]:="携帯型端末"
	$端末種[5]:="車載端末"
	$端末種[7]:="券売機"
	$端末種[8]:="券売機"
	$端末種[9]:="入金機"
	$端末種[18]:="券売機"
	$端末種[20]:="券売機等"
	$端末種[21]:="券売機等"
	$端末種[22]:="改札機"
	$端末種[23]:="簡易改札機"
	$端末種[24]:="窓口端末"
	$端末種[25]:="窓口端末"
	$端末種[26]:="改札端末"
	$端末種[27]:="携帯電話"
	$端末種[28]:="乗継精算機"
	$端末種[29]:="連絡改札機"
	$端末種[31]:="簡易入金機"
	$端末種[70]:="VIEW ALTTE"
	$端末種[72]:="VIEW ALTTE"
	$端末種[199]:="物販端末"
	$端末種[200]:="自販機"
	
	This:C1470.端末種:=$端末種
	
	var $処理 : Collection
	
	$処理:=New collection:C1472
	$処理[1]:="運賃支払(改札出場)"
	$処理[2]:="チャージ"
	$処理[3]:="券購(磁気券購入)"
	$処理[4]:="精算"
	$処理[5]:="精算 (入場精算)"
	$処理[6]:="窓出 (改札窓口処理)"
	$処理[7]:="新規 (新規発行)"
	$処理[8]:="控除 (窓口控除)"
	$処理[13]:="バス (PiTaPa系)"
	$処理[15]:="バス (IruCa系)"
	$処理[17]:="再発 (再発行処理)"
	$処理[19]:="支払 (新幹線利用)"
	$処理[20]:="入A (入場時オートチャージ)"
	$処理[21]:="出A (出場時オートチャージ)"
	$処理[31]:="入金 (バスチャージ)"
	$処理[35]:="券購 (バス路面電車企画券購入)"
	$処理[70]:="物販"
	$処理[72]:="特典 (特典チャージ)"
	$処理[73]:="入金 (レジ入金)"
	$処理[74]:="物販取消"
	$処理[75]:="入物 (入場物販)"
	$処理[198]:="物現 (現金併用物販)"
	$処理[203]:="入物 (入場現金併用物販)"
	$処理[132]:="精算 (他社精算"
	$処理[133]:="精算 (他社入場精算)"
	
	This:C1470.処理:=$処理
	
Function _getByte($hex : Text)->$byte : Integer
	
	$byte:=Position:C15($hex; "0123456789abcdef"; *)-1
	
Function _getHex($data : Text)->$bytes : Blob
	
	$len:=Length:C16($data)
	
	If (($len%2)=0)
		
		C_BLOB:C604($bytes)
		SET BLOB SIZE:C606($bytes; $len\2)
		
		For ($i; 1; $len; 2)
			$hex:=Substring:C12($data; $i; 2)
			$bytes{$i\2}:=(This:C1470._getByte($hex[[1]]) << 4)+This:C1470._getByte($hex[[2]])
		End for 
	End if 
	
Function _parse_090f($data : Text)->$obj : Object
	
	$端末種:=This:C1470.端末種
	$処理:=This:C1470.処理
	
	$bytes:=This:C1470._getHex($data)
	
	If (BLOB size:C605($bytes)#0)
		
		$obj:=New object:C1471
		
		$term:=$端末種[$bytes{0}]
		$proc:=$処理[$bytes{1}]
		
		$obj.端末種:=$term
		$obj.処理:=$proc
		
		$year:=($bytes{4} >> 1)+2000
		$month:=(($bytes{4} & 1) << 3)+($bytes{5} >> 5)
		$day:=$bytes{5} & (1 << 5)-1
		$date:=Add to date:C393(!00-00-00!; $year; $month; $day)
		$obj.日付:=$date
		
		$num:=(($bytes{12} << 16)+($bytes{13} << 8)+$bytes{14})  // 連番 (SeqNo)
		$balance:=(($bytes{11} << 8)+$bytes{10})
		$obj.残高:=$balance
		
		Case of 
			: (New collection:C1472(70; 73; 74; 75; 198; 203).indexOf($bytes{1})#-1)  //shopping
				$hour:=$bytes{6} >> 3
				$min:=(($bytes{6} & 7) << 3)+($bytes{7} >> 5)
				$sec:=($bytes{7} & 0x001F) << 1  //2秒単位
				$time:=Time:C179(($hour*3600)+($min+60)+$sec)
				$reg:=(($bytes{8} << 8)+$bytes{9})  //物販端末ID
				$obj.利用時刻:=Time string:C180($time)
				$obj.利用:="物販"
			: (New collection:C1472(13; 15; 31; 35).indexOf($bytes{1})#-1)  //bus
				$out_line:=($bytes{6} << 8)+$bytes{7}
				$out_sta:=($bytes{8} << 8)+$bytes{9}
				$obj.バス事業者コード:=$out_line
				$obj.バス停留所コード:=$out_sta
				$obj.利用:="バス"
			Else 
				
				$area:=$bytes{5}
				$region_in:=(($bytes{15}) & 0x00C0) >> 6
				$region_out:=(($bytes{15}) & 0x0030) >> 4
				$in_line:=$bytes{6}
				$in_sta:=$bytes{7}
				$out_line:=$bytes{8}
				$out_sta:=$bytes{9}
				
				$obj.入場駅:=This:C1470._get_station_name($region_in; $in_line; $in_sta)
				$obj.出場駅:=This:C1470._get_station_name($region_out; $out_line; $out_sta)
				$obj.利用:="鉄道"
		End case 
		
	End if 
	
Function parse($values : Collection)->$accumulator : Collection
	
	$accumulator:=New collection:C1472
	
	For each ($value; $values)
		$accumulator.push(This:C1470._parse_090f($value))
	End for each 
	
Function _get_station_name($areaCode : Integer; $lineCode : Integer; $stationCode : Integer)->$stationName : Text
	
	
	If ($areaCode=0) & ($lineCode=0) & ($stationCode=0)
		
	Else 
		
		var $station : Object
		
		$stations:=This:C1470.stationCode.query("AreaCode == :1 and LineCode == :2 and StationCode == :3"; String:C10($areaCode); String:C10($lineCode); String:C10($stationCode))
		
		If ($stations.length#0)
			$station:=$stations[0]
			$stationName:=New collection:C1472($station.LineName; "線"; $station.StationName; "駅").join()
		End if 
		
	End if 