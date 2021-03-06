#include "protobuf/qml/service_generator.h"

namespace protobuf {
namespace qml {

ServiceGenerator::ServiceGenerator(const google::protobuf::ServiceDescriptor* t)
    : t_(t) {
  if (t_) {
    for (int i = 0; i < t_->method_count(); ++i) {
      auto method = t_->method(i);
      IOType in(method->input_type());
      IOType o(method->output_type());
      method_generators_.emplace_back(method, in.qualified_name,
                                      o.qualified_name);
      imports_.insert(std::move(in));
      imports_.insert(std::move(o));
    }
  }
}

void ServiceGenerator::generateImports(google::protobuf::io::Printer& p) {
  p.Print(
      "import QtQuick 2.5\n"
      "import Protobuf 1.0 as PB\n"
      "\n");

  for (auto& import : imports_) {
    p.Print("import '$file_name$' as $capital_name$\n", "file_name",
            import.file_path, "capital_name", import.import_name);
  }
}

void ServiceGenerator::generateClientQmlFile(google::protobuf::io::Printer& p) {
  if (!t_) throw std::logic_error("Cannot generate for null descriptor.");
  generateImports(p);

  p.Print(
      "\n"
      "Item {\n"
      "  id: root\n"
      "  property var channel\n");

  for (auto& g : method_generators_) {
    p.Print("\n");
    g.generateMethod(p);
  }

  p.Print("}\n");
}

void ServiceGenerator::generateServerQmlFile(google::protobuf::io::Printer& p) {
  if (!t_) throw std::logic_error("Cannot generate for null descriptor.");
  generateImports(p);

  p.Print(
      "\n"
      "PB.RpcService {\n"
      "  id: root\n"
      "  methods: [\n");
  for (int i = 0; i < t_->method_count(); ++i) {
    auto method = t_->method(i);
    p.Print("    $method_name$Method,\n", "method_name",
            uncapitalizeFirstLetter(method->name()));
  }
  p.Print("  ]\n");
  for (auto& g : method_generators_) {
    p.Print("\n");
    g.generateServerMethod(p);
  }

  p.Print("}\n");
}

MethodGenerator::MethodGenerator(const google::protobuf::MethodDescriptor* t,
                                 const std::string& input_type_name,
                                 const std::string& output_type_name)
    : t_(t) {
  auto capital_name = t_->name();
  auto camel_name = uncapitalizeFirstLetter(t_->name());

  variables = {
      {"camel_name", camel_name},
      {"capital_name", capital_name},
      {"service_name", t_->service()->full_name()},
      {"input_type", input_type_name},
      {"output_type", output_type_name},
      {"index", std::to_string(t_->index())},
  };

  auto cl = t_->client_streaming();
  auto sv = t_->server_streaming();
  if (cl && sv) {
    variables.insert(std::make_pair("method_type", "ReaderWriter"));
    variables.insert(std::make_pair("server_method_type", "ReaderWriter"));
  } else if (cl) {
    variables.insert(std::make_pair("method_type", "Writer"));
    variables.insert(std::make_pair("server_method_type", "Reader"));
  } else if (sv) {
    variables.insert(std::make_pair("method_type", "Reader"));
    variables.insert(std::make_pair("server_method_type", "Writer"));
  } else {
    variables.insert(std::make_pair("method_type", "Unary"));
    variables.insert(std::make_pair("server_method_type", "Unary"));
  }
}

void MethodGenerator::generateMethod(google::protobuf::io::Printer& p) {
  if (!t_->client_streaming()) {
    withPayload(p);
  } else {
    withoutPayload(p);
  }
  generateMethodElement(p);
}

void MethodGenerator::generateServerMethod(google::protobuf::io::Printer& p) {
  p.Print(variables,
          "property var $camel_name$\n"
          "  PB.Server$server_method_type$Method {\n"
          "    id: $camel_name$Method\n"
          "    methodName: '/$service_name$/$capital_name$'\n"
          "    readType: $input_type$\n"
          "    writeType: $output_type$\n"
          "    index: $index$\n"
          "    handler: $camel_name$\n"
          "  }\n");
}

void MethodGenerator::generateMethodElement(google::protobuf::io::Printer& p) {
  p.Print(variables,
          "  PB.$method_type$Method {\n"
          "    id: $camel_name$Method\n"
          "    methodName: '/$service_name$/$capital_name$'\n"
          "    channel: root.channel || null\n"
          "    readType: $output_type$\n"
          "    writeType: $input_type$\n"
          "  }\n");
}

void MethodGenerator::withPayload(google::protobuf::io::Printer& p) {
  p.Print(variables,
          "  function $camel_name$(data, callback) {\n"
          "    'use strict';\n"
          "    return $camel_name$Method.call(data, callback);\n"
          "  }\n");
}

void MethodGenerator::withoutPayload(google::protobuf::io::Printer& p) {
  p.Print(variables,
          "  function $camel_name$(callback) {\n"
          "    'use strict';\n"
          "    return $camel_name$Method.call(callback);\n"
          "  }\n");
}
}
}
