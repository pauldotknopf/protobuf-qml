import QtQuick 2.4
import QtTest 1.1
import Protobuf 1.0 as Protobuf
import 'recursive_test.pb.js' as Test1

Item {
  Protobuf.MemoryBuffer {
    id: buffer
  }

  TestCase {
    name: 'RecursiveTest'

    function test_simple() {
      skip('TODO: Fix recursive');
      var foo = new Test1.Foo();
    }
  }
}