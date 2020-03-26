<?php
include_once "";

$loop = \React\EventLoop\Factory::create();
$reactConnector = new \React\Socket\Connector($loop, [
]);
$connector = new \Ratchet\Client\Connector($loop, $reactConnector);
$that = $this;
$url = "";
//todo 切换期间，每次都开一个新连接
$connector($url, [], [])
    ->then(function (\Ratchet\Client\WebSocket $conn) use ($that, $obj) {
        $conn->on('message', function (\Ratchet\RFC6455\Messaging\MessageInterface $msg) use ($conn, $that, $obj) {
            $arr = json_decode($msg, true);
            //heartbeat
            if ($arr['code'] == 10000) {
                echo $msg . "\n";
                return;
            }
            if (!empty($arr['data']['msgId'])) {
                if ($arr['data']['type'] == 1) {
                    $msgId = $arr['data']['msgId'];
                    try {
                        //如果有错误，先catch错误，断掉连接，然后把错误抛出
                        $ret = $obj->ack_message($arr['data']['body']);
                    } catch (\Throwable $e) {
                        $conn->close();
                        throw $e;
                    }
                    $ackData = [
                        "type" => 2,
                        "topic" => $this->topicName,
                        "group" => $this->queueName,
                        "msgId" => $arr['data']['msgId'],
                        "status" => $ret ? 0 : 1,
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
    }, function (\Exception $e) use ($loop, $that) {
        echo $e->getMessage();
        $loop->stop();
    });
$loop->run();