/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <unistd.h>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "example/helloworld.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

DEFINE_int32(sleep_ms, 1000, "");
DEFINE_int32(port, 50051, "Port to be listened.");

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
  std::string addr_;

 public:
  explicit GreeterServiceImpl(const std::string& addr) : addr_(addr) {}
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    std::string prefix("Hello ");
    std::cout << addr_ << ": " << prefix + request->name() << std::endl;
    reply->set_message(prefix + request->name());
    usleep(FLAGS_sleep_ms * 1000);
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:");
  server_address += std::to_string(FLAGS_port);
  GreeterServiceImpl service(server_address);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {
  FLAGS_logtostderr = true;
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  RunServer();
  return 0;
}
