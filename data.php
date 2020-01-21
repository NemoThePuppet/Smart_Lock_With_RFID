<?php
$connectstr_dbname = '';
$connectstr_dbusername = '';
$connectstr_dbpassword = '';

foreach ($_SERVER as $key => $value) {
    if (strpos($key, "MYSQLCONNSTR_localdb") !== 0) {
        continue;
    }
    
    $connectstr_dbhost = preg_replace("/^.*Data Source=(.+?);.*$/", "\\1", $value);
    $connectstr_dbname = preg_replace("/^.*Database=(.+?);.*$/", "\\1", $value);
    $connectstr_dbusername = preg_replace("/^.*User Id=(.+?);.*$/", "\\1", $value);
    $connectstr_dbpassword = preg_replace("/^.*Password=(.+?)$/", "\\1", $value);
}

$link = mysqli_connect($connectstr_dbhost, $connectstr_dbusername, $connectstr_dbpassword,$connectstr_dbname);


if($_SERVER["REQUEST_METHOD"] == 'POST'){
    $insert = "(";
    $values = "(";
    
    $tagUID = '0';
    $register = 0;
    if(isset($_GET["register"])){ $records = $_GET["register"];}
    if(isset($_GET["tagUID"])){ $tagUID = $_GET["tagUID"];}
    
    foreach ($_GET as $key => $value) {
        $insert .= $key . ", ";
        $values .= $value . ", ";
    }
    
    $insert = substr($insert, 0, -2) . ")";
    $values = substr($values, 0, -2) . ")";
    
    foreach($_GET as $key => $value) {
        if (strstr($key, 'register'))
        {
            $register = $value;
        }
    }
    
    if($register == 1){$query = "insert into data (Tag_UID) value (" . $tagUID . ")";}  // register
    else if($register == 0){$query = "delete from data where Tag_UID = " . $tagUID;}    // deregister
    
    
    
    $result = $link->query($query);
    echo "{ \"error\":\"" . $link->error . "\"}";
    
} else if($_SERVER["REQUEST_METHOD"] == 'GET'){
    
    $records = 0;
    $tagUID = '0';
    
    if(isset($_GET["records"])){ $records = $_GET["records"];}
    if(isset($_GET["tagUID"])){ $tagUID = $_GET["tagUID"];}
    
    
    if($records == 1){ $query = "select tag_uid, access_level from data order by id";}
    
    if($tagUID != '0'){ $query = "select access_level from data where Tag_UID = " . $tagUID;}
    
    
    else{$query = "select * from data order by id";}
        
    // $result = $link->query($query);
    $result = mysqli_query($link, $query) or die(mysqli_error($link));
    $data = mysqli_fetch_object($result);
    
    // $dbdata = array();
    // while ( $row = $result->fetch_assoc())  {
    //   $dbdata[]=$row;
    // }
    
    if (empty($data)) {
        echo '0';
    }else{
        echo $data->access_level;
    }    
}

mysqli_close($link);
?>