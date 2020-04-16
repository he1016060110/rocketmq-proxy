<?php
/**
 * Created by PhpStorm.
 * User: hexi
 * Date: 2020/4/16
 * Time: 5:16 PM
 */

require dirname(__FILE__) . '/vendor/autoload.php';

include_once dirname(__FILE__) . '/Proxy/RMQProxyClient.php';
include_once dirname(__FILE__) . '/Proxy/ProduceReply.php';
include_once dirname(__FILE__) . '/Proxy/ProduceRequest.php';
include_once dirname(__FILE__) . '/Proxy/ConsumeReply.php';
include_once dirname(__FILE__) . '/Proxy/ConsumeAckReply.php';
include_once dirname(__FILE__) . '/Proxy/ConsumeRequest.php';
include_once dirname(__FILE__) . '/Proxy/ConsumeAckRequest.php';
include_once dirname(__FILE__) . '/GPBMetadata/Proxy.php';

class Client
{
    private $_client;

    function __construct($server)
    {
        $this->_client = new Proxy\RMQProxyClient($server, [
            'credentials' => Grpc\ChannelCredentials::createInsecure(),
        ]);
    }

    public function consume($topic, $group)
    {
        $request = new Proxy\ConsumeRequest();
        $request->setTopic($topic);
        $request->setConsumerGroup($group);
        list($reply, $status) = $this->_client->Consume($request)->wait();
        /**
         * @var $reply Proxy\ConsumeReply
         */
        if ($reply->getCode() === 0) {
            $msgId = $reply->getMsgId();
            echo "consume msg id[" . $msgId . "] body[" . $reply->getBody() . "]\n";
            $request = new Proxy\ConsumeAckRequest();
            $request->setTopic($topic);
            $request->setConsumerGroup($group);
            $request->setMsgId($msgId);
            list($reply, $status) = $this->_client->ConsumeAck($request)->wait();
            if ($reply->getCode() === 0) {
                echo "consume msg ack id[" . $msgId . "]\n";
            } else {
                throw new Exception("consume ack error!msg:" . $reply->getErrorMsg() . " msgId[" . $msgId . "]");
            }
        } else {
            throw new Exception("consume error!msg:" . $reply->getErrorMsg());
        }
    }

    public function produce($topic, $group, $delay, $body, $tag)
    {
        $client = new Proxy\RMQProxyClient('192.168.1.78:8090', [
            'credentials' => Grpc\ChannelCredentials::createInsecure(),
        ]);
        $request = new Proxy\ProduceRequest();
        $request->setBody($body);
        $request->setGroup($group);
        $request->setDelayLevel($delay);
        $request->setTag($tag);
        $request->setTopic($topic);
        list($reply, $status) = $client->Produce($request)->wait();
        /**
         * @var $reply Proxy\ProduceReply
         */
        if ($reply->getCode() === 0) {
            echo "produce msg id[" . $reply->getMsgId() . "] msg[" . $body . "]\n";
        } else {
            throw new Exception("consume ack error!msg:" . $reply->getErrorMsg());
        }
    }

}
