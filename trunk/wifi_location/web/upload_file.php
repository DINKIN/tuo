<?php
	$user_id = $_POST["user_id"];

if ($_FILES["file"]["error"] > 0)
{
  echo "Error: " . $_FILES["file"]["error"] . "<br />";
}
else
{

  echo "Upload: " . $_FILES["file"]["name"] . "<br />";
  echo "Type: " . $_FILES["file"]["type"] . "<br />";
  echo "Size: " . ($_FILES["file"]["size"] / 1024) . " Kb<br />";
  echo "Stored in: " . $_FILES["file"]["tmp_name"];

  $filename = $user_id . "_" . $_FILES["file"]["name"];

  // save file
  if (file_exists("upload/" . $_FILES["file"]["name"]))
  {
    echo $_FILES["file"]["name"] . " already exists. ";
    exit;
  }
  else
  {
    move_uploaded_file($_FILES["file"]["tmp_name"], "./upload/" . $filename);
    echo "Stored in: " . "upload/" . $_FILES["file"]["name"];
  }

	//setup connection
	include "connect_db_wifilocation.php";
	
	$query = "insert into data_source (filename, user_id) value ('" . $filename . "', " . $user_id . ")";
	$result = mysql_query($query, $conn);
	$query = "select * from data_source  where filename = '" . $filename . "' and  user_id = " . $user_id . ";";
	$result = mysql_query($query, $conn);
	
	$row = mysql_fetch_array($result, MYSQL_ASSOC);
	$source_index = $row["source_index"];

  // process file
  $file=fopen("./upload/" . $filename,"r") or exit("Unable to open file!");

  echo "<br/>";
  $line = fgets($file);
  $line = fgets($file);
  $line = fgets($file);
  $line = fgets($file);
  
  $last_lat = 0;
  $last_lng = 0;
	$fp_index = -1;
  while(!feof($file))
  {
    $line = fgets($file);
		list($latitude_str, $longitude_str,	$ssid, $type, $bssid_str, $other) = split("\t", $line, 6);
		
		if(strlen($latitude_str) <= 0) continue;
		
		list($lat_sign, $latitude) = split(" ", $latitude_str);
		if($lat_sign == "S") $latitude = -$latitude;
		list($lng_sign, $longitude) = split(" ", $longitude_str);
		if($lng_sign == "W") $longitude = -$longitude;
		list($left, $bssid, $right) = split(" ", $bssid_str);
	
		// insert into record table
		$query = "insert into record (str_bssid, latitude, longitude, source_index) value ('" . $bssid . "', " . $latitude . ", " . $longitude . "," . $source_index . ")";
		$result = mysql_query($query, $conn);

		// insert fingerprint
		if($latitude != $last_lat || $longitude != $last_lng) {
			$last_lat = $latitude;
			$last_lng = $longitude;

			$query = "insert into fingerprint (latitude, longitude, source_index) value ( " . $latitude . ", " . $longitude . "," . $source_index . ")";
			$result = mysql_query($query, $conn);

			$query = "select * from fingerprint where abs(latitude - " . $latitude . ") < 1E-6 and abs(longitude - " . $longitude . ") < 1E-6 and source_index = " . $source_index . ";";
			$result = mysql_query($query, $conn);

			$row = mysql_fetch_array($result, MYSQL_ASSOC);
			$fp_index = $row["fp_index"];
		}

		$query = "select * from ap where str_bssid = '" . $bssid . "';";
		$result = mysql_query($query, $conn);
		
		$ap_index = 0;
		$num = mysql_num_rows($result);
		if ( $num >= 1 ) {   
			$row = mysql_fetch_array($result, MYSQL_ASSOC);
			$ap_index = $row["ap_index"];
		} else {
			$query = "insert into ap (str_bssid) value ('" . $bssid . "')";
			$result = mysql_query($query, $conn);

			$query = "select * from ap where str_bssid = '" . $bssid . "';";
			$result = mysql_query($query, $conn);
			$row = mysql_fetch_array($result, MYSQL_ASSOC);
			$ap_index = $row["ap_index"];
		}
		
		$query = "insert into ap_fingerprint (ap_index, fp_index) value ( " . $ap_index . ", " . $fp_index . ")";
		$result = mysql_query($query, $conn);

		// echo $query;
		// echo $ap_index . " " . $fp_index . " " . $latitude . " " . $longitude . " " .	$bssid . "<br/>";

  }


  fclose($file);
}
?>