﻿<?
  include "connect_db_wifilocation.php";
  $query = "SELECT * FROM fingerprint LIMIT 0, 300";
  $result = mysql_query($query, $conn);
  $num_results = mysql_num_rows($result);
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

    // map.setCenter(new GLatLng(39.917,116.397), 14);
    map.setCenter(new GLatLng(39.9799383, 116.3465950), 14);
    
    /*
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
    */

<?
  while($row = mysql_fetch_array($result, MYSQL_ASSOC)) {
    echo "map.addOverlay(new GMarker(new GLatLng(" . $row["latitude"] . ", ". $row["longitude"] . ")));";
  }
?>

  }
}

//]]>
</script>

</head>

<body onLoad="load()" onUnload="GUnload()">
<div id="doc" style="padding:10px;width:740px;margin:0 auto;">

<div id="hd">
  <div style="width:140px;float:left;"></div>
  <div style="width:295px;float:left;">
    <h2 class="pageName">您的位置</h2>
  </div>
  <div style="width:295px;float:right;">
  <form action="upload_file.php" method="post" enctype="multipart/form-data">
    <label for="file">Filename:</label>
    <input type="file" name="file" id="file" /> 
    <input type="submit" name="submit" value="Submit" />
  </form>
  </div>
</div>

<div id="bd" style="clear:both;margin-top:1em;">
  <div id="column-left" style="float:left;width:580px;">
    <div>
      <div id="map" style="width: 800px; height: 600px">
      </div>
    </div>
  </div>
  <div id="ft" style="clear:both;">
  </div>
</div>
</body>
</html>
