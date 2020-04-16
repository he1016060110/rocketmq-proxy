<?php

include_once dirname(__FILE__) . "/Common.php";
$client = new Client('192.168.1.78:8090');
$client->produce("PHPTest", "test", 0, "gogogo", "*");
