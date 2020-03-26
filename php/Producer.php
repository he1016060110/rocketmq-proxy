<?php

$loop = \React\EventLoop\Factory::create();
$reactConnector = new \React\Socket\Connector($loop, [
]);
$connector = new \Ratchet\Client\Connector($loop, $reactConnector);
$that = $this;
$msgId = false;
$url = "";
//todo 切换期间，每次都开一个新连接
$connector($url, [], [])
    ->then(function (\Ratchet\Client\WebSocket $conn) use ($that, $body, &$msgId) {
        $conn->on('message', function (\Ratchet\RFC6455\Messaging\MessageInterface $msg) use ($conn, $that, $body, &$msgId) {
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
            "topic" => $that->topicName,
            "group" => $that->topicName,
            "body" => $body,
            "delayLevel" => $that->delayLevel,
        ];
        $json = json_encode($data);
        $conn->send($json);
    }, function (\Exception $e) use ($loop, $that) {
        $loop->stop();
    });

$loop->run();
var_dump($msgId);