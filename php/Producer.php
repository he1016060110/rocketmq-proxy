<?php

require dirname(__FILE__).'/vendor/autoload.php';

@include_once dirname(__FILE__).'/Helloworld/GreeterClient.php';
@include_once dirname(__FILE__).'/Helloworld/ProduceReply.php';
@include_once dirname(__FILE__).'/Helloworld/ProduceRequest.php';
@include_once dirname(__FILE__).'/GPBMetadata/Proxy.php';

function greet($name)
{
    $client = new Helloworld\GreeterClient('localhost:50051', [
        'credentials' => Grpc\ChannelCredentials::createInsecure(),
    ]);
    $request = new Helloworld\HelloRequest();
    $request->setName($name);
    list($reply, $status) = $client->SayHello($request)->wait();
    $message = $reply->getMessage();

    return $message;
}

$name = !empty($argv[1]) ? $argv[1] : 'world';
echo greet($name)."\n";
