<?php

include_once __DIR__ . "/vendor/autoload.php";


$url = "ws://127.0.0.1:8090/consumerEndpoint";
$topicName = "Test";
$queueName = "Test";

$loop = \React\EventLoop\Factory::create();
$reactConnector = new \React\Socket\Connector($loop, [
]);

$connector = new \Ratchet\Client\Connector($loop, $reactConnector);
$connector($url, [], [])
    ->then(function (\Ratchet\Client\WebSocket $conn) use ($topicName, $queueName) {
        $conn->on('message', function (\Ratchet\RFC6455\Messaging\MessageInterface $msg) use ($conn, $topicName, $queueName) {
            $arr = json_decode($msg, true);
            //heartbeat
            if ($arr['code'] == 10000) {
                echo $msg . "\n";
                return;
            }
            if (!empty($arr['data']['msgId'])) {
                if ($arr['data']['type'] == 1) {
                    printf("msg:%s\n", $msg);
                    $ackData = [
                        "type" => 2,
                        "topic" => $topicName,
                        "group" => $queueName,
                        "msgId" => $arr['data']['msgId'],
                        "status" => 0,
                    ];
                    $conn->send(json_encode($ackData));
                } else {
                    $data = [
                        "type" => 1,
                        "topic" => $topicName,
                        "group" => $queueName,
                    ];
                    $json = json_encode($data);
                    $conn->send($json);
                }
            } else {
                $conn->close();
            }
        });
        $data = [
            "type" => 1,
            "topic" => $topicName,
            "group" => $queueName,
        ];
        $json = json_encode($data);
        $conn->send($json);
    }, function (\Exception $e) use ($loop) {
        echo $e->getMessage();
        $loop->stop();
    });
$loop->run();