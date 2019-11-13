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

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include "example/helloworld.grpc.pb.h"

#include "absl/strings/str_split.h"
#include "absl/strings/match.h"

// Maybe improper includes?
#include "src/core/ext/filters/client_channel/resolver_registry.h"
#include "src/core/ext/filters/client_channel/resolver.h"
#include "src/core/ext/filters/client_channel/parse_address.h"
#include "src/core/lib/iomgr/executor.h"

using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

DEFINE_string(addresses,
              "127.0.0.1:50051,127.0.0.1:50052,127.0.0.1:50053",
              "Addresses for server");

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string& user) {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

class MyResolver : public grpc_core::Resolver {
 public:
  explicit MyResolver(grpc_core::ResolverArgs args)
      : Resolver(args.combiner, std::move(args.result_handler)) {
    LOG(INFO) << "MyResolver ctor";
    channel_args_ = grpc_channel_args_copy(args.args);
    RegisterSelf(this);
  }

  ~MyResolver() override {
    grpc_channel_args_destroy(channel_args_);
  }

  void StartLocked() override {
    LOG(INFO) << "StartLocked.";
    SetAddresses(FLAGS_addresses);
  }

  void RequestReresolutionLocked() override {
    LOG(INFO) << "RequestReresolutionLocked";
  }

  void ResetBackoffLocked() override {
    LOG(INFO) << "ResetBackoffLocked";
  }

  void ShutdownLocked() override {
    LOG(INFO) << "ShutdownLocked.";
  }

 public:
  struct InternalClosureArgs {
    grpc_closure closure;
    MyResolver* self;
  };

  void SetAddresses(absl::string_view addresses, bool internal = false) {
    auto addrs = absl::StrSplit(addresses, ",");
    result_.addresses.clear();
    for (auto& addr : addrs) {
      std::string str_addr(addr.data(), addr.size());
      LOG(INFO) << "Load: " << str_addr;
      grpc_resolved_address resolved;
      if (!grpc_parse_ipv4_hostport(str_addr.c_str(), &resolved, true)) {
        LOG(WARNING) << "cannot resolve ip: " << str_addr;
        continue;
      }
      result_.addresses.emplace_back(resolved, nullptr);
    }
    if (internal) {
      Notify();
    } else {
      auto args = new InternalClosureArgs;
      args->self = this;
      GRPC_CLOSURE_INIT(&args->closure, OnSetAddresses, args, nullptr);
      grpc_core::Executor::Run(&args->closure, GRPC_ERROR_NONE,
                               grpc_core::ExecutorType::RESOLVER);
    }
  }

 public:
  static MyResolver* g_self;

  static MyResolver* Self() {
    return g_self;
  }
  static void RegisterSelf(MyResolver* ptr) {
    g_self = ptr;
  }

 private:
  void Notify() {
    GRPC_CLOSURE_INIT(&closure_, NotifyCallback, this, nullptr);
    combiner()->Run(&closure_, GRPC_ERROR_NONE);
  }

  static void NotifyCallback(void* args, grpc_error* /*error*/) {
    auto* resolver = static_cast<MyResolver*>(args);
    resolver->NotifyImpl();
  }

  static void OnSetAddresses(void* args, grpc_error* /*error*/) {
    auto* closure_args = static_cast<InternalClosureArgs*>(args);
    closure_args->self->NotifyImpl();
    delete closure_args;
  }

  void NotifyImpl() {
    LOG(INFO) << "NotifyImpl";
    Result tmp = result_;
    tmp.args = grpc_channel_args_copy(channel_args_);
    result_handler()->ReturnResult(std::move(tmp));
  }

 private:
  grpc_core::Resolver::Result result_;
  grpc_closure closure_;
  grpc_channel_args* channel_args_ = nullptr;
};

// static init.
MyResolver* MyResolver::g_self = nullptr;

class MyResolverFactory : public grpc_core::ResolverFactory {
 public:
  bool IsValidUri(const grpc_uri* /*uri*/) const override { return true; }

  grpc_core::OrphanablePtr<grpc_core::Resolver> CreateResolver(
      grpc_core::ResolverArgs args) const override {
    return grpc_core::MakeOrphanable<MyResolver>(std::move(args));
  }

  const char* scheme() const override { return "mine"; }
};

int main(int argc, char** argv) {
  FLAGS_logtostderr = true;
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  grpc_core::ExecCtx exec_ctx;

  grpc_core::ResolverRegistry::Builder::RegisterResolverFactory(
      grpc_core::MakeUnique<MyResolverFactory>());

  ChannelArguments args;
  args.SetLoadBalancingPolicyName("round_robin");  // load balancing policy

  auto channel = grpc::CreateCustomChannel(
      "mine:///123", grpc::InsecureChannelCredentials(), args);

  GreeterClient greeter(channel);
  std::string user(argc < 2 ? "world" : argv[1]);

  while (true) {
    std::cout << "\ncmd: sayhello(default)/reresolve" << std::endl;
    std::cout << ">>> ";

    std::string line;
    if (!std::getline(std::cin, line)) {
      break;
    }

    absl::string_view cmd = line;
    if (cmd.empty() || cmd == "sayhello") {
      std::string reply = greeter.SayHello(user);
      std::cout << "Greeter received: " << reply << std::endl;
    } else if (absl::StartsWith(cmd, "reresolve ")) {
      cmd.remove_prefix(strlen("reresolve "));
      std::cout << "new addrs: " << cmd << std::endl;
      MyResolver::Self()->SetAddresses(cmd);
    } else {
      std::cerr << "what cmd?" << std::endl;
    }
  }

  std::cout << "Bye" << std::endl;
  return 0;
}
