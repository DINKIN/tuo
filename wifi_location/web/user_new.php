<?
if(isset($_POST["submit"])){
	//unset variable
	unset($_POST["submit"]);

	//setup connection
	include "connect_db_wifilocation.php";
	
	//generate query
	$username = $_POST["username"];
	$password = md5($_POST["password"]);
	
	$query = "SELECT * FROM user WHERE name='".$username."' AND password='".$password."'";
	$result = mysql_query($query, $conn);
	
	$num = mysql_num_rows($result);

	if ( $num >= 1 ) {   
    	// A matching row was found - the user is authenticated as 'admin'. 
    	$resArray = mysql_fetch_array($result, MYSQL_ASSOC);
    	$HTTP_SESSION_VARS['user_id'] = $resArray['user_id'];
   		$HTTP_SESSION_VARS['user_type'] = 'admin';
   		$HTTP_SESSION_VARS['user_name'] = $username;
    	echo "<meta http-equiv=\"Refresh\" content=\"0;url=./index.php\">"; 
	} else {
			// there was no match found, so the login failed
    	unset($HTTP_SESSION_VARS['user_type']);
    	$HTTP_SESSION_VARS['error'] = true;
    	//echo "<meta http-equiv=\"Refresh\" content=\"0;url=index.php\">"; 
  }  
}
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<!-- DW6 -->
<head>
<!-- Copyright 2005 Macromedia, Inc. All rights reserved. -->
<title>我的位置</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

<script src="http://ditu.google.com/maps?file=api&amp;v=2&amp;key=ABQIAAAA_FUf5iyYoIXJv6-FBb8-fRTytPVdZL13uf4iIQBHPMvHSNoWCBSqemO7Scy15l6XYXecyeyj_d6gqw"
  type="text/javascript"></script>

<script type="text/javascript">
//<![CDATA[

function load() {
  if (GBrowserIsCompatible()) {
    var map = new GMap2(document.getElementById("map"));
		map.addControl(new GLargeMapControl());
		map.addControl(new GMapTypeControl());
		map.addControl(new GOverviewMapControl());

		var geocoder = new GClientGeocoder();
		address = "北京航空航天大学 北京";
		geocoder.getLatLng(address,function(point) {
      		if (!point) {
        		alert(address + " We tried to find your network, and this was the best estimate we could do. If it's incorrect, just move the map to the correct location.");
      		} else {
        		map.setCenter(point, 13);
      		}
    	  }
  		);
  }
}

//]]>
</script>

</head>

<body onLoad="load()" onUnload="GUnload()">
<div id="doc" style="padding:10px;width:800px;margin:0 auto;">
  <div style="width:500px;float:left;">
		<form method="post" action='index.php', name="login" >
			用户名： <input id="username" name="username" size="15" type="text">
			密  码： <input id="password" name="password" size="15" value="" type="password">
			<input class="button" name="submit" value="登陆" type="submit">
    	<a href=user_new.php">注册</a>
		</form>
  </div>
  <div style="width:300px;float:left;">
		<?
   	if(isset($HTTP_SESSION_VARS['error'])) echo "用户名密码错误";		
		?>
	</div>
</div>
<div id="doc" style="padding:10px;width:800px;margin:0 auto;">
	<div id="bd" style="clear:both;margin-top:1em;">
	  <div id="map" style="width: 800px; height: 600px">
	</div>
</div>
<script type="text/javascript" src="http://cetrk.com/pages/scripts/0009/8661.js"> </script>

</body>
</html>
