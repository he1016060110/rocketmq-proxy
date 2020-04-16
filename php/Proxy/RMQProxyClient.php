<?php
// GENERATED CODE -- DO NOT EDIT!

namespace Proxy;

/**
 */
class RMQProxyClient extends \Grpc\BaseStub {

    /**
     * @param string $hostname hostname
     * @param array $opts channel options
     * @param \Grpc\Channel $channel (optional) re-use channel object
     */
    public function __construct($hostname, $opts, $channel = null) {
        parent::__construct($hostname, $opts, $channel);
    }

    /**
     * @param \Proxy\ProduceRequest $argument input argument
     * @param array $metadata metadata
     * @param array $options call options
     */
    public function Produce(\Proxy\ProduceRequest $argument,
      $metadata = [], $options = []) {
        return $this->_simpleRequest('/Proxy.RMQProxy/Produce',
        $argument,
        ['\Proxy\ProduceReply', 'decode'],
        $metadata, $options);
    }

    /**
     * @param \Proxy\ConsumeRequest $argument input argument
     * @param array $metadata metadata
     * @param array $options call options
     */
    public function Consume(\Proxy\ConsumeRequest $argument,
      $metadata = [], $options = []) {
        return $this->_simpleRequest('/Proxy.RMQProxy/Consume',
        $argument,
        ['\Proxy\ConsumeReply', 'decode'],
        $metadata, $options);
    }

    /**
     * @param \Proxy\ConsumeAckRequest $argument input argument
     * @param array $metadata metadata
     * @param array $options call options
     */
    public function ConsumeAck(\Proxy\ConsumeAckRequest $argument,
      $metadata = [], $options = []) {
        return $this->_simpleRequest('/Proxy.RMQProxy/ConsumeAck',
        $argument,
        ['\Proxy\ConsumeAckReply', 'decode'],
        $metadata, $options);
    }

}
