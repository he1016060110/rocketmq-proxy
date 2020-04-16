<?php

include_once dirname(__FILE__) . "/Common.php";
$client = new Client('localhost:8080');
$client->produce("PHPTest", "test", 0, "gogogo", "*");
