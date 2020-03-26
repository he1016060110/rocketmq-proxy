<?php

$msgId = false;
$url = "localhost:8080";
$body = "this is test!";
$topicName = "Test";
$delayLevel = 0;
$loop = \React\EventLoop\Factory::create();
$reactConnector = new \React\Socket\Connector($loop, [
]);
$connector = new \Ratchet\Client\Connector($loop, $reactConnector);

$connector($url, [], [])
    ->then(function (\Ratchet\Client\WebSocket $conn) use ($body, &$msgId, $topicName, $delayLevel) {
        $conn->on('message', function (\Ratchet\RFC6455\Messaging\MessageInterface $msg) use ($conn, $body, &$msgId, $topicName, $delayLevel) {
            $arr = json_decode($msg, true);
            if (isset($arr["code"]) && $arr["code"] == "0") {
                $msgId = $arr['data']['msgId'];
            }
            $conn->close();
        });
        $conn->on('close', function ($code = null, $reason = null) {
        });
        $data = [
            "tag" => "*",
            "topic" => $topicName,
            "group" => $topicName,
            "body" => $body,
            "delayLevel" => $delayLevel,
        ];
        $json = json_encode($data);
        $conn->send($json);
    }, function (\Exception $e) use ($loop) {
        echo $e->getMessage();
        $loop->stop();
    });

$loop->run();
var_dump($msgId);