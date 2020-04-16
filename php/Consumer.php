<?php


include_once dirname(__FILE__) . "/Common.php";

$client = new Client('localhost:8080');
$client->consume("PHPTest", "test");