#!/usr/bin/env bash

protoc --proto_path=protos  --php_out=./php   --grpc_out=./php   --plugin=protoc-gen-grpc=/usr/local/bin/grpc_php_plugin protos/Proxy.proto
protoc --proto_path=protos  --cpp_out=./   --grpc_out=./   --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin protos/Proxy.proto